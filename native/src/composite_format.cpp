#include "composite_format.hpp"
#include "calendar.hpp"
#include "complex_number.hpp"
#include "decimal.hpp"
#include "diagnostics.hpp"
#include "formatting.hpp"
#include "number_format.hpp"
#include "runtime_values.hpp"
#include "type_names.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <type_traits>

namespace abc {
namespace {
constexpr std::size_t text_limit=1'048'576;
Error invalid_format(SourceLocation location) {
    return Error("E_FORMAT",location,"Invalid composite format or argument index",
        "Некорректный составной формат или индекс аргумента");
}
Error unsupported_number(SourceLocation location) {
    return Error("E_UNSUPPORTED",location,"Numeric format specifier is not supported",
        "Этот числовой формат ещё не поддерживается");
}
bool floating(const Value& value) {
    return std::holds_alternative<float>(value) || std::holds_alternative<double>(value);
}
double floating_value(const Value& value) {
    return std::holds_alternative<float>(value)?static_cast<double>(std::get<float>(value)):std::get<double>(value);
}
std::u16string general_float(const Value& value,int precision,char exponent,SourceLocation location) {
    const auto number=floating_value(value);
    if (number==0) return u"0";
    if (std::isnan(number)) return u"NaN";
    if (std::isinf(number)) return number<0?u"-Infinity":u"Infinity";
    if (precision==0) precision=std::holds_alternative<float>(value)?7:15;
    char buffer[160];
    const auto result=std::to_chars(buffer,buffer+sizeof buffer,number,std::chars_format::general,precision);
    if (result.ec!=std::errc{}) throw runtime_error(RuntimeError::number_format,location);
    std::replace(buffer,result.ptr,'e',exponent);
    return std::u16string(buffer,result.ptr);
}
struct DecimalDigits {
    std::string digits;
    int point=1;
    bool negative=false;
};
struct NumericFormat {
    char16_t symbol;
    char16_t letter;
    int precision;
};
NumericFormat numeric_format(std::u16string_view specifier,SourceLocation location) {
    const auto symbol=specifier.front();
    const auto letter=static_cast<char16_t>(symbol>=u'a' && symbol<=u'z'?symbol-u'a'+u'A':symbol);
    int precision=-1;
    if (specifier.size()>1) {
        precision=0;
        for (const auto digit:specifier.substr(1)) {
            if (digit<u'0' || digit>u'9' || precision>=10) throw unsupported_number(location);
            precision=precision*10+(digit-u'0');
        }
    }
    return {symbol,letter,precision};
}
DecimalDigits exact_decimal_digits(std::u16string_view text) {
    DecimalDigits result;
    result.negative=text.starts_with(u'-');
    if (result.negative) text.remove_prefix(1);
    const auto point=text.find(u'.');
    result.point=static_cast<int>(point==text.npos?text.size():point);
    for (const auto digit:text) if (digit!=u'.') result.digits.push_back(static_cast<char>(digit));
    const auto leading_zeros=result.digits.find_first_not_of('0');
    if (leading_zeros==result.digits.npos) return {"0",1,false};
    result.digits.erase(0,leading_zeros);
    result.point-=static_cast<int>(leading_zeros);
    return result;
}
DecimalDigits decimal_digits(const Value& value,SourceLocation location) {
    DecimalDigits result;
    if (!floating(value)) {
        const auto text=format_field(value,0,std::nullopt,location);
        result.negative=text.starts_with(u'-');
        result.digits.assign(text.begin()+result.negative,text.end());
        result.point=static_cast<int>(result.digits.size());
        return result;
    }
    const auto number=floating_value(value);
    result.negative=number<0;
    char buffer[40];
    const auto precision=std::holds_alternative<float>(value)?6:14;
    const auto converted=std::to_chars(buffer,buffer+sizeof buffer,std::abs(number),std::chars_format::scientific,precision);
    if (converted.ec!=std::errc{}) throw runtime_error(RuntimeError::number_format,location);
    const std::string_view text(buffer,static_cast<std::size_t>(converted.ptr-buffer));
    const auto exponent=text.find('e');
    if (exponent==text.npos) throw runtime_error(RuntimeError::number_format,location);
    auto power=text.substr(exponent+1);
    if (power.starts_with('+')) power.remove_prefix(1);
    const auto parsed=std::from_chars(power.data(),power.data()+power.size(),result.point);
    if (parsed.ec!=std::errc{}) throw runtime_error(RuntimeError::number_format,location);
    ++result.point;
    for (const auto ch:text.substr(0,exponent)) if (ch!='.') result.digits.push_back(ch);
    return result;
}
std::string rounded_digits(const std::string& digits,int width) {
    if (width<0) return "0";
    const auto kept=static_cast<std::size_t>(width);
    auto result=digits.substr(0,kept);
    result.resize(kept,'0');
    if (kept<digits.size() && digits[kept]>='5') {
        const auto position=result.find_last_not_of('9');
        if (position==result.npos) {
            std::fill(result.begin(),result.end(),'0');
            result.insert(result.begin(),'1');
        } else {
            ++result[position];
            std::fill(result.begin()+static_cast<std::ptrdiff_t>(position+1),result.end(),'0');
        }
    }
    return result;
}
void append_checked(std::u16string& result,std::u16string_view text,SourceLocation location) {
    if (text.size()>text_limit-result.size()) throw runtime_error(RuntimeError::string_limit,location);
    result.append(text);
}
std::vector<std::size_t> group_positions(std::size_t width,const NumberFormatSettings& format) {
    std::vector<std::size_t> positions;
    if (format.groups.empty() || format.group.empty()) return positions;
    std::size_t group=0;
    while (format.groups[group]>0 && width>static_cast<std::size_t>(format.groups[group])) {
        width-=static_cast<std::size_t>(format.groups[group]);
        positions.push_back(width);
        if (group+1<format.groups.size()) ++group;
    }
    return positions;
}
std::u16string signed_fixed(std::u16string text,const NumberFormatSettings& format,int pattern,SourceLocation location) {
    std::u16string result;
    if (pattern==0) result.push_back(u'(');
    else if (pattern==1 || pattern==2) {
        append_checked(result,format.negative,location);
        if (pattern==2) append_checked(result,u" ",location);
    }
    append_checked(result,text,location);
    if (pattern==0) append_checked(result,u")",location);
    else if (pattern==3 || pattern==4) {
        if (pattern==4) append_checked(result,u" ",location);
        append_checked(result,format.negative,location);
    }
    return result;
}
std::u16string fixed_number(const DecimalDigits& number,int precision,bool grouped,
    const NumberFormatSettings& format,SourceLocation location) {
    const auto fraction=static_cast<std::size_t>(precision);
    auto digits=rounded_digits(number.digits,number.point+precision);
    if (digits.size()<=fraction) digits.insert(0,fraction+1-digits.size(),'0');
    std::u16string result;
    const auto integer_length=digits.size()-fraction;
    auto cuts=grouped?group_positions(integer_length,format):std::vector<std::size_t>{};
    for (std::size_t i=0;i<integer_length;++i) {
        if (!cuts.empty() && cuts.back()==i) {
            append_checked(result,format.group,location);
            cuts.pop_back();
        }
        const auto digit=static_cast<char16_t>(digits[i]);
        append_checked(result,std::u16string_view(&digit,1),location);
    }
    if (fraction) {
        append_checked(result,format.decimal,location);
        append_checked(result,std::u16string(digits.begin()+static_cast<std::ptrdiff_t>(integer_length),digits.end()),location);
    }
    if (number.negative && digits.find_first_not_of('0')!=digits.npos)
        return signed_fixed(std::move(result),format,grouped?format.pattern:1,location);
    return result;
}
std::u16string scientific_number(DecimalDigits number,int precision,char16_t exponent,bool general=false) {
    auto mantissa=rounded_digits(number.digits,precision+1);
    if (mantissa.size()>static_cast<std::size_t>(precision+1)) {
        mantissa.pop_back();
        ++number.point;
    }
    if (general)
        while (mantissa.size()>1 && mantissa.back()=='0') mantissa.pop_back();
    std::u16string result;
    if (number.negative) result.push_back(u'-');
    result.push_back(static_cast<char16_t>(mantissa.front()));
    if (mantissa.size()>1) {
        result.push_back(u'.');
        result.append(mantissa.begin()+1,mantissa.end());
    }
    result.push_back(exponent);
    const auto power=number.point-1;
    result.push_back(power<0?u'-':u'+');
    const auto digits=std::to_string(std::abs(power));
    const auto width=general?2U:3U;
    if (digits.size()<width) result.append(width-digits.size(),u'0');
    result.append(digits.begin(),digits.end());
    return result;
}
std::u16string hexadecimal(const Value& value,std::size_t precision,bool upper,SourceLocation location) {
    const auto bits=std::visit([&](const auto& number)->std::uint64_t {
        using T=std::decay_t<decltype(number)>;
        if constexpr (std::is_integral_v<T> && !std::is_same_v<T,bool> && !std::is_same_v<T,char16_t>)
            return static_cast<std::make_unsigned_t<T>>(number);
        else throw invalid_format(location);
    },value);
    char buffer[16];
    const auto result=std::to_chars(buffer,buffer+sizeof buffer,bits,16);
    if (result.ec!=std::errc{}) throw runtime_error(RuntimeError::number_format,location);
    std::u16string text;
    const auto length=static_cast<std::size_t>(result.ptr-buffer);
    if (length<precision) text.append(precision-length,u'0');
    for (auto ch=buffer;ch!=result.ptr;++ch)
        text.push_back(static_cast<char16_t>(upper && *ch>='a'?*ch-'a'+'A':*ch));
    return text;
}
std::u16string render_argument(const Program& program,const Value& argument,std::u16string_view specifier,const NumberFormatSettings& format,
    SourceLocation location) {
    const auto& object=std::get<std::shared_ptr<Aggregate>>(argument);
    if (!object || object->nil) return {};
    if (object->items.empty()) return u"System.Object";
    if (object->items.size()!=1 || !object->items.front()) throw runtime_error(RuntimeError::invalid_bytecode,location);
    const auto& item=*object->items.front();
    const auto& value=item.value;
    if (std::holds_alternative<NilString>(value)) return {};
    if (compound(item.type)) {
        const auto& shape=type_info(program,item.type);
        if (shape.kind==CompoundKind::array)
            return std::get<std::u16string>(reflected_type_property(program,{item.type},TypeOperation::full_name,location,{}));
        if (complex_number_type(shape)) return complex_dotnet_text(value,specifier,format,location);
        if (calendar_record(shape)) return calendar_text(value,specifier,location);
        if (decimal_type(shape)) return format_decimal(value,specifier,format,location);
    }
    if (!scalar(item.type))
        throw Error("E_UNSUPPORTED",location,"Formatting this object type is not supported",
            "Форматирование этого типа объекта ещё не поддерживается");
    if (!integer_type(item.type) && !floating(value)) return format_scalar(value,location);
    return format_number(value,item.type,specifier,format,location);
}
bool digit(char16_t ch) { return ch>=u'0' && ch<=u'9'; }
}

std::u16string format_number(const Value& value,Type type,std::u16string_view specifier,
    const NumberFormatSettings& format,SourceLocation location) {
    if (specifier.empty()) return localize_number(format_scalar(value,location),format,location);
    const auto [symbol,letter,precision]=numeric_format(specifier,location);
    if (letter==u'D') {
        if (!integer_type(type)) throw invalid_format(location);
        auto text=format_scalar(value,location);
        const auto negative=text.starts_with(u'-');
        const auto digits=text.size()-negative;
        if (precision>=0 && static_cast<std::size_t>(precision)>digits)
            text.insert(negative,static_cast<std::size_t>(precision)-digits,u'0');
        return localize_number(text,format,location);
    }
    if (letter==u'X') return hexadecimal(value,static_cast<std::size_t>(std::max(0,precision)),symbol==u'X',location);
    if (letter==u'G' && floating(value))
        return localize_number(general_float(value,std::max(0,precision),symbol==u'g'?'e':'E',location),format,location);
    if (letter==u'G') {
        const auto number=decimal_digits(value,location);
        if (precision<=0 || number.point<=precision)
            return localize_number(format_scalar(value,location),format,location);
        return localize_number(scientific_number(number,precision-1,symbol==u'g'?u'e':u'E',true),format,location);
    }
    if (letter!=u'F' && letter!=u'N' && letter!=u'E') throw unsupported_number(location);
    if (floating(value) && !std::isfinite(floating_value(value)))
        return localize_number(format_scalar(value,location),format,location);
    const auto number=decimal_digits(value,location);
    if (letter==u'E') return localize_number(scientific_number(number,precision<0?6:precision,symbol),format,location);
    return fixed_number(number,precision<0?format.digits:precision,letter==u'N',format,location);
}

std::u16string format_decimal(const Value& value,std::u16string_view specifier,
    const NumberFormatSettings& format,SourceLocation location) {
    const auto text=decimal_text(value);
    if (specifier.empty()) return localize_number(text,format,location);
    const auto [symbol,letter,precision]=numeric_format(specifier,location);
    if (letter==u'D' || letter==u'X') throw invalid_format(location);
    if (letter==u'G' && precision<=0) return localize_number(text,format,location);
    const auto number=exact_decimal_digits(text);
    if (letter==u'E') return localize_number(scientific_number(number,precision<0?6:precision,symbol),format,location);
    if (letter==u'F' || letter==u'N')
        return fixed_number(number,precision<0?format.digits:precision,letter==u'N',format,location);
    if (letter!=u'G') throw unsupported_number(location);

    auto rounded=number;
    rounded.digits=rounded_digits(number.digits,precision);
    if (rounded.digits.size()>static_cast<std::size_t>(precision)) {
        rounded.digits.pop_back();
        ++rounded.point;
    }
    if (rounded.point>precision || rounded.point<-3)
        return localize_number(scientific_number(number,precision-1,symbol==u'g'?u'e':u'E',true),format,location);
    while (rounded.digits.size()>1 && rounded.digits.back()=='0') rounded.digits.pop_back();
    return fixed_number(rounded,std::max(0,static_cast<int>(rounded.digits.size())-rounded.point),false,format,location);
}
std::u16string format_scalar(const Value& value,SourceLocation location) {
    if (floating(value)) return general_float(value,0,'E',location);
    return format_field(value,0,std::nullopt,location);
}
std::u16string localize_number(std::u16string_view input,const NumberFormatSettings& format,SourceLocation location) {
    if (input==u"NaN") return format.nan;
    if (input==u"Infinity") return format.positive_infinity;
    if (input==u"-Infinity") return format.negative_infinity;
    std::u16string result;
    for (const auto ch:input) {
        std::u16string_view part(&ch,1);
        if (ch==u'.') part=format.decimal;
        else if (ch==u'+') part=format.positive;
        else if (ch==u'-') part=format.negative;
        append_checked(result,part,location);
    }
    return result;
}
void composite_format(const Program& program,const Value& format,const Value& arguments,const NumberFormatSettings& settings,SourceLocation location,
    const std::function<void(std::u16string_view)>& append) {
    const auto& args=std::get<std::shared_ptr<Aggregate>>(arguments);
    if (std::holds_alternative<NilString>(format) || !args || args->nil)
        throw runtime_error(RuntimeError::nil_reference,location);
    const auto& text=string_text(format);
    std::size_t cursor=0;
    const auto next=[&](char16_t ch){return cursor<text.size() && text[cursor]==ch;};
    const auto spaces=[&]{while (next(u' ')) ++cursor;};
    const auto number=[&] {
        if (cursor==text.size() || !digit(text[cursor])) throw invalid_format(location);
        std::size_t result=0;
        do {
            const auto value=static_cast<std::size_t>(text[cursor++]-u'0');
            if (result>(text_limit-value)/10) throw invalid_format(location);
            result=result*10+value;
        } while (cursor<text.size() && digit(text[cursor]));
        return result;
    };
    while (cursor<text.size()) {
        const auto ch=text[cursor++];
        if (ch!=u'{' && ch!=u'}') {
            append(std::u16string_view(&ch,1));
            continue;
        }
        if (next(ch)) {
            ++cursor;
            append(std::u16string_view(&ch,1));
            continue;
        }
        if (ch==u'}') throw invalid_format(location);
        const auto index=number();
        if (index>=args->items.size()) throw invalid_format(location);
        spaces();
        bool left=false;
        std::size_t width=0;
        if (next(u',')) {
            ++cursor;
            spaces();
            left=next(u'-');
            if (left) ++cursor;
            width=number();
            spaces();
        }
        std::u16string specifier;
        if (next(u':')) {
            ++cursor;
            while (cursor<text.size()) {
                const auto part=text[cursor];
                if (part==u'}' && (cursor+1==text.size() || text[cursor+1]!=u'}')) break;
                ++cursor;
                if (part==u'{' || part==u'}') {
                    if (!next(part)) throw invalid_format(location);
                    ++cursor;
                }
                specifier.push_back(part);
            }
        }
        if (!next(u'}')) throw invalid_format(location);
        ++cursor;
        const auto rendered=render_argument(program,args->items[index]->value,specifier,settings,location);
        const auto padding=width>rendered.size()?width-rendered.size():0U;
        if (padding && !left) append(std::u16string(padding,u' '));
        append(rendered);
        if (padding && left) append(std::u16string(padding,u' '));
    }
}
}

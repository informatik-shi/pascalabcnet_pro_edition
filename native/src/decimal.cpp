#include "decimal.hpp"
#include "big_integer_runtime.hpp"
#include "composite_format.hpp"
#include "number_format.hpp"
#include "runtime_values.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <limits>
#include <type_traits>

namespace abc {
namespace {
using M=DecimalMethod;
using Words=std::array<std::uint32_t,4>;
constexpr unsigned scale_limit=28;
constexpr std::uint32_t sign_mask=0x80000000U,scale_mask=0x00ff0000U;
constexpr std::array double_powers{
    1e0,1e1,1e2,1e3,1e4,1e5,1e6,1e7,1e8,1e9,1e10,1e11,1e12,1e13,1e14,
    1e15,1e16,1e17,1e18,1e19,1e20,1e21,1e22,1e23,1e24,1e25,1e26,1e27,1e28
};
struct Decimal {
    BigInteger coefficient;
    unsigned scale=0;
    bool negative=false;
};
M method(std::size_t operation) {return static_cast<M>(operation&255U);}
Type payload(std::size_t operation) {return static_cast<Type>(operation>>8U);}
Error decimal_overflow(SourceLocation location) {
    return Error("E_OVERFLOW",location,"Decimal overflow","Переполнение Decimal");
}
Error invalid_decimal(SourceLocation location) {
    return Error("E_FORMAT",location,"Invalid Decimal text","Некорректная строка Decimal");
}
Error invalid_bits(SourceLocation location) {
    return Error("E_ARGUMENT",location,"Invalid Decimal bit representation","Некорректное битовое представление Decimal");
}
Words words(const Value& value) {
    const auto& fields=std::get<std::shared_ptr<Aggregate>>(value)->items;
    return {std::get<std::uint32_t>(fields[0]->value),std::get<std::uint32_t>(fields[1]->value),
        std::get<std::uint32_t>(fields[2]->value),std::get<std::uint32_t>(fields[3]->value)};
}
Decimal parts(const Value& value) {
    const auto data=words(value);
    const auto low=static_cast<std::uint64_t>(data[0])|(static_cast<std::uint64_t>(data[1])<<32U);
    auto coefficient=BigInteger::from_unsigned(low);
    if (data[2]) coefficient=coefficient.add(BigInteger::from_unsigned(data[2]).shift_left(64));
    return {std::move(coefficient),(data[3]&scale_mask)>>16U,(data[3]&sign_mask)!=0};
}
Value make_decimal(Values& values,Type type,const Words& data) {
    auto value=std::get<std::shared_ptr<Aggregate>>(values.zero(type,false,true));
    for (std::size_t i=0;i<data.size();++i) value->items[i]->value=data[i];
    return value;
}
Value make_decimal(Values& values,Type type,const Decimal& parts) {
    const auto low=parts.coefficient.bit_and(BigInteger::from_unsigned(UINT64_MAX)).to_unsigned();
    const auto high=static_cast<std::uint32_t>(parts.coefficient.shift_right(64).to_unsigned());
    const Words data{static_cast<std::uint32_t>(low),static_cast<std::uint32_t>(low>>32U),high,
        (parts.scale<<16U)|(parts.negative?sign_mask:0U)};
    return make_decimal(values,type,data);
}
const BigInteger& power10(unsigned power) {
    static const auto powers=[] {
        std::array<BigInteger,57> table;
        table[0]=BigInteger::from_unsigned(1);
        const auto ten=BigInteger::from_unsigned(10);
        for (std::size_t i=1;i<table.size();++i) table[i]=table[i-1].multiply(ten);
        return table;
    }();
    return powers.at(power);
}
BigInteger signed_coefficient(const Decimal& value,unsigned scale) {
    auto coefficient=scale==value.scale?value.coefficient:value.coefficient.multiply(power10(scale-value.scale));
    return value.negative?coefficient.negated():coefficient;
}
BigInteger rounded(BigInteger quotient,const BigInteger& remainder,const BigInteger& divisor) {
    const auto twice=remainder.shift_left(1);
    if (twice>divisor || (twice==divisor && quotient.odd())) quotient=quotient.add(power10(0));
    return quotient;
}
Decimal fit(BigInteger magnitude,unsigned scale,bool negative,SourceLocation location) {
    for (auto cut=scale>scale_limit?scale-scale_limit:0U;cut<=scale;++cut) {
        if (cut==0 && magnitude.bits()<=96) return {std::move(magnitude),scale,negative};
        if (cut==0) continue;
        const auto& divisor=power10(cut);
        auto [quotient,remainder]=magnitude.divide(divisor);
        if (quotient.bits()>96) continue;
        quotient=rounded(std::move(quotient),remainder,divisor);
        if (quotient.bits()<=96) return {std::move(quotient),scale-cut,negative};
    }
    throw decimal_overflow(location);
}
Decimal divide_decimal(const Decimal& left,const Decimal& right,SourceLocation location) {
    if (right.coefficient.zero()) throw runtime_error(RuntimeError::division_zero,location);
    const bool negative=left.negative!=right.negative;
    const auto difference=static_cast<int>(left.scale)-static_cast<int>(right.scale);
    const auto lowest_scale=static_cast<unsigned>(std::max(0,difference));
    const auto numerator=difference<0?left.coefficient.multiply(power10(static_cast<unsigned>(-difference))):left.coefficient;
    const auto& denominator=right.coefficient;
    auto [initial,remainder]=numerator.divide(denominator);
    if (remainder.zero()) return fit(std::move(initial),lowest_scale,negative,location);
    if (initial.bits()>96) throw decimal_overflow(location);

    // Find the most precise representable quotient and round only once.
    for (auto scale=static_cast<int>(scale_limit);scale>=static_cast<int>(lowest_scale);--scale) {
        const auto additional=static_cast<unsigned>(scale)-lowest_scale;
        auto [result,residue]=numerator.multiply(power10(additional)).divide(denominator);
        if (result.bits()>96) continue;
        result=rounded(std::move(result),residue,denominator);
        if (result.bits()>96) continue;
        auto final_scale=static_cast<unsigned>(scale);
        if (result.zero()) final_scale=0;
        while (final_scale) {
            auto [next,tail]=result.divide(power10(1));
            if (!tail.zero()) break;
            result=std::move(next);
            --final_scale;
        }
        return {std::move(result),final_scale,negative};
    }
    throw decimal_overflow(location);
}
Decimal add_decimal(const Decimal& left,const Decimal& right,bool subtract,SourceLocation location) {
    const auto scale=std::max(left.scale,right.scale);
    const auto first=signed_coefficient(left,scale),second=signed_coefficient(right,scale);
    const auto sum=subtract?first.subtract(second):first.add(second);
    return fit(sum.absolute(),scale,sum.negative(),location);
}
Decimal multiply_decimal(const Decimal& left,const Decimal& right,SourceLocation location) {
    return fit(left.coefficient.multiply(right.coefficient),left.scale+right.scale,
        left.negative!=right.negative,location);
}
bool smaller_magnitude(const Decimal& left,const Decimal& right) {
    const auto scale=std::max(left.scale,right.scale);
    return signed_coefficient(left,scale).absolute()<signed_coefficient(right,scale).absolute();
}
Decimal round_decimal(Decimal input,unsigned scale,M call,bool away_from_zero=false) {
    if (input.scale<=scale) return input;
    const auto& divisor=power10(input.scale-scale);
    auto [quotient,remainder]=input.coefficient.divide(divisor);
    if (call==M::round) {
        if (!away_from_zero) quotient=rounded(std::move(quotient),remainder,divisor);
        else if (remainder.shift_left(1)>=divisor) quotient=quotient.add(power10(0));
    }
    else if (!remainder.zero() && ((call==M::floor && input.negative) || (call==M::ceiling && !input.negative)))
        quotient=quotient.add(power10(0));
    return {std::move(quotient),scale,input.negative};
}
Decimal remainder_decimal(const Decimal& left,Decimal divisor,SourceLocation location) {
    if (divisor.coefficient.zero()) throw runtime_error(RuntimeError::division_zero,location);
    if (smaller_magnitude(left,divisor)) return left;
    divisor.negative=left.negative;
    // The source runtime reduces the dividend before division. Its
    // intermediate operations use Decimal precision and may reduce scale.
    auto reduced=add_decimal(left,divisor,true,location);
    if (reduced.coefficient.zero()) reduced.negative=left.negative;
    const auto quotient=round_decimal(divide_decimal(reduced,divisor,location),0,M::truncate);
    const auto multiple=multiply_decimal(quotient,divisor,location);
    auto result=add_decimal(reduced,multiple,true,location);
    if (result.negative!=reduced.negative) {
        const Decimal near_zero{power10(0),27,false};
        if (smaller_magnitude(near_zero,result)) result=add_decimal(result,divisor,false,location);
        else result.negative=reduced.negative;
    }
    return result;
}
Decimal fit_digits(std::string_view text,unsigned scale,bool negative,SourceLocation location) {
    const auto first=text.find_first_not_of('0');
    text=first==std::string_view::npos?std::string_view{}:text.substr(first);
    if (text.size()>static_cast<std::size_t>(scale)+29) throw decimal_overflow(location);
    auto cut=std::max(scale>scale_limit?scale-scale_limit:0U,
        text.size()>29?static_cast<unsigned>(text.size()-29):0U);
    for (;cut<=scale;++cut) {
        const auto kept=cut<text.size()?text.size()-cut:0;
        auto coefficient=kept?BigInteger::parse(text.substr(0,kept)):BigInteger{};
        if (cut && cut<=text.size()) {
            const auto first_dropped=text[kept];
            const auto sticky=first_dropped=='5' &&
                text.substr(kept+1).find_first_not_of('0')!=std::string_view::npos;
            if (first_dropped>'5' || (first_dropped=='5' && (sticky || coefficient.odd())))
                coefficient=coefficient.add(power10(0));
        }
        if (coefficient.bits()<=96) return {std::move(coefficient),scale-cut,negative};
    }
    throw decimal_overflow(location);
}
std::u16string_view trim(std::u16string_view input) {
    constexpr std::u16string_view whitespace=u" \t\n\r\v\f";
    const auto first=input.find_first_not_of(whitespace);
    if (first==std::u16string_view::npos) return {};
    return input.substr(first,input.find_last_not_of(whitespace)-first+1);
}
Decimal parse_decimal(std::u16string_view text,const NumberFormatSettings& format,SourceLocation location) {
    text=trim(text);
    bool negative=false;
    const auto consume_sign=[&](bool suffix) {
        const auto consume=[&](std::u16string_view sign) {
            if (sign.empty()) return false;
            if (!suffix && text.starts_with(sign)) {text.remove_prefix(sign.size());return true;}
            if (suffix && text.ends_with(sign)) {text.remove_suffix(sign.size());return true;}
            return false;
        };
        if (consume(format.positive)) return true;
        negative=consume(format.negative);
        return negative;
    };
    if (!consume_sign(false)) (void)consume_sign(true);
    std::string digits;
    digits.reserve(text.size());
    bool point=false;
    unsigned scale=0;
    while (!text.empty()) {
        if (text.front()>=u'0' && text.front()<=u'9') {
            digits.push_back(static_cast<char>(text.front()));
            if (point) ++scale;
            text.remove_prefix(1);
        } else if (!point && !format.decimal.empty() && text.starts_with(format.decimal)) {
            text.remove_prefix(format.decimal.size());
            point=true;
        } else if (!point && !digits.empty() && !format.group.empty() && text.starts_with(format.group))
            text.remove_prefix(format.group.size());
        else throw invalid_decimal(location);
    }
    if (digits.empty()) throw invalid_decimal(location);
    return fit_digits(digits,scale,negative,location);
}
template<class F> Decimal from_float(F number,SourceLocation location) {
    if (!std::isfinite(number)) throw decimal_overflow(location);
    if (number==0) return {};
    constexpr auto precision=std::is_same_v<F,float>?7:15;
    auto magnitude=std::abs(static_cast<double>(number));
    int binary_exponent=0;
    std::frexp(magnitude,&binary_exponent);
    if (binary_exponent<-94) return {};
    if (binary_exponent>96) throw decimal_overflow(location);
    // Convert to an integer of at most 7 or 15 significant digits before
    // rounding, using the binary exponent to estimate the decimal scale.
    const auto decimal_exponent=(binary_exponent*19728)>>16;
    auto scale=std::min(precision-1-decimal_exponent,static_cast<int>(scale_limit));
    if (scale>=0) magnitude*=double_powers[static_cast<unsigned>(scale)];
    else if (scale!=-1 || magnitude>=double_powers[precision])
        magnitude/=double_powers[static_cast<unsigned>(-scale)];
    else scale=0;
    if (magnitude<double_powers[precision-1] && scale<static_cast<int>(scale_limit)) {
        magnitude*=10;
        ++scale;
    }
    auto mantissa=static_cast<std::uint64_t>(magnitude);
    const auto fraction=magnitude-static_cast<double>(mantissa);
    if (fraction>0.5 || (fraction==0.5 && (mantissa&1))) ++mantissa;
    if (!mantissa) return {};
    if (scale<0)
        return fit(BigInteger::from_unsigned(mantissa).multiply(power10(static_cast<unsigned>(-scale))),
            0,std::signbit(number),location);
    for (auto count=std::min(precision-1,scale);count>0 && mantissa%10==0;--count) {
        mantissa/=10;
        --scale;
    }
    return {BigInteger::from_unsigned(mantissa),static_cast<unsigned>(scale),std::signbit(number)};
}
std::string ascii_text(const Decimal& decimal) {
    auto text=decimal.coefficient.text();
    if (decimal.scale) {
        if (text.size()<=decimal.scale) text.insert(0,decimal.scale+1-text.size(),'0');
        text.insert(text.size()-decimal.scale,1,'.');
    }
    if (decimal.negative && !decimal.coefficient.zero()) text.insert(text.begin(),'-');
    return text;
}
BigInteger truncate(const Decimal& value) {
    const auto coefficient=value.scale?value.coefficient.divide(power10(value.scale)).first:value.coefficient;
    return value.negative?coefficient.negated():coefficient;
}
template<class T> T to_integral(const Decimal& value,SourceLocation location) {
    const auto number=truncate(value);
    if constexpr (std::numeric_limits<T>::is_signed) {
        const auto result=number.to_signed();
        if (result<std::numeric_limits<T>::min() || result>std::numeric_limits<T>::max()) throw decimal_overflow(location);
        return static_cast<T>(result);
    } else {
        const auto result=number.to_unsigned();
        if (result>std::numeric_limits<T>::max()) throw decimal_overflow(location);
        return static_cast<T>(result);
    }
}
double to_real(const Words& data) {
    const auto low=static_cast<std::uint64_t>(data[0])|(static_cast<std::uint64_t>(data[1])<<32U);
    const auto high=static_cast<double>(data[2])*0x1p64;
    const auto value=(static_cast<double>(low)+high)/double_powers[(data[3]&scale_mask)>>16U];
    return (data[3]&sign_mask)?-value:value;
}
Value bits_array(Values& values,Type type,const Words& data) {
    auto result=values.array(type,data.size());
    for (std::size_t i=0;i<data.size();++i) result->items[i]->value=std::bit_cast<std::int32_t>(data[i]);
    return result;
}
}
bool decimal_record_shape(const TypeInfo& info) {
    if (!decimal_type(info) || info.name!="decimal" || info.kind!=CompoundKind::record
        || info.element!=Type::invalid || info.key!=Type::invalid || info.fields.size()!=4) return false;
    constexpr std::array<std::string_view,4> fields{"_lo","_mid","_hi","_flags"};
    for (std::size_t i=0;i<fields.size();++i)
        if (info.fields[i].name!=fields[i] || info.fields[i].type!=Type::longword || info.fields[i].global)
            return false;
    return true;
}
bool decimal_rounding_enum(const TypeInfo& shape) {
    return shape.kind==CompoundKind::enumeration && shape.name=="system.midpointrounding"
        && !shape.enum_flags && shape.enum_members.size()==2
        && shape.enum_members[0].name=="toeven" && shape.enum_members[0].value==0
        && shape.enum_members[1].name=="awayfromzero" && shape.enum_members[1].value==1;
}
bool decimal_signature(std::span<const TypeInfo> types,Type owner,std::size_t operation) {
    if (!compound(owner) || static_cast<unsigned>(owner)-64U>=types.size()
        || !decimal_record_shape(types[static_cast<unsigned>(owner)-64U])) return false;
    const auto call=method(operation);
    if (call>M::hash_code) return false;
    const bool provider=call==M::to_string_provider || call==M::to_string_specifier_provider;
    const bool rounding=call==M::round_mode || call==M::round_scale_mode;
    if (!provider && !rounding && call!=M::from_bits && call!=M::get_bits) return (operation>>8U)==0;
    if ((operation>>8U)>65535U) return false;
    const auto array=payload(operation);
    if (!compound(array) || static_cast<unsigned>(array)-64U>=types.size()) return false;
    const auto& shape=types[static_cast<unsigned>(array)-64U];
    if (provider) return shape.kind==CompoundKind::number_format;
    if (rounding) return decimal_rounding_enum(shape);
    return shape.kind==CompoundKind::array && shape.rank==1 && shape.element==Type::integer;
}
std::vector<Type> decimal_parameters(Type owner,std::size_t operation) {
    switch (method(operation)) {
    case M::zero: case M::one: case M::minus_one: case M::minimum: case M::maximum: return {};
    case M::from_signed: return {Type::int64};
    case M::from_unsigned: return {Type::uint64};
    case M::from_character: return {Type::character};
    case M::from_single: return {Type::single};
    case M::from_real: return {Type::real};
    case M::from_parts: return {Type::integer,Type::integer,Type::integer,Type::boolean,Type::byte};
    case M::from_bits: return {payload(operation)};
    case M::parse: return {Type::string};
    case M::try_parse: return {Type::string,reference_type(owner)};
    case M::to_string_specifier: return {owner,Type::string};
    case M::to_string_provider: return {owner,payload(operation)};
    case M::to_string_specifier_provider: return {owner,Type::string,payload(operation)};
    case M::round_scale: return {owner,Type::integer};
    case M::round_mode: return {owner,payload(operation)};
    case M::round_scale_mode: return {owner,Type::integer,payload(operation)};
    case M::add: case M::subtract: case M::multiply: case M::equal: case M::unequal:
    case M::less: case M::less_equal: case M::greater: case M::greater_equal: case M::compare:
    case M::divide: case M::remainder:
        return {owner,owner};
    default: return {owner};
    }
}
Type decimal_result(Type owner,std::size_t operation) {
    switch (method(operation)) {
    case M::get_bits: return payload(operation);
    case M::to_string: case M::to_string_specifier: case M::to_string_provider: case M::to_string_specifier_provider:
        return Type::string;
    case M::to_int32: case M::compare: case M::hash_code: return Type::integer;
    case M::to_int64: return Type::int64;
    case M::to_uint32: return Type::longword;
    case M::to_uint64: return Type::uint64;
    case M::to_byte: return Type::byte;
    case M::to_shortint: return Type::shortint;
    case M::to_smallint: return Type::smallint;
    case M::to_word: return Type::word;
    case M::to_character: return Type::character;
    case M::to_real: return Type::real;
    case M::to_single: return Type::single;
    case M::try_parse: case M::equal: case M::unequal: case M::less: case M::less_equal:
    case M::greater: case M::greater_equal: return Type::boolean;
    default: return owner;
    }
}
Value decimal_invoke(Values& values,Type owner,std::size_t operation,std::span<const Value> args) {
    return checked_big(values.location,[&]()->Value {
        const auto call=method(operation);
        const auto location=values.location;
        const auto build=[&](const Decimal& value){return make_decimal(values,owner,value);};
        const auto build_words=[&](const Words& value){return make_decimal(values,owner,value);};
        switch (call) {
        case M::zero: return build_words({0,0,0,0});
        case M::one: return build_words({1,0,0,0});
        case M::minus_one: return build_words({1,0,0,sign_mask});
        case M::minimum: return build_words({UINT32_MAX,UINT32_MAX,UINT32_MAX,sign_mask});
        case M::maximum: return build_words({UINT32_MAX,UINT32_MAX,UINT32_MAX,0});
        case M::from_signed: {
            const auto value=std::get<std::int64_t>(args[0]);
            return build({BigInteger::from_signed(value).absolute(),0,value<0});
        }
        case M::from_unsigned: return build({BigInteger::from_unsigned(std::get<std::uint64_t>(args[0])),0,false});
        case M::from_character: return build({BigInteger::from_unsigned(std::get<char16_t>(args[0])),0,false});
        case M::from_single: return build(from_float(std::get<float>(args[0]),location));
        case M::from_real: return build(from_float(std::get<double>(args[0]),location));
        case M::from_parts: {
            const auto scale=std::get<std::uint8_t>(args[4]);
            if (scale>scale_limit) throw Error("E_RANGE",location,"Decimal scale must be between 0 and 28",
                "Масштаб Decimal должен быть от 0 до 28");
            const auto word=[&](std::size_t i){return std::bit_cast<std::uint32_t>(std::get<std::int32_t>(args[i]));};
            return build_words({word(0),word(1),word(2),(static_cast<std::uint32_t>(scale)<<16U)
                |(std::get<bool>(args[3])?sign_mask:0U)});
        }
        case M::from_bits: {
            const auto& array=std::get<std::shared_ptr<Aggregate>>(args[0]);
            if (!array || array->nil) throw runtime_error(RuntimeError::nil_reference,location);
            Words data{};
            if (array->items.size()!=data.size()) throw invalid_bits(location);
            for (std::size_t i=0;i<data.size();++i)
                data[i]=std::bit_cast<std::uint32_t>(std::get<std::int32_t>(array->items[i]->value));
            return decimal_from_bits(values,owner,data);
        }
        case M::get_bits: return bits_array(values,payload(operation),words(args[0]));
        case M::parse:
            if (std::holds_alternative<NilString>(args[0])) throw runtime_error(RuntimeError::nil_reference,location);
            return build(parse_decimal(string_text(args[0]),number_settings(values),location));
        case M::try_parse: {
            auto result=Decimal{};
            bool success=true;
            try { result=parse_decimal(string_text(args[0]),number_settings(values),location); }
            catch (const Error& error) {
                if (error.code!="E_FORMAT" && error.code!="E_OVERFLOW") throw;
                success=false;
            }
            values.write(std::get<std::shared_ptr<Cell>>(args[1]),build(result));
            return success;
        }
        case M::to_string: case M::to_string_specifier: case M::to_string_provider:
        case M::to_string_specifier_provider: {
            const bool specifier=call==M::to_string_specifier || call==M::to_string_specifier_provider;
            const bool provider=call==M::to_string_provider || call==M::to_string_specifier_provider;
            return format_decimal(args[0],specifier?std::u16string_view(string_text(args[1])):std::u16string_view{},
                number_settings(values,provider?&args.back():nullptr),location);
        }
        case M::to_single: return static_cast<float>(to_real(words(args[0])));
        case M::to_real: return to_real(words(args[0]));
        case M::hash_code: {
            const auto converted=to_real(words(args[0]));
            if (converted==0) return std::int32_t{0};
            const auto bits=std::bit_cast<std::uint64_t>(converted);
            const auto low=static_cast<std::uint32_t>(bits)&0xfffffff0U;
            const auto high=static_cast<std::uint32_t>(bits>>32U);
            return std::bit_cast<std::int32_t>(high^low);
        }
        case M::negate: {
            auto data=words(args[0]);
            data[3]^=sign_mask;
            return build_words(data);
        }
        default: break;
        }
        const auto left=parts(args[0]);
        switch (call) {
        case M::to_int32: return to_integral<std::int32_t>(left,location);
        case M::to_int64: return to_integral<std::int64_t>(left,location);
        case M::to_uint32: return to_integral<std::uint32_t>(left,location);
        case M::to_uint64: return to_integral<std::uint64_t>(left,location);
        case M::to_byte: return to_integral<std::uint8_t>(left,location);
        case M::to_shortint: return to_integral<std::int8_t>(left,location);
        case M::to_smallint: return to_integral<std::int16_t>(left,location);
        case M::to_word: return to_integral<std::uint16_t>(left,location);
        case M::to_character: return static_cast<char16_t>(to_integral<std::uint16_t>(left,location));
        case M::truncate: case M::floor: case M::ceiling:
            return build(round_decimal(left,0,call));
        case M::round: case M::round_scale: case M::round_mode: case M::round_scale_mode: {
            const bool has_scale=call==M::round_scale || call==M::round_scale_mode;
            const bool has_mode=call==M::round_mode || call==M::round_scale_mode;
            const auto scale=has_scale?std::get<std::int32_t>(args[1]):0;
            if (scale<0 || scale>static_cast<int>(scale_limit))
                throw Error("E_RANGE",location,"Decimal rounding scale must be between 0 and 28",
                    "Масштаб округления Decimal должен быть от 0 до 28");
            const auto mode=has_mode?std::get<EnumValue>(args.back()).value:0;
            if (mode<0 || mode>1) throw Error("E_ARGUMENT",location,"Invalid MidpointRounding value",
                "Недопустимое значение MidpointRounding");
            return build(round_decimal(left,static_cast<unsigned>(scale),M::round,mode==1));
        }
        default: break;
        }
        const auto right=parts(args[1]);
        if (call==M::divide) return build(divide_decimal(left,right,location));
        if (call==M::remainder) return build(remainder_decimal(left,right,location));
        if (call==M::multiply) return build(multiply_decimal(left,right,location));
        if (call==M::add || call==M::subtract) return build(add_decimal(left,right,call==M::subtract,location));
        const auto scale=std::max(left.scale,right.scale);
        const auto first=signed_coefficient(left,scale),second=signed_coefficient(right,scale);
        const auto order=first.compare(second);
        switch (call) {
        case M::equal: return order==0;
        case M::unequal: return order!=0;
        case M::less: return order<0;
        case M::less_equal: return order<=0;
        case M::greater: return order>0;
        case M::greater_equal: return order>=0;
        case M::compare: return static_cast<std::int32_t>((order>0)-(order<0));
        default: throw runtime_error(RuntimeError::invalid_bytecode,location);
        }
    });
}
std::u16string decimal_text(const Value& value) {
    const auto text=ascii_text(parts(value));
    return std::u16string(text.begin(),text.end());
}
std::array<std::uint32_t,4> decimal_bits(const Value& value) {
    return words(value);
}
Value decimal_from_bits(Values& values,Type owner,const std::array<std::uint32_t,4>& data) {
    const auto flags=data[3];
    if ((flags&~(sign_mask|scale_mask)) || (flags&scale_mask)>(scale_limit<<16U))
        throw invalid_bits(values.location);
    return make_decimal(values,owner,data);
}
}

#include "stdlib.hpp"
#include "diagnostics.hpp"
#include "number.hpp"
#include "integer_values.hpp"

#include <algorithm>
#include <charconv>
#include <limits>
#include <type_traits>

namespace abc::stdlib {
namespace {
bool ascii_space(char16_t c) { return c == u' ' || (c >= 9 && c <= 13); }
bool unicode_space(char16_t c) {
    return ascii_space(c) || c == 0x85 || c == 0xa0 || c == 0x1680
        || (c >= 0x2000 && c <= 0x200a) || c == 0x2028 || c == 0x2029
        || c == 0x202f || c == 0x205f || c == 0x3000;
}
std::u16string_view number_text(std::u16string_view text, bool unicode) {
    auto space = unicode ? unicode_space : ascii_space;
    // Numeric .NET parsing permits NUL padding only after all whitespace.
    if (!unicode) while (!text.empty() && text.back() == 0) text.remove_suffix(1);
    while (!text.empty() && space(text.front())) text.remove_prefix(1);
    while (!text.empty() && space(text.back())) text.remove_suffix(1);
    return text;
}
template<class T> Value parse_integer(std::u16string_view text, SourceLocation location) {
    constexpr bool small = sizeof(T) == 4;
    const auto name = small ? "integer" : "int64";
    text = number_text(text, small);
    bool negative = false;
    if (!text.empty() && (text.front() == u'+' || text.front() == u'-')) {
        negative = text.front() == u'-'; text.remove_prefix(1);
    }
    if (text.empty()) throw runtime_error(RuntimeError::number_text, location, name);
    using U = std::make_unsigned_t<T>;
    const U maximum = static_cast<U>(std::numeric_limits<T>::max());
    const U limit = negative ? maximum + U{1} : maximum;
    U magnitude = 0;
    bool overflow = false;
    for (auto c : text) {
        if (c < u'0' || c > u'9') throw runtime_error(RuntimeError::number_text, location, name);
        const U digit = static_cast<U>(c - u'0');
        if (magnitude > (limit - digit) / 10) overflow = true;
        if constexpr (small) if (overflow) throw runtime_error(RuntimeError::number_range, location, name);
        if (!overflow) magnitude = magnitude * 10 + digit;
    }
    if (overflow) throw runtime_error(RuntimeError::number_range, location, name);
    if (negative && magnitude == maximum + U{1}) return std::numeric_limits<T>::min();
    return static_cast<T>(negative ? -static_cast<T>(magnitude) : static_cast<T>(magnitude));
}

double parse_decimal(std::u16string_view source, std::u16string_view separator, char16_t group, SourceLocation location, bool allow_group=true) {
    if (separator.empty()) throw runtime_error(RuntimeError::decimal_separator, location);
    const auto text = number_text(source, false);
    std::string normalized;
    normalized.reserve(text.size());
    std::size_t position = 0;
    if (position < text.size() && (text[position] == u'+' || text[position] == u'-'))
        normalized += static_cast<char>(text[position++]);
    bool digit = false, decimal = false, exponent = false, exponent_digit = false;
    while (position < text.size()) {
        const auto c = text[position];
        if (c >= u'0' && c <= u'9') {
            normalized += static_cast<char>(c); ++position;
            if (exponent) exponent_digit = true; else digit = true;
        } else if (!exponent && !decimal && text.substr(position).starts_with(separator)) {
            normalized += '.'; position += separator.size(); decimal = true;
        } else if (!exponent && digit && (c == u'e' || c == u'E')) {
            normalized += 'e'; ++position; exponent = true;
            if (position < text.size() && (text[position] == u'+' || text[position] == u'-'))
                normalized += static_cast<char>(text[position++]);
        } else if (allow_group && !exponent && !decimal && digit && c == group) {
            // Global nfi uses '.', while NumberFormat(separator) uses ','.
            ++position;
        } else throw runtime_error(RuntimeError::number_text, location, "real");
    }
    if (!digit || (exponent && !exponent_digit)) throw runtime_error(RuntimeError::number_text, location, "real");
    auto view = std::string_view(normalized);
    if (view.starts_with('+')) view.remove_prefix(1);
    double result = 0;
    if (!parse_real(view, result)) {
        // Unlike source literals, .NET string conversion rounds tiny numbers to zero.
        auto magnitude = view;
        const bool negative = magnitude.starts_with('-');
        if (negative) magnitude.remove_prefix(1);
        const auto e = magnitude.find_first_of("eE");
        const auto mantissa = magnitude.substr(0, e);
        int power = 0;
        if (e != std::string_view::npos) {
            auto exponent_text = magnitude.substr(e + 1);
            bool minus = exponent_text.starts_with('-');
            if (minus || exponent_text.starts_with('+')) exponent_text.remove_prefix(1);
            for (auto c : exponent_text) power = std::min(1'000'000, power * 10 + (c - '0'));
            if (minus) power = -power;
        }
        const auto dot = mantissa.find('.');
        const auto before = dot == std::string_view::npos ? mantissa.size() : dot;
        int digit_index = 0;
        bool nonzero = false;
        for (auto c : mantissa) {
            if (c == '.') continue;
            if (c != '0') { nonzero = true; break; }
            ++digit_index;
        }
        if (!nonzero || power + static_cast<int>(before) - digit_index - 1 <= -324)
            return negative ? -0.0 : 0.0;
        throw runtime_error(RuntimeError::number_range, location, "real");
    }
    return result;
}

std::u16string format_number(const Value& value, SourceLocation location) {
    char buffer[128];
    std::to_chars_result converted;
    if (const auto* n = std::get_if<std::int32_t>(&value)) converted = std::to_chars(buffer, buffer + sizeof buffer, *n);
    else if (const auto* wide = std::get_if<std::int64_t>(&value)) converted = std::to_chars(buffer, buffer + sizeof buffer, *wide);
    else converted = std::to_chars(buffer, buffer + sizeof buffer, std::get<double>(value), std::chars_format::general);
    if (converted.ec != std::errc{}) throw runtime_error(RuntimeError::number_format, location);
    std::string text(buffer, converted.ptr);
    if (const auto e = text.find('e'); e != std::string::npos) {
        int power = 0;
        auto digits = std::string_view(text).substr(e + 1);
        if (digits.starts_with('+')) digits.remove_prefix(1);
        const auto parsed = std::from_chars(digits.data(), digits.data() + digits.size(), power);
        if (parsed.ec != std::errc{}) throw runtime_error(RuntimeError::number_format, location);
        if (power >= -4 && power < 17) {
            const bool negative = text.front() == '-';
            std::string mantissa = text.substr(negative ? 1 : 0, e - (negative ? 1 : 0));
            if (const auto dot = mantissa.find('.'); dot != std::string::npos) mantissa.erase(dot, 1);
            const auto point = power + 1;
            if (point <= 0) text = "0." + std::string(static_cast<std::size_t>(-point), '0') + mantissa;
            else if (static_cast<std::size_t>(point) >= mantissa.size()) text = mantissa + std::string(static_cast<std::size_t>(point) - mantissa.size(), '0');
            else { text = mantissa; text.insert(static_cast<std::size_t>(point), "."); }
            if (negative) text.insert(text.begin(), '-');
        } else text[e] = 'E';
    }
    return std::u16string(text.begin(), text.end());
}
} // namespace

std::optional<Value> try_parse_number(Type type, const Value& source, SourceLocation location) {
    if (std::holds_alternative<NilString>(source)) {
        if (type==Type::integer) throw runtime_error(RuntimeError::nil_reference,location);
        return std::nullopt;
    }
    try {
        const auto& source_text=string_text(source);
        if (type==Type::integer) return parse_integer<std::int32_t>(source_text,location);
        if (type==Type::int64) return parse_integer<std::int64_t>(source_text,location);
        if (type==Type::real) return parse_decimal(source_text,u".",u',',location,false);
        if (type==Type::uint64 || type==Type::longword) {
            auto text=number_text(source_text,false);
            const bool negative=!text.empty() && text.front()==u'-';
            if (!text.empty() && (text.front()==u'+' || text.front()==u'-')) text.remove_prefix(1);
            std::uint64_t magnitude=0;
            if (text.empty()) return std::nullopt;
            const auto limit=type==Type::uint64 ? UINT64_MAX : static_cast<std::uint64_t>(UINT32_MAX);
            for (auto c:text) {
                if (c<u'0' || c>u'9') return std::nullopt;
                const auto digit=static_cast<unsigned>(c-u'0');
                if (magnitude>(limit-digit)/10) return std::nullopt;
                magnitude=magnitude*10+digit;
            }
            if (negative && magnitude) return std::nullopt;
            return type==Type::uint64 ? Value{magnitude} : Value{static_cast<std::uint32_t>(magnitude)};
        }
        if (small_integer(type)) return checked_small_integer(type,std::get<std::int64_t>(parse_integer<std::int64_t>(source_text,location)),location);
        throw runtime_error(RuntimeError::invalid_bytecode,location);
    } catch (const Error& error) {
        if (error.code!="E_FORMAT" && error.code!="E_OVERFLOW") throw;
        return std::nullopt;
    }
}

std::pair<std::u16string,std::int32_t> read_string_word(const Value& source, std::int32_t position,
    SourceLocation location, const ExecutionOptions& options, bool skip_only) {
    if (std::holds_alternative<NilString>(source)) throw runtime_error(RuntimeError::nil_reference,location);
    const auto& text=string_text(source);
    if (position<1 || (!skip_only && static_cast<std::size_t>(position)>text.size()+1)) throw runtime_error(RuntimeError::index_bounds,location);
    auto index=static_cast<std::size_t>(position-1);
    const auto check=[&] { if ((index&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,location); };
    while (index<text.size() && unicode_space(text[index])) { check(); ++index; }
    const auto start=index;
    if (!skip_only) while (index<text.size() && !unicode_space(text[index])) { check(); ++index; }
    return {skip_only ? std::u16string{} : text.substr(start,index-start),static_cast<std::int32_t>(index+1)};
}
std::pair<std::int32_t,std::int32_t> read_string_integer(const Value& source, std::int32_t position,
    SourceLocation location, const ExecutionOptions& options) {
    if (std::holds_alternative<NilString>(source)) throw runtime_error(RuntimeError::nil_reference,location);
    const auto& text=string_text(source);
    if (position<1) throw runtime_error(RuntimeError::index_bounds,location);
    auto index=static_cast<std::size_t>(position-1);
    while (index<text.size() && unicode_space(text[index])) {
        if ((index&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,location);
        ++index;
    }
    bool negative=false;
    if (index<text.size() && (text[index]==u'+' || text[index]==u'-')) { negative=text[index]==u'-'; ++index; }
    if (index>=text.size() || text[index]<u'0' || text[index]>u'9') throw runtime_error(RuntimeError::number_text,location,"integer");
    std::uint32_t magnitude=0; const std::uint32_t limit=negative ? 2'147'483'648U : 2'147'483'647U;
    while (index<text.size() && text[index]>=u'0' && text[index]<=u'9') {
        if ((index&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,location);
        const auto digit=static_cast<unsigned>(text[index]-u'0');
        if (magnitude>(limit-digit)/10) throw runtime_error(RuntimeError::number_range,location,"integer");
        magnitude=magnitude*10+digit; ++index;
        // ParseCoreFrom stops immediately after recognizing the special Int32 minimum.
        if (negative && magnitude==2'147'483'648U) break;
    }
    const auto value=magnitude==2'147'483'648U ? INT32_MIN : negative ? -static_cast<std::int32_t>(magnitude) : static_cast<std::int32_t>(magnitude);
    return {value,static_cast<std::int32_t>(index+1)};
}

std::optional<Value> invoke_text(Function function, std::span<const Value> arguments, SourceLocation location) {
    switch (function) {
    case Function::string_length:
        if (std::holds_alternative<NilString>(arguments[0])) throw runtime_error(RuntimeError::nil_reference,location);
        return static_cast<std::int32_t>(string_text(arguments[0]).size());
    case Function::to_digit: {
        const auto ch=std::get<char16_t>(arguments[0]);
        if (ch<u'0' || ch>u'9') throw runtime_error(RuntimeError::character_digit,location);
        return std::int32_t(ch-u'0');
    }
    case Function::string_of_char: {
        const auto length=std::get<std::int32_t>(arguments[1]);
        if (length<0) throw runtime_error(RuntimeError::string_length,location);
        if (length>1'048'576) throw runtime_error(RuntimeError::string_limit,location);
        return std::u16string(static_cast<std::size_t>(length),std::get<char16_t>(arguments[0]));
    }
    case Function::repeat_string: {
        const bool reversed=value_type(arguments[0])==Type::integer;
        const auto& text=string_text(arguments[reversed ? 1 : 0]);
        const auto count=std::get<std::int32_t>(arguments[reversed ? 0 : 1]);
        if (count<=0 || text.empty()) return std::u16string{};
        if (text.size()>1'048'576 || static_cast<std::size_t>(count)>1'048'576/text.size())
            throw runtime_error(RuntimeError::string_limit,location);
        std::u16string result; result.reserve(text.size()*static_cast<std::size_t>(count));
        for (std::int32_t i=0;i<count;++i) result+=text;
        return result;
    }
    case Function::resize_string: {
        const auto& original = string_text(arguments[0]);
        const auto length = std::get<std::int32_t>(arguments[1]);
        if (length < 0) throw runtime_error(RuntimeError::string_length, location);
        if (length > 1'048'576) throw runtime_error(RuntimeError::string_limit, location);
        auto result = original;
        result.resize(static_cast<std::size_t>(length), u' ');
        return result;
    }
    case Function::insert_string: {
        const auto& substring = string_text(arguments[0]);
        const auto& original = string_text(arguments[1]);
        const auto index = std::get<std::int32_t>(arguments[2]);
        constexpr std::size_t limit = 1'048'576;
        if (original.size() > limit || substring.size() > limit - original.size())
            throw runtime_error(RuntimeError::string_limit, location);
        const auto start = index < 1 ? std::size_t{0}
            : std::min(original.size(), static_cast<std::size_t>(index - 1));
        auto result = original;
        result.insert(start, substring);
        return result;
    }
    case Function::delete_string: {
        if (std::holds_alternative<NilString>(arguments[0])) return arguments[0];
        const auto& original = string_text(arguments[0]);
        const auto index = std::get<std::int32_t>(arguments[1]), count = std::get<std::int32_t>(arguments[2]);
        if (index < 1 || count <= 0 || static_cast<std::size_t>(index - 1) >= original.size()) return original;
        auto result = original;
        result.erase(static_cast<std::size_t>(index - 1), static_cast<std::size_t>(count));
        return result;
    }
    case Function::int_to_str: case Function::float_to_str: return format_number(arguments[0], location);
    case Function::str_to_int:
        if (std::holds_alternative<NilString>(arguments[0])) throw runtime_error(RuntimeError::nil_reference,location);
        return parse_integer<std::int32_t>(string_text(arguments[0]), location);
    case Function::str_to_int64:
        if (std::holds_alternative<NilString>(arguments[0])) return std::int64_t{0};
        return parse_integer<std::int64_t>(string_text(arguments[0]), location);
    case Function::to_integer_default: case Function::to_real_default: {
        const auto parsed=try_parse_number(function==Function::to_integer_default ? Type::integer : Type::real,arguments[0],location);
        return parsed ? *parsed : arguments[1];
    }
    case Function::is_integer: case Function::is_real:
        return try_parse_number(function==Function::is_integer ? Type::integer : Type::real,arguments[0],location).has_value();
    case Function::parse_real: case Function::str_to_real: {
        if (arguments.size()==2 && string_text(arguments[1]).empty()) throw runtime_error(RuntimeError::decimal_separator,location);
        if (std::holds_alternative<NilString>(arguments[0])) {
            if (function==Function::parse_real) throw runtime_error(RuntimeError::nil_reference,location);
            return 0.0;
        }
        return parse_decimal(string_text(arguments[0]), arguments.size() == 2
            ? std::u16string_view(string_text(arguments[1])) : u".", arguments.size() == 2 ? u',' : u'.', location);
    }
    case Function::compare_str: {
        const auto& left = string_text(arguments[0]);
        const auto& right = string_text(arguments[1]);
        for (std::size_t i = 0; i < std::min(left.size(), right.size()); ++i)
            if (left[i] != right[i]) return std::int32_t(left[i]) - std::int32_t(right[i]);
        return static_cast<std::int32_t>(left.size()) - static_cast<std::int32_t>(right.size());
    }
    case Function::trim: case Function::trim_left: case Function::trim_right: {
        if (std::holds_alternative<NilString>(arguments[0])) throw runtime_error(RuntimeError::nil_reference,location);
        auto text = std::u16string_view(string_text(arguments[0]));
        if (function != Function::trim_right) while (!text.empty() && text.front() == u' ') text.remove_prefix(1);
        if (function != Function::trim_left) while (!text.empty() && text.back() == u' ') text.remove_suffix(1);
        return std::u16string(text);
    }
    case Function::left_str: case Function::right_str: {
        const auto& text = string_text(arguments[0]);
        const auto count = std::get<std::int32_t>(arguments[1]);
        if (count <= 0) return std::u16string{};
        const auto length = std::min(text.size(), static_cast<std::size_t>(count));
        return text.substr(function == Function::left_str ? 0 : text.size() - length, length);
    }
    case Function::reverse_string: {
        if (std::holds_alternative<NilString>(arguments[0])) throw runtime_error(RuntimeError::nil_reference,location);
        auto text = string_text(arguments[0]);
        std::size_t start = 0, length = text.size();
        if (arguments.size() == 3) {
            const auto index = std::get<std::int32_t>(arguments[1]), count = std::get<std::int32_t>(arguments[2]);
            if (index < 1 || count < 0 || static_cast<std::size_t>(index - 1) > text.size()
                || static_cast<std::size_t>(count) > text.size() - static_cast<std::size_t>(index - 1))
                throw runtime_error(RuntimeError::string_slice, location);
            start = static_cast<std::size_t>(index - 1); length = static_cast<std::size_t>(count);
        }
        std::reverse(text.begin() + static_cast<std::ptrdiff_t>(start), text.begin() + static_cast<std::ptrdiff_t>(start + length));
        return text;
    }
    default: return std::nullopt;
    }
}
} // namespace abc::stdlib

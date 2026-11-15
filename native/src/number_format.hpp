#pragma once
#include "abc/core.hpp"
#include <span>

namespace abc {
struct Values;
struct NumberFormatSettings {
    std::u16string decimal=u".",group=u",",positive=u"+",negative=u"-",nan=u"NaN";
    std::u16string positive_infinity=u"Infinity",negative_infinity=u"-Infinity";
    std::int32_t digits=2,pattern=1;
    std::vector<std::int32_t> groups{3};
};
enum class NumberFormatMethod : std::uint8_t {
    create, factory, invariant, current, readonly_copy, is_readonly, get, set, to_string, float_text,
    parse_real, set_decimal, set_format
};
enum class NumberFormatField : std::uint8_t {
    decimal_separator, group_separator, positive_sign, negative_sign, nan_symbol,
    positive_infinity, negative_infinity, decimal_digits, negative_pattern, group_sizes
};
bool number_format_signature(const Program&,Type,std::size_t);
std::vector<Type> number_format_parameters(Type,Type,std::size_t);
Type number_format_result(Type,Type,std::size_t);
Value number_format_invoke(Values&,Type,std::size_t,std::span<const Value>);
Type known_number_format_type(const Program&);
const NumberFormatSettings& number_settings(const Values&,const Value* provider=nullptr);
const NumberFormatSettings& pabc_number_settings(const Values&);
}

#pragma once
#include "abc/core.hpp"
#include <array>
#include <span>

namespace abc {
struct Values;
enum class DecimalMethod : std::uint8_t {
    zero, one, minus_one, minimum, maximum,
    from_signed, from_unsigned, from_single, from_real, from_parts, from_bits, get_bits,
    parse, try_parse, to_string, to_int32, to_int64, to_uint32, to_uint64, to_single, to_real,
    negate, add, subtract, multiply, equal, unequal, less, less_equal, greater, greater_equal, compare,
    divide, remainder, round, round_scale, truncate, floor, ceiling,
    to_string_specifier, to_string_provider, to_string_specifier_provider
};
inline bool decimal_type(const TypeInfo& type) {return type.native_record==NativeRecord::decimal_value;}
bool decimal_record_shape(const TypeInfo&);
bool decimal_signature(std::span<const TypeInfo>,Type,std::size_t);
std::vector<Type> decimal_parameters(Type,std::size_t);
Type decimal_result(Type,std::size_t);
Value decimal_invoke(Values&,Type,std::size_t,std::span<const Value>);
std::u16string decimal_text(const Value&);
// Decimal fields are ordered by reflection in files written by PascalABC.NET.
inline constexpr std::array<std::size_t,4> decimal_file_order{3,2,0,1};
std::array<std::uint32_t,4> decimal_bits(const Value&);
Value decimal_from_bits(Values&,Type,const std::array<std::uint32_t,4>&);
}

#pragma once
#include "abc/core.hpp"
#include <span>

namespace abc {
struct Values;
enum class ComplexMethod : std::uint8_t {
    construct, from_real, from_tuple, from_polar, zero, one, imaginary_one,
    real_part, imaginary_part, magnitude, phase, negate, conjugate, reciprocal,
    add, subtract, multiply, divide, equal, not_equal, equals,
    square_root, exponential, logarithm, logarithm_base, logarithm10, power, sine, cosine, to_string
};
bool complex_signature(const Program&,Type,std::size_t);
std::vector<Type> complex_parameters(Type,std::size_t);
Type complex_result(Type,std::size_t);
Value complex_invoke(Values&,Type,std::size_t,std::span<const Value>);
std::u16string complex_pascal_text(const Value&,SourceLocation);
}

#pragma once
#include "abc/core.hpp"
#include <span>

namespace abc {
struct Values;
struct NumberFormatSettings;
enum class ComplexMethod : std::uint8_t {
    construct, from_real, from_tuple, from_polar, zero, one, imaginary_one,
    real_part, imaginary_part, magnitude, phase, negate, conjugate, reciprocal,
    add, subtract, multiply, divide, equal, not_equal, equals,
    square_root, exponential, logarithm, logarithm_base, logarithm10, power, sine, cosine,
    tangent, hyperbolic_sine, hyperbolic_cosine, hyperbolic_tangent, arc_sine, arc_cosine, arc_tangent,
    hash_code, equals_boxed, to_string, to_string_specifier, to_string_provider, to_string_specifier_provider
};
bool complex_signature(const Program&,Type,std::size_t);
std::vector<Type> complex_parameters(Type,std::size_t);
Type complex_result(Type,std::size_t);
Value complex_invoke(Values&,Type,std::size_t,std::span<const Value>);
std::u16string complex_pascal_text(const Value&,const NumberFormatSettings*,SourceLocation);
std::u16string complex_dotnet_text(const Value&,std::u16string_view,const NumberFormatSettings&,SourceLocation);
}

#pragma once
#include "abc/core.hpp"
#include <span>

namespace abc::stdlib {
enum class Function {
    abs, sqr, sign, minimum, maximum, sin, sinh, cos, cosh, tan, tanh,
    arcsin, arccos, arctan, exp, ln, log2, log10, logn, sqrt, cbrt, isqrt,
    power, round, trunc, int_part, frac, floor, ceil, deg_to_rad, rad_to_deg,
    length, copy, pos, last_pos, concat
};
struct Signature {
    std::string_view name;
    std::vector<Type> parameters;
    Type result;
    Function function;
};
// One catalog is shared by the type checker, bytecode verifier and VM.
std::span<const Signature> signatures();
std::optional<Value> constant(std::string_view name);
Value invoke(std::size_t id, std::span<const Value> arguments, SourceLocation location);
} // namespace abc::stdlib

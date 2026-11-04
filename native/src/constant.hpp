#pragma once
#include "hir.hpp"

namespace abc::hir {
// Returns no value for expressions requiring runtime state or library calls.
std::optional<Value> constant_value(const Expr& expression,const std::vector<TypeInfo>& types={});
}
namespace abc {
// Internal VM entry used only for an isolated verified scalar expression.
Value execute_scalar_constant(const Program& program);
}

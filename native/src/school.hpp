#pragma once
#include "stdlib.hpp"

namespace abc::stdlib {
Value invoke_school(Function function, std::span<const Value> arguments, SourceLocation location, const ExecutionOptions& options);
} // namespace abc::stdlib

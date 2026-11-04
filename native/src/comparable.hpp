#pragma once
#include "abc/core.hpp"
#include <span>

namespace abc {
bool comparable_type(const std::vector<TypeInfo>& types,Type type);
bool comparable_signature(const Program& program,Type type,ComparableMethod method);
std::vector<Type> comparable_parameters(Type type,ComparableMethod method);
Type comparable_result(Type type,ComparableMethod method);
Value comparable_invoke(const Program& program,Type type,ComparableMethod method,std::span<const Value> arguments,SourceLocation location,const ExecutionOptions& options);
} // namespace abc

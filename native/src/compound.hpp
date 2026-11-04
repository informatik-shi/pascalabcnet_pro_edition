#pragma once
#include "abc/core.hpp"
#include <span>
namespace abc {
struct Values;
bool compound_signature(const Program& program,Type owner,CompoundMethod method);
bool compound_reference(CompoundMethod method);
std::vector<Type> compound_parameters(const std::vector<TypeInfo>& types,Type owner,CompoundMethod method);
std::vector<Type> compound_parameters(const Program& program,Type owner,CompoundMethod method);
Type compound_result(Type owner,CompoundMethod method);
Value compound_invoke(Values& values,Type owner,CompoundMethod method,std::span<const Value> arguments,const ExecutionOptions& options);
}

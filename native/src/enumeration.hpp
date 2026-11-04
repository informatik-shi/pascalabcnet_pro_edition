#pragma once
#include "abc/core.hpp"
#include <span>

namespace abc {
bool enumeration(const std::vector<TypeInfo>& types,Type type);
bool enumeration(const Program& program,Type type);
std::vector<Type> enum_parameters(Type owner,EnumMethod method,Type other=Type::invalid);
Type enum_result(Type owner,EnumMethod method);
EnumValue enum_advance(EnumValue value,std::int64_t step,SourceLocation location);
std::u16string enum_text(const Program& program,EnumValue value,SourceLocation location,bool force_flags=false);
Value enum_invoke(const Program& program,Type owner,EnumMethod method,std::span<const Value> arguments,SourceLocation location);
}

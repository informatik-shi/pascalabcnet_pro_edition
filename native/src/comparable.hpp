#pragma once
#include "abc/core.hpp"
#include <span>

namespace abc {
struct Values;
Value comparable_invoke(Values&,Type,ComparableMethod,std::span<const Value>,const ExecutionOptions&);
int compare_culture_strings(std::u16string_view first,std::u16string_view second,SourceLocation location,
    const ExecutionOptions& options,bool ignore_case=false);
enum class TextSearch { anywhere, prefix, suffix };
bool find_culture_text(std::u16string_view text,std::u16string_view pattern,TextSearch mode,
    SourceLocation,const ExecutionOptions&);
bool comparable_type(const std::vector<TypeInfo>& types,Type type);
bool comparable_signature(const Program& program,Type type,ComparableMethod method);
std::vector<Type> comparable_parameters(Type type,ComparableMethod method);
Type comparable_result(Type type,ComparableMethod method);
Value comparable_invoke(const Program& program,Type type,ComparableMethod method,std::span<const Value> arguments,SourceLocation location,const ExecutionOptions& options);
} // namespace abc

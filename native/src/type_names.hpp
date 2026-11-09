#pragma once
#include "abc/core.hpp"
namespace abc {
enum class TypeOperation { value_name, runtime_type, name, full_name, name_space, assembly_qualified_name };
bool reflected_type_valid(const Program&,const ReflectedType&);
bool type_operation_valid(const Program&,Type,TypeOperation);
ReflectedType reflected_value_type(const Program&,const Value&,Type,SourceLocation,const ExecutionOptions&);
std::u16string reflected_type_name(const Program&,const ReflectedType&,SourceLocation,const ExecutionOptions&);
Value reflected_type_property(const Program&,const ReflectedType&,TypeOperation,SourceLocation,const ExecutionOptions&);
bool module_name_valid(std::string_view);
}

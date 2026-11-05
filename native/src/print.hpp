#pragma once
#include "abc/core.hpp"
#include <span>
#include <functional>
namespace abc {
struct Values;
enum class PrintMethod {scalar_print,scalar_println,sequence_print,sequence_println,sequence_map_lines,matrix,matrix_real};
Type print_sequence_type(const std::vector<TypeInfo>& types,Type element);
bool print_signature(const Program& program,Type self,PrintMethod method,Type callback=Type::invalid);
Type print_result(Type self,PrintMethod method);
std::vector<Type> print_parameters(Type self,PrintMethod method,Type callback=Type::invalid);
Value print_invoke(Values& values,Type self,PrintMethod method,Type callback,std::span<const Value> arguments,
    const std::function<const Value&()>& delimiter,const std::function<bool()>& matrix_format,
    const std::function<void(std::u16string_view)>& emit,const ExecutionOptions& options);
}

#pragma once
#include "runtime_values.hpp"
#include <span>

namespace abc {
inline std::vector<Type> array_operation_parameters(const Program& program,Type array,ArrayOperation method) {
    const auto element=type_info(program,array).element;
    if(method>=ArrayOperation::index){std::vector<Type> result{array,element};if(method==ArrayOperation::index_from || method==ArrayOperation::index_range || method==ArrayOperation::last_from || method==ArrayOperation::last_range)result.push_back(Type::integer);if(method==ArrayOperation::index_range || method==ArrayOperation::last_range)result.push_back(Type::integer);return result;}
    if (method==ArrayOperation::fill_array) return {Type::integer,element};
    if (method==ArrayOperation::fill_matrix) return {Type::integer,Type::integer,element};
    if (method==ArrayOperation::equal_array || method==ArrayOperation::equal_matrix) return {array,array};
    return {array};
}
bool array_operation_signature_valid(const Program& program,Type array,Type result,ArrayOperation method);
Value array_operation(Values& values,Type array,Type result,ArrayOperation method,std::span<const Value> arguments,const ExecutionOptions& options);
}

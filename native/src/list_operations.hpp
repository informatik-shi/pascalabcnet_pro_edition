#pragma once
#include "runtime_values.hpp"
#include <span>

namespace abc {
inline Type list_operation_result(Type source,ListOperation method) {
    return method==ListOperation::remove ? Type::boolean : method==ListOperation::insert || method==ListOperation::remove_range ? Type::unit
        : method==ListOperation::get_range ? source : Type::integer;
}
inline std::vector<Type> list_operation_parameters(Type element,ListOperation method) {
    if (method==ListOperation::insert) return {Type::integer,element};
    if (method==ListOperation::get_range || method==ListOperation::remove_range) return {Type::integer,Type::integer};
    if (method==ListOperation::index_range || method==ListOperation::last_range) return {element,Type::integer,Type::integer};
    if (method==ListOperation::index_from || method==ListOperation::last_from) return {element,Type::integer};
    return {element};
}
bool list_operation_signature_valid(const Program& program,Type source,Type result,ListOperation method);
Value list_operation(Values& values,const std::shared_ptr<Aggregate>& source,ListOperation method,std::span<const Value> arguments,const ExecutionOptions& options);
}

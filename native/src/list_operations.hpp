#pragma once
#include "runtime_values.hpp"
#include <span>

namespace abc {
inline std::vector<Type> list_copy_parameters(Type array,ListCopy method) {
    if (method==ListCopy::range) return {Type::integer,array,Type::integer,Type::integer};
    return method==ListCopy::offset ? std::vector<Type>{array,Type::integer} : std::vector<Type>{array};
}
bool list_copy_signature_valid(const Program& program,Type list,Type array,ListCopy method);
void list_copy(Values& values,const std::shared_ptr<Aggregate>& source,ListCopy method,std::span<const Value> arguments,const ExecutionOptions& options);
inline Type list_capacity_result(Type list,ListCapacity method) {
    return method==ListCapacity::construct ? list : method==ListCapacity::set || method==ListCapacity::trim ? Type::unit : Type::integer;
}
inline bool list_capacity_argument(ListCapacity method) {
    return method==ListCapacity::set || method==ListCapacity::ensure || method==ListCapacity::construct;
}
Value list_capacity(Values& values,Type type,const std::shared_ptr<Aggregate>& source,ListCapacity method,std::int32_t capacity,const ExecutionOptions& options);
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
bool list_bulk_signature_valid(const Program& program,Type target,Type input);
void list_bulk(Values& values,const std::shared_ptr<Aggregate>& target,const std::shared_ptr<Aggregate>& input,bool insert,std::int32_t index,const ExecutionOptions& options);
bool list_operation_signature_valid(const Program& program,Type source,Type result,ListOperation method);
bool list_contains(Values&,const std::shared_ptr<Aggregate>&,const Value&,const ExecutionOptions&);
Value list_operation(Values& values,const std::shared_ptr<Aggregate>& source,ListOperation method,std::span<const Value> arguments,const ExecutionOptions& options);
}

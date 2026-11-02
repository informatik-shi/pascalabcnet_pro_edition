#pragma once
#include "sequences.hpp"
#include "runtime_values.hpp"
#include <span>

namespace abc {
inline bool matrix_column(MatrixMethod method) {
    return method==MatrixMethod::column || method==MatrixMethod::column_sequence || method==MatrixMethod::columns
        || method==MatrixMethod::column_sequences || method==MatrixMethod::swap_columns || method==MatrixMethod::set_column;
}
inline bool matrix_sequence(MatrixMethod method) {
    return method==MatrixMethod::row_sequence || method==MatrixMethod::column_sequence || method==MatrixMethod::row_sequences || method==MatrixMethod::column_sequences;
}
inline bool matrix_outer(MatrixMethod method) {
    return method==MatrixMethod::rows || method==MatrixMethod::columns || method==MatrixMethod::row_sequences || method==MatrixMethod::column_sequences;
}
inline bool matrix_swap(MatrixMethod method) { return method==MatrixMethod::swap_rows || method==MatrixMethod::swap_columns; }
inline bool matrix_set(MatrixMethod method) { return method==MatrixMethod::set_row || method==MatrixMethod::set_column; }
inline std::vector<Type> matrix_parameters(Type array,MatrixMethod method) {
    if (matrix_outer(method)) return {};
    if (matrix_swap(method)) return {Type::integer,Type::integer};
    if (matrix_set(method)) return {Type::integer,array};
    return {Type::integer};
}
bool matrix_signature_valid(const Program& program,Type source,Type result,Type array,MatrixMethod method);
Value matrix_operation(Values& values,Type type,Type array_type,const std::shared_ptr<Aggregate>& source,MatrixMethod method,std::span<const Value> arguments,const ExecutionOptions& options);
bool matrix_line_resume(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options);
}

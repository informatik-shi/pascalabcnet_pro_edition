#pragma once
#include "runtime_values.hpp"
#include <span>

namespace abc {
inline bool matrix_build_flat(MatrixBuild method) {
    return method==MatrixBuild::flat_values || method==MatrixBuild::rows_flat || method==MatrixBuild::columns_flat;
}
inline std::vector<Type> matrix_build_parameters(Type input,MatrixBuild method) {
    if (matrix_build_flat(method)) return {Type::integer,Type::integer,input};
    return {input};
}
bool matrix_build_signature_valid(const Program& program,Type result,Type input,MatrixBuild method);
Value matrix_build(Values& values,Type type,MatrixBuild method,std::span<const Value> arguments,const ExecutionOptions& options);
}

#pragma once
#include "sequences.hpp"
#include <span>

namespace abc {
inline bool generator_index(GeneratorMethod method) { return method==GeneratorMethod::index_sequence || method==GeneratorMethod::index_array; }
inline bool generator_pair(GeneratorMethod method) { return method==GeneratorMethod::iterate_two || method==GeneratorMethod::sequence_two || method==GeneratorMethod::array_two || method==GeneratorMethod::while_two; }
inline bool generator_while(GeneratorMethod method) { return method==GeneratorMethod::while_one || method==GeneratorMethod::while_two; }
inline bool generator_bounded(GeneratorMethod method) { return method==GeneratorMethod::sequence_one || method==GeneratorMethod::sequence_two; }
inline bool generator_sequence(GeneratorMethod method) { return method==GeneratorMethod::index_sequence || method==GeneratorMethod::iterate_one || method==GeneratorMethod::iterate_two || generator_bounded(method) || generator_while(method); }
inline std::vector<Type> generator_parameters(Type element,Type callback,GeneratorMethod method) {
    if (generator_index(method)) return {Type::integer,callback,Type::integer};
    if (method==GeneratorMethod::matrix) return {Type::integer,Type::integer,callback};
    std::vector<Type> result;
    if (generator_bounded(method) || method==GeneratorMethod::array_one || method==GeneratorMethod::array_two) result.push_back(Type::integer);
    result.push_back(element); if (generator_pair(method)) result.push_back(element); result.push_back(callback);
    return result;
}
bool generator_signature_valid(const Program& program,Type result,Type callback,GeneratorMethod method);
Value generate_values(Values& values,Type type,GeneratorMethod method,std::span<const Value> arguments,const ExecutionOptions& options,std::shared_ptr<Aggregate> predicate={});
bool generator_resume(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options);
}

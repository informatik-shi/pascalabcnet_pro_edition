#pragma once
#include "abc/core.hpp"
#include <span>

namespace abc {
struct Values;
enum class SortedSetMethod {
    construct, construct_source, construct_comparer, construct_source_comparer,
    count, minimum, maximum, comparer, contains, add, remove, clear, membership,
    copy_array, copy_index, copy_count, try_get_value, reverse
};
Type sorted_set_type_for(const std::vector<TypeInfo>&,Type element,CompoundKind kind);
bool sorted_set_signature(const Program&,Type owner,SortedSetMethod);
std::vector<Type> sorted_set_parameters(const std::vector<TypeInfo>&,Type owner,SortedSetMethod);
Type sorted_set_result(const std::vector<TypeInfo>&,Type owner,SortedSetMethod);
Value sorted_set_invoke(Values&,Type owner,SortedSetMethod,std::span<const Value>,const ExecutionOptions&);
}

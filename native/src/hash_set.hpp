#pragma once
#include "abc/core.hpp"
#include <span>
#include <unordered_map>

namespace abc {
struct Values;
struct HashSetState {
    std::unordered_multimap<std::size_t,std::size_t> lookup;
    std::vector<std::size_t> hashes,free;
    std::size_t capacity=0;
    // HashSet physical slot/bucket arrays; Dictionary retains its existing lookup.
    std::shared_ptr<Aggregate> comparer;
    std::vector<std::shared_ptr<Cell>> slots;
    std::vector<std::int32_t> codes,next,buckets;
    std::int32_t last_index=0,free_head=-1;
};
struct HashSetCursor {
    std::size_t slot=0;
    std::uint64_t index=0;
    bool positioned=false,available=false;
    Value current=std::int32_t{0};
};
enum class HashSetMethod {
    construct,construct_sequence,construct_copy,count,add,remove,contains,membership,clear,
    union_with,except_with,intersect_with,symmetric_with,set_equals,subset,proper_subset,superset,proper_superset,overlaps,
    equal,not_equal,unite,subtract,intersect,add_copy,remove_copy,less,less_equal,greater,greater_equal,
    ref_add,ref_remove,ref_union,ref_except,
    copy_array,copy_index,copy_count,remove_where,trim_excess,try_get_value,copy_value,construct_comparer,construct_sequence_comparer,comparer
};
bool hash_set_reference(HashSetMethod method);
bool hash_set_sequence_method(HashSetMethod method);
bool hash_set_signature(const Program& program,Type owner,Type sequence,HashSetMethod method);
std::vector<Type> hash_set_parameters(const std::vector<TypeInfo>& types,Type owner,Type sequence,HashSetMethod method);
Type hash_set_result(Type owner,HashSetMethod method,Type payload=Type::invalid);
Value hash_set_invoke(Values& values,Type owner,Type sequence,HashSetMethod method,std::span<const Value> arguments,const ExecutionOptions& options);
bool hash_set_resume(Values& values,const std::shared_ptr<Aggregate>& cursor,std::uint64_t index,const ExecutionOptions& options);
}

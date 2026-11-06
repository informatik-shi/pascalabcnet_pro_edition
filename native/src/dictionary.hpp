#pragma once
#include "abc/core.hpp"
#include <span>
namespace abc {
struct Values;
enum class DictionaryMethod {
 construct,construct_capacity,construct_copy,from_pairs,from_tuples,from_sequences,
 count,add,get_item,set_item,contains_key,contains_value,try_get_value,remove,clear,
 get_default,get_or_add_value,get_or_add_factory,keys,values,copy_value,update,membership,
 merge,without_key,without_keys,remove_keys,pair_construct,pair_to_string,view_count,view_contains,view_copy_to,each_count,each,to_dictionary,to_dictionary_elements,
 get_enumerator,enumerator_move,enumerator_move_copy,enumerator_current,enumerator_dispose
};
std::shared_ptr<Aggregate> dictionary_sequence_source(const SequenceState& state,SourceLocation location);
bool dictionary_sequence_resume(Values& values,const std::shared_ptr<Aggregate>& cursor,std::uint64_t index,const ExecutionOptions& options);
bool dictionary_enumerator_metadata(const Program& program,Type iterator);
Type dictionary_value_type(const std::vector<TypeInfo>& types,Type owner);
bool dictionary_signature(const Program& program,Type owner,Type payload,DictionaryMethod method);
std::vector<Type> dictionary_parameters(const std::vector<TypeInfo>& types,Type owner,Type payload,DictionaryMethod method);
Type dictionary_result(const std::vector<TypeInfo>& types,Type owner,Type payload,DictionaryMethod method);
Value dictionary_invoke(Values& values,Type owner,Type payload,DictionaryMethod method,std::span<const Value> arguments,const ExecutionOptions& options);
}

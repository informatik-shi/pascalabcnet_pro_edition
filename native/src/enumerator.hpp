#pragma once
#include "abc/core.hpp"
#include <span>
namespace abc {
struct Values;
enum class EnumeratorMethod { get,box_dictionary,move,current,dispose,reset,get_collection,box_collection,collection_move,collection_move_copy,collection_current,collection_dispose,collection_reset };
bool collection_enumerator_metadata(const Program&,Type);
Type enumerator_parameter(Type owner,EnumeratorMethod);
bool enumerator_signature(const Program&,Type owner,Type payload,EnumeratorMethod);
Type enumerator_result(const Program&,Type owner,Type payload,EnumeratorMethod);
Value enumerator_invoke(Values&,Type owner,Type payload,EnumeratorMethod,const Value&,const ExecutionOptions&);
}

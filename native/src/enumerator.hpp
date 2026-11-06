#pragma once
#include "abc/core.hpp"
#include <span>
namespace abc {
struct Values;
enum class EnumeratorMethod { get,box_dictionary,move,current,dispose,reset };
bool enumerator_signature(const Program&,Type owner,Type payload,EnumeratorMethod);
Type enumerator_result(const Program&,Type owner,Type payload,EnumeratorMethod);
Value enumerator_invoke(Values&,Type owner,Type payload,EnumeratorMethod,const Value&,const ExecutionOptions&);
}

#pragma once
#include "abc/core.hpp"
#include <span>
namespace abc {
struct Values;
enum class NewSetMethod {
    construct,construct_array,cast_array,cast_hash,copy_value,clone,hash,raw_hash,raw_reference,
    count,add,remove,contains,add_range,to_string,to_hash,assign,
    item_add,item_remove,union_assign,except_assign,intersect_assign,
    unite,subtract,intersect,equal,not_equal,less,less_equal,greater,greater_equal,membership
};
bool new_set_signature(const Program& program,Type owner,NewSetMethod method,bool reference);
std::vector<Type> new_set_parameters(const std::vector<TypeInfo>& types,Type owner,NewSetMethod method,bool reference);
Type new_set_result(const std::vector<TypeInfo>& types,Type owner,NewSetMethod method);
Value new_set_invoke(Values& values,Type owner,NewSetMethod method,bool reference,std::span<const Value> arguments,const ExecutionOptions& options);
std::shared_ptr<Aggregate> new_set_hash(Values& values,const NewSetValue& value);
std::shared_ptr<Aggregate> new_set_sequence(Values& values,Type sequence,NewSetValue value);
}

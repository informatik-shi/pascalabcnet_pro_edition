#pragma once
#include "abc/core.hpp"
#include <span>
namespace abc {
struct Values;
enum class NewSetMethod {
    construct,construct_array,cast_array,cast_hash,copy_value,clone,hash,raw_hash,raw_reference,
    count,add,remove,contains,add_range,to_string,to_hash,assign,
    item_add,item_remove,union_assign,except_assign,intersect_assign,
    unite,subtract,intersect,equal,not_equal,less,less_equal,greater,greater_equal,membership,literal_ranges,from_sequence,include,exclude,
    empty_unite_left,empty_unite_right,empty_subtract_left,empty_subtract_right,empty_intersect_left,empty_intersect_right,
    empty_equal_left,empty_equal_right,empty_not_equal_left,empty_not_equal_right,empty_less_left,empty_less_right,
    empty_less_equal_left,empty_less_equal_right,empty_greater_left,empty_greater_right,empty_greater_equal_left,empty_greater_equal_right,
    empty_array_unite_left,empty_array_unite_right,empty_array_subtract_left,empty_array_subtract_right,empty_array_intersect_left,empty_array_intersect_right
};
enum class EmptyMethod { singleton,convert,binary,to_string,to_set };
Type empty_collection_type(const std::vector<TypeInfo>& types);
bool empty_signature(const Program& program,Type target,EmptyMethod method);
std::vector<Type> empty_parameters(const std::vector<TypeInfo>& types,EmptyMethod method);
Value empty_invoke(Values& values,Type target,EmptyMethod method,std::span<const Value> arguments,const ExecutionOptions& options);
bool new_set_signature(const Program& program,Type owner,NewSetMethod method,bool reference);
std::vector<Type> new_set_parameters(const std::vector<TypeInfo>& types,Type owner,NewSetMethod method,bool reference);
Type new_set_result(const std::vector<TypeInfo>& types,Type owner,NewSetMethod method);
Value new_set_invoke(Values& values,Type owner,NewSetMethod method,bool reference,std::span<const Value> arguments,const ExecutionOptions& options);
std::shared_ptr<Aggregate> new_set_hash(Values& values,const NewSetValue& value);
std::shared_ptr<Aggregate> new_set_sequence(Values& values,Type sequence,NewSetValue value);
}

#pragma once
#include "abc/core.hpp"
#include <span>
namespace abc {
struct Values;
enum class FixedMethod:std::uint8_t { view,index_view,index,copy_value,copy_assign,equal,not_equal };
const FixedArrayShape& fixed_array_shape(const Program&,Type);
bool pascal_copy_type(const std::vector<TypeInfo>&,Type);
Value fixed_array_construct(Values&,Type);
Value pascal_value_copy(Values&,Type,const Value&,bool initialize_record);
bool fixed_signature_valid(const Program&,Type,std::size_t);
std::vector<Type> fixed_parameters(const std::vector<TypeInfo>&,std::span<const FixedArrayShape>,Type,std::size_t);
Type fixed_result(const std::vector<TypeInfo>&,Type,std::size_t);
Value fixed_invoke(Values&,Type,std::size_t,std::span<const Value>);
}

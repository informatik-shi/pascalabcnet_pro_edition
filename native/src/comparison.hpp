#pragma once
#include "abc/core.hpp"
#include <span>
namespace abc {
struct Values;
Type comparable_contract(const std::vector<TypeInfo>&,Type element);
bool default_comparable(const std::vector<TypeInfo>&,Type element);
int comparison_value(Values&,Type element,const Value&,const Value&,bool null_safe,bool ordinal_strings,const ExecutionOptions&);
Value comparer_create(Values&,Type owner,const Value* callback,const ExecutionOptions&);
Value comparer_invoke(Values&,Type owner,const Value& receiver,std::span<const Value>,const ExecutionOptions&);
bool comparer_is_default(const Aggregate&);
}

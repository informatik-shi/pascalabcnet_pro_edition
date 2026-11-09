#pragma once
#include "abc/core.hpp"
namespace abc {
struct Values;
bool callable_compatible(const std::vector<TypeInfo>&,Type,Type);
bool callable_equal(const Aggregate&,const Aggregate&);
std::size_t callable_hash(const Aggregate&);
Value callable_conversion(Values&,Type,const Value&,bool require_non_nil);
}

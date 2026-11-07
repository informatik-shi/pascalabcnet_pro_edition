#pragma once
#include "abc/core.hpp"
#include <span>
namespace abc {
struct Values;
Type equatable_contract(const std::vector<TypeInfo>&,Type);
bool default_equatable(const std::vector<TypeInfo>&,Type);
bool equality_value(Values&,Type,const Value&,const Value&,const ExecutionOptions&);
std::int32_t equality_hash(Values&,Type,const Value&,const ExecutionOptions&);
Value equality_create(Values&,Type,const ExecutionOptions&);
Value equality_invoke(Values&,Type,std::size_t,const Value&,std::span<const Value>,const ExecutionOptions&);
std::uint32_t equality_implementation(const Program&,Type);
}

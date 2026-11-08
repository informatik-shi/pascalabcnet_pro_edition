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
Type equality_contract(const std::vector<TypeInfo>&,Type);
bool equality_default(const Program&,const std::shared_ptr<Aggregate>&);
bool equality_policy_same(const Program&,const std::shared_ptr<Aggregate>&,const std::shared_ptr<Aggregate>&);
bool equality_policy_value(Values&,Type,const std::shared_ptr<Aggregate>&,const Value&,const Value&,const ExecutionOptions&);
std::int32_t equality_policy_hash(Values&,Type,const std::shared_ptr<Aggregate>&,const Value&,const ExecutionOptions&);
std::uint32_t equality_implementation(const Program&,Type);
std::int32_t equality_index(Values&,Type,std::span<const std::shared_ptr<Cell>>,const Value&,std::int32_t,std::int32_t,bool,const ExecutionOptions&,std::uint64_t* budget=nullptr);
}

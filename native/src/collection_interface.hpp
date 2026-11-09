#pragma once
#include "abc/core.hpp"
#include <span>
namespace abc {
struct Values;
bool native_collection_type(const std::vector<TypeInfo>&,Type from,Type to);
bool native_enumerable_type(const std::vector<TypeInfo>&,Type from,Type to);
Type sequence_collection_owner(const Program&,const std::shared_ptr<Aggregate>&);
bool native_collection_target(const Program&,Type owner,const Value& receiver);
Value native_collection_invoke(Values&,Type owner,std::size_t slot,const Value& receiver,std::span<const Value>,const ExecutionOptions&);
std::shared_ptr<Aggregate> native_collection_sequence(Values&,Type target,const std::shared_ptr<Aggregate>&);
}

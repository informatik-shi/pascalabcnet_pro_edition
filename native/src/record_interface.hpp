#pragma once
#include "abc/core.hpp"
#include <span>
namespace abc {
struct Values;
bool native_record_interface_type(const std::vector<TypeInfo>&,Type from,Type to);
bool native_record_interface_target(const Program&,Type owner,const Value&);
Value native_record_interface_invoke(Values&,Type owner,std::size_t slot,const Value&,std::span<const Value>,const ExecutionOptions&);
std::shared_ptr<Aggregate> record_interface_copy(Values&,const std::shared_ptr<Aggregate>&,bool boxed);
inline const Aggregate* record_box_identity(const Aggregate* object){
 return object&&object->sequence&&object->sequence->provider==SequenceProvider::ordinal_range&&object->sequence->source&&object->sequence->source->boxed?object->sequence->source.get():object;
}
inline bool record_box_reference(const Aggregate* object){return object&&(object->boxed||record_box_identity(object)!=object);}
inline const std::shared_ptr<Aggregate>& record_box_value(const std::shared_ptr<Aggregate>& object){return record_box_identity(object.get())!=object.get()?object->sequence->source:object;}
}

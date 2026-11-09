#include "collection_interface.hpp"
#include "interfaces.hpp"
#include "runtime_values.hpp"
#include "sequences.hpp"
#include "enumerator.hpp"
#include "equality.hpp"
#include "list_operations.hpp"
#include "dictionary.hpp"
#include <array>
namespace abc {
namespace {
using Object=std::shared_ptr<Aggregate>;
bool native_kind(const TypeInfo& type){return (type.kind==CompoundKind::array&&type.rank==1)||type.kind==CompoundKind::list||type.kind==CompoundKind::hash_set||type.kind==CompoundKind::dictionary||type.kind==CompoundKind::dictionary_keys||type.kind==CompoundKind::dictionary_values;}
void checkpoint(Values& values,const ExecutionOptions& options,std::uint64_t& work){if(work>=options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,values.location);if((work++&255U)==0&&options.cancelled&&options.cancelled())throw runtime_error(RuntimeError::cancelled,values.location);}
}
bool native_collection_type(const std::vector<TypeInfo>& types,Type from,Type to){
 if(!compound(from)||!compound(to)||static_cast<unsigned>(from)-64U>=types.size()||static_cast<unsigned>(to)-64U>=types.size())return false;
 const auto& source=types[static_cast<unsigned>(from)-64U];const auto& target=types[static_cast<unsigned>(to)-64U];return source.element==target.element&&((target.kind==CompoundKind::collection_interface&&native_kind(source))||(target.kind==CompoundKind::list_interface&&((source.kind==CompoundKind::array&&source.rank==1)||source.kind==CompoundKind::list))||(target.kind==CompoundKind::dictionary_interface&&source.kind==CompoundKind::dictionary&&source.key==target.key));
}
bool native_enumerable_type(const std::vector<TypeInfo>& types,Type from,Type to){
 if(!compound(from)||!compound(to)||static_cast<unsigned>(from)-64U>=types.size()||static_cast<unsigned>(to)-64U>=types.size())return false;
 const auto& source=types[static_cast<unsigned>(from)-64U];const auto& target=types[static_cast<unsigned>(to)-64U];return target.kind==CompoundKind::sequence&&native_kind(source)&&source.element==target.element;
}
Type sequence_collection_owner(const Program& program,const Object& sequence){
 if(!sequence||sequence->nil||!sequence->sequence||sequence->sequence->provider!=SequenceProvider::user_source)return Type::invalid;
 const auto source=sequence->sequence->source;if(!source||source->nil)return Type::invalid;
 const auto element=type_info(program,sequence->type).element;
 for(std::size_t i=0;i<program.types.size();++i)if(program.types[i].kind==CompoundKind::collection_interface&&program.types[i].element==element&&interface_compatible(program.types,source->type,static_cast<Type>(64+i)))return static_cast<Type>(64+i);
 return Type::invalid;
}
bool native_collection_target(const Program& program,Type owner,const Value& value){const auto* object=std::get_if<Object>(&value);return object&&*object&&native_collection_type(program.types,(*object)->type,owner);}
std::shared_ptr<Aggregate> native_collection_sequence(Values& values,Type target,const Object& source){
 const auto kind=type_info(values.program,source->type).kind;
 const auto provider=kind==CompoundKind::array?SequenceProvider::array:kind==CompoundKind::list?SequenceProvider::list:kind==CompoundKind::hash_set?SequenceProvider::hash_set:kind==CompoundKind::dictionary?SequenceProvider::dictionary_entries:kind==CompoundKind::dictionary_keys?SequenceProvider::dictionary_keys:SequenceProvider::dictionary_values;
 return sequence_create(values,target,source,provider);
}
Value native_collection_invoke(Values& values,Type owner,std::size_t slot,const Value& receiver,std::span<const Value> arguments,const ExecutionOptions& options){
 const bool indexed=type_info(values.program,owner).kind==CompoundKind::list_interface;
 const bool dictionary=type_info(values.program,owner).kind==CompoundKind::dictionary_interface;
 if(!native_collection_target(values.program,owner,receiver)||slot>=(dictionary?16U:indexed?13U:8U)||arguments.size()!=interface_contract(values.program.types,owner)[slot].parameters.size())throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 const auto object=std::get<Object>(receiver);if(object->nil)throw runtime_error(RuntimeError::nil_reference,values.location);std::uint64_t work=0;checkpoint(values,options,work);
 const auto shape=type_info(values.program,object->type);const auto kind=shape.kind;const bool view=kind==CompoundKind::dictionary_keys||kind==CompoundKind::dictionary_values;
 const bool fixed=kind==CompoundKind::array,readonly=fixed||view;
 if(slot==1)return readonly;
 if((slot==2||slot==3||slot==6)&&readonly)throw Error("E_READONLY",values.location,fixed?"Collection has a fixed size":"Collection is read-only",fixed?"Коллекция имеет фиксированный размер":"Коллекция доступна только для чтения");
 if(dictionary&&slot>=8){
  std::vector<Value> input{receiver};input.insert(input.end(),arguments.begin(),arguments.end());Type payload=Type::invalid;
  if(slot==8||slot==9){const auto view_kind=slot==8?CompoundKind::dictionary_keys:CompoundKind::dictionary_values;for(std::size_t i=0;i<values.program.types.size();++i)if(values.program.types[i].kind==view_kind&&values.program.types[i].key==object->type){payload=static_cast<Type>(64+i);break;}if(payload==Type::invalid)throw runtime_error(RuntimeError::invalid_bytecode,values.location);}
  const auto method=slot==8?DictionaryMethod::keys:slot==9?DictionaryMethod::values:slot==10?DictionaryMethod::get_item:slot==11?DictionaryMethod::set_item:slot==12?DictionaryMethod::contains_key:slot==13?DictionaryMethod::add:slot==14?DictionaryMethod::remove:DictionaryMethod::try_get_value;
  return dictionary_invoke(values,object->type,payload,method,input,options);
 }
 if(slot>=8){
  if(fixed&&(slot==11||slot==12))throw Error("E_READONLY",values.location,"Collection has a fixed size","Коллекция имеет фиксированный размер");
  if(slot==10)return kind==CompoundKind::list?list_operation(values,object,ListOperation::index,arguments,options):Value{equality_index(values,shape.element,object->items,arguments[0],0,static_cast<std::int32_t>(object->items.size()),false,options)};
  if(slot==11)return list_operation(values,object,ListOperation::insert,arguments,options);
  const auto index=std::get<std::int32_t>(arguments[0]);if(index<0||static_cast<std::size_t>(index)>=object->items.size())throw runtime_error(RuntimeError::list_index,values.location);
  if(slot==8)return values.copy(object->items[static_cast<std::size_t>(index)]->value);
  if(slot==9){values.write(object->items[static_cast<std::size_t>(index)],arguments[1]);return false;}
  values.list_erase(object,static_cast<std::size_t>(index),1);++object->version;return false;
 }
 if(slot==7){auto sequence=type_info(values.program,owner).interfaces.read().parents[0];if(indexed||dictionary)sequence=type_info(values.program,sequence).interfaces.read().parents[0];const auto iterator=interface_contract(values.program.types,sequence)[0].result;return enumerator_invoke(values,sequence,iterator,EnumeratorMethod::get,native_collection_sequence(values,sequence,object),options);}
 if(kind==CompoundKind::list){
  if(slot==0)return static_cast<std::int32_t>(object->items.size());
  if(slot==2){const std::array<Value,1> item{arguments[0]};values.list_insert_values(object,object->items.size(),item);++object->version;return false;}
  if(slot==3){values.list_erase(object,0,object->items.size());++object->version;return false;}
  if(slot==4)return list_contains(values,object,arguments[0],options);
  if(slot==5){list_copy(values,object,ListCopy::offset,arguments,options);return false;}
  if(slot==6)return list_operation(values,object,ListOperation::remove,arguments,options);
 }
 if(kind==CompoundKind::array){
  if(slot==0)return static_cast<std::int32_t>(object->items.size());
  if(slot==4)return equality_index(values,shape.element,object->items,arguments[0],0,static_cast<std::int32_t>(object->items.size()),false,options)>=0;
  if(slot==5){const auto destination=std::get<Object>(arguments[0]);if(!destination||destination->nil)throw runtime_error(RuntimeError::nil_reference,values.location);const auto index=std::get<std::int32_t>(arguments[1]);if(index<0)throw runtime_error(RuntimeError::list_copy_destination_index,values.location);if(static_cast<std::size_t>(index)>destination->items.size()||object->items.size()>destination->items.size()-static_cast<std::size_t>(index))throw runtime_error(RuntimeError::list_copy_destination_range,values.location);for(std::size_t i=0;i<object->items.size();++i){checkpoint(values,options,work);values.write(destination->items[static_cast<std::size_t>(index)+i],object->items[i]->value);}return false;}
 }
 std::vector<Value> input;input.reserve(arguments.size()+1);input.push_back(receiver);input.insert(input.end(),arguments.begin(),arguments.end());
 if(kind==CompoundKind::hash_set){
  const auto method=slot==0?HashSetMethod::count:slot==2?HashSetMethod::add:slot==3?HashSetMethod::clear:slot==4?HashSetMethod::contains:slot==5?HashSetMethod::copy_index:HashSetMethod::remove;
  return hash_set_invoke(values,object->type,Type::invalid,method,input,options);
 }
 if(view){const auto method=slot==0?DictionaryMethod::view_count:slot==4?DictionaryMethod::view_contains:DictionaryMethod::view_copy_to;return dictionary_invoke(values,object->type,slot==5?interface_contract(values.program.types,owner)[5].parameters[0].type:Type::invalid,method,input,options);}
 if(kind==CompoundKind::dictionary){
  if(slot==2){const auto pair=std::get<Object>(arguments[0]);const std::array<Value,3> add{receiver,pair->items[0]->value,pair->items[1]->value};return dictionary_invoke(values,object->type,Type::invalid,DictionaryMethod::add,add,options);}
  if(slot==6){if(!std::get<bool>(dictionary_invoke(values,object->type,Type::invalid,DictionaryMethod::pair_contains,input,options)))return false;const auto pair=std::get<Object>(arguments[0]);const std::array<Value,2> remove{receiver,pair->items[0]->value};(void)dictionary_invoke(values,object->type,Type::invalid,DictionaryMethod::remove,remove,options);return true;}
  const auto method=slot==0?DictionaryMethod::count:slot==3?DictionaryMethod::clear:slot==4?DictionaryMethod::pair_contains:DictionaryMethod::pair_copy_to;
  return dictionary_invoke(values,object->type,slot==5?interface_contract(values.program.types,owner)[5].parameters[0].type:Type::invalid,method,input,options);
 }
 throw runtime_error(RuntimeError::invalid_bytecode,values.location);
}
}

#include "enumerator.hpp"
#include "runtime_values.hpp"
#include "dictionary.hpp"
#include "value_hash.hpp"
#include "sequences.hpp"
#include "new_set.hpp"
#include "interfaces.hpp"
#include <array>
namespace abc {
namespace {
using Object=std::shared_ptr<Aggregate>;
bool valid(const Program& p,Type type){return compound(type) && static_cast<unsigned>(type)-64U<p.types.size();}
bool flat(SequenceProvider p){return collection_sequence(p) || p==SequenceProvider::hash_set || p==SequenceProvider::new_set || dictionary_provider(p) || p==SequenceProvider::string;}
bool no_dispose(const SequenceState& s){return flat(s.provider) || (s.provider==SequenceProvider::generated && (s.generator_method==GeneratorMethod::iterate_one || s.generator_method==GeneratorMethod::iterate_two)) || s.provider==SequenceProvider::repeat || ((s.provider==SequenceProvider::range || s.provider==SequenceProvider::stepped_range) && s.seed && s.seed->type!=Type::character) || s.provider==SequenceProvider::big_range || s.provider==SequenceProvider::partition || s.provider==SequenceProvider::real_range;}
bool clears(Values& values,const SequenceState& s){
 if(s.provider==SequenceProvider::generated && s.generator_method==GeneratorMethod::index_sequence)return true;
 if((s.provider==SequenceProvider::range || s.provider==SequenceProvider::stepped_range) && s.seed && s.seed->type==Type::character)return true;
 if(s.provider==SequenceProvider::file_lines || s.provider==SequenceProvider::directory)return true;
 if(s.provider!=SequenceProvider::transform || (s.transform_method!=SequenceTransform::where && s.transform_method!=SequenceTransform::select))return false;
 const auto arity=s.callable ? type_info(values.program,s.callable->type).parameters.size() : values.program.routines.at(s.callback).parameters.size();
 return arity==1;
}
Object make(Values& values,Type type){
 auto result=std::get<Object>(values.zero(type,true));result->enumerator=std::make_unique<EnumeratorState>();
 const auto element=type_info(values.program,type).element;result->items.push_back(values.cell(element,values.zero(element,false,true)));return result;
}
Value boxed(Values& values,const std::shared_ptr<Cell>& cell,DictionaryMethod method,const ExecutionOptions& options){
 const bool reference=method==DictionaryMethod::enumerator_move || method==DictionaryMethod::enumerator_reset;
 const std::array<Value,1> args{reference ? Value{cell} : cell->value};
 return dictionary_invoke(values,cell->type,Type::invalid,method,args,options);
}
}
bool collection_enumerator_metadata(const Program& p,Type type){
 if(!valid(p,type))return false;const auto& e=type_info(p,type);
 if(!collection_enumerator_type(e) || e.kind!=CompoundKind::record || e.name!="collectionenumerator" || !valid(p,e.key) || e.fields.size()!=4)return false;
 const auto& owner=type_info(p,e.key);if((owner.kind!=CompoundKind::list && owner.kind!=CompoundKind::hash_set) || e.element!=owner.element)return false;
 const std::array<std::string_view,4> names{"$source","$version","$index","$current"};const std::array<Type,4> fields{e.key,Type::uint64,Type::integer,e.element};
 for(std::size_t i=0;i<4;++i)if(e.fields[i].name!=names[i] || e.fields[i].type!=fields[i] || e.fields[i].global)return false;
 return true;
}
Type enumerator_parameter(Type owner,EnumeratorMethod method){return method==EnumeratorMethod::collection_move || method==EnumeratorMethod::collection_reset ? reference_type(owner) : owner;}
bool enumerator_signature(const Program& p,Type owner,Type payload,EnumeratorMethod method){
 if(method>EnumeratorMethod::collection_reset || !valid(p,owner))return false;
 const auto& source=type_info(p,owner);
 if(method==EnumeratorMethod::get_collection)return collection_enumerator_metadata(p,payload) && type_info(p,payload).key==owner;
 if(method>=EnumeratorMethod::collection_move)return payload==Type::invalid && collection_enumerator_metadata(p,owner);
 if(method==EnumeratorMethod::get || method==EnumeratorMethod::box_dictionary || method==EnumeratorMethod::box_collection){
  if(!valid(p,payload) || type_info(p,payload).kind!=CompoundKind::enumerator || type_info(p,payload).element!=source.element)return false;
  return method==EnumeratorMethod::get ? source.kind==CompoundKind::sequence : method==EnumeratorMethod::box_collection ? collection_enumerator_metadata(p,owner) : dictionary_enumerator_metadata(p,owner);
 }
 return payload==Type::invalid && source.kind==CompoundKind::enumerator;
}
Type enumerator_result(const Program& p,Type owner,Type payload,EnumeratorMethod method){
 if(method==EnumeratorMethod::get || method==EnumeratorMethod::box_dictionary || method==EnumeratorMethod::get_collection || method==EnumeratorMethod::box_collection)return payload;
 if(method==EnumeratorMethod::move || method==EnumeratorMethod::collection_move || method==EnumeratorMethod::collection_move_copy)return Type::boolean;
 if(method==EnumeratorMethod::current || method==EnumeratorMethod::collection_current)return type_info(p,owner).element;
 return Type::unit;
}
Value enumerator_invoke(Values& values,Type owner,Type payload,EnumeratorMethod method,const Value& argument,const ExecutionOptions& options) try {
 if(!enumerator_signature(values.program,owner,payload,method))throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 ValueHashWork work{values,options};work.step();
 if(method>=EnumeratorMethod::collection_move){
  auto iterator=std::get<Object>(method==EnumeratorMethod::collection_move || method==EnumeratorMethod::collection_reset ? std::get<std::shared_ptr<Cell>>(argument)->value : method==EnumeratorMethod::collection_move_copy ? values.copy(argument) : argument);
  if(!iterator || iterator->nil || iterator->items.size()!=4)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
  if(method==EnumeratorMethod::collection_current)return values.copy(iterator->items[3]->value);
  if(method==EnumeratorMethod::collection_dispose)return false;
  const auto source=std::get<Object>(iterator->items[0]->value);if(!source || source->nil)throw runtime_error(RuntimeError::nil_reference,values.location);
  if(std::get<std::uint64_t>(iterator->items[1]->value)!=source->version)throw runtime_error(RuntimeError::collection_changed,values.location);
  const auto element=type_info(values.program,owner).element;
  if(method==EnumeratorMethod::collection_reset){values.write(iterator->items[2],std::int32_t{0});values.write(iterator->items[3],values.zero(element,false,true));return false;}
  auto index=static_cast<std::uint32_t>(std::get<std::int32_t>(iterator->items[2]->value));
  while(index<source->items.size()){
   work.step();const auto cell=source->items[index++];values.write(iterator->items[2],static_cast<std::int32_t>(index));
   if(cell){values.write(iterator->items[3],cell->value);return true;}
  }
  values.write(iterator->items[2],static_cast<std::int32_t>(static_cast<std::uint32_t>(source->items.size())+1U));values.write(iterator->items[3],values.zero(element,false,true));return false;
 }
 const auto object=std::get<Object>(argument);
 if(!object || object->nil)throw runtime_error(RuntimeError::nil_reference,values.location);
 if(method==EnumeratorMethod::get_collection){auto result=std::get<Object>(values.zero(payload));values.write(result->items[0],argument);values.write(result->items[1],object->version);return result;}
 if(method>=EnumeratorMethod::move && method<=EnumeratorMethod::reset && type_info(values.program,object->type).kind==CompoundKind::object_class){const auto slot=method==EnumeratorMethod::move ? 0U : method==EnumeratorMethod::current ? 1U : method==EnumeratorMethod::dispose ? 2U : 3U;return interface_invoke(values,owner,slot,argument,{},options);}
 if(method==EnumeratorMethod::box_dictionary || method==EnumeratorMethod::box_collection){auto result=make(values,payload);result->enumerator->boxed=values.cell(owner,values.copy(argument));return result;}
 if(method==EnumeratorMethod::get){
  if(object->sequence && object->sequence->provider==SequenceProvider::user_source)return interface_invoke(values,owner,0,object->sequence->source,{},options);
  if(object->sequence && (object->sequence->provider==SequenceProvider::list || object->sequence->provider==SequenceProvider::hash_set || object->sequence->provider==SequenceProvider::new_set)){
   const auto& sequence=*object->sequence;const auto source=sequence.provider==SequenceProvider::new_set ? (sequence.seed ? new_set_hash(values,std::get<NewSetValue>(sequence.seed->value)) : Object{}) : sequence.source;
   if(!source || source->nil)throw runtime_error(RuntimeError::nil_reference,values.location);
   for(std::size_t i=0;i<values.program.types.size();++i){work.step();const auto& type=values.program.types[i];if(collection_enumerator_type(type) && type.key==source->type){const auto record=static_cast<Type>(64+i);auto iterator=enumerator_invoke(values,source->type,record,EnumeratorMethod::get_collection,source,options);return enumerator_invoke(values,record,payload,EnumeratorMethod::box_collection,iterator,options);}}
  }
  if(object->sequence && dictionary_provider(object->sequence->provider)){
   const auto source=object->sequence->source;if(!source || source->nil)throw runtime_error(RuntimeError::nil_reference,values.location);
   for(std::size_t i=0;i<values.program.types.size();++i){work.step();const auto& t=values.program.types[i];if(dictionary_enumerator_type(t) && t.key==source->type){
    const auto record=static_cast<Type>(64+i);const std::array<Value,1> input{source};auto iterator=dictionary_invoke(values,source->type,record,DictionaryMethod::get_enumerator,input,options);
    return enumerator_invoke(values,record,payload,EnumeratorMethod::box_dictionary,iterator,options);
   }}
  }
  auto prototype=object;
  bool empty_range=object->sequence && object->sequence->provider==SequenceProvider::range && !object->sequence->count && object->sequence->seed && object->sequence->seed->type==Type::integer;
  if(object->sequence && object->sequence->provider==SequenceProvider::ordinal_range){const auto source=object->sequence->source;if(source && source->items.size()==2 && source->items[0]->type==Type::integer)empty_range=std::get<std::int32_t>(source->items[0]->value)>std::get<std::int32_t>(source->items[1]->value);}
  if(empty_range){
   Type array=Type::invalid;for(std::size_t i=0;i<values.program.types.size();++i){work.step();const auto& t=values.program.types[i];if(t.kind==CompoundKind::array && t.rank==1 && t.element==Type::integer){array=static_cast<Type>(64+i);break;}}
   if(array==Type::invalid)throw runtime_error(RuntimeError::invalid_bytecode,values.location);prototype=sequence_create(values,owner,values.array(array,0),SequenceProvider::array);
  }
  const bool empty=prototype->sequence && prototype->sequence->provider==SequenceProvider::array && prototype->sequence->source && prototype->sequence->source->items.empty();
  if(empty)for(const auto& entry:values.empty_enumerators){work.step();if(entry.first==payload)if(const auto existing=entry.second.lock())return existing;}
  auto result=make(values,payload);auto& state=*result->enumerator;state.prototype=prototype;state.cursor=sequence_begin(values,prototype,options);
  if(empty){bool replaced=false;for(auto& entry:values.empty_enumerators){work.step();if(entry.first==payload){entry.second=result;replaced=true;break;}}if(!replaced)values.empty_enumerators.push_back({payload,result});}
  return result;
 }
 if(!object->enumerator || object->items.size()!=1)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 auto& state=*object->enumerator;
 if(state.boxed){
  if(collection_enumerator_type(type_info(values.program,state.boxed->type))){
   const auto operation=method==EnumeratorMethod::move ? EnumeratorMethod::collection_move : method==EnumeratorMethod::current ? EnumeratorMethod::collection_current : method==EnumeratorMethod::reset ? EnumeratorMethod::collection_reset : EnumeratorMethod::collection_dispose;
   return enumerator_invoke(values,state.boxed->type,Type::invalid,operation,enumerator_parameter(state.boxed->type,operation)==reference_type(state.boxed->type) ? Value{state.boxed} : state.boxed->value,options);
  }
  const auto operation=method==EnumeratorMethod::move ? DictionaryMethod::enumerator_move : method==EnumeratorMethod::current ? DictionaryMethod::enumerator_current : method==EnumeratorMethod::reset ? DictionaryMethod::enumerator_reset : DictionaryMethod::enumerator_dispose;
  return boxed(values,state.boxed,operation,options);
 }
 if(!state.cursor || !state.cursor->sequence)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 const auto provider=state.cursor->sequence->provider;const bool collection=flat(provider);
 if(method==EnumeratorMethod::current){
  if(provider==SequenceProvider::string && state.disposed && state.index)throw runtime_error(RuntimeError::nil_reference,values.location);
  if((provider==SequenceProvider::array || provider==SequenceProvider::string) && !state.available)throw runtime_error(RuntimeError::iterator_position,values.location);
  if(provider==SequenceProvider::array)return values.copy(state.cursor->sequence->source->items.at(static_cast<std::size_t>(state.index-1))->value);
  return values.copy(object->items[0]->value);
 }
 if(method==EnumeratorMethod::dispose){
  if(provider==SequenceProvider::string){if(!state.disposed)state.index=state.cursor->sequence->count+1;state.disposed=true;state.available=false;return false;}
  if(!no_dispose(*state.cursor->sequence)){
   const bool clear=clears(values,*state.cursor->sequence);sequence_dispose(values,state.cursor,options);state.disposed=true;state.finished=true;
   if(clear)values.write(object->items[0],values.zero(type_info(values.program,owner).element,false,true));
  }return false;
 }
 if(method==EnumeratorMethod::reset){
  if(provider==SequenceProvider::string && state.disposed){state.index=0;state.available=false;values.write(object->items[0],char16_t{0});return false;}
  if(!collection)throw runtime_error(RuntimeError::iterator_reset,values.location);
  sequence_check(state.cursor,values.location);state.cursor=sequence_begin(values,state.prototype,options);state.index=0;state.available=false;state.finished=false;state.disposed=false;
  values.write(object->items[0],values.zero(type_info(values.program,owner).element,false,true));return false;
 }
 if(provider==SequenceProvider::string && state.disposed)throw runtime_error(RuntimeError::nil_reference,values.location);
 if(state.disposed || (state.finished && !collection))return false;
 if(state.index>static_cast<std::uint64_t>(INT64_MAX))throw runtime_error(RuntimeError::integer_overflow,values.location);
 const bool next=sequence_resume(values,state.cursor,state.index,options);
 if(next){values.write(object->items[0],sequence_at(values,state.cursor,static_cast<std::int64_t>(state.index),options));++state.index;state.available=true;return true;}
 state.available=false;state.finished=true;
 if(collection || clears(values,*state.cursor->sequence))values.write(object->items[0],values.zero(type_info(values.program,owner).element,false,true));
 if(!collection)sequence_dispose(values,state.cursor,options);
 return false;
}catch(const std::bad_alloc&){throw runtime_error(RuntimeError::allocation_limit,values.location);}
}

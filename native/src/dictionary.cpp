#include "dictionary.hpp"
#include "runtime_values.hpp"
#include "value_hash.hpp"
#include "sequences.hpp"
#include "formatting.hpp"
#include <algorithm>
#include <array>
#include <limits>

namespace abc {
namespace {
const TypeInfo& info(const std::vector<TypeInfo>& types,Type type){return types.at(static_cast<unsigned>(type)-64U);}
bool valid(const Program& p,Type t){return compound(t) && static_cast<unsigned>(t)-64U<p.types.size();}
Type sequence(const std::vector<TypeInfo>& types,Type element){
 for(std::size_t i=0;i<types.size();++i)if(types[i].kind==CompoundKind::sequence && types[i].element==element)return static_cast<Type>(64U+i);
 return Type::invalid;
}
Type function(const std::vector<TypeInfo>& types,Type input,Type result){for(std::size_t i=0;i<types.size();++i){const auto& type=types[i];if(type.kind==CompoundKind::callable && type.element==result && type.parameters==std::vector<CallableParameter>{{input,false,false}})return static_cast<Type>(64+i);}return Type::invalid;}
bool pair_shape(const Program& p,Type t,Type key,Type value,bool tuple){
 if(!valid(p,t))return false;const auto& pair=type_info(p,t);
 return (tuple ? pair.kind==CompoundKind::tuple : pair.kind==CompoundKind::record && key_value_pair_type(pair))
  && pair.fields.size()==2 && pair.fields[0].type==key && pair.fields[1].type==value;
}
}
Type dictionary_value_type(const std::vector<TypeInfo>& types,Type owner){return info(types,info(types,owner).element).fields.at(1).type;}
bool dictionary_signature(const Program& p,Type owner,Type payload,DictionaryMethod m){
 if(m>DictionaryMethod::to_dictionary_elements || !valid(p,owner))return false;const auto& d=type_info(p,owner);
 if(m==DictionaryMethod::pair_construct || m==DictionaryMethod::pair_to_string)
  return d.kind==CompoundKind::record && key_value_pair_type(d) && d.fields.size()==2 && payload==Type::invalid;
 if(m==DictionaryMethod::view_count || m==DictionaryMethod::view_contains || m==DictionaryMethod::view_copy_to){
  if((d.kind!=CompoundKind::dictionary_keys && d.kind!=CompoundKind::dictionary_values) || !valid(p,d.key) || type_info(p,d.key).kind!=CompoundKind::dictionary)return false;
  if(m!=DictionaryMethod::view_copy_to)return payload==Type::invalid;
  return valid(p,payload) && type_info(p,payload).kind==CompoundKind::array && type_info(p,payload).rank==1 && type_info(p,payload).element==d.element;
 }
 if(d.kind!=CompoundKind::dictionary || !valid(p,d.element))return false;
 const auto& pair=type_info(p,d.element);if(pair.fields.size()!=2 || !pair_shape(p,d.element,d.key,pair.fields[1].type,false))return false;
 const auto value=pair.fields[1].type;
 if(m==DictionaryMethod::each_count || m==DictionaryMethod::each || m==DictionaryMethod::to_dictionary || m==DictionaryMethod::to_dictionary_elements){
  if(!valid(p,payload) || type_info(p,payload).kind!=CompoundKind::sequence)return false;const auto source=type_info(p,payload).element;
  if(m==DictionaryMethod::each_count)return source==d.key && value==Type::integer;
  if(m==DictionaryMethod::each)return source==d.key && function(p.types,source,value)!=Type::invalid;
  return (m!=DictionaryMethod::to_dictionary || source==value) && function(p.types,source,d.key)!=Type::invalid && (m!=DictionaryMethod::to_dictionary_elements || function(p.types,source,value)!=Type::invalid);
 }
 if(m==DictionaryMethod::keys || m==DictionaryMethod::values){
  if(!valid(p,payload))return false;const auto& view=type_info(p,payload);
  return view.kind==(m==DictionaryMethod::keys ? CompoundKind::dictionary_keys : CompoundKind::dictionary_values)
   && view.key==owner && view.element==(m==DictionaryMethod::keys ? d.key : value);
 }
 if(m==DictionaryMethod::get_or_add_factory){
  if(!valid(p,payload))return false;const auto& f=type_info(p,payload);
  return f.kind==CompoundKind::callable && f.element==value && f.parameters==std::vector<CallableParameter>{{d.key,false,false}};
 }
 if(m==DictionaryMethod::from_pairs || m==DictionaryMethod::from_tuples || m==DictionaryMethod::from_sequences || m==DictionaryMethod::without_keys || m==DictionaryMethod::remove_keys){
  if(!valid(p,payload) || type_info(p,payload).kind!=CompoundKind::sequence)return false;
  const auto element=type_info(p,payload).element;
  if(m==DictionaryMethod::from_sequences)return element==value && sequence(p.types,d.key)!=Type::invalid;
  if(m==DictionaryMethod::without_keys || m==DictionaryMethod::remove_keys)return element==d.key;
  return pair_shape(p,element,d.key,value,m==DictionaryMethod::from_tuples);
 }
 return payload==Type::invalid;
}
std::vector<Type> dictionary_parameters(const std::vector<TypeInfo>& types,Type owner,Type payload,DictionaryMethod m){
 if(m==DictionaryMethod::view_count)return {owner};
 if(m==DictionaryMethod::view_contains)return {owner,info(types,owner).element};
 if(m==DictionaryMethod::view_copy_to)return {owner,payload,Type::integer};
 if(m==DictionaryMethod::pair_construct)return {info(types,owner).fields[0].type,info(types,owner).fields[1].type};
 if(m==DictionaryMethod::pair_to_string)return {owner};
 if(m==DictionaryMethod::construct)return {};
 if(m==DictionaryMethod::construct_capacity)return {Type::integer};
 if(m==DictionaryMethod::construct_copy || m==DictionaryMethod::copy_value)return {owner};
 if(m==DictionaryMethod::from_pairs || m==DictionaryMethod::from_tuples)return {payload};
 const auto& d=info(types,owner);const auto value=dictionary_value_type(types,owner);
 if(m==DictionaryMethod::each_count)return {payload};
 if(m==DictionaryMethod::each)return {payload,function(types,info(types,payload).element,value)};
 if(m==DictionaryMethod::to_dictionary)return {payload,function(types,info(types,payload).element,d.key)};
 if(m==DictionaryMethod::to_dictionary_elements)return {payload,function(types,info(types,payload).element,d.key),function(types,info(types,payload).element,value)};
 if(m==DictionaryMethod::from_sequences)return {sequence(types,d.key),payload};
 if(m==DictionaryMethod::membership)return {d.key,owner};
 std::vector<Type> result{owner};
 if(m==DictionaryMethod::count || m==DictionaryMethod::clear || m==DictionaryMethod::keys || m==DictionaryMethod::values)return result;
 if(m==DictionaryMethod::update || m==DictionaryMethod::merge){result.push_back(owner);return result;}
 if(m==DictionaryMethod::remove_keys || m==DictionaryMethod::without_keys){result.push_back(payload);return result;}
 if(m==DictionaryMethod::contains_value){result.push_back(value);return result;}
 result.push_back(d.key);
 if(m==DictionaryMethod::add || m==DictionaryMethod::set_item || m==DictionaryMethod::get_default || m==DictionaryMethod::get_or_add_value)result.push_back(value);
 if(m==DictionaryMethod::try_get_value)result.push_back(reference_type(value));
 if(m==DictionaryMethod::get_or_add_factory)result.push_back(payload);
 return result;
}
Type dictionary_result(const std::vector<TypeInfo>& types,Type owner,Type payload,DictionaryMethod m){
 if(m==DictionaryMethod::each_count || m==DictionaryMethod::each || m==DictionaryMethod::to_dictionary || m==DictionaryMethod::to_dictionary_elements)return owner;
 if(m<=DictionaryMethod::from_sequences || m==DictionaryMethod::copy_value || m==DictionaryMethod::merge || m==DictionaryMethod::without_key || m==DictionaryMethod::without_keys || m==DictionaryMethod::pair_construct)return owner;
 if(m==DictionaryMethod::view_count)return Type::integer;
 if(m==DictionaryMethod::view_contains)return Type::boolean;
 if(m==DictionaryMethod::pair_to_string)return Type::string;
 if(m==DictionaryMethod::keys || m==DictionaryMethod::values)return payload;
 if(m==DictionaryMethod::count)return Type::integer;
 if(m==DictionaryMethod::get_item || m==DictionaryMethod::get_default || m==DictionaryMethod::get_or_add_value || m==DictionaryMethod::get_or_add_factory)return dictionary_value_type(types,owner);
 if(m==DictionaryMethod::contains_key || m==DictionaryMethod::contains_value || m==DictionaryMethod::try_get_value || m==DictionaryMethod::remove || m==DictionaryMethod::membership)return Type::boolean;
 return Type::unit;
}
namespace {
constexpr auto missing=std::numeric_limits<std::size_t>::max();
using Object=std::shared_ptr<Aggregate>;
bool nil(const Value& key){
 if(std::holds_alternative<NilString>(key))return true;
 if(const auto* p=std::get_if<Object>(&key))return !*p || (*p)->nil;
 if(const auto* p=std::get_if<std::shared_ptr<TextFile>>(&key))return !*p;
 if(const auto* p=std::get_if<ReflectedType>(&key))return p->target==Type::invalid;
 return false;
}
void require_key(const Value& key,SourceLocation location){if(nil(key))throw runtime_error(RuntimeError::dictionary_nil_key,location);}
void require(const Object& d,SourceLocation location){
 if(!d || d->nil)throw runtime_error(RuntimeError::nil_reference,location);
 if(!d->hash_set)throw runtime_error(RuntimeError::invalid_bytecode,location);
}
const Object& entry(const Object& d,std::size_t slot){return std::get<Object>(d->items[slot]->value);}
void reserve(ValueHashWork& work,const Object& d,std::size_t size){
 if(size<=d->hash_set->capacity)return;
 const auto growth=size-d->hash_set->capacity;
 if(size>static_cast<std::size_t>(INT32_MAX) || growth>work.values.list_storage_remaining)throw runtime_error(RuntimeError::allocation_limit,work.values.location);
 work.values.list_storage_remaining-=growth;
 d->items.reserve(size);d->hash_set->hashes.reserve(size);d->hash_set->free.reserve(size);d->hash_set->capacity=size;
}
std::size_t find(ValueHashWork& work,const Object& d,const Value& key,std::size_t code){
 const auto range=d->hash_set->lookup.equal_range(code);
 for(auto it=range.first;it!=range.second;++it){work.step();if(value_equal(work,entry(d,it->second)->items[0]->value,key))return it->second;}
 return missing;
}
std::size_t find(ValueHashWork& work,const Object& d,const Value& key){
 require_key(key,work.values.location);return find(work,d,key,value_hash(work,key));
}
Object make_pair(Values& values,Type pair,const Value& key,const Value& value){
 auto result=std::get<Object>(values.zero(pair));values.write(result->items[0],key);values.write(result->items[1],value);return result;
}
void insert(ValueHashWork& work,const Object& d,const Value& key,const Value& value,bool add,bool allow_nil=false){
 if(!allow_nil)require_key(key,work.values.location);const auto code=value_hash(work,key);const auto found=find(work,d,key,code);
 if(found!=missing){
  if(add)throw runtime_error(RuntimeError::dictionary_duplicate_key,work.values.location);
  work.values.write(entry(d,found)->items[1],value);++d->version;return;
 }
 auto& state=*d->hash_set;const bool reuse=!state.free.empty();const auto slot=reuse ? state.free.back() : d->items.size();
 if(!reuse && slot==state.capacity)reserve(work,d,state.capacity ? state.capacity*2U : 3U);
 const auto pair=type_info(work.values.program,d->type).element;
 auto cell=work.values.cell(pair,make_pair(work.values,pair,key,value));state.lookup.emplace(code,slot);
 if(reuse){state.free.pop_back();d->items[slot]=std::move(cell);state.hashes[slot]=code;}
 else{d->items.push_back(std::move(cell));state.hashes.push_back(code);}
 ++d->count;++d->version;
}
bool remove(ValueHashWork& work,const Object& d,const Value& key){
 const auto slot=find(work,d,key);if(slot==missing)return false;
 auto& state=*d->hash_set;const auto range=state.lookup.equal_range(state.hashes[slot]);
 for(auto it=range.first;it!=range.second;++it)if(it->second==slot){state.lookup.erase(it);break;}
 d->items[slot].reset();state.free.push_back(slot);--d->count;++d->version;return true;
}
Object make(Values& values,Type owner){return std::get<Object>(values.zero(owner,true));}
Object copy(ValueHashWork& work,Type owner,const Object& source){
 require(source,work.values.location);auto result=make(work.values,owner);reserve(work,result,source->count);
 for(const auto& cell:source->items){work.step();if(cell){const auto& p=std::get<Object>(cell->value);insert(work,result,p->items[0]->value,p->items[1]->value,true);}}
 return result;
}
template<class F>void each(ValueHashWork& work,const Object& source,F action){
 const auto cursor=sequence_begin(work.values,source);
 for(std::uint64_t i=0;sequence_resume(work.values,cursor,i,work.options);++i){work.step();action(sequence_at(work.values,cursor,static_cast<std::int64_t>(i),work.options));}
}
void update(ValueHashWork& work,const Object& target,const Object& source){
 if(!source || source->nil)return;require(source,work.values.location);const auto version=source->version;
 for(std::size_t i=0;i<source->items.size();++i){
  work.step();if(source->version!=version)throw runtime_error(RuntimeError::collection_changed,work.values.location);
  if(const auto& cell=source->items[i]){const auto pair=std::get<Object>(cell->value);require(target,work.values.location);insert(work,target,pair->items[0]->value,pair->items[1]->value,false);}
 }
 if(source->version!=version)throw runtime_error(RuntimeError::collection_changed,work.values.location);
}
}
std::shared_ptr<Aggregate> dictionary_sequence_source(const SequenceState& state,SourceLocation location){
 if(!dictionary_provider(state.provider) || !state.source || state.source->nil)throw runtime_error(RuntimeError::invalid_bytecode,location);
 auto source=state.source;
 if(state.provider!=SequenceProvider::dictionary_entries){
  if(source->items.size()!=1 || !source->items[0] || !std::holds_alternative<std::shared_ptr<Aggregate>>(source->items[0]->value))throw runtime_error(RuntimeError::invalid_bytecode,location);
  source=std::get<std::shared_ptr<Aggregate>>(source->items[0]->value);
 }
 require(source,location);return source;
}
bool dictionary_sequence_resume(Values& values,const std::shared_ptr<Aggregate>& cursor,std::uint64_t index,const ExecutionOptions& options){
 auto& state=*cursor->sequence;const auto source=dictionary_sequence_source(state,values.location);
 if(!state.hash_cursor)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 auto& position=*state.hash_cursor;if(position.positioned && position.index==index)return position.available;
 if(position.positioned && index<position.index)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 ValueHashWork work{values,options};auto logical=position.positioned ? position.index+1 : 0;
 for(;logical<=index;++logical){
  bool found=false;
  while(position.slot<source->items.size()){
   work.step();const auto& cell=source->items[position.slot++];if(!cell)continue;
   if(state.provider==SequenceProvider::dictionary_entries)position.current=values.copy(cell->value);
   else{const auto& pair=std::get<std::shared_ptr<Aggregate>>(cell->value);position.current=values.copy(pair->items[state.provider==SequenceProvider::dictionary_keys ? 0U : 1U]->value);}
   found=true;break;
  }
  position.positioned=true;position.index=logical;position.available=found;if(!found)return false;
 }
 return true;
}
Value dictionary_invoke(Values& values,Type owner,Type payload,DictionaryMethod m,std::span<const Value> args,const ExecutionOptions& options){
 if(!dictionary_signature(values.program,owner,payload,m) || args.size()!=dictionary_parameters(values.program.types,owner,payload,m).size())throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 try{
  ValueHashWork work{values,options};work.step();
  if(m==DictionaryMethod::view_count || m==DictionaryMethod::view_contains || m==DictionaryMethod::view_copy_to){
   const auto view=std::get<Object>(args[0]);if(!view || view->nil)throw runtime_error(RuntimeError::nil_reference,values.location);
   if(view->items.size()!=1)throw runtime_error(RuntimeError::invalid_bytecode,values.location);const auto d=std::get<Object>(view->items[0]->value);require(d,values.location);
   const bool keys=type_info(values.program,owner).kind==CompoundKind::dictionary_keys;
   if(m==DictionaryMethod::view_count)return static_cast<std::int32_t>(d->count);
   if(m==DictionaryMethod::view_contains){if(keys)return find(work,d,args[1])!=missing;for(const auto& cell:d->items){work.step();if(cell && value_equal(work,std::get<Object>(cell->value)->items[1]->value,args[1]))return true;}return false;}
   const auto array=std::get<Object>(args[1]);if(!array || array->nil)throw runtime_error(RuntimeError::nil_reference,values.location);
   const auto index=std::get<std::int32_t>(args[2]);if(index<0 || static_cast<std::size_t>(index)>array->items.size())throw runtime_error(RuntimeError::dictionary_copy_index,values.location);
   if(d->count>array->items.size()-static_cast<std::size_t>(index))throw runtime_error(RuntimeError::dictionary_copy_range,values.location);
   auto at=static_cast<std::size_t>(index);for(const auto& cell:d->items){work.step();if(cell)values.write(array->items[at++],std::get<Object>(cell->value)->items[keys ? 0U : 1U]->value);}return false;
  }
  if(m==DictionaryMethod::pair_construct)return make_pair(values,owner,args[0],args[1]);
  if(m==DictionaryMethod::pair_to_string){
   const auto& pair=std::get<Object>(args[0]);std::u16string text=u"[";
   for(std::size_t i=0;i<2;++i){if(i)text+=u", ";const auto& value=pair->items[i]->value;
    const auto part=nil(value) ? std::u16string{} : object_text(values.program,value,pair->items[i]->type,values.location,options);
    if(part.size()>1'048'576-text.size())throw runtime_error(RuntimeError::string_limit,values.location);text+=part;
   }
   if(text.size()>=1'048'576)throw runtime_error(RuntimeError::string_limit,values.location);text+=u"]";return text;
  }
  if(m==DictionaryMethod::construct || m==DictionaryMethod::construct_capacity){
   if(m==DictionaryMethod::construct_capacity && std::get<std::int32_t>(args[0])<0)throw runtime_error(RuntimeError::dictionary_capacity,values.location);
   auto result=make(values,owner);if(m==DictionaryMethod::construct_capacity)reserve(work,result,static_cast<std::uint32_t>(std::get<std::int32_t>(args[0])));return result;
  }
  if(m==DictionaryMethod::construct_copy || m==DictionaryMethod::copy_value){const auto source=std::get<Object>(args[0]);if(m==DictionaryMethod::copy_value && (!source || source->nil))throw runtime_error(RuntimeError::dictionary_nil_copy,values.location);return copy(work,owner,source);}
  if(m==DictionaryMethod::each_count || m==DictionaryMethod::each || m==DictionaryMethod::to_dictionary || m==DictionaryMethod::to_dictionary_elements){
   const auto source=std::get<Object>(args[0]);if(!source || source->nil)throw runtime_error(RuntimeError::nil_reference,values.location);
   if(m!=DictionaryMethod::each_count)for(std::size_t i=1;i<args.size();++i)(void)values.callable_target(std::get<Object>(args[i]));
   if(m==DictionaryMethod::each_count){
    auto groups=make(values,owner);each(work,source,[&](const Value& key){const auto code=value_hash(work,key),slot=find(work,groups,key,code);if(slot==missing)insert(work,groups,key,std::int32_t{1},true,true);else{const auto cell=entry(groups,slot)->items[1];const auto count=std::get<std::int32_t>(cell->value);if(count==INT32_MAX)throw runtime_error(RuntimeError::integer_overflow,values.location);values.write(cell,std::int32_t{count+1});}});
    auto result=make(values,owner);reserve(work,result,groups->count);for(const auto& cell:groups->items){work.step();if(cell){const auto& pair=std::get<Object>(cell->value);insert(work,result,pair->items[0]->value,pair->items[1]->value,true);}}return result;
   }
   auto result=make(values,owner);each(work,source,[&](const Value& item){const std::array<Value,1> input{item};const auto key=m==DictionaryMethod::each ? values.copy(item) : values.invoke_value(std::get<Object>(args[1]),input);const auto value=m==DictionaryMethod::to_dictionary ? values.copy(item) : values.invoke_value(std::get<Object>(args[m==DictionaryMethod::each ? 1U : 2U]),input);insert(work,result,key,value,true);});return result;
  }
  if(m==DictionaryMethod::from_pairs || m==DictionaryMethod::from_tuples){
   // The source's sequence overload completes ToArray before the first Add.
   std::vector<Value> snapshot;each(work,std::get<Object>(args[0]),[&](const Value& pair){
    if(snapshot.size()>=values.remaining)throw runtime_error(RuntimeError::allocation_limit,values.location);
    snapshot.push_back(values.copy(pair));
   });
   auto result=make(values,owner);reserve(work,result,snapshot.size());
   for(const auto& item:snapshot){work.step();const auto& pair=std::get<Object>(item);insert(work,result,pair->items[0]->value,pair->items[1]->value,true);}return result;
  }
  if(m==DictionaryMethod::from_sequences){
   auto result=make(values,owner);const auto keys=sequence_begin(values,std::get<Object>(args[0])),items=sequence_begin(values,std::get<Object>(args[1]));
   for(std::uint64_t i=0;;++i){work.step();const auto k=sequence_resume(values,keys,i,options),v=sequence_resume(values,items,i,options);
    if(k!=v)throw runtime_error(RuntimeError::dictionary_lengths,values.location);if(!k)break;
    insert(work,result,sequence_at(values,keys,static_cast<std::int64_t>(i),options),sequence_at(values,items,static_cast<std::int64_t>(i),options),true);
   }return result;
  }
  const auto object=std::get<Object>(args[m==DictionaryMethod::membership ? 1 : 0]);
  if(m==DictionaryMethod::update){update(work,object,std::get<Object>(args[1]));return false;}
  if(m==DictionaryMethod::remove_keys){const auto source=std::get<Object>(args[1]);if(source && !source->nil)each(work,source,[&](const Value& key){require(object,values.location);remove(work,object,key);});return false;}
  require(object,values.location);
  if(m==DictionaryMethod::count)return static_cast<std::int32_t>(object->count);
  if(m==DictionaryMethod::keys || m==DictionaryMethod::values){
   auto& cached=m==DictionaryMethod::keys ? object->dictionary_keys : object->dictionary_values;if(auto view=cached.lock())return view;
   auto view=std::get<Object>(values.zero(payload,true));view->items.push_back(values.cell(owner,object));cached=view;return view;
  }
  if(m==DictionaryMethod::merge){auto result=copy(work,owner,object);update(work,result,std::get<Object>(args[1]));return result;}
  if(m==DictionaryMethod::without_key){auto result=copy(work,owner,object);remove(work,result,args[1]);return result;}
  if(m==DictionaryMethod::without_keys || m==DictionaryMethod::remove_keys){
   auto result=m==DictionaryMethod::without_keys ? copy(work,owner,object) : object;const auto source=std::get<Object>(args[1]);
   if(source && !source->nil)each(work,source,[&](const Value& key){remove(work,result,key);});
   return m==DictionaryMethod::without_keys ? Value{result} : Value{false};
  }
  if(m==DictionaryMethod::contains_value){
   for(const auto& cell:object->items){work.step();if(cell && value_equal(work,std::get<Object>(cell->value)->items[1]->value,args[1]))return true;}return false;
  }
  if(m==DictionaryMethod::clear){
   if(!object->items.empty()){object->items.clear();object->hash_set->hashes.clear();object->hash_set->lookup.clear();object->hash_set->free.clear();object->count=0;++object->version;}return false;
  }
  const auto& key=args[m==DictionaryMethod::membership ? 0 : 1];
  if(m==DictionaryMethod::add || m==DictionaryMethod::set_item){insert(work,object,key,args[2],m==DictionaryMethod::add);return false;}
  if(m==DictionaryMethod::remove)return remove(work,object,key);
  const auto slot=find(work,object,key);
  if(m==DictionaryMethod::contains_key || m==DictionaryMethod::membership)return slot!=missing;
  if(m==DictionaryMethod::try_get_value){
   const auto target=std::get<std::shared_ptr<Cell>>(args[2]);const auto value=slot==missing ? values.zero(dictionary_value_type(values.program.types,owner),false,true) : entry(object,slot)->items[1]->value;
   values.write(target,value);return slot!=missing;
  }
  if(slot!=missing)return values.copy(entry(object,slot)->items[1]->value);
  if(m==DictionaryMethod::get_default)return values.copy(args[2]);
  if(m==DictionaryMethod::get_or_add_value || m==DictionaryMethod::get_or_add_factory){
   Value result=args[2];if(m==DictionaryMethod::get_or_add_factory){const std::array<Value,1> input{key};result=values.invoke_value(std::get<Object>(args[2]),input);}
   insert(work,object,key,result,false);return values.copy(result);
  }
  throw runtime_error(RuntimeError::dictionary_missing_key,values.location);
 }catch(const std::bad_alloc&){throw runtime_error(RuntimeError::allocation_limit,values.location);}
 catch(const std::length_error&){throw runtime_error(RuntimeError::allocation_limit,values.location);}
}
}

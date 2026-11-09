#include "interfaces.hpp"
#include "runtime_values.hpp"
#include "sequences.hpp"
#include "collection_interface.hpp"
#include "record_interface.hpp"
#include <array>
#include <set>
namespace abc {
InterfaceStorage::InterfaceStorage(const InterfaceStorage& other){if(other.value)value=std::make_unique<InterfaceData>(*other.value);}
InterfaceStorage::~InterfaceStorage()=default;
InterfaceStorage& InterfaceStorage::operator=(const InterfaceStorage& other){if(this!=&other){auto copy=other.value ? std::make_unique<InterfaceData>(*other.value) : nullptr;value=std::move(copy);}return *this;}
namespace {
bool valid(const std::vector<TypeInfo>& types,Type type){return compound(type) && static_cast<unsigned>(type)-64U<types.size();}
const TypeInfo& info(const std::vector<TypeInfo>& types,Type type){return types.at(static_cast<unsigned>(type)-64U);}
const InterfaceImplementation* implementation(const TypeInfo& type,Type target){for(const auto& item:type.interfaces.read().implementations)if(item.target==target)return &item;return nullptr;}
}
bool interface_kind(const TypeInfo& type){return type.kind==CompoundKind::user_interface || type.kind==CompoundKind::enumerator || type.kind==CompoundKind::sequence || type.kind==CompoundKind::comparable_interface || type.kind==CompoundKind::comparer_interface || type.kind==CompoundKind::equatable_interface || type.kind==CompoundKind::equality_comparer_interface || type.kind==CompoundKind::collection_interface || type.kind==CompoundKind::list_interface || type.kind==CompoundKind::dictionary_interface || type.kind==CompoundKind::set_interface;}
bool interface_property_match(const InterfaceMember& a,const InterfaceMember& b){
 if(a.kind==InterfaceMemberKind::method||b.kind==InterfaceMemberKind::method||a.kind==b.kind)return false;
 const auto& get=a.kind==InterfaceMemberKind::get?a:b;const auto& set=a.kind==InterfaceMemberKind::set?a:b;
 return get.default_indexer==set.default_indexer&&set.parameters.size()==get.parameters.size()+1&&get.result==set.parameters.back().type&&std::equal(get.parameters.begin(),get.parameters.end(),set.parameters.begin());
}
bool interface_member_key_equal(const InterfaceMember& a,const InterfaceMember& b){return a.name==b.name&&a.kind==b.kind&&(a.kind!=InterfaceMemberKind::method||(a.parameters.size()==b.parameters.size()&&std::equal(a.parameters.begin(),a.parameters.end(),b.parameters.begin(),[](const auto& x,const auto& y){return x.type==y.type&&x.by_reference==y.by_reference;})));}
std::vector<InterfaceMember> interface_contract(const std::vector<TypeInfo>& types,Type type){
 if(!valid(types,type))return {};
 const auto& t=info(types,type);if(t.kind==CompoundKind::user_interface)return t.interfaces.read().members;
 if(t.kind==CompoundKind::sequence){for(std::size_t i=0;i<types.size();++i)if(types[i].kind==CompoundKind::enumerator && types[i].element==t.element)return {{"getenumerator",static_cast<Type>(64+i),{},InterfaceMemberKind::method}};throw Error("E_BYTECODE",{},"Invalid interface metadata");}
 if(t.kind==CompoundKind::comparable_interface)return {{"compareto",Type::integer,{{t.element,false,false}},InterfaceMemberKind::method}};
 if(t.kind==CompoundKind::comparer_interface)return {{"compare",Type::integer,{{t.element,false,false},{t.element,false,false}},InterfaceMemberKind::method}};
 if(t.kind==CompoundKind::equatable_interface)return {{"equals",Type::boolean,{{t.element,false,false}},InterfaceMemberKind::method}};
 if(t.kind==CompoundKind::equality_comparer_interface)return {{"equals",Type::boolean,{{t.element,false,false},{t.element,false,false}},InterfaceMemberKind::method},{"gethashcode",Type::integer,{{t.element,false,false}},InterfaceMemberKind::method}};
 if(t.kind==CompoundKind::dictionary_interface){
  if(t.interfaces.read().parents.size()!=1)throw Error("E_BYTECODE",{},"Invalid dictionary interface metadata");
  const auto value=info(types,t.element).fields.at(1).type;
  const auto collection=[&](Type element){for(std::size_t i=0;i<types.size();++i)if(types[i].kind==CompoundKind::collection_interface&&types[i].element==element)return static_cast<Type>(64+i);throw Error("E_BYTECODE",{},"Missing dictionary collection interface");};
  auto members=interface_contract(types,t.interfaces.read().parents[0]);members.push_back({"keys",collection(t.key),{},InterfaceMemberKind::get});members.push_back({"values",collection(value),{},InterfaceMemberKind::get});members.push_back({"item",value,{{t.key,false,false}},InterfaceMemberKind::get,true});members.push_back({"item",Type::unit,{{t.key,false,false},{value,false,false}},InterfaceMemberKind::set,true});members.push_back({"containskey",Type::boolean,{{t.key,false,false}},InterfaceMemberKind::method});members.push_back({"add",Type::unit,{{t.key,false,false},{value,false,false}},InterfaceMemberKind::method});members.push_back({"remove",Type::boolean,{{t.key,false,false}},InterfaceMemberKind::method});members.push_back({"trygetvalue",Type::boolean,{{t.key,false,false},{value,true,false}},InterfaceMemberKind::method});return members;
 }
 if(t.kind==CompoundKind::set_interface){
  if(t.interfaces.read().parents.size()!=1)throw Error("E_BYTECODE",{},"Invalid set interface metadata");
  const auto parent=t.interfaces.read().parents[0];const auto sequence=info(types,parent).interfaces.read().parents.at(0);
  auto members=interface_contract(types,parent);
  // ISet.Add returns bool and hides the inherited procedure. An ICollection
  // upcast retains its separate eight-slot contract and implementation map.
  members.erase(members.begin()+2);members.push_back({"add",Type::boolean,{{t.element,false,false}},InterfaceMemberKind::method});
  for(const auto* name:{"exceptwith","intersectwith","ispropersubsetof","ispropersupersetof","issubsetof","issupersetof","overlaps","setequals","symmetricexceptwith","unionwith"}){
   const bool query=std::string_view(name).starts_with("is")||std::string_view(name)=="overlaps"||std::string_view(name)=="setequals";
   members.push_back({name,query?Type::boolean:Type::unit,{{sequence,false,false}},InterfaceMemberKind::method});
  }return members;
 }
 if(t.kind==CompoundKind::list_interface){
  if(t.interfaces.read().parents.size()!=1)throw Error("E_BYTECODE",{},"Invalid list interface metadata");
  auto members=interface_contract(types,t.interfaces.read().parents[0]);members.push_back({"item",t.element,{{Type::integer,false,false}},InterfaceMemberKind::get,true});members.push_back({"item",Type::unit,{{Type::integer,false,false},{t.element,false,false}},InterfaceMemberKind::set,true});members.push_back({"indexof",Type::integer,{{t.element,false,false}},InterfaceMemberKind::method});members.push_back({"insert",Type::unit,{{Type::integer,false,false},{t.element,false,false}},InterfaceMemberKind::method});members.push_back({"removeat",Type::unit,{{Type::integer,false,false}},InterfaceMemberKind::method});return members;
 }
 if(t.kind==CompoundKind::collection_interface){
  std::vector<InterfaceMember> members{{"count",Type::integer,{},InterfaceMemberKind::get},{"isreadonly",Type::boolean,{},InterfaceMemberKind::get},
   {"add",Type::unit,{{t.element,false,false}},InterfaceMemberKind::method},{"clear",Type::unit,{},InterfaceMemberKind::method},
   {"contains",Type::boolean,{{t.element,false,false}},InterfaceMemberKind::method},{"copyto",Type::unit,{{t.key,false,false},{Type::integer,false,false}},InterfaceMemberKind::method},
   {"remove",Type::boolean,{{t.element,false,false}},InterfaceMemberKind::method}};
  if(t.interfaces.read().parents.size()!=1)throw Error("E_BYTECODE",{},"Invalid collection interface metadata");
  auto inherited=interface_contract(types,t.interfaces.read().parents[0]);members.insert(members.end(),inherited.begin(),inherited.end());return members;
 }
 if(t.kind==CompoundKind::enumerator)return {{"movenext",Type::boolean,{},InterfaceMemberKind::method},{"current",t.element,{},InterfaceMemberKind::get},{"dispose",Type::unit,{},InterfaceMemberKind::method},{"reset",Type::unit,{},InterfaceMemberKind::method}};
 return {};
}
bool interface_compatible(const std::vector<TypeInfo>& types,Type from,Type to){
 if(!valid(types,from) || !valid(types,to) || !interface_kind(info(types,to)))return false;
 if(from==to)return true;const auto& source=info(types,from);
 if(native_collection_type(types,from,to)||native_record_interface_type(types,from,to))return true;
 if(source.kind==CompoundKind::object_class)return implementation(source,to)!=nullptr;
 if(source.kind==CompoundKind::equality_comparer)return info(types,to).kind==CompoundKind::equality_comparer_interface && source.element==info(types,to).element;
 if(source.kind==CompoundKind::comparer)return info(types,to).kind==CompoundKind::comparer_interface && source.element==info(types,to).element;
 if(!interface_kind(source))return false;
 std::vector<Type> pending{from};std::set<Type> seen;
 while(!pending.empty()){const auto current=pending.back();pending.pop_back();if(current==to)return true;if(!valid(types,current) || !seen.insert(current).second)continue;for(auto parent:info(types,current).interfaces.read().parents)pending.push_back(parent);}
 return false;
}
bool interface_cast_valid(const Program& p,Type from,Type to,bool checked){
 if(interface_compatible(p.types,from,to))return true;
 if(!checked || !valid(p.types,from) || !valid(p.types,to) || !interface_kind(info(p.types,from)))return false;
 return interface_kind(info(p.types,to)) || ((info(p.types,to).kind==CompoundKind::object_class || ordinal_range_type(info(p.types,to)) || real_interval_type(info(p.types,to)) || info(p.types,to).kind==CompoundKind::array || info(p.types,to).kind==CompoundKind::list || info(p.types,to).kind==CompoundKind::hash_set || info(p.types,to).kind==CompoundKind::dictionary || info(p.types,to).kind==CompoundKind::dictionary_keys || info(p.types,to).kind==CompoundKind::dictionary_values) && interface_compatible(p.types,to,from));
}
void verify_interfaces(const Program& p){
 std::size_t names=0;
 auto signature=[&](const InterfaceMember& member){
  if(member.name.empty() || member.parameters.size()>15 || (member.kind!=InterfaceMemberKind::method && member.kind!=InterfaceMemberKind::get && member.kind!=InterfaceMemberKind::set) || (member.result!=Type::unit && !valid_value_type(p,member.result)))throw Error("E_BYTECODE",{},"Invalid interface member signature");
  if(member.name.size()>1'048'576-names)throw Error("E_BYTECODE",{},"Interface metadata limit exceeded");names+=member.name.size();
  for(std::size_t i=0;i<member.parameters.size();++i){const auto& a=member.parameters[i];if(!valid_value_type(p,a.type) || (a.variadic && (i+1!=member.parameters.size() || a.by_reference || !compound(a.type) || type_info(p,a.type).kind!=CompoundKind::array || type_info(p,a.type).rank!=1)))throw Error("E_BYTECODE",{},"Invalid interface member signature");}
  if(member.kind!=InterfaceMemberKind::method){if((member.kind==InterfaceMemberKind::get&&member.result==Type::unit)||(member.kind==InterfaceMemberKind::set&&(member.result!=Type::unit||member.parameters.empty())))throw Error("E_BYTECODE",{},"Invalid interface property signature");for(const auto& parameter:member.parameters)if(parameter.by_reference||parameter.variadic)throw Error("E_BYTECODE",{},"Invalid interface property signature");}
  if(member.default_indexer&&(member.kind==InterfaceMemberKind::method||member.parameters.size()<(member.kind==InterfaceMemberKind::get?1U:2U)))throw Error("E_BYTECODE",{},"Invalid default interface property");
 };
 for(std::size_t i=0;i<p.types.size();++i){const auto& t=p.types[i];
  if(t.kind!=CompoundKind::user_interface && t.kind!=CompoundKind::collection_interface && t.kind!=CompoundKind::list_interface && t.kind!=CompoundKind::dictionary_interface && t.kind!=CompoundKind::set_interface && (!t.interfaces.read().parents.empty() || !t.interfaces.read().members.empty()))throw Error("E_BYTECODE",{},"Interface contract on a non-interface type");
  if(t.kind!=CompoundKind::object_class && !t.interfaces.read().implementations.empty())throw Error("E_BYTECODE",{},"Interface implementation on a non-class type");
  if(t.kind==CompoundKind::user_interface){
   if(t.name.empty() || t.element!=Type::invalid || t.key!=Type::invalid || !t.fields.empty() || t.interfaces.read().parents.size()>16 || t.interfaces.read().members.size()>256 || !t.parameters.empty() || !t.enum_members.empty() || t.enum_flags || t.rank!=1)throw Error("E_BYTECODE",{},"Invalid interface metadata");
   std::vector<InterfaceMember> keys;
   for(const auto& m:t.interfaces.read().members){signature(m);if(std::any_of(keys.begin(),keys.end(),[&](const auto& previous){return interface_member_key_equal(previous,m);}))throw Error("E_BYTECODE",{},"Duplicate interface member");keys.push_back(m);}
   std::string default_property;for(const auto& member:t.interfaces.read().members)if(member.default_indexer){if(!default_property.empty()&&default_property!=member.name)throw Error("E_BYTECODE",{},"Ambiguous default interface property");default_property=member.name;}
   for(const auto& a:t.interfaces.read().members)for(const auto& b:t.interfaces.read().members)if(a.name==b.name && a.kind!=b.kind && !interface_property_match(a,b))throw Error("E_BYTECODE",{},"Invalid interface property signature");
   std::set<Type> parents;
   for(const auto parent:t.interfaces.read().parents){if(!valid(p.types,parent) || !interface_kind(info(p.types,parent)) || !parents.insert(parent).second)throw Error("E_BYTECODE",{},"Invalid interface parent");for(const auto& inherited:interface_contract(p.types,parent))if(std::find(t.interfaces.read().members.begin(),t.interfaces.read().members.end(),inherited)==t.interfaces.read().members.end())throw Error("E_BYTECODE",{},"Missing inherited interface member");}
   std::vector<std::pair<Type,std::size_t>> pending{{static_cast<Type>(64+i),0}};std::set<Type> active,done;
   while(!pending.empty()){auto& [type,next]=pending.back();if(!active.contains(type))active.insert(type);const auto& current=info(p.types,type);if(next==current.interfaces.read().parents.size()){active.erase(type);done.insert(type);pending.pop_back();continue;}const auto parent=current.interfaces.read().parents[next++];if(!valid(p.types,parent) || !interface_kind(info(p.types,parent)) || active.contains(parent))throw Error("E_BYTECODE",{},"Cyclic interface inheritance");if(!done.contains(parent))pending.push_back({parent,0});}
  }
  if(t.interfaces.read().implementations.size()>64)throw Error("E_BYTECODE",{},"Interface implementation limit exceeded");std::set<Type> targets;
  for(const auto& item:t.interfaces.read().implementations){
   if(!valid(p.types,item.target) || !interface_kind(info(p.types,item.target)) || !targets.insert(item.target).second)throw Error("E_BYTECODE",{},"Invalid interface implementation target");const auto members=interface_contract(p.types,item.target);if(item.routines.size()!=members.size())throw Error("E_BYTECODE",{},"Incomplete interface implementation");
   for(std::size_t n=0;n<members.size();++n){const auto id=item.routines[n];if(id>=p.routines.size())throw Error("E_BYTECODE",{},"Invalid interface implementation routine");const auto& routine=p.routines[id];const auto& contract=members[n];if(routine.owner!=static_cast<Type>(64+i) || routine.constructor || (routine.access==MemberAccess::explicit_interface_member&&contract.kind!=InterfaceMemberKind::method) || (contract.kind==InterfaceMemberKind::method && routine.access!=MemberAccess::public_member&&routine.access!=MemberAccess::explicit_interface_member) || routine.result!=contract.result || routine.parameters.size()!=contract.parameters.size()+1 || routine.parameters[0].type!=routine.owner || routine.parameters[0].by_reference || routine.parameters[0].variadic || !routine.captures.empty())throw Error("E_BYTECODE",{},"Incompatible interface implementation routine");for(std::size_t a=0;a<contract.parameters.size();++a){const auto& actual=routine.parameters[a+1];const auto& wanted=contract.parameters[a];if(actual.type!=wanted.type || actual.by_reference!=wanted.by_reference || actual.variadic!=wanted.variadic)throw Error("E_BYTECODE",{},"Incompatible interface implementation routine");}}
   for(auto parent:info(p.types,item.target).interfaces.read().parents)if(!implementation(t,parent))throw Error("E_BYTECODE",{},"Missing inherited interface implementation");
  }
  if(t.kind==CompoundKind::object_class)for(std::size_t id=0;id<p.routines.size();++id)if(p.routines[id].owner==static_cast<Type>(64+i)&&p.routines[id].access==MemberAccess::explicit_interface_member){bool used=false;for(const auto& target:t.interfaces.read().implementations)used|=std::find(target.routines.begin(),target.routines.end(),id)!=target.routines.end();if(!used)throw Error("E_BYTECODE",{},"Unused explicit interface implementation");}
 }
}
Value interface_cast(Values& values,Type from,Type to,bool checked,const Value& value){
 if(!interface_cast_valid(values.program,from,to,checked))throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 auto object=std::get<std::shared_ptr<Aggregate>>(value);if(!object || object->nil){if(type_info(values.program,to).kind==CompoundKind::record)throw runtime_error(RuntimeError::nil_reference,values.location);return values.zero(to);}
 if(object->type==to&&type_info(values.program,to).kind!=CompoundKind::record)return object;
 if(record_box_identity(object.get())!=object.get())object=object->sequence->source;
 if(object->sequence && object->sequence->source && (object->sequence->provider==SequenceProvider::user_source||object->sequence->provider==SequenceProvider::array||object->sequence->provider==SequenceProvider::list||object->sequence->provider==SequenceProvider::hash_set||dictionary_provider(object->sequence->provider)))object=object->sequence->source;
 if(type_info(values.program,to).kind==CompoundKind::record){if(object->boxed&&object->type==to)return record_interface_copy(values,object,false);throw Error("E_CAST",values.location,"Value cannot be unboxed to the requested record","Нельзя распаковать значение в заданный тип записи");}
 if(object && !object->nil && (object->type==to || interface_compatible(values.program.types,object->type,to) || native_enumerable_type(values.program.types,object->type,to))){
  if(!object->boxed&&native_record_interface_type(values.program.types,object->type,to))object=record_interface_copy(values,object,true);
  if(type_info(values.program,to).kind==CompoundKind::sequence){const auto key=std::make_pair(reinterpret_cast<std::uintptr_t>(object.get()),to);auto found=values.user_enumerables.find(key);if(found!=values.user_enumerables.end())if(auto cached=found->second.lock())return cached;
   auto result=type_info(values.program,object->type).kind==CompoundKind::object_class?sequence_create(values,to,object,SequenceProvider::user_source):native_collection_sequence(values,to,object);values.user_enumerables[key]=result;return result;}return object;
 }
 if(type_info(values.program,to).kind==CompoundKind::object_class)throw Error("E_CAST",values.location,"Value cannot be converted to the requested class","Нельзя преобразовать значение к заданному классу");
 throw Error("E_CAST",values.location,"Value does not implement the requested interface","Значение не реализует требуемый интерфейс");
}
std::size_t interface_target(Values& values,Type owner,std::size_t slot,const Value& receiver,const ExecutionOptions& options){
 if(!valid(values.program.types,owner) || !interface_kind(type_info(values.program,owner)))throw runtime_error(RuntimeError::invalid_bytecode,values.location);const auto contract=interface_contract(values.program.types,owner);if(slot>=contract.size())throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 if(!options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,values.location);if(options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,values.location);
 const auto object=std::get<std::shared_ptr<Aggregate>>(receiver);if(!object || object->nil)throw runtime_error(RuntimeError::nil_reference,values.location);const auto& type=type_info(values.program,object->type);const auto* table=implementation(type,owner);if(type.kind!=CompoundKind::object_class || !table || slot>=table->routines.size() )throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 return table->routines[slot];
}
Value interface_invoke(Values& values,Type owner,std::size_t slot,const Value& receiver,std::span<const Value> arguments,const ExecutionOptions& options){
 if(native_record_interface_target(values.program,owner,receiver))return native_record_interface_invoke(values,owner,slot,receiver,arguments,options);
 if(native_collection_target(values.program,owner,receiver))return native_collection_invoke(values,owner,slot,receiver,arguments,options);
 const auto id=interface_target(values,owner,slot,receiver,options);if(!values.invoke || arguments.size()+1!=values.program.routines[id].parameters.size())throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 std::vector<Value> input;input.reserve(arguments.size()+1);input.push_back(receiver);input.insert(input.end(),arguments.begin(),arguments.end());return values.invoke(id,input);
}
}

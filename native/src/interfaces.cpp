#include "interfaces.hpp"
#include "runtime_values.hpp"
#include "sequences.hpp"
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
bool interface_kind(const TypeInfo& type){return type.kind==CompoundKind::user_interface || type.kind==CompoundKind::enumerator || type.kind==CompoundKind::sequence;}
std::vector<InterfaceMember> interface_contract(const std::vector<TypeInfo>& types,Type type){
 if(!valid(types,type))return {};
 const auto& t=info(types,type);if(t.kind==CompoundKind::user_interface)return t.interfaces.read().members;
 if(t.kind==CompoundKind::sequence){for(std::size_t i=0;i<types.size();++i)if(types[i].kind==CompoundKind::enumerator && types[i].element==t.element)return {{"getenumerator",static_cast<Type>(64+i),{},InterfaceMemberKind::method}};throw Error("E_BYTECODE",{},"Invalid interface metadata");}
 if(t.kind==CompoundKind::enumerator)return {{"movenext",Type::boolean,{},InterfaceMemberKind::method},{"current",t.element,{},InterfaceMemberKind::get},{"dispose",Type::unit,{},InterfaceMemberKind::method},{"reset",Type::unit,{},InterfaceMemberKind::method}};
 return {};
}
bool interface_compatible(const std::vector<TypeInfo>& types,Type from,Type to){
 if(!valid(types,from) || !valid(types,to) || !interface_kind(info(types,to)))return false;
 if(from==to)return true;const auto& source=info(types,from);
 if(source.kind==CompoundKind::object_class)return implementation(source,to)!=nullptr;
 if(!interface_kind(source))return false;
 std::vector<Type> pending{from};std::set<Type> seen;
 while(!pending.empty()){const auto current=pending.back();pending.pop_back();if(current==to)return true;if(!valid(types,current) || !seen.insert(current).second)continue;for(auto parent:info(types,current).interfaces.read().parents)pending.push_back(parent);}
 return false;
}
bool interface_cast_valid(const Program& p,Type from,Type to,bool checked){
 return interface_compatible(p.types,from,to) || (checked && valid(p.types,from) && valid(p.types,to) && interface_kind(info(p.types,from)) && interface_kind(info(p.types,to)));
}
void verify_interfaces(const Program& p){
 std::size_t names=0;
 auto signature=[&](const InterfaceMember& member){
  if(member.name.empty() || member.parameters.size()>15 || (member.kind!=InterfaceMemberKind::method && member.kind!=InterfaceMemberKind::get && member.kind!=InterfaceMemberKind::set) || (member.result!=Type::unit && !valid_value_type(p,member.result)))throw Error("E_BYTECODE",{},"Invalid interface member signature");
  if(member.name.size()>1'048'576-names)throw Error("E_BYTECODE",{},"Interface metadata limit exceeded");names+=member.name.size();
  for(std::size_t i=0;i<member.parameters.size();++i){const auto& a=member.parameters[i];if(!valid_value_type(p,a.type) || (a.variadic && (i+1!=member.parameters.size() || a.by_reference || !compound(a.type) || type_info(p,a.type).kind!=CompoundKind::array || type_info(p,a.type).rank!=1)))throw Error("E_BYTECODE",{},"Invalid interface member signature");}
  if((member.kind==InterfaceMemberKind::get && (member.result==Type::unit || !member.parameters.empty())) || (member.kind==InterfaceMemberKind::set && (member.result!=Type::unit || member.parameters.size()!=1 || member.parameters[0].by_reference || member.parameters[0].variadic)))throw Error("E_BYTECODE",{},"Invalid interface property signature");
 };
 for(std::size_t i=0;i<p.types.size();++i){const auto& t=p.types[i];
  if(t.kind!=CompoundKind::user_interface && (!t.interfaces.read().parents.empty() || !t.interfaces.read().members.empty()))throw Error("E_BYTECODE",{},"Interface contract on a non-interface type");
  if(t.kind!=CompoundKind::object_class && !t.interfaces.read().implementations.empty())throw Error("E_BYTECODE",{},"Interface implementation on a non-class type");
  if(t.kind==CompoundKind::user_interface){
   if(t.name.empty() || t.element!=Type::invalid || t.key!=Type::invalid || !t.fields.empty() || t.interfaces.read().parents.size()>16 || t.interfaces.read().members.size()>256 || !t.parameters.empty() || !t.enum_members.empty() || t.enum_flags || t.rank!=1)throw Error("E_BYTECODE",{},"Invalid interface metadata");
   std::set<std::pair<std::string,InterfaceMemberKind>> keys;
   for(const auto& m:t.interfaces.read().members){signature(m);if(!keys.insert({m.name,m.kind}).second)throw Error("E_BYTECODE",{},"Duplicate interface member");}
   for(const auto& a:t.interfaces.read().members)for(const auto& b:t.interfaces.read().members)if(a.name==b.name && a.kind!=b.kind && (a.kind==InterfaceMemberKind::method || b.kind==InterfaceMemberKind::method || (a.kind==InterfaceMemberKind::get ? a.result : a.parameters[0].type)!=(b.kind==InterfaceMemberKind::get ? b.result : b.parameters[0].type)))throw Error("E_BYTECODE",{},"Invalid interface property signature");
   std::set<Type> parents;
   for(const auto parent:t.interfaces.read().parents){if(!valid(p.types,parent) || !interface_kind(info(p.types,parent)) || !parents.insert(parent).second)throw Error("E_BYTECODE",{},"Invalid interface parent");for(const auto& inherited:interface_contract(p.types,parent))if(std::find(t.interfaces.read().members.begin(),t.interfaces.read().members.end(),inherited)==t.interfaces.read().members.end())throw Error("E_BYTECODE",{},"Missing inherited interface member");}
   std::vector<std::pair<Type,std::size_t>> pending{{static_cast<Type>(64+i),0}};std::set<Type> active,done;
   while(!pending.empty()){auto& [type,next]=pending.back();if(!active.contains(type))active.insert(type);const auto& current=info(p.types,type);if(next==current.interfaces.read().parents.size()){active.erase(type);done.insert(type);pending.pop_back();continue;}const auto parent=current.interfaces.read().parents[next++];if(!valid(p.types,parent) || !interface_kind(info(p.types,parent)) || active.contains(parent))throw Error("E_BYTECODE",{},"Cyclic interface inheritance");if(!done.contains(parent))pending.push_back({parent,0});}
  }
  if(t.interfaces.read().implementations.size()>64)throw Error("E_BYTECODE",{},"Interface implementation limit exceeded");std::set<Type> targets;
  for(const auto& item:t.interfaces.read().implementations){
   if(!valid(p.types,item.target) || !interface_kind(info(p.types,item.target)) || !targets.insert(item.target).second)throw Error("E_BYTECODE",{},"Invalid interface implementation target");const auto members=interface_contract(p.types,item.target);if(item.routines.size()!=members.size())throw Error("E_BYTECODE",{},"Incomplete interface implementation");
   for(std::size_t n=0;n<members.size();++n){const auto id=item.routines[n];if(id>=p.routines.size())throw Error("E_BYTECODE",{},"Invalid interface implementation routine");const auto& routine=p.routines[id];const auto& contract=members[n];if(routine.owner!=static_cast<Type>(64+i) || routine.constructor || (contract.kind==InterfaceMemberKind::method && routine.access!=MemberAccess::public_member) || routine.result!=contract.result || routine.parameters.size()!=contract.parameters.size()+1 || routine.parameters[0].type!=routine.owner || routine.parameters[0].by_reference || routine.parameters[0].variadic || !routine.captures.empty())throw Error("E_BYTECODE",{},"Incompatible interface implementation routine");for(std::size_t a=0;a<contract.parameters.size();++a){const auto& actual=routine.parameters[a+1];const auto& wanted=contract.parameters[a];if(actual.type!=wanted.type || actual.by_reference!=wanted.by_reference || actual.variadic!=wanted.variadic)throw Error("E_BYTECODE",{},"Incompatible interface implementation routine");}}
   for(auto parent:info(p.types,item.target).interfaces.read().parents)if(!implementation(t,parent))throw Error("E_BYTECODE",{},"Missing inherited interface implementation");
  }
 }
}
Value interface_cast(Values& values,Type from,Type to,bool checked,const Value& value){
 if(!interface_cast_valid(values.program,from,to,checked))throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 auto object=std::get<std::shared_ptr<Aggregate>>(value);if(!object || object->nil)return values.zero(to);
 if(object->sequence && object->sequence->provider==SequenceProvider::user_source)object=object->sequence->source;
 if(object && !object->nil && interface_compatible(values.program.types,object->type,to)){
  if(type_info(values.program,to).kind==CompoundKind::sequence){const auto key=std::make_pair(reinterpret_cast<std::uintptr_t>(object.get()),to);auto found=values.user_enumerables.find(key);if(found!=values.user_enumerables.end())if(auto cached=found->second.lock())return cached;
   auto result=sequence_create(values,to,object,SequenceProvider::user_source);values.user_enumerables[key]=result;return result;}return object;
 }
 throw Error("E_CAST",values.location,"Value does not implement the requested interface","Значение не реализует требуемый интерфейс");
}
std::size_t interface_target(Values& values,Type owner,std::size_t slot,const Value& receiver,const ExecutionOptions& options){
 if(!valid(values.program.types,owner) || !interface_kind(type_info(values.program,owner)))throw runtime_error(RuntimeError::invalid_bytecode,values.location);const auto contract=interface_contract(values.program.types,owner);if(slot>=contract.size())throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 if(!options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,values.location);if(options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,values.location);
 const auto object=std::get<std::shared_ptr<Aggregate>>(receiver);if(!object || object->nil)throw runtime_error(RuntimeError::nil_reference,values.location);const auto& type=type_info(values.program,object->type);const auto* table=implementation(type,owner);if(type.kind!=CompoundKind::object_class || !table || slot>=table->routines.size() )throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 return table->routines[slot];
}
Value interface_invoke(Values& values,Type owner,std::size_t slot,const Value& receiver,std::span<const Value> arguments,const ExecutionOptions& options){
 const auto id=interface_target(values,owner,slot,receiver,options);if(!values.invoke || arguments.size()+1!=values.program.routines[id].parameters.size())throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 std::vector<Value> input;input.reserve(arguments.size()+1);input.push_back(receiver);input.insert(input.end(),arguments.begin(),arguments.end());return values.invoke(id,input);
}
}

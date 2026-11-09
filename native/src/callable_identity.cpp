#include "callable_identity.hpp"
#include "runtime_values.hpp"
namespace abc {
bool callable_compatible(const std::vector<TypeInfo>& types,Type from,Type to){
 if(!compound(from)||!compound(to)||static_cast<unsigned>(from)-64U>=types.size()||static_cast<unsigned>(to)-64U>=types.size())return false;
 const auto& a=types[static_cast<unsigned>(from)-64U];const auto& b=types[static_cast<unsigned>(to)-64U];
 if(a.kind!=CompoundKind::callable||b.kind!=CompoundKind::callable||a.element!=b.element||a.parameters.size()!=b.parameters.size())return false;
 for(std::size_t n=0;n<a.parameters.size();++n)if(a.parameters[n].type!=b.parameters[n].type||a.parameters[n].by_reference!=b.parameters[n].by_reference)return false;
 return true;
}
bool callable_equal(const Aggregate& a,const Aggregate& b){
 if(a.nil||b.nil)return a.nil==b.nil;
 if(a.type!=b.type)return false;
 if(a.delegate_source||b.delegate_source)return a.delegate_source&&a.delegate_source==b.delegate_source;
 return a.routine==b.routine&&a.captures==b.captures;
}
std::size_t callable_hash(const Aggregate& value){
 if(value.nil)return 0;
 std::size_t result=static_cast<unsigned>(value.type);
 const auto mix=[&](std::size_t n){result^=n+static_cast<std::size_t>(0x9e3779b9U)+(result<<6U)+(result>>2U);};
 if(value.delegate_source)mix(std::hash<const void*>{}(value.delegate_source.get()));
 else {mix(value.routine.value_or(0));for(const auto& cell:value.captures)mix(std::hash<const void*>{}(cell.get()));}
 return result;
}
Value callable_conversion(Values& values,Type to,const Value& value,bool require_non_nil){
 const auto source=std::get<std::shared_ptr<Aggregate>>(value);
 if(!source||source->nil){if(require_non_nil)throw Error("E_ARGUMENT",values.location,"Delegate target cannot be nil","Цель делегата не может быть nil");return values.zero(to);}
 if(!callable_compatible(values.program.types,source->type,to)||!source->routine)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 if(source->type==to)return source;
 if(!values.remaining)throw runtime_error(RuntimeError::allocation_limit,values.location);--values.remaining;
 auto result=std::get<std::shared_ptr<Aggregate>>(values.zero(to,true));result->routine=source->routine;result->captures=source->captures;result->delegate_source=source;
 if(values.observe_object)values.observe_object(result);return result;
}
}

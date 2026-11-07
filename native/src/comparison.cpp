#include "comparison.hpp"
#include "comparable.hpp"
#include "interfaces.hpp"
#include "runtime_values.hpp"
#include "enumeration.hpp"
#include "big_integer_runtime.hpp"
#include <array>
#include <cmath>
#include <type_traits>
namespace abc {
namespace {
using Object=std::shared_ptr<Aggregate>;
bool nil(const Value& value){if(std::holds_alternative<NilString>(value))return true;if(const auto* p=std::get_if<Object>(&value))return !*p || (*p)->nil;if(const auto* p=std::get_if<std::shared_ptr<TextFile>>(&value))return !*p;if(const auto* p=std::get_if<ReflectedType>(&value))return p->target==Type::invalid;return false;}
[[noreturn]] void unsupported(SourceLocation location){throw Error("E_COMPARE",location,"Values do not support comparison","Значения не поддерживают сравнение");}
}
Type comparable_contract(const std::vector<TypeInfo>& types,Type element){for(std::size_t i=0;i<types.size();++i)if(types[i].kind==CompoundKind::comparable_interface && types[i].element==element)return static_cast<Type>(64+i);return Type::invalid;}
bool default_comparable(const std::vector<TypeInfo>& types,Type element){if(scalar(element) || enumeration(types,element))return true;const auto target=comparable_contract(types,element);return target!=Type::invalid && interface_compatible(types,element,target);}
int comparison_value(Values& values,Type type,const Value& a,const Value& b,bool null_safe,bool ordinal_strings,const ExecutionOptions& options){
 if(!options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,values.location);if(options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,values.location);
 const bool an=nil(a),bn=nil(b);if(null_safe && (an || bn))return an ? bn ? 0 : -1 : 1;
 if(type==Type::string){if(ordinal_strings)return compare_strings(a,b);if(an)throw runtime_error(RuntimeError::nil_reference,values.location);if(bn)return 1;return ordinal_strings ? compare_strings(a,b) : compare_culture_strings(string_text(a),string_text(b),values.location,options);}
 if(compound(type) && !enumeration(values.program,type)){
  const auto target=comparable_contract(values.program.types,type);
  if(target!=Type::invalid && interface_compatible(values.program.types,type,target)){if(an)throw runtime_error(RuntimeError::nil_reference,values.location);const std::array<Value,1> arguments{b};return std::get<std::int32_t>(interface_invoke(values,target,0,a,arguments,options));}
  const auto kind=type_info(values.program,type).kind;if(null_safe && kind!=CompoundKind::record && std::get<Object>(a)==std::get<Object>(b))return 0;
  unsupported(values.location);
 }
 if(!scalar(type) && !enumeration(values.program,type)){if(null_safe && values.equal(a,b))return 0;unsupported(values.location);}
 if(const auto* number=std::get_if<BigInteger>(&a))big_work((number->bits()+std::get<BigInteger>(b).bits())/28+1,values.location,options);
 return std::visit([&](const auto& x)->int {using T=std::decay_t<decltype(x)>;
  if constexpr(std::is_arithmetic_v<T> || std::is_same_v<T,BigInteger>){const auto y=std::get<T>(b);if constexpr(std::is_integral_v<T> && !std::is_same_v<T,bool> && sizeof(T)<4)return int(x)-int(y);if constexpr(std::is_floating_point_v<T>){if(std::isnan(x))return std::isnan(y) ? 0 : -1;if(std::isnan(y))return 1;}return x<y ? -1 : x>y ? 1 : 0;}
  else if constexpr(std::is_same_v<T,EnumValue>){const auto y=std::get<T>(b);return x.value<y.value ? -1 : x.value>y.value ? 1 : 0;}
  else{unsupported(values.location);}
 },a);
}
bool comparer_is_default(const Aggregate& value){return value.items.size()==1 && std::get<Object>(value.items[0]->value)->nil;}
Value comparer_create(Values& values,Type owner,const Value* callback,const ExecutionOptions& options){
 const auto& type=type_info(values.program,owner);if(type.kind!=CompoundKind::comparer)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 if(!options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,values.location);if(options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,values.location);
 if(callback)(void)values.callable_target(std::get<Object>(*callback));
 else for(const auto& item:values.default_comparers)if(item.first==owner)if(auto cached=item.second.lock())return cached;
 if(!values.remaining)throw runtime_error(RuntimeError::allocation_limit,values.location);--values.remaining;
 auto result=std::get<Object>(values.zero(owner,true));result->items.push_back(values.cell(type.key,callback ? *callback : values.zero(type.key)));
 if(values.observe_object)values.observe_object(result);if(!callback)values.default_comparers.push_back({owner,result});return result;
}
Value comparer_invoke(Values& values,Type owner,const Value& receiver,std::span<const Value> arguments,const ExecutionOptions& options){
 const auto& type=type_info(values.program,owner);if(type.kind!=CompoundKind::comparer_interface || arguments.size()!=2)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 const auto object=std::get<Object>(receiver);if(!object || object->nil)throw runtime_error(RuntimeError::nil_reference,values.location);
 if(type_info(values.program,object->type).kind==CompoundKind::object_class)return interface_invoke(values,owner,0,receiver,arguments,options);
 const auto& actual=type_info(values.program,object->type);if(actual.kind!=CompoundKind::comparer || actual.element!=type.element || object->items.size()!=1)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 const auto function=std::get<Object>(object->items[0]->value);
 if(function->nil)return std::int32_t{comparison_value(values,type.element,arguments[0],arguments[1],true,false,options)};
 return values.invoke_value(function,arguments);
}
}

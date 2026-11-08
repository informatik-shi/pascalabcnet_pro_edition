#include "equality.hpp"
#include "interfaces.hpp"
#include "enumeration.hpp"
#include "runtime_values.hpp"
#include "value_hash.hpp"
#include "big_integer_runtime.hpp"
#include <array>
#include <bit>
#include <charconv>
#include <cmath>
#include <type_traits>
namespace abc {
namespace {
using Object=std::shared_ptr<Aggregate>;
bool nil(const Value& v){if(std::holds_alternative<NilString>(v))return true;if(const auto* p=std::get_if<Object>(&v))return !*p || (*p)->nil;if(const auto* p=std::get_if<std::shared_ptr<TextFile>>(&v))return !*p;if(const auto* p=std::get_if<ReflectedType>(&v))return p->target==Type::invalid;return false;}
void guard(Values& values,const ExecutionOptions& options){if(!options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,values.location);if(options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,values.location);}
std::int32_t signed_hash(std::uint32_t n){return std::bit_cast<std::int32_t>(n);}
}
bool equality_nil(const Value& value){return nil(value);}
Type equatable_contract(const std::vector<TypeInfo>& types,Type element){for(std::size_t i=0;i<types.size();++i)if(types[i].kind==CompoundKind::equatable_interface && types[i].element==element)return static_cast<Type>(64+i);return Type::invalid;}
bool default_equatable(const std::vector<TypeInfo>& types,Type element){if(scalar(element))return true;const auto contract=equatable_contract(types,element);return contract!=Type::invalid && interface_compatible(types,element,contract);}
bool equality_value(Values& values,Type type,const Value& a,const Value& b,const ExecutionOptions& options){
 guard(values,options);const bool an=nil(a),bn=nil(b);if(an || bn)return an==bn;
 const auto contract=equatable_contract(values.program.types,type);
 if(contract!=Type::invalid && interface_compatible(values.program.types,type,contract)){const std::array<Value,1> arguments{b};return std::get<bool>(interface_invoke(values,contract,0,a,arguments,options));}
 ValueHashWork work{values,options};return value_equal(work,a,b);
}
std::int32_t equality_hash(Values& values,Type type,const Value& value,const ExecutionOptions& options){
 guard(values,options);if(nil(value))return 0;ValueHashWork work{values,options};
 if(type==Type::string){std::uint32_t first=5381,second=5381;const auto& s=string_text(value);std::size_t i=0;while(i<s.size() && s[i]){work.step();first=(first*33U)^s[i++];if(i==s.size() || !s[i])break;work.step();second=(second*33U)^s[i++];}return signed_hash(first+second*1566083941U);}
 if(type==Type::big_integer){const auto& v=std::get<BigInteger>(value);big_work(v.bits()/16+1,values.location,options);auto s=v.text(16);const bool negative=!s.empty() && s[0]=='-';if(negative)s.erase(0,1);if(s.size()<=8){std::uint32_t n=0;std::from_chars(s.data(),s.data()+s.size(),n,16);if(n<=2147483647U)return signed_hash(negative ? 0U-n : n);}std::uint32_t result=negative ? 0xffffffffU : 1U;std::size_t pos=0;auto count=s.size()%8;if(!count)count=8;while(pos<s.size()){work.step();std::uint32_t n=0;std::from_chars(s.data()+pos,s.data()+pos+count,n,16);result=std::rotl(result,7)^n;pos+=count;count=8;}return signed_hash(result);}
 return std::visit([&](const auto& v)->std::int32_t{using T=std::decay_t<decltype(v)>;
  if constexpr(std::is_same_v<T,float>)return v==0 ? 0 : signed_hash(std::bit_cast<std::uint32_t>(v));
  else if constexpr(std::is_same_v<T,double>){if(v==0)return 0;const auto bits=std::bit_cast<std::uint64_t>(v);return signed_hash(static_cast<std::uint32_t>(bits)^static_cast<std::uint32_t>(bits>>32U));}
  else if constexpr(std::is_same_v<T,std::int64_t> || std::is_same_v<T,std::uint64_t>){const auto bits=static_cast<std::uint64_t>(v);return signed_hash(static_cast<std::uint32_t>(bits)^static_cast<std::uint32_t>(bits>>32U));}
  else if constexpr(std::is_same_v<T,std::int16_t>){const auto n=static_cast<std::uint32_t>(static_cast<std::int32_t>(v));return signed_hash(static_cast<std::uint16_t>(v)|(n<<16U));}
  else if constexpr(std::is_same_v<T,std::int8_t>){const auto n=static_cast<std::uint32_t>(static_cast<std::int32_t>(v));return signed_hash(n^(n<<8U));}
  else if constexpr(std::is_same_v<T,char16_t>){const auto n=static_cast<std::uint32_t>(v);return signed_hash(n|(n<<16U));}
  else if constexpr(std::is_integral_v<T>)return signed_hash(static_cast<std::uint32_t>(v));
  else if constexpr(std::is_same_v<T,EnumValue>)return v.value;
  else return signed_hash(static_cast<std::uint32_t>(value_hash(work,value)));
 },value);
}
Value equality_create(Values& values,Type owner,const ExecutionOptions& options){
 guard(values,options);if(type_info(values.program,owner).kind!=CompoundKind::equality_comparer)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 for(const auto& cached:values.default_equalities)if(cached.first==owner)if(auto object=cached.second.lock())return object;
 if(!values.remaining)throw runtime_error(RuntimeError::allocation_limit,values.location);--values.remaining;auto object=std::get<Object>(values.zero(owner,true));if(values.observe_object)values.observe_object(object);values.default_equalities.push_back({owner,object});return object;
}
Value equality_invoke(Values& values,Type owner,std::size_t slot,const Value& receiver,std::span<const Value> arguments,const ExecutionOptions& options){
 const auto& type=type_info(values.program,owner);if(type.kind!=CompoundKind::equality_comparer_interface || slot>1 || arguments.size()!=(slot ? 1U : 2U))throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 const auto object=std::get<Object>(receiver);if(!object || object->nil)throw runtime_error(RuntimeError::nil_reference,values.location);
 const auto& actual=type_info(values.program,object->type);if(actual.kind==CompoundKind::object_class)return interface_invoke(values,owner,slot,receiver,arguments,options);
 if(actual.kind!=CompoundKind::equality_comparer || actual.element!=type.element)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 if(slot)return equality_hash(values,type.element,arguments[0],options);return equality_value(values,type.element,arguments[0],arguments[1],options);
}
Type equality_contract(const std::vector<TypeInfo>& types,Type element){for(std::size_t i=0;i<types.size();++i)if(types[i].kind==CompoundKind::equality_comparer_interface && types[i].element==element)return static_cast<Type>(64+i);return Type::invalid;}
bool equality_default(const Program& p,const std::shared_ptr<Aggregate>& comparer){return !comparer || comparer->nil || type_info(p,comparer->type).kind==CompoundKind::equality_comparer;}
bool equality_policy_same(const Program& p,const std::shared_ptr<Aggregate>& a,const std::shared_ptr<Aggregate>& b){return a==b || (equality_default(p,a) && equality_default(p,b));}
bool equality_policy_value(Values& values,Type element,const std::shared_ptr<Aggregate>& comparer,const Value& a,const Value& b,const ExecutionOptions& options){if(equality_default(values.program,comparer))return equality_value(values,element,a,b,options);const auto contract=equality_contract(values.program.types,element);if(contract==Type::invalid)throw runtime_error(RuntimeError::invalid_bytecode,values.location);const std::array<Value,2> arguments{a,b};return std::get<bool>(equality_invoke(values,contract,0,comparer,arguments,options));}
std::int32_t equality_policy_hash(Values& values,Type element,const std::shared_ptr<Aggregate>& comparer,const Value& a,const ExecutionOptions& options){if(equality_default(values.program,comparer))return equality_hash(values,element,a,options);const auto contract=equality_contract(values.program.types,element);if(contract==Type::invalid)throw runtime_error(RuntimeError::invalid_bytecode,values.location);const std::array<Value,1> arguments{a};return std::get<std::int32_t>(equality_invoke(values,contract,1,comparer,arguments,options));}
std::int32_t equality_index(Values& values,Type element,std::span<const std::shared_ptr<Cell>> cells,const Value& item,std::int32_t start,std::int32_t count,bool last,const ExecutionOptions& options,std::uint64_t* budget){
 ValueHashWork work{values,options,budget ? *budget : 0};for(std::int64_t n=0;n<count;++n){work.step();if(budget)*budget=work.done;const auto index=static_cast<std::int64_t>(start)+(last ? -n : n);if(index<0 || static_cast<std::uint64_t>(index)>=cells.size())throw runtime_error(RuntimeError::index_bounds,values.location);const auto cell=cells[static_cast<std::size_t>(index)];const auto value=cell ? cell->value : values.zero(element,false,true);if(equality_value(values,element,value,item,options))return static_cast<std::int32_t>(index);}
 return -1;
}
std::uint32_t equality_implementation(const Program& p,Type element){return element==Type::byte ? 20U : enumeration(p,element) ? 21U : default_equatable(p.types,element) ? 18U : 19U;}
}

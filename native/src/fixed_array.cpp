#include "fixed_array.hpp"
#include "runtime_values.hpp"
#include "record_initialization.hpp"
#include "integer_values.hpp"
#include <algorithm>
#include <array>
#include <bit>
namespace abc {
namespace {
const TypeInfo& info(const std::vector<TypeInfo>& types,Type t){return types.at(static_cast<unsigned>(t)-64U);}
const FixedArrayShape& shape(std::span<const FixedArrayShape> shapes,Type t){const auto p=std::find_if(shapes.begin(),shapes.end(),[&](const auto& a){return a.type==t;});if(p==shapes.end())throw runtime_error(RuntimeError::invalid_bytecode,{});return *p;}
bool fixed(const std::vector<TypeInfo>& types,Type t){return compound(t)&&static_cast<unsigned>(t)-64U<types.size()&&info(types,t).kind==CompoundKind::fixed_array;}
std::shared_ptr<Aggregate> object(const Value& v,SourceLocation location){const auto p=std::get<std::shared_ptr<Aggregate>>(v);if(!p||p->nil)throw runtime_error(RuntimeError::nil_reference,location);return p;}
std::shared_ptr<Aggregate> storage(const Value& v,SourceLocation location){const auto p=object(v,location);if(p->items.size()!=1)throw runtime_error(RuntimeError::invalid_bytecode,location);return object(p->items[0]->value,location);}
}
const FixedArrayShape& fixed_array_shape(const Program& p,Type t){return shape(p.fixed_arrays,t);}
bool pascal_copy_type(const std::vector<TypeInfo>& types,Type t){const auto walk=[&](auto&& self,Type type,unsigned depth)->bool{if(depth>64||!compound(type)||static_cast<unsigned>(type)-64U>=types.size())return false;const auto& s=info(types,type);if(s.kind==CompoundKind::fixed_array)return true;if(s.kind!=CompoundKind::record)return false;return std::any_of(s.fields.begin(),s.fields.end(),[&](const auto& f){return self(self,f.type,depth+1);});};return walk(walk,t,0);}
Value fixed_array_construct(Values& values,Type type){
 const auto location=values.location;
 const auto& s=fixed_array_shape(values.program,type);const auto& descriptor=type_info(values.program,type);const auto count=static_cast<std::size_t>(static_cast<std::int64_t>(s.upper)-s.lower+1);
 auto data=values.array(descriptor.key,count);initialize_record_array(values,data,record_initializer_operand(values.program,descriptor.element));values.location=location;
 if(!values.remaining)throw runtime_error(RuntimeError::allocation_limit,values.location);--values.remaining;
 auto result=std::make_shared<Aggregate>();result->type=type;result->items.push_back(values.cell(descriptor.key,std::move(data)));return result;
}
Value pascal_value_copy(Values& values,Type type,const Value& value,bool initialize){
 const auto location=values.location;
 const auto& descriptor=type_info(values.program,type);
 if(descriptor.kind==CompoundKind::fixed_array){
  // The emitted source Clone constructs its destination before dereferencing
  // the source, so record element initializers run even for a nil source.
  auto result=fixed_array_construct(values,type);const auto input=storage(value,values.location),output=storage(result,values.location);
  if(input->items.size()!=output->items.size())throw runtime_error(RuntimeError::invalid_bytecode,values.location);
  for(std::size_t n=0;n<input->items.size();++n){const auto& item=input->items[n];values.write(output->items[n],pascal_copy_type(values.program.types,item->type)?pascal_value_copy(values,item->type,item->value,true):item->value,false);}return result;
 }
 auto result=initialize?initialized_default(values,type,false,false,record_initializer_operand(values.program,type)):values.zero(type,false,true);values.location=location;
 const auto input=object(value,values.location),output=object(result,values.location);
 for(std::size_t n=0;n<descriptor.fields.size();++n){const auto element=descriptor.fields[n].type;values.write(output->items[n],pascal_copy_type(values.program.types,element)?pascal_value_copy(values,element,input->items[n]->value,initialize):input->items[n]->value,false);}return result;
}
std::vector<Type> fixed_parameters(const std::vector<TypeInfo>& types,std::span<const FixedArrayShape> shapes,Type owner,std::size_t code){
 if(code==static_cast<unsigned>(FixedMethod::index))return {shape(shapes,owner).index};if(code>=static_cast<unsigned>(FixedMethod::equal))return {owner,owner};(void)types;return {owner};
}
Type fixed_result(const std::vector<TypeInfo>& types,Type owner,std::size_t code){return code>=static_cast<unsigned>(FixedMethod::equal)?Type::boolean:code==static_cast<unsigned>(FixedMethod::index)?Type::integer:code<=static_cast<unsigned>(FixedMethod::index_view)?info(types,owner).key:owner;}
bool fixed_signature_valid(const Program& p,Type owner,std::size_t code){
 if(code>static_cast<unsigned>(FixedMethod::not_equal)||!pascal_copy_type(p.types,owner))return false;
 return code==static_cast<unsigned>(FixedMethod::copy_value)||code==static_cast<unsigned>(FixedMethod::copy_assign)||fixed(p.types,owner);
}
Value fixed_invoke(Values& values,Type owner,std::size_t code,std::span<const Value> args){
 if(code>=static_cast<unsigned>(FixedMethod::equal)){
  const auto a=storage(args[0],values.location),b=storage(args[1],values.location);bool same=true;
  for(std::size_t n=0;n<a->items.size();++n){const auto& x=a->items[n];const auto& y=b->items.at(n);
   if(fixed(values.program.types,x->type)){const std::array<Value,2> pair{pascal_value_copy(values,x->type,x->value,true),pascal_value_copy(values,x->type,y->value,true)};same=std::get<bool>(fixed_invoke(values,x->type,static_cast<unsigned>(FixedMethod::equal),pair));}
   else same=values.equal(x->value,y->value);
   if(!same)break;
  }return code==static_cast<unsigned>(FixedMethod::equal)?same:!same;
 }
 if(code==static_cast<unsigned>(FixedMethod::copy_value)||code==static_cast<unsigned>(FixedMethod::copy_assign))return pascal_value_copy(values,owner,args[0],code==static_cast<unsigned>(FixedMethod::copy_value));
 if(code==static_cast<unsigned>(FixedMethod::index)){const auto& s=fixed_array_shape(values.program,owner);std::uint32_t index=0;if(s.index==Type::character)index=std::get<char16_t>(args[0]);else if(s.index==Type::boolean)index=std::get<bool>(args[0]);else if(compound(s.index))index=static_cast<std::uint32_t>(std::get<EnumValue>(args[0]).value);else index=static_cast<std::uint32_t>(integral_number(args[0],values.location,"index"));return std::bit_cast<std::int32_t>(index-static_cast<std::uint32_t>(s.lower));}
 if(code==static_cast<unsigned>(FixedMethod::index_view)){const auto p=std::get<std::shared_ptr<Aggregate>>(args[0]);if(!p||p->nil)return values.zero(type_info(values.program,owner).key,false,true);}
 return storage(args[0],values.location);
}
}

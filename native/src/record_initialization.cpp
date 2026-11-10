#include "record_initialization.hpp"
#include "fixed_array.hpp"
#include "runtime_values.hpp"
#include <array>
namespace abc {
std::size_t record_initializer_operand(const Program& p,Type type){
 if(!compound(type)||static_cast<unsigned>(type)-64U>=p.types.size())return 0;
 const auto& info=type_info(p,type);if(info.kind!=CompoundKind::record||info.native_record!=NativeRecord::none)return 0;
 const auto name=info.name+".$initialize_fields";std::size_t result=0;
 for(std::size_t n=0;n<p.routines.size();++n){const auto& r=p.routines[n];if(r.owner!=type||r.name!=name)continue;
  if(result||r.constructor||r.access!=MemberAccess::private_member||r.result!=Type::unit||r.parameters.size()!=1||r.parameters[0].type!=type||!r.parameters[0].by_reference||r.parameters[0].variadic||!r.captures.empty())throw runtime_error(RuntimeError::invalid_bytecode,{});result=n+1;
 }return result;
}
namespace {
void initialize(Values& values,const std::shared_ptr<Cell>& cell,std::size_t operand){
 if(!operand)return;if(!values.invoke||operand>values.program.routines.size())throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 const std::array<Value,1> argument{cell};(void)values.invoke(operand-1,argument);
}
}
Value initialized_default(Values& values,Type type,bool constructed,bool generic,std::size_t initializer){
 if(compound(type)&&type_info(values.program,type).kind==CompoundKind::fixed_array&&!generic)return fixed_array_construct(values,type);
 if(!generic&&compound(type)&&type_info(values.program,type).kind==CompoundKind::binary_file&&type_info(values.program,type).element!=Type::invalid)binary_element_probe(values,type_info(values.program,type).element);
 auto result=values.zero(type,constructed,generic);if(initializer)initialize(values,values.cell(type,result),initializer);return result;
}
void initialize_record_array(Values& values,const std::shared_ptr<Aggregate>& array,std::size_t initializer,std::size_t first){
 if(compound(type_info(values.program,array->type).element)&&type_info(values.program,type_info(values.program,array->type).element).kind==CompoundKind::fixed_array){for(std::size_t n=first;n<array->items.size();++n)array->items[n]->value=fixed_array_construct(values,type_info(values.program,array->type).element);return;}
 if(compound(type_info(values.program,array->type).element)&&type_info(values.program,type_info(values.program,array->type).element).kind==CompoundKind::binary_file&&pascal_copy_type(values.program.types,type_info(values.program,type_info(values.program,array->type).element).element)){for(std::size_t n=first;n<array->items.size();++n)array->items[n]->value=initialized_default(values,type_info(values.program,array->type).element,false,false,0);return;}
 if(!initializer)return;
 for(std::size_t n=first;n<array->items.size();++n)initialize(values,array->items[n],initializer);
}
}

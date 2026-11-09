#include "record_interface.hpp"
#include "runtime_values.hpp"
#include "interfaces.hpp"
#include "ordinal_range.hpp"
#include "sequences.hpp"
#include "enumerator.hpp"
#include <array>
namespace abc {
bool native_record_interface_type(const std::vector<TypeInfo>& types,Type from,Type to){
 if(!compound(from)||!compound(to)||static_cast<unsigned>(from)-64U>=types.size()||static_cast<unsigned>(to)-64U>=types.size())return false;
 const auto& source=types[static_cast<unsigned>(from)-64U];const auto& target=types[static_cast<unsigned>(to)-64U];
 return ordinal_range_type(source)&&(((target.kind==CompoundKind::collection_interface||target.kind==CompoundKind::sequence)&&source.element==target.element)||(target.kind==CompoundKind::equatable_interface&&target.element==from));
}
std::shared_ptr<Aggregate> record_interface_copy(Values& values,const std::shared_ptr<Aggregate>& source,bool boxed){
 if(!source||source->nil||type_info(values.program,source->type).kind!=CompoundKind::record)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 auto result=std::make_shared<Aggregate>();result->type=source->type;result->boxed=boxed;
 for(const auto& field:source->items)result->items.push_back(values.cell(field->type,values.copy(field->value)));
 if(boxed&&values.observe_object)values.observe_object(result);return result;
}
bool native_record_interface_target(const Program& program,Type owner,const Value& receiver){
 const auto* object=std::get_if<std::shared_ptr<Aggregate>>(&receiver);return object&&*object&&native_record_interface_type(program.types,(*object)->type,owner);
}
Value native_record_interface_invoke(Values& values,Type owner,std::size_t slot,const Value& receiver,std::span<const Value> arguments,const ExecutionOptions& options){
 if(!native_record_interface_target(values.program,owner,receiver)||slot>=interface_contract(values.program.types,owner).size()||arguments.size()!=interface_contract(values.program.types,owner)[slot].parameters.size())throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 if(!options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,values.location);if(options.cancelled&&options.cancelled())throw runtime_error(RuntimeError::cancelled,values.location);
 const auto object=std::get<std::shared_ptr<Aggregate>>(receiver);if(object->nil)throw runtime_error(RuntimeError::nil_reference,values.location);
 const auto& target=type_info(values.program,owner);
 if(target.kind==CompoundKind::equatable_interface){const std::array<Value,2> input{receiver,arguments[0]};return ordinal_range_invoke(values,object->type,OrdinalRangeMethod::equal,input,options);}
 const auto sequence=target.kind==CompoundKind::sequence?owner:target.interfaces.read().parents[0];
 if(target.kind==CompoundKind::sequence||slot==7){const auto iterator=interface_contract(values.program.types,sequence)[0].result;return enumerator_invoke(values,sequence,iterator,EnumeratorMethod::get,sequence_create(values,sequence,object,SequenceProvider::ordinal_range),options);}
 if(slot==0)return ordinal_range_count(object);if(slot==1)return true;
 if(slot==2||slot==3||slot==6)throw Error("E_READONLY",values.location,"Collection is read-only","Коллекция доступна только для чтения");
 std::vector<Value> input{receiver};input.insert(input.end(),arguments.begin(),arguments.end());
 return ordinal_range_invoke(values,object->type,slot==4?OrdinalRangeMethod::contains:OrdinalRangeMethod::copy_to,input,options);
}
}

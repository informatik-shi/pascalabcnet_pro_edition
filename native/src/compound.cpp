#include "compound.hpp"
#include "runtime_values.hpp"
#include "big_integer_runtime.hpp"
#include "list_operations.hpp"
#include "stdlib.hpp"
#include <array>

namespace abc {
bool compound_reference(CompoundMethod method){return method<=CompoundMethod::big_multiply;}
bool compound_signature(const Program& program,Type owner,CompoundMethod method) {
    if(method>CompoundMethod::list_repeat_reverse)return false;
    if(method<=CompoundMethod::text_repeat)return owner==Type::string;
    if(method<=CompoundMethod::big_multiply)return owner==Type::big_integer;
    if(!compound(owner) || static_cast<unsigned>(owner)-64U>=program.types.size())return false;
    const auto kind=type_info(program,owner).kind;
    return method==CompoundMethod::endpoint_add ? endpoint_collection(kind) : kind==CompoundKind::list;
}
std::vector<Type> compound_parameters(const Program& program,Type owner,CompoundMethod method) {return compound_parameters(program.types,owner,method);}
std::vector<Type> compound_parameters(const std::vector<TypeInfo>& types,Type owner,CompoundMethod method) {
    if(method==CompoundMethod::text_add)return {reference_type(owner),Type::string};
    if(method==CompoundMethod::text_integer || method==CompoundMethod::text_repeat)return {reference_type(owner),Type::integer};
    if(method==CompoundMethod::text_real)return {reference_type(owner),Type::real};
    if(method<=CompoundMethod::big_multiply)return {reference_type(owner),owner};
    if(method==CompoundMethod::list_repeat)return {owner,Type::integer};
    if(method==CompoundMethod::list_repeat_reverse)return {Type::integer,owner};
    return {owner,method==CompoundMethod::list_add || method==CompoundMethod::endpoint_add ? types.at(static_cast<unsigned>(owner)-64U).element : owner};
}
Type compound_result(Type owner,CompoundMethod method) {
    return method==CompoundMethod::list_concat || method==CompoundMethod::list_repeat || method==CompoundMethod::list_repeat_reverse ? owner : Type::unit;
}
namespace {
void require(const std::shared_ptr<Aggregate>& value,SourceLocation location) {if(!value || value->nil)throw runtime_error(RuntimeError::nil_reference,location);}
std::shared_ptr<Aggregate> copy_list(Values& values,const std::shared_ptr<Aggregate>& source,const ExecutionOptions& options) {
    big_work(source->items.size(),values.location,options);
    auto result=values.array(source->type,source->items.size(),true);
    for(std::size_t i=0;i<source->items.size();++i){
        if((i&255U)==0 && options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,values.location);
        values.write(result->items[i],source->items[i]->value);values.own_list_cell(result->items[i],result);
    }
    result->version=0;return result;
}
}
Value compound_invoke(Values& values,Type owner,CompoundMethod method,std::span<const Value> arguments,const ExecutionOptions& options) {
    if(!compound_signature(values.program,owner,method) || arguments.size()!=2)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    try {
        if(compound_reference(method)) {
            const auto& target=std::get<std::shared_ptr<Cell>>(arguments[0]);
            if(!target || target->type!=owner)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
            // A var parameter observes the current cell only after all caller arguments.
            const auto& right=arguments[1];Value result;
            if(owner==Type::big_integer){
                result=big_binary(method==CompoundMethod::big_add ? Op::add : method==CompoundMethod::big_subtract ? Op::subtract : Op::multiply,std::get<BigInteger>(target->value),std::get<BigInteger>(right),values.location,options);
                const auto& number=std::get<BigInteger>(result);if(!number.shares_storage(std::get<BigInteger>(target->value)) && !number.shares_storage(std::get<BigInteger>(right)))values.number(result);
            } else if(method==CompoundMethod::text_repeat) {
                const auto count=std::get<std::int32_t>(right);big_work(count>0 ? static_cast<std::uint32_t>(count) : 0,values.location,options);
                const std::array<Value,2> args{target->value,right};result=*stdlib::invoke_text(stdlib::Function::repeat_string,args,values.location);
            } else {
                Value suffix=right;
                if(method==CompoundMethod::text_integer || method==CompoundMethod::text_real){const std::array<Value,1> args{right};suffix=*stdlib::invoke_text(method==CompoundMethod::text_integer ? stdlib::Function::int_to_str : stdlib::Function::float_to_str,args,values.location);}
                const auto& first=string_text(target->value);const auto& last=string_text(suffix);
                if(first.size()>1'048'576 || last.size()>1'048'576-first.size())throw runtime_error(RuntimeError::string_limit,values.location);
                big_work((first.size()+last.size())/64+1,values.location,options);result=first+last;
            }
            values.write(target,result);return false;
        }
        const auto reversed=method==CompoundMethod::list_repeat_reverse;
        const auto object=std::get<std::shared_ptr<Aggregate>>(arguments[reversed ? 1 : 0]);require(object,values.location);
        const auto element=type_info(values.program,owner).element;
        if(method==CompoundMethod::endpoint_add){
            if(!object->ends)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
            object->ends->push_back(values.cell(element,values.copy(arguments[1])));return false;
        }
        if(method==CompoundMethod::list_add){
            values.grow_list(object,object->items.size()+1);auto cell=values.cell(element,values.copy(arguments[1]));values.own_list_cell(cell,object);object->items.push_back(std::move(cell));++object->version;return false;
        }
        if(method==CompoundMethod::list_range || method==CompoundMethod::list_concat){
            const auto right=std::get<std::shared_ptr<Aggregate>>(arguments[1]);require(right,values.location);
            auto result=method==CompoundMethod::list_concat ? copy_list(values,object,options) : object;
            list_bulk(values,result,right,false,0,options);return method==CompoundMethod::list_concat ? Value{result} : Value{false};
        }
        const auto count=std::get<std::int32_t>(arguments[reversed ? 0 : 1]);
        if(count<0)throw runtime_error(RuntimeError::list_repeat_range,values.location);
        big_work(static_cast<std::uint32_t>(count),values.location,options);
        auto result=values.array(owner,0,true);
        for(std::int32_t i=0;i<count;++i){
            if((i&255)==0 && options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,values.location);
            list_bulk(values,result,object,false,0,options);
        }
        return result;
    }catch(const std::bad_alloc&){throw runtime_error(RuntimeError::allocation_limit,values.location);}
     catch(const std::length_error&){throw runtime_error(RuntimeError::allocation_limit,values.location);}
}
}

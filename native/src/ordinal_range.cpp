#include "ordinal_range.hpp"
#include "runtime_values.hpp"
#include "sequences.hpp"
#include "formatting.hpp"
#include <algorithm>
#include <bit>
#include <limits>

namespace abc {
namespace {
const TypeInfo& info(const std::vector<TypeInfo>& types,Type type){return types.at(static_cast<unsigned>(type)-64U);}
std::int32_t wrap(std::uint32_t value){return std::bit_cast<std::int32_t>(value);}
std::int32_t number(const Value& value){return std::holds_alternative<char16_t>(value) ? static_cast<std::int32_t>(std::get<char16_t>(value)) : std::get<std::int32_t>(value);}
std::int32_t low(const std::shared_ptr<Aggregate>& value){return number(value->items[0]->value);}
std::int32_t high(const std::shared_ptr<Aggregate>& value){return number(value->items[1]->value);}
Value at(const std::shared_ptr<Aggregate>& value,std::uint32_t index){
    const auto n=static_cast<std::uint32_t>(low(value))+index;
    return std::holds_alternative<char16_t>(value->items[0]->value) ? Value{static_cast<char16_t>(n)} : Value{wrap(n)};
}
void work(Values& values,std::uint64_t index,const ExecutionOptions& options){
    if(index>=options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,values.location);
    if((index&255U)==0 && options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,values.location);
}
}
Type ordinal_range_array(const std::vector<TypeInfo>& types,Type owner){
    const auto element=info(types,owner).element;
    for(std::size_t i=0;i<types.size();++i)if(types[i].kind==CompoundKind::array && types[i].rank==1 && types[i].element==element)return static_cast<Type>(64+i);
    return Type::invalid;
}
bool ordinal_range_signature(const Program& p,Type owner,OrdinalRangeMethod method){
    return method>=OrdinalRangeMethod::construct && method<=OrdinalRangeMethod::membership_uint64 && compound(owner) && static_cast<unsigned>(owner)-64U<p.types.size()
        && ordinal_range_type(type_info(p,owner)) && ((method!=OrdinalRangeMethod::contains_real && method!=OrdinalRangeMethod::membership_real && method<OrdinalRangeMethod::contains_longword) || type_info(p,owner).element==Type::integer)
        && ((method!=OrdinalRangeMethod::to_array && method!=OrdinalRangeMethod::copy_to) || ordinal_range_array(p.types,owner)!=Type::invalid);
}
std::vector<Type> ordinal_range_parameters(const std::vector<TypeInfo>& types,Type owner,OrdinalRangeMethod method){
    const auto element=info(types,owner).element;
    if(method==OrdinalRangeMethod::construct)return {element,element};
    if(method>=OrdinalRangeMethod::membership_longword)return {method==OrdinalRangeMethod::membership_longword ? Type::longword : Type::uint64,owner};
    if(method==OrdinalRangeMethod::membership || method==OrdinalRangeMethod::membership_real)return {method==OrdinalRangeMethod::membership_real ? Type::real : element,owner};
    std::vector<Type> result{owner};
    if(method==OrdinalRangeMethod::contains)result.push_back(element);
    if(method==OrdinalRangeMethod::contains_real)result.push_back(Type::real);
    if(method==OrdinalRangeMethod::contains_longword || method==OrdinalRangeMethod::contains_uint64)result.push_back(method==OrdinalRangeMethod::contains_longword ? Type::longword : Type::uint64);
    if(method==OrdinalRangeMethod::equal || method==OrdinalRangeMethod::not_equal)result.push_back(owner);
    if(method==OrdinalRangeMethod::copy_to){result.push_back(ordinal_range_array(types,owner));result.push_back(Type::integer);}
    if(method==OrdinalRangeMethod::step)result.push_back(Type::integer);
    return result;
}
Type ordinal_range_result(const std::vector<TypeInfo>& types,Type owner,OrdinalRangeMethod method){
    if(method==OrdinalRangeMethod::construct)return owner;
    if(method==OrdinalRangeMethod::low || method==OrdinalRangeMethod::high)return info(types,owner).element;
    if(method==OrdinalRangeMethod::count || method==OrdinalRangeMethod::hash_code)return Type::integer;
    if(method==OrdinalRangeMethod::to_string)return Type::string;
    if(method==OrdinalRangeMethod::copy_to)return Type::unit;
    if(method==OrdinalRangeMethod::to_array)return ordinal_range_array(types,owner);
    if(method==OrdinalRangeMethod::step || method==OrdinalRangeMethod::reverse)return info(types,owner).key;
    return Type::boolean;
}
std::int32_t ordinal_range_count(const std::shared_ptr<Aggregate>& value){
    // The shipped unit uses unchecked int32 subtraction/addition before Math.Max.
    return std::max(std::int32_t{0},wrap(static_cast<std::uint32_t>(high(value))-static_cast<std::uint32_t>(low(value))+1U));
}
std::int32_t ordinal_range_hash(const std::shared_ptr<Aggregate>& value){
    const bool chars=std::holds_alternative<char16_t>(value->items[0]->value);
    const auto code=[&](std::int32_t n){const auto u=static_cast<std::uint32_t>(n);return chars ? u|(u<<16U) : u;};
    return wrap(code(low(value))*668265263U ^ code(high(value)));
}
std::u16string ordinal_range_text(const std::shared_ptr<Aggregate>& value){
    const bool chars=std::holds_alternative<char16_t>(value->items[0]->value);
    if(chars)return std::u16string{u'\''}+std::get<char16_t>(value->items[0]->value)+u"'..'"+std::get<char16_t>(value->items[1]->value)+u"'";
    return format_field(value->items[0]->value,0,std::nullopt,{})+u".."+format_field(value->items[1]->value,0,std::nullopt,{});
}
std::shared_ptr<Aggregate> ordinal_range_begin(Values& values,Type sequence,const std::shared_ptr<Aggregate>& value){
    const auto a=low(value),b=high(value);
    const auto count=b<a ? 0 : wrap(static_cast<std::uint32_t>(b)-static_cast<std::uint32_t>(a)+1U);
    if(count<0)throw runtime_error(RuntimeError::sequence_count_range,values.location);
    auto result=sequence_repeat(values,sequence,count,value->items[0]->value);result->sequence->provider=SequenceProvider::range;return result;
}
Value ordinal_range_collect(Values& values,const std::shared_ptr<Aggregate>& value,Type collection,const ExecutionOptions& options){
    const auto count=ordinal_range_count(value);if(collection==Type::invalid)return count;
    auto result=values.array(collection,count);
    const bool list=type_info(values.program,collection).kind==CompoundKind::list;
    for(std::int32_t i=0;i<count;++i){work(values,static_cast<std::uint64_t>(i),options);const auto& cell=result->items[static_cast<std::size_t>(i)];values.write(cell,at(value,static_cast<std::uint32_t>(i)));if(list)values.own_list_cell(cell,result);}
    return result;
}
Value ordinal_range_invoke(Values& values,Type owner,OrdinalRangeMethod method,std::span<const Value> arguments,const ExecutionOptions& options){
    if(!ordinal_range_signature(values.program,owner,method) || arguments.size()!=ordinal_range_parameters(values.program.types,owner,method).size())throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    if(method==OrdinalRangeMethod::construct){auto result=std::get<std::shared_ptr<Aggregate>>(values.zero(owner));for(std::size_t i=0;i<2;++i)values.write(result->items[i],arguments[i]);return result;}
    const bool membership=method==OrdinalRangeMethod::membership || method==OrdinalRangeMethod::membership_real || method>=OrdinalRangeMethod::membership_longword;
    const auto value=std::get<std::shared_ptr<Aggregate>>(arguments[membership ? 1U : 0U]);
    if(method==OrdinalRangeMethod::low || method==OrdinalRangeMethod::high)return value->items[method==OrdinalRangeMethod::high ? 1U : 0U]->value;
    if(method==OrdinalRangeMethod::count)return ordinal_range_count(value);
    if(method==OrdinalRangeMethod::is_empty)return low(value)>high(value);
    if(method==OrdinalRangeMethod::contains || method==OrdinalRangeMethod::contains_real || method==OrdinalRangeMethod::contains_longword || method==OrdinalRangeMethod::contains_uint64 || membership){
        const auto& item=arguments[membership ? 0U : 1U];
        const auto x=method==OrdinalRangeMethod::contains_real || method==OrdinalRangeMethod::membership_real ? std::get<double>(item)
            : std::holds_alternative<std::uint32_t>(item) ? static_cast<double>(wrap(std::get<std::uint32_t>(item)))
            : std::holds_alternative<std::uint64_t>(item) ? static_cast<double>(wrap(static_cast<std::uint32_t>(std::get<std::uint64_t>(item)))) : static_cast<double>(number(item));
        return x>=low(value) && x<=high(value);
    }
    if(method==OrdinalRangeMethod::equal || method==OrdinalRangeMethod::not_equal){
        const auto other=std::get<std::shared_ptr<Aggregate>>(arguments[1]);const bool equal=low(value)==low(other) && high(value)==high(other);return method==OrdinalRangeMethod::equal ? equal : !equal;
    }
    if(method==OrdinalRangeMethod::hash_code)return ordinal_range_hash(value);
    if(method==OrdinalRangeMethod::to_string)return ordinal_range_text(value);
    if(method==OrdinalRangeMethod::to_array)return ordinal_range_collect(values,value,ordinal_range_array(values.program.types,owner),options);
    if(method==OrdinalRangeMethod::copy_to){
        const auto count=ordinal_range_count(value),index=std::get<std::int32_t>(arguments[2]);const auto array=std::get<std::shared_ptr<Aggregate>>(arguments[1]);
        if(!array || array->nil)throw runtime_error(RuntimeError::nil_reference,values.location);
        const auto available=wrap(static_cast<std::uint32_t>(array->items.size())-static_cast<std::uint32_t>(index));
        const auto last=wrap(static_cast<std::uint32_t>(std::min(count,available))-1U);
        for(std::int64_t i=0;i<=last;++i){work(values,static_cast<std::uint64_t>(i),options);const auto target=wrap(static_cast<std::uint32_t>(i)+static_cast<std::uint32_t>(index));if(target<0 || static_cast<std::size_t>(target)>=array->items.size())throw runtime_error(RuntimeError::index_bounds,values.location);values.write(array->items[static_cast<std::size_t>(target)],at(value,static_cast<std::uint32_t>(i)));}
        return false;
    }
    const bool reverse=method==OrdinalRangeMethod::reverse;
    return sequence_range_step(values,type_info(values.program,owner).key,value->items[reverse ? 1U : 0U]->value,value->items[reverse ? 0U : 1U]->value,reverse ? -1 : std::get<std::int32_t>(arguments[1]));
}
}

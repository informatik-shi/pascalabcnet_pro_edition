#include "real_interval.hpp"
#include "runtime_values.hpp"
#include "formatting.hpp"
#include <bit>
#include <cmath>

namespace abc {
bool real_interval_signature(const Program& program,Type owner,RealIntervalMethod method){
    return method>=RealIntervalMethod::construct && method<=RealIntervalMethod::membership && compound(owner)
        && static_cast<unsigned>(owner)-64U<program.types.size() && real_interval_type(type_info(program,owner));
}
std::vector<Type> real_interval_parameters(Type owner,RealIntervalMethod method){
    if(method==RealIntervalMethod::construct)return {Type::real,Type::real};
    if(method==RealIntervalMethod::membership)return {Type::real,owner};
    if(method==RealIntervalMethod::equal || method==RealIntervalMethod::not_equal)return {owner,owner};
    return {owner};
}
Type real_interval_result(Type owner,RealIntervalMethod method){
    if(method==RealIntervalMethod::construct)return owner;
    if(method==RealIntervalMethod::low || method==RealIntervalMethod::high || method==RealIntervalMethod::size)return Type::real;
    if(method==RealIntervalMethod::hash_code)return Type::integer;
    if(method==RealIntervalMethod::to_string)return Type::string;
    return Type::boolean;
}
std::u16string real_interval_text(const std::shared_ptr<Aggregate>& value,SourceLocation location){
    return format_field(value->items[0]->value,0,std::nullopt,location)+u".."+format_field(value->items[1]->value,0,std::nullopt,location);
}
std::int32_t real_interval_hash(const std::shared_ptr<Aggregate>& value){
    const auto code=[](const Value& value){const auto number=std::get<double>(value);const auto bits=number==0 ? std::uint64_t{0} : std::bit_cast<std::uint64_t>(number);return static_cast<std::uint32_t>(bits)^static_cast<std::uint32_t>(bits>>32U);};
    return std::bit_cast<std::int32_t>(code(value->items[0]->value)*668265263U ^ code(value->items[1]->value));
}
Value real_interval_invoke(Values& values,Type owner,RealIntervalMethod method,std::span<const Value> arguments){
    if(!real_interval_signature(values.program,owner,method) || arguments.size()!=real_interval_parameters(owner,method).size())throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    if(method==RealIntervalMethod::construct){auto value=std::get<std::shared_ptr<Aggregate>>(values.zero(owner));for(std::size_t i=0;i<2;++i)values.write(value->items[i],arguments[i]);return value;}
    const auto value=std::get<std::shared_ptr<Aggregate>>(arguments[method==RealIntervalMethod::membership ? 1U : 0U]);
    const auto low=std::get<double>(value->items[0]->value),high=std::get<double>(value->items[1]->value);
    if(method==RealIntervalMethod::low)return low;
    if(method==RealIntervalMethod::high)return high;
    if(method==RealIntervalMethod::size){const auto difference=high-low,result=difference>=0 ? difference : 0.0;if(!std::isfinite(result))throw runtime_error(RuntimeError::real_overflow,values.location);return result;}
    if(method==RealIntervalMethod::is_empty)return low>high;
    if(method==RealIntervalMethod::membership){const auto x=std::get<double>(arguments[0]);return x>=low && x<=high;}
    if(method==RealIntervalMethod::hash_code)return real_interval_hash(value);
    if(method==RealIntervalMethod::to_string)return real_interval_text(value,values.location);
    const auto other=std::get<std::shared_ptr<Aggregate>>(arguments[1]);const bool equal=low==std::get<double>(other->items[0]->value) && high==std::get<double>(other->items[1]->value);return method==RealIntervalMethod::equal ? equal : !equal;
}
}

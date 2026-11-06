#include "single.hpp"
#include "number.hpp"
#include "integer_values.hpp"
#include <cmath>
#include <limits>
#include <type_traits>
namespace abc {
static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559,"single requires IEEE binary32");
float single_number(const Value& value,SourceLocation location){
    const auto result=std::visit([&](const auto& number)->float{using T=std::decay_t<decltype(number)>;
        if constexpr(std::is_arithmetic_v<T> && !std::is_same_v<T,bool>)return static_cast<float>(number);
        else if constexpr(std::is_same_v<T,BigInteger>)return static_cast<float>(number.to_real());
        else throw runtime_error(RuntimeError::invalid_bytecode,location);
    },value);
    if(!std::isfinite(result))throw runtime_error(RuntimeError::real_overflow,location);return result;
}
Value floating_integer_cast(Type target,const Value& value,SourceLocation location){
    const auto number=std::holds_alternative<float>(value) ? static_cast<double>(std::get<float>(value)) : std::get<double>(value);
    if(!std::isfinite(number))throw runtime_error(RuntimeError::number_range,location,type_name(target));
    const auto integral=std::trunc(number);
    if(wide_unsigned(target)){
        const auto upper=std::ldexp(1.0,target==Type::uint64 ? 64 : 32);
        if(integral<0 || integral>=upper)throw runtime_error(RuntimeError::number_range,location,type_name(target));
        return checked_unsigned_integer(target,Value{static_cast<std::uint64_t>(integral)},location);
    }
    if(integral<-std::ldexp(1.0,63) || integral>=std::ldexp(1.0,63))throw runtime_error(RuntimeError::number_range,location,type_name(target));
    const auto signed_value=static_cast<std::int64_t>(integral);
    return small_integer(target) ? checked_small_integer(target,signed_value,location) : checked_signed_integer(target,Value{signed_value},location);
}
float single_binary(Op operation,float left,float right,SourceLocation location){
    float result=0;
    switch(operation){
    case Op::add:result=left+right;break;case Op::subtract:result=left-right;break;case Op::multiply:result=left*right;break;
    case Op::divide_real:if(right==0)throw runtime_error(RuntimeError::division_zero,location);result=left/right;break;
    default:throw runtime_error(RuntimeError::invalid_bytecode,location);
    }
    if(!std::isfinite(result))throw runtime_error(RuntimeError::real_overflow,location);return result;
}
}

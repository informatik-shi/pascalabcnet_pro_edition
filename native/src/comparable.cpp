#include "comparable.hpp"
#include "comparison.hpp"
#include "runtime_values.hpp"
#include "enumeration.hpp"
#include "big_integer_runtime.hpp"
#include <type_traits>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unicode/ucol.h>
#endif

namespace abc {
bool comparable_type(const std::vector<TypeInfo>& types,Type type) {return default_comparable(types,type);}
std::vector<Type> comparable_parameters(Type type,ComparableMethod method) {
    if(method==ComparableMethod::compare_to || method==ComparableMethod::clamp_top || method==ComparableMethod::clamp_bottom)return {type,type};
    if(method==ComparableMethod::between || method==ComparableMethod::ordinal_between)return {type,type,type,Type::boolean};
    return {type,type,type};
}
Type comparable_result(Type type,ComparableMethod method) {
    if(method==ComparableMethod::compare_to)return Type::integer;
    return method==ComparableMethod::clamp || method==ComparableMethod::clamp_top || method==ComparableMethod::clamp_bottom ? type : Type::boolean;
}
bool comparable_signature(const Program& program,Type type,ComparableMethod method) {
    if(method>ComparableMethod::compare_to || !comparable_type(program.types,type))return false;
    if(method==ComparableMethod::ordinal_in_range || method==ComparableMethod::ordinal_between)return type==Type::string;
    if(method==ComparableMethod::clamp_top || method==ComparableMethod::clamp_bottom)return type==Type::integer || type==Type::real;
    return true;
}
int compare_culture_strings(std::u16string_view a,std::u16string_view b,SourceLocation location,const ExecutionOptions& options) {
    big_work((a.size()+b.size())/64+1,location,options);
    if(a.size()>INT32_MAX || b.size()>INT32_MAX)throw runtime_error(RuntimeError::string_limit,location);
#ifdef _WIN32
    static_assert(sizeof(wchar_t)==sizeof(char16_t));
    const auto result=CompareStringEx(L"en-US",NORM_LINGUISTIC_CASING,reinterpret_cast<LPCWCH>(a.data()),static_cast<int>(a.size()),reinterpret_cast<LPCWCH>(b.data()),static_cast<int>(b.size()),nullptr,nullptr,0);
    if(!result)throw runtime_error(RuntimeError::collation_unavailable,location);
    return result-CSTR_EQUAL;
#else
    struct Collator {
        UCollator* value=nullptr;
        Collator(){UErrorCode status=U_ZERO_ERROR;value=ucol_open("en_US",&status);if(U_FAILURE(status)){if(value)ucol_close(value);value=nullptr;}}
        ~Collator(){if(value)ucol_close(value);}
        Collator(const Collator&)=delete;Collator& operator=(const Collator&)=delete;
    };
    thread_local Collator collator;
    if(!collator.value)throw runtime_error(RuntimeError::collation_unavailable,location);
    static_assert(sizeof(UChar)==sizeof(char16_t));
    return static_cast<int>(ucol_strcoll(collator.value,reinterpret_cast<const UChar*>(a.data()),static_cast<int32_t>(a.size()),reinterpret_cast<const UChar*>(b.data()),static_cast<int32_t>(b.size())));
#endif
}
Value comparable_invoke(const Program& program,Type type,ComparableMethod method,std::span<const Value> arguments,SourceLocation location,const ExecutionOptions& options) {
    Values values{program,options.allocation_limit,location};return comparable_invoke(values,type,method,arguments,options);
}
Value comparable_invoke(Values& values,Type type,ComparableMethod method,std::span<const Value> arguments,const ExecutionOptions& options) {
    const auto& program=values.program;const auto location=values.location;
    if(!comparable_signature(program,type,method) || arguments.size()!=comparable_parameters(type,method).size())throw runtime_error(RuntimeError::invalid_bytecode,location);
    const bool ordinal=method==ComparableMethod::ordinal_in_range || method==ComparableMethod::ordinal_between;
    const auto order=[&](const Value& a,const Value& b){return comparison_value(values,type,a,b,false,ordinal,options);};
    const auto& value=arguments[0];const auto& low=arguments[1];
    if(method==ComparableMethod::compare_to)return std::int32_t{order(value,low)};
    if(method==ComparableMethod::clamp_top)return order(value,low)<0 ? value : low;
    if(method==ComparableMethod::clamp_bottom)return order(value,low)>0 ? value : low;
    const auto& high=arguments[2];
    if(method==ComparableMethod::clamp) {
        if(order(low,high)>0)throw runtime_error(RuntimeError::clamp_range,location);
        if(order(value,low)<0)return low;
        if(order(value,high)>0)return high;
        return value;
    }
    const auto a=order(value,low);
    if(method==ComparableMethod::in_range || method==ComparableMethod::ordinal_in_range)return a>=0 && order(value,high)<=0;
    const auto b=order(value,high);const bool reversed=order(low,high)>0;
    const auto first=reversed ? b : a,last=reversed ? a : b;
    return std::get<bool>(arguments[3]) ? first>=0 && last<=0 : first>0 && last<0;
}
} // namespace abc

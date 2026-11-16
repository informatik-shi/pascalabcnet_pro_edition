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
#include <unicode/usearch.h>
#endif

namespace abc {
namespace {
#ifdef _WIN32
constexpr DWORD ignore_text_case=NORM_IGNORECASE|NORM_IGNOREKANATYPE|NORM_IGNOREWIDTH;
#else
class Collator {
public:
    UCollator* value=nullptr;
    explicit Collator(bool ignore_case){
        UErrorCode status=U_ZERO_ERROR;
        if(ignore_case){
            // Framework English comparisons expand these letters as units.
            constexpr std::u16string_view rules=u"&ss=ß=ẞ&ae=æ=Æ&oe=œ=Œ";
            value=ucol_openRules(reinterpret_cast<const UChar*>(rules.data()),static_cast<int32_t>(rules.size()),
                UCOL_DEFAULT,UCOL_SECONDARY,nullptr,&status);
        }else value=ucol_open("en_US",&status);
        if(U_FAILURE(status)){if(value)ucol_close(value);value=nullptr;}
    }
    ~Collator(){if(value)ucol_close(value);}
    Collator(const Collator&)=delete;Collator& operator=(const Collator&)=delete;
};
const UCollator* culture_collator(bool ignore_case,SourceLocation location) {
    const auto selected=[ignore_case]{
        if(ignore_case){thread_local Collator folded(true);return folded.value;}
        thread_local Collator original(false);return original.value;
    }();
    if(!selected)throw runtime_error(RuntimeError::collation_unavailable,location);
    return selected;
}
#endif
}
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
int compare_culture_strings(std::u16string_view a,std::u16string_view b,SourceLocation location,
    const ExecutionOptions& options,bool ignore_case) {
    big_work((a.size()+b.size())/64+1,location,options);
    if(a.size()>INT32_MAX || b.size()>INT32_MAX)throw runtime_error(RuntimeError::string_limit,location);
#ifdef _WIN32
    static_assert(sizeof(wchar_t)==sizeof(char16_t));
    const DWORD flags=NORM_LINGUISTIC_CASING | (ignore_case ? ignore_text_case : 0);
    const auto result=CompareStringEx(L"en-US",flags,reinterpret_cast<LPCWCH>(a.data()),static_cast<int>(a.size()),reinterpret_cast<LPCWCH>(b.data()),static_cast<int>(b.size()),nullptr,nullptr,0);
    if(!result)throw runtime_error(RuntimeError::collation_unavailable,location);
    return result-CSTR_EQUAL;
#else
    static_assert(sizeof(UChar)==sizeof(char16_t));
    return static_cast<int>(ucol_strcoll(culture_collator(ignore_case,location),reinterpret_cast<const UChar*>(a.data()),
        static_cast<int32_t>(a.size()),reinterpret_cast<const UChar*>(b.data()),static_cast<int32_t>(b.size())));
#endif
}
bool find_culture_text(std::u16string_view text,std::u16string_view pattern,TextSearch mode,
    SourceLocation location,const ExecutionOptions& options) {
    big_work((text.size()+pattern.size())/64+1,location,options);
    if(text.size()>INT32_MAX || pattern.size()>INT32_MAX)throw runtime_error(RuntimeError::string_limit,location);
    if(pattern.empty())return true;
    if(text.empty())return false;
    const auto text_length=static_cast<int32_t>(text.size()),pattern_length=static_cast<int32_t>(pattern.size());
#ifdef _WIN32
    const DWORD position=mode==TextSearch::prefix ? FIND_STARTSWITH
        : mode==TextSearch::suffix ? FIND_ENDSWITH : FIND_FROMSTART;
    ::SetLastError(ERROR_SUCCESS);
    const auto index=::FindNLSStringEx(L"en-US",position|NORM_LINGUISTIC_CASING|ignore_text_case,
        reinterpret_cast<LPCWCH>(text.data()),text_length,reinterpret_cast<LPCWCH>(pattern.data()),
        pattern_length,nullptr,nullptr,nullptr,0);
    if(index<0 && ::GetLastError()!=ERROR_SUCCESS)throw runtime_error(RuntimeError::collation_unavailable,location);
    return index>=0;
#else
    UErrorCode status=U_ZERO_ERROR;
    const std::unique_ptr<UStringSearch,decltype(&usearch_close)> search(
        usearch_openFromCollator(reinterpret_cast<const UChar*>(pattern.data()),pattern_length,
            reinterpret_cast<const UChar*>(text.data()),text_length,culture_collator(true,location),nullptr,&status),&usearch_close);
    if(U_FAILURE(status) || !search)throw runtime_error(RuntimeError::collation_unavailable,location);
    const auto index=mode==TextSearch::suffix ? usearch_last(search.get(),&status) : usearch_first(search.get(),&status);
    if(U_FAILURE(status))throw runtime_error(RuntimeError::collation_unavailable,location);
    if(index==USEARCH_DONE)return false;
    if(mode==TextSearch::prefix)return index==0;
    if(mode==TextSearch::suffix)return index+usearch_getMatchedLength(search.get())==text_length;
    return true;
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

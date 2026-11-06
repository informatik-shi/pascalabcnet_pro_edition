#pragma once

#include <charconv>
#include <cmath>
#include <cstdlib>
#include <locale.h>
#include <string>
#include <string_view>
#include <type_traits>
#ifdef __APPLE__
#include <xlocale.h>
#endif

namespace abc {

template<class Float> inline bool parse_binary_float(std::string_view text, Float& value,bool allow_underflow=false) {
    if (text.empty() || text.front() == '+') return false;
#ifdef ABC_HAS_FLOAT_FROM_CHARS
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if(result.ec == std::errc{} && result.ptr == text.data() + text.size() && std::isfinite(value))return true;
    if(!allow_underflow || result.ec!=std::errc::result_out_of_range || result.ptr!=text.data()+text.size())return false;
#endif
    // Older libc++ releases lack floating from_chars. Use a private C locale:
    // iostream conversion on libc++ rejects representable subnormal numbers.
    std::size_t position=text.front()=='-' ? 1 : 0;
    bool digits=false,nonzero=false;
    const auto take_digits=[&] {
        while (position<text.size() && text[position]>='0' && text[position]<='9') {
            digits=true; nonzero=nonzero || text[position]!='0'; ++position;
        }
    };
    take_digits();
    if (position<text.size() && text[position]=='.') { ++position; take_digits(); }
    if (!digits) return false;
    if (position<text.size() && (text[position]=='e' || text[position]=='E')) {
        ++position;
        if (position<text.size() && (text[position]=='+' || text[position]=='-')) ++position;
        const auto start=position;
        while (position<text.size() && text[position]>='0' && text[position]<='9') ++position;
        if (position==start) return false;
    }
    if (position!=text.size()) return false;
    struct NumericLocale {
#ifdef _WIN32
        _locale_t handle=_create_locale(LC_NUMERIC,"C");
        ~NumericLocale() { if (handle) _free_locale(handle); }
#else
        locale_t handle=newlocale(LC_NUMERIC_MASK,"C",nullptr);
        ~NumericLocale() { if (handle) freelocale(handle); }
#endif
    };
    static const NumericLocale classic;
    if (!classic.handle) return false;
    const std::string terminated(text);
    char* end=nullptr;
#ifdef _WIN32
    const Float parsed=[&]{if constexpr(std::is_same_v<Float,float>)return _strtof_l(terminated.c_str(),&end,classic.handle);else return _strtod_l(terminated.c_str(),&end,classic.handle);}();
#else
    const Float parsed=[&]{if constexpr(std::is_same_v<Float,float>)return strtof_l(terminated.c_str(),&end,classic.handle);else return strtod_l(terminated.c_str(),&end,classic.handle);}();
#endif
    // ERANGE also describes valid subnormals; reject only overflow or a
    // nonzero mantissa rounded all the way to zero (source literal policy).
    if (end!=terminated.c_str()+terminated.size() || !std::isfinite(parsed) || (parsed==0 && nonzero && !allow_underflow)) return false;
    value=parsed; return true;
}
inline bool parse_real(std::string_view text,double& value){return parse_binary_float(text,value);}
inline bool parse_single(std::string_view text,float& value){return parse_binary_float(text,value,true);}

} // namespace abc

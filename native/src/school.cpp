#include "school.hpp"
#include "diagnostics.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace abc::stdlib {
Value invoke_school(Function function, std::span<const Value> arguments, SourceLocation location, const ExecutionOptions& options) {
    constexpr std::u16string_view alphabet=u"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const auto check=[&](std::size_t index) {
        if ((index&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,location);
    };
    if (function>=Function::school_sin_degrees && function<=Function::school_tan_degrees) {
        const auto radians=std::get<double>(arguments[0])*std::numbers::pi/180.0;
        if (!std::isfinite(radians)) throw runtime_error(RuntimeError::real_overflow,location);
        const auto result=function==Function::school_sin_degrees ? std::sin(radians) : function==Function::school_cos_degrees ? std::cos(radians) : std::tan(radians);
        if (!std::isfinite(result)) throw runtime_error(RuntimeError::real_overflow,location);
        return result;
    }
    if (function==Function::school_dec) {
        const auto base=std::get<std::int32_t>(arguments[1]);
        if (base<2 || base>36) throw runtime_error(RuntimeError::school_base,location);
        if (std::holds_alternative<NilString>(arguments[0])) throw runtime_error(RuntimeError::nil_reference,location);
        const auto& text=string_text(arguments[0]);
        const auto digit=[](char16_t c)->unsigned {
            if (c>=u'0' && c<=u'9') return c-u'0';
            if (c>=u'A' && c<=u'Z') return c-u'A'+10;
            if (c>=u'a' && c<=u'z') return c-u'a'+10;
            return 36;
        };
        // The source validates every character before its BigInteger accumulation.
        for (std::size_t i=0;i<text.size();++i) {
            check(i); if (digit(text[i])>=static_cast<unsigned>(base)) throw runtime_error(RuntimeError::school_digit,location);
        }
        std::int64_t result=0;
        for (std::size_t i=0;i<text.size();++i) {
            check(i); const auto d=digit(text[i]);
            if (result>(INT64_MAX-d)/base) throw runtime_error(RuntimeError::integer_overflow,location);
            result=result*base+d;
        }
        return result;
    }
    const auto number=std::get<std::int64_t>(arguments[0]);
    if (function==Function::school_perfect_square) {
        if (number<0) return std::int64_t{-1};
        const auto low=number&15;
        if (low!=0 && low!=1 && low!=4 && low!=9) return std::int64_t{-1};
        const auto root=static_cast<std::uint64_t>(std::floor(std::sqrt(static_cast<double>(number))+0.5));
        return root*root==static_cast<std::uint64_t>(number) ? static_cast<std::int64_t>(root) : std::int64_t{-1};
    }
    if (function==Function::school_bin_format) {
        const auto magnitude=number<0 ? std::uint64_t{0}-static_cast<std::uint64_t>(number) : static_cast<std::uint64_t>(number);
        std::size_t natural=1; for (auto n=magnitude;n>255;n>>=8) ++natural;
        const auto requested=arguments.size()>1 ? std::get<std::int32_t>(arguments[1]) : 0;
        const auto count=requested>0 ? std::max(natural,static_cast<std::size_t>(requested)) : natural;
        const auto separator=arguments.size()>2 ? std::u16string_view(string_text(arguments[2])) : std::u16string_view{u" "};
        constexpr std::size_t limit=1'048'576;
        if (count>limit/8 || (count>1 && separator.size()>(limit-count*8)/(count-1))) throw runtime_error(RuntimeError::string_limit,location);
        std::u16string result; result.reserve(count*8+(count-1)*separator.size());
        for (std::size_t i=0;i<count;++i) {
            check(i); if (i) result+=separator;
            const auto index=count-i-1; const auto byte=index<8 ? (magnitude>>(index*8))&255U : 0;
            for (int bit=7;bit>=0;--bit) result+=((byte>>bit)&1U) ? u'1' : u'0';
        }
        return result;
    }
    const auto base=function==Function::school_bin ? 2 : function==Function::school_oct ? 8 : function==Function::school_hex ? 16 : std::get<std::int32_t>(arguments[1]);
    if (base<2 || base>36) throw runtime_error(RuntimeError::school_base,location);
    if (function==Function::school_to_base && number<=0) return std::u16string{u"0"};
    if (number==INT64_MIN) throw runtime_error(RuntimeError::integer_overflow,location);
    auto magnitude=static_cast<std::uint64_t>(number<0 ? -number : number);
    std::u16string result;
    do { result+=alphabet[static_cast<std::size_t>(magnitude%base)]; magnitude/=base; } while (magnitude);
    std::reverse(result.begin(),result.end()); return result;
}
} // namespace abc::stdlib

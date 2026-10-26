#include "school.hpp"
#include "diagnostics.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <array>

namespace abc::stdlib {
namespace {
bool small_prime(std::int32_t value, SourceLocation location, const ExecutionOptions& options) {
    if (value<2) return false;
    if (!(value&1)) return value==2;
    const auto number=static_cast<std::uint32_t>(value);
    for (std::uint32_t divisor=3,steps=0;divisor<=number/divisor;divisor+=2,++steps) {
        if ((steps&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,location);
        if (number%divisor==0) return false;
    }
    return true;
}
class Modular63 {
    std::uint64_t modulus, steps=0;
    SourceLocation location;
    const ExecutionOptions& options;
public:
    Modular63(std::uint64_t n, SourceLocation l, const ExecutionOptions& o):modulus(n),location(l),options(o) {}
    std::uint64_t multiply(std::uint64_t a, std::uint64_t b) {
        std::uint64_t result=0;
        while (b) {
            if ((steps++&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,location);
            // Every residue is below 2^63, so both sums fit uint64 on every target.
            if (b&1U) result=(result+a)%modulus;
            b>>=1;
            if (b) a=(a+a)%modulus;
        }
        return result;
    }
    std::uint64_t power(std::uint64_t a, std::uint64_t exponent) {
        std::uint64_t result=1;
        while (exponent) {
            if (exponent&1U) result=multiply(result,a);
            exponent>>=1;
            if (exponent) a=multiply(a,a);
        }
        return result;
    }
};
bool large_prime(std::uint64_t number, SourceLocation location, const ExecutionOptions& options) {
    constexpr std::array<unsigned,12> bases{2,3,5,7,11,13,17,19,23,29,31,37};
    for (auto base:bases) if (number%base==0) return false;
    auto odd=number-1; unsigned twos=0;
    while (!(odd&1U)) { odd>>=1; ++twos; }
    Modular63 arithmetic(number,location,options);
    // Sorenson/Webster, Theorem 1.1: the first twelve prime bases cover int64.
    // https://arxiv.org/abs/1509.00864
    for (auto base:bases) {
        auto residue=arithmetic.power(base,odd);
        if (residue==1 || residue==number-1) continue;
        bool passed=false;
        for (unsigned i=1;i<twos;++i) {
            residue=arithmetic.multiply(residue,residue);
            if (residue==number-1) { passed=true; break; }
            if (residue==1) break;
        }
        if (!passed) return false;
    }
    return true;
}
}
Value invoke_school(Function function, std::span<const Value> arguments, SourceLocation location, const ExecutionOptions& options) {
    constexpr std::u16string_view alphabet=u"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const auto check=[&](std::size_t index) {
        if ((index&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,location);
    };
    if (function==Function::school_is_prime) {
        const auto number=std::holds_alternative<std::int32_t>(arguments[0]) ? std::int64_t(std::get<std::int32_t>(arguments[0])) : std::get<std::int64_t>(arguments[0]);
        // The original BPSW entry delegates every n <= MaxInt to integer(n).
        if (number<INT32_MIN) throw runtime_error(RuntimeError::integer_overflow,location);
        return number<=INT32_MAX ? small_prime(static_cast<std::int32_t>(number),location,options)
            : large_prime(static_cast<std::uint64_t>(number),location,options);
    }
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

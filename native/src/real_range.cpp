#include "real_range.hpp"
#include "runtime_values.hpp"
#include "big_integer_runtime.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <limits>

namespace abc {
namespace {
// Internal Decimal96 profile: signed 96-bit coefficient, decimal scale 0..28.
// Double conversion follows .NET 10 Decimal.DecCalc.VarDecFromR8 (MIT):
// https://github.com/dotnet/runtime/blob/v10.0.0/src/libraries/System.Private.CoreLib/src/System/Decimal.DecCalc.cs
// The existing licenses/DotNet-Runtime-MIT.txt applies to that adaptation.
struct Decimal96 { BigInteger units; unsigned scale=0; };
class DecimalMath {
    Values& values_;
    const ExecutionOptions& options_;
    std::uint64_t work_=0;
    static constexpr std::size_t scratch_cells=256;
    void work(std::uint64_t count) {
        if (count>options_.instruction_limit-work_) throw runtime_error(RuntimeError::instruction_limit,values_.location);
        work_+=count;
        if (options_.cancelled && options_.cancelled()) throw runtime_error(RuntimeError::cancelled,values_.location);
    }
    BigInteger binary(Op op,const BigInteger& a,const BigInteger& b) {
        const auto x=a.bits()/28+1,y=b.bits()/28+1;
        work(op==Op::multiply || op==Op::divide_int ? x*y : x+y);
        return checked_big(values_.location,[&] {
            if (op==Op::add) return a.add(b);
            if (op==Op::subtract) return a.subtract(b);
            return a.multiply(b);
        });
    }
    BigInteger power(unsigned n) {
        work(n+1);
        return checked_big(values_.location,[&]{return BigInteger::from_unsigned(10).power(n);});
    }
    BigInteger rounded(const BigInteger& numerator,const BigInteger& denominator) {
        work((numerator.bits()/28+1)*(denominator.bits()/28+1));
        auto [quotient,remainder]=checked_big(values_.location,[&]{return numerator.divide(denominator);});
        const auto twice=binary(Op::add,remainder,remainder);
        if (twice>denominator || (twice==denominator && quotient.odd())) quotient=binary(Op::add,quotient,BigInteger::from_unsigned(1));
        return quotient;
    }
    Decimal96 normalize(const BigInteger& units,unsigned scale) {
        const bool negative=units.negative(); const auto magnitude=units.absolute();
        for (unsigned drop=scale>28 ? scale-28 : 0;drop<=scale;++drop) {
            auto coefficient=drop ? rounded(magnitude,power(drop)) : magnitude;
            if (coefficient.bits()<=96) return {negative ? coefficient.negated() : coefficient,scale-drop};
        }
        throw runtime_error(RuntimeError::decimal_overflow,values_.location);
    }
public:
    explicit DecimalMath(Values& values,const ExecutionOptions& options):values_(values),options_(options) {
        // All private intermediates stay below 512 bits. Reserve the bounded
        // live scratch quota once, and release it before returning the plan.
        if (values_.remaining<scratch_cells) throw runtime_error(RuntimeError::allocation_limit,values_.location);
        values_.remaining-=scratch_cells;
    }
    ~DecimalMath() { values_.remaining+=scratch_cells; }
    DecimalMath(const DecimalMath&)=delete;
    DecimalMath& operator=(const DecimalMath&)=delete;
    Decimal96 integer(std::int64_t n) { work(1); return {BigInteger::from_signed(n),0}; }
    Decimal96 real(double input) {
        static_assert(sizeof(double)==8 && std::numeric_limits<double>::is_iec559);
        static constexpr std::array<double,29> powers={1,1e1,1e2,1e3,1e4,1e5,1e6,1e7,1e8,1e9,1e10,1e11,1e12,1e13,1e14,1e15,1e16,1e17,1e18,1e19,1e20,1e21,1e22,1e23,1e24,1e25,1e26,1e27,1e28};
        work(1);
        const int exponent=static_cast<int>((std::bit_cast<std::uint64_t>(input)>>52)&2047U)-1022;
        if (exponent< -94) return integer(0);
        if (exponent>96) throw runtime_error(RuntimeError::decimal_overflow,values_.location);
        const bool negative=input<0; double scaled=std::abs(input);
        int scale=14-((exponent*19728)>>16);
        if (scale>=0) { scale=std::min(scale,28); scaled*=powers[static_cast<unsigned>(scale)]; }
        else if (scale!= -1 || scaled>=1e15) scaled/=powers[static_cast<unsigned>(-scale)];
        else scale=0;
        if (scaled<1e14 && scale<28) { scaled*=10; ++scale; }
        auto mantissa=static_cast<std::uint64_t>(scaled);
        const auto fraction=scaled-static_cast<double>(mantissa);
        if (fraction>0.5 || (fraction==0.5 && (mantissa&1U))) ++mantissa;
        if (!mantissa) return integer(0);
        while (scale>0 && mantissa%10==0) { mantissa/=10; --scale; }
        auto units=BigInteger::from_unsigned(mantissa);
        if (scale<0) { units=binary(Op::multiply,units,power(static_cast<unsigned>(-scale))); scale=0; }
        return normalize(negative ? units.negated() : units,static_cast<unsigned>(scale));
    }
    Decimal96 subtract(const Decimal96& a,const Decimal96& b) {
        const auto scale=std::max(a.scale,b.scale);
        const auto x=binary(Op::multiply,a.units,power(scale-a.scale));
        const auto y=binary(Op::multiply,b.units,power(scale-b.scale));
        return normalize(binary(Op::subtract,x,y),scale);
    }
    Decimal96 add(const Decimal96& a,const Decimal96& b) { return subtract(a,Decimal96{b.units.negated(),b.scale}); }
    Decimal96 multiply(const Decimal96& a,const Decimal96& b) { return normalize(binary(Op::multiply,a.units,b.units),a.scale+b.scale); }
    Decimal96 divide(const Decimal96& a,const Decimal96& b) {
        if (b.units.zero()) throw runtime_error(RuntimeError::division_zero,values_.location);
        const bool negative=a.units.negative()!=b.units.negative();
        const auto numerator=binary(Op::multiply,a.units.absolute(),power(b.scale));
        const auto denominator=binary(Op::multiply,b.units.absolute(),power(a.scale));
        for (int scale=28;scale>=0;--scale) {
            auto units=rounded(binary(Op::multiply,numerator,power(static_cast<unsigned>(scale))),denominator);
            if (units.bits()<=96) return {negative ? units.negated() : units,static_cast<unsigned>(scale)};
        }
        throw runtime_error(RuntimeError::decimal_overflow,values_.location);
    }
    int compare(const Decimal96& a,const Decimal96& b) {
        const auto scale=std::max(a.scale,b.scale);
        const auto x=binary(Op::multiply,a.units,power(scale-a.scale));
        const auto y=binary(Op::multiply,b.units,power(scale-b.scale));
        work(x.bits()/28+y.bits()/28+1); return x.compare(y);
    }
    std::int32_t round_integer(const Decimal96& n) {
        const auto magnitude=rounded(n.units.absolute(),power(n.scale));
        if (n.units.negative() || magnitude>BigInteger::from_unsigned(INT32_MAX)) throw runtime_error(RuntimeError::integer_overflow,values_.location);
        return static_cast<std::int32_t>(magnitude.to_unsigned());
    }
};
}
RealRangePlan real_range_plan(Values& values,double first,double last,double step,const ExecutionOptions& options) {
    if (step==0) throw runtime_error(RuntimeError::sequence_step_zero,values.location);
    if ((step>0 && last<first) || (step<0 && last>first)) return {};
    if (first==last) return {1,0,false,true};
    DecimalMath math(values,options);
    const auto stride=math.real(step),a=math.real(first),b=math.real(last);
    auto scaled=math.divide(stride,math.subtract(a,b)); scaled.units=scaled.units.absolute();
    const auto one=math.integer(1);
    const auto n=math.round_integer(math.divide(one,scaled));
    const auto delta=math.real(1e-14),product=math.multiply(scaled,math.integer(n));
    const auto lower=math.subtract(one,delta),upper=math.add(one,delta);
    const auto low=math.compare(product,lower);
    const bool snap=low>=0 && math.compare(product,upper)<=0;
    return {static_cast<std::uint64_t>(n)+(snap || low<0 ? 1U : 0U),n,snap,false};
}
} // namespace abc

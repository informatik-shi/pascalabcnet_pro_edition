#include "abc/big_integer.hpp"
#include "tommath.h"
#include <bit>
#include <cmath>
#include <limits>
#include <new>
#include <stdexcept>

namespace abc {
namespace {
void check(mp_err error) {
    if (error==MP_MEM) throw std::bad_alloc{};
    if (error!=MP_OKAY) throw std::domain_error("Invalid multiple-precision operation");
}
[[noreturn]] void limit() { throw std::length_error("BigInteger bit limit exceeded"); }
}
struct BigInteger::Rep {
    mp_int number{};
    Rep() { check(mp_init_size(&number,1)); }
    ~Rep() { mp_clear(&number); }
    Rep(const Rep&)=delete;
    Rep& operator=(const Rep&)=delete;
};
const BigInteger::Rep& BigInteger::rep() const {
    static const Rep empty;
    return rep_ ? *rep_ : empty;
}
BigInteger BigInteger::finish(std::shared_ptr<Rep> value) {
    const auto count=mp_count_bits(&value->number);
    if (static_cast<std::size_t>(count)>max_bits) limit();
    return count ? BigInteger{std::move(value)} : BigInteger{};
}
BigInteger BigInteger::from_signed(std::int64_t value) {
    if (!value) return {};
    auto result=std::make_shared<Rep>(); mp_set_i64(&result->number,value); return finish(std::move(result));
}
BigInteger BigInteger::from_unsigned(std::uint64_t value) {
    if (!value) return {};
    auto result=std::make_shared<Rep>(); mp_set_u64(&result->number,value); return finish(std::move(result));
}
BigInteger BigInteger::from_real(double value) {
    static_assert(std::numeric_limits<double>::is_iec559 && std::numeric_limits<double>::digits==53 && sizeof(double)==sizeof(std::uint64_t));
    if (!std::isfinite(value)) throw std::domain_error("Nonfinite BigInteger conversion");
    if (std::abs(value)<1.0) return {};
    const auto raw=std::bit_cast<std::uint64_t>(value);
    const auto exponent=static_cast<int>((raw>>52)&2047U)-1023-52;
    const auto fraction=(raw&((std::uint64_t{1}<<52)-1))|(std::uint64_t{1}<<52);
    auto result=std::make_shared<Rep>(); mp_set_u64(&result->number,fraction);
    if (exponent>=0) check(mp_mul_2d(&result->number,exponent,&result->number));
    else check(mp_div_2d(&result->number,-exponent,&result->number,nullptr));
    if (raw>>63) check(mp_neg(&result->number,&result->number));
    return finish(std::move(result));
}
BigInteger BigInteger::parse(std::string_view text,unsigned base) {
    if (base<2 || base>36 || text.empty()) throw std::invalid_argument("Invalid BigInteger text");
    const bool minus=text.front()=='-';
    if (text.front()=='-' || text.front()=='+') text.remove_prefix(1);
    if (text.empty()) throw std::invalid_argument("Invalid BigInteger text");
    for (const auto c:text) {
        const auto digit=c>='0' && c<='9' ? static_cast<unsigned>(c-'0')
            : c>='A' && c<='Z' ? static_cast<unsigned>(c-'A'+10) : c>='a' && c<='z' ? static_cast<unsigned>(c-'a'+10) : 36U;
        if (digit>=base) throw std::invalid_argument("Invalid BigInteger digit");
    }
    const auto significant=text.find_first_not_of('0');
    if (significant==std::string_view::npos) return {};
    text.remove_prefix(significant);
    // At least one bit per significant digit; enforce before backend allocation.
    if (text.size()>max_bits) limit();
    unsigned bits_per_digit=0; for (auto n=base;n>1;n>>=1) ++bits_per_digit;
    const auto lower_bits=(text.size()-1)*bits_per_digit+1;
    if (lower_bits>max_bits) limit();
    std::string terminated; if (minus) terminated+='-'; terminated+=text;
    auto result=std::make_shared<Rep>(); check(mp_read_radix(&result->number,terminated.c_str(),static_cast<int>(base)));
    return finish(std::move(result));
}
std::string BigInteger::text(unsigned base) const {
    if (base<2 || base>36) throw std::invalid_argument("Invalid BigInteger radix");
    if (zero()) return "0";
    std::string result(bits()+2,'\0'); std::size_t written=0;
    check(mp_to_radix(&rep().number,result.data(),result.size(),&written,static_cast<int>(base)));
    if (!written || written>result.size()) throw std::logic_error("Invalid BigInteger output size");
    result.resize(written-1); return result;
}
std::size_t BigInteger::bits() const { return rep_ ? static_cast<std::size_t>(mp_count_bits(&rep_->number)) : 0; }
bool BigInteger::negative() const { return rep_ && rep_->number.sign==MP_NEG; }
bool BigInteger::odd() const { return rep_ && (rep_->number.dp[0]&1U)!=0; }
std::uint64_t BigInteger::to_unsigned() const {
    if (negative() || bits()>64) throw std::overflow_error("BigInteger cannot fit uint64");
    return mp_get_mag_u64(&rep().number);
}
std::int64_t BigInteger::to_signed() const {
    if (bits()>64) throw std::overflow_error("BigInteger cannot fit int64");
    const auto magnitude=mp_get_mag_u64(&rep().number);
    if (!negative()) {
        if (magnitude>INT64_MAX) throw std::overflow_error("BigInteger cannot fit int64");
        return static_cast<std::int64_t>(magnitude);
    }
    const auto boundary=std::uint64_t{1}<<63;
    if (magnitude>boundary) throw std::overflow_error("BigInteger cannot fit int64");
    return magnitude==boundary ? INT64_MIN : -static_cast<std::int64_t>(magnitude);
}
double BigInteger::to_real() const {
    const auto count=bits();
    if (!count) return 0.0;
    double result;
    if (count<=53) result=static_cast<double>(mp_get_mag_u64(&rep().number));
    else if (count>1024) result=std::numeric_limits<double>::infinity();
    else {
        const auto bit=[&](std::size_t position) { return (rep_->number.dp[position/MP_DIGIT_BIT]>>(position%MP_DIGIT_BIT))&1U; };
        std::uint64_t top=0;
        for (std::size_t i=0;i<53;++i) top=(top<<1)|bit(count-i-1);
        // System.Numerics.BigInteger / NumericsHelpers.GetDoubleFromParts
        // discards low mantissa bits instead of nearest-even rounding.
        result=std::ldexp(static_cast<double>(top),static_cast<int>(count-53));
    }
    return negative() ? -result : result;
}
int BigInteger::compare(const BigInteger& other) const { return mp_cmp(&rep().number,&other.rep().number); }
BigInteger BigInteger::negated() const {
    if (zero()) return {};
    auto result=std::make_shared<Rep>(); check(mp_neg(&rep().number,&result->number)); return finish(std::move(result));
}
BigInteger BigInteger::absolute() const { return negative() ? negated() : *this; }
BigInteger BigInteger::combine(Binary operation,const BigInteger& other) const {
    if (operation==Binary::multiply) {
        if (zero() || other.zero()) return {};
        if (bits()+other.bits()-1>max_bits) limit();
    }
    auto result=std::make_shared<Rep>(); const auto a=&rep().number,b=&other.rep().number;
    mp_err error=MP_VAL;
    switch (operation) {
    case Binary::add: error=mp_add(a,b,&result->number); break;
    case Binary::subtract: error=mp_sub(a,b,&result->number); break;
    case Binary::multiply: error=mp_mul(a,b,&result->number); break;
    case Binary::bit_and: error=mp_and(a,b,&result->number); break;
    case Binary::bit_or: error=mp_or(a,b,&result->number); break;
    case Binary::bit_xor: error=mp_xor(a,b,&result->number); break;
    case Binary::gcd: error=mp_gcd(a,b,&result->number); break;
    }
    check(error); return finish(std::move(result));
}
BigInteger BigInteger::add(const BigInteger& other) const { return combine(Binary::add,other); }
BigInteger BigInteger::subtract(const BigInteger& other) const { return combine(Binary::subtract,other); }
BigInteger BigInteger::multiply(const BigInteger& other) const { return combine(Binary::multiply,other); }
BigInteger BigInteger::bit_and(const BigInteger& other) const { return combine(Binary::bit_and,other); }
BigInteger BigInteger::bit_or(const BigInteger& other) const { return combine(Binary::bit_or,other); }
BigInteger BigInteger::bit_xor(const BigInteger& other) const { return combine(Binary::bit_xor,other); }
BigInteger BigInteger::gcd(const BigInteger& other) const { return combine(Binary::gcd,other); }
std::pair<BigInteger,BigInteger> BigInteger::divide(const BigInteger& other) const {
    if (other.zero()) throw std::domain_error("BigInteger division by zero");
    auto quotient=std::make_shared<Rep>(),remainder=std::make_shared<Rep>();
    check(mp_div(&rep().number,&other.rep().number,&quotient->number,&remainder->number));
    return {finish(std::move(quotient)),finish(std::move(remainder))};
}
BigInteger BigInteger::complement() const {
    auto result=std::make_shared<Rep>(); check(mp_complement(&rep().number,&result->number)); return finish(std::move(result));
}
BigInteger BigInteger::shift(bool left,std::uint64_t count) const {
    if (zero() || !count) return *this;
    if (left && count>max_bits-bits()) limit();
    if (!left && count>=bits()) return negative() ? from_signed(-1) : BigInteger{};
    auto result=std::make_shared<Rep>();
    check(left ? mp_mul_2d(&rep().number,static_cast<int>(count),&result->number)
        : mp_signed_rsh(&rep().number,static_cast<int>(count),&result->number));
    return finish(std::move(result));
}
BigInteger BigInteger::shift_left(std::int32_t count) const { return shift(count>=0,count<0 ? static_cast<std::uint64_t>(-std::int64_t(count)) : static_cast<std::uint64_t>(count)); }
BigInteger BigInteger::shift_right(std::int32_t count) const { return shift(count<0,count<0 ? static_cast<std::uint64_t>(-std::int64_t(count)) : static_cast<std::uint64_t>(count)); }
BigInteger BigInteger::square_root() const {
    if (negative()) throw std::domain_error("Negative BigInteger square root");
    auto result=std::make_shared<Rep>(); check(mp_sqrt(&rep().number,&result->number)); return finish(std::move(result));
}
BigInteger BigInteger::power(std::uint32_t exponent) const {
    if (!exponent) return from_signed(1);
    if (zero()) return {};
    if (bits()==1) return from_signed(negative() && (exponent&1U) ? -1 : 1);
    if (std::uint64_t(bits()-1)*exponent>=max_bits || exponent>INT32_MAX) limit();
    auto result=std::make_shared<Rep>(); check(mp_expt_n(&rep().number,static_cast<int>(exponent),&result->number)); return finish(std::move(result));
}
} // namespace abc

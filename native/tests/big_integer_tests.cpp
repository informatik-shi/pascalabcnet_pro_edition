#include "abc/big_integer.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
std::size_t checks=0,failures=0;
void require(bool condition,const char* description) {
    ++checks; if (!condition) { ++failures; std::cerr<<"FAIL: "<<description<<'\n'; }
}
template<class E,class F> void raises(F action,const char* description) {
    try { action(); require(false,description); }
    catch (const E&) { require(true,description); }
    catch (...) { require(false,description); }
}
using B=abc::BigInteger;
}
int main() {
    for (std::int64_t a= -65;a<=65;++a) for (std::int64_t b= -65;b<=65;++b) {
        const auto x=B::from_signed(a),y=B::from_signed(b);
        require(x.add(y).to_signed()==a+b,"signed addition");
        require(x.subtract(y).to_signed()==a-b,"signed subtraction");
        require(x.multiply(y).to_signed()==a*b,"signed multiplication");
        require(x.bit_and(y).to_signed()==(a&b),"signed bitwise and");
        require(x.bit_or(y).to_signed()==(a|b),"signed bitwise or");
        require(x.bit_xor(y).to_signed()==(a^b),"signed bitwise xor");
        require((x<y)==(a<b) && (x==y)==(a==b),"signed comparison");
        if (b) {
            const auto [q,r]=x.divide(y);
            require(q.to_signed()==a/b && r.to_signed()==a%b,"division truncates toward zero and preserves remainder sign");
            require(q.multiply(y).add(r)==x,"division reconstruction");
        }
    }
    for (std::int64_t a= -65;a<=65;++a) for (std::int32_t shift=0;shift<=20;++shift) {
        const auto x=B::from_signed(a);
        const auto divisor=std::int64_t{1}<<shift;
        const auto right=a>=0 ? a/divisor : -((-a+divisor-1)/divisor);
        require(x.shift_left(shift).to_signed()==a*divisor,"left shift");
        require(x.shift_right(shift).to_signed()==right,"arithmetic right shift");
        require(x.shift_left(-shift).to_signed()==right && x.shift_right(-shift).to_signed()==a*divisor,"negative shifts reverse direction");
        require(x.complement().to_signed()==~a,"signed complement");
    }
    for (auto value:{INT64_MIN,INT64_MIN+1,std::int64_t{-1},std::int64_t{0},std::int64_t{1},INT64_MAX}) {
        const auto x=B::from_signed(value);
        require(x.to_signed()==value && B::parse(std::to_string(value))==x,"int64 boundaries");
    }
    const auto max_unsigned=B::from_unsigned(UINT64_MAX);
    require(max_unsigned.text()=="18446744073709551615" && max_unsigned.to_unsigned()==UINT64_MAX,"uint64 full magnitude");
    raises<std::overflow_error>([&]{(void)max_unsigned.to_signed();},"positive int64 overflow");
    raises<std::overflow_error>([]{(void)B::parse("-9223372036854775809").to_signed();},"negative int64 overflow");
    raises<std::overflow_error>([]{(void)B::from_signed(-1).to_unsigned();},"negative uint64 overflow");
    require(B::parse("-000").zero() && !B::parse("-000").negative(),"negative zero normalization");
    require(B::parse("+000ff",16).to_signed()==255 && B::from_signed(-255).text(16)=="-FF","radix signs and case");
    for (const auto text:{"","+","-"," 1","1 ","1x","1\n","12.0"}) raises<std::invalid_argument>([&]{(void)B::parse(text);},"whole text parsing");
    raises<std::invalid_argument>([]{(void)B::parse(std::string_view{"1\0",2});},"embedded NUL rejected");
    raises<std::domain_error>([]{(void)B::from_signed(1).divide(B{});},"division by zero");
    const auto two=B::from_signed(2),one=B::from_signed(1);
    const auto large=two.power(200).add(one),square=large.multiply(large);
    require(square.square_root()==large && square.subtract(one).square_root()==large.subtract(one),"exact large integer square root");
    require(large.negated().gcd(large.multiply(B::from_signed(3)))==large,"positive gcd of signed large operands");
    raises<std::domain_error>([]{(void)B::from_signed(-1).square_root();},"negative square root");
    auto factorial=one;
    for (std::int64_t i=2;i<=100;++i) factorial=factorial.multiply(B::from_signed(i));
    require(factorial.text()=="93326215443944152681699238856266700490715968264381621468592963895217599993229915608941463976156518286253697920827223758251185210916864000000000000000000000000","100 factorial");
    require(B::from_signed(-1).power(UINT32_MAX)==B::from_signed(-1) && B{}.power(0)==one,"power degenerate cases");
    for (const auto value:{0.0,-0.0,0.99,-0.99,1.99,-1.99,std::nextafter(1.0,0.0),9007199254740992.0})
        require(B::from_real(value).to_signed()==static_cast<std::int64_t>(value),"real conversion truncation");
    require(B::parse("9007199254740993").to_real()==9007199254740992.0 && B::parse("9007199254740995").to_real()==9007199254740994.0
        && B::parse("-9007199254740995").to_real()==-9007199254740994.0,"source real conversion discards low bits");
    const auto p64=std::ldexp(1.0,64);
    require(B::parse("18446744073709553664").to_real()==p64 && B::parse("18446744073709555711").to_real()==p64
        && B::parse("18446744073709555712").to_real()==std::nextafter(p64,std::numeric_limits<double>::infinity()),"mantissa truncation across backend limbs");
    const auto max_real=std::numeric_limits<double>::max();
    require(B::from_real(max_real).to_real()==max_real && B::from_real(-max_real).to_real()==-max_real,"finite real boundaries");
    require(B::from_real(max_real).add(two.power(970)).to_real()==max_real && std::isinf(two.power(1024).to_real()),"source real overflow boundary");
    raises<std::domain_error>([]{(void)B::from_real(std::numeric_limits<double>::infinity());},"nonfinite input");
    const auto boundary=one.shift_left(static_cast<std::int32_t>(B::max_bits-1));
    require(boundary.bits()==B::max_bits,"bit limit boundary");
    raises<std::length_error>([&]{(void)boundary.shift_left(1);},"left shift bit limit");
    raises<std::length_error>([]{(void)B::parse(std::string(B::max_bits+1,'1'),2);},"input bit limit");
    require(one.shift_left(INT32_MIN).zero() && B::from_signed(-1).shift_right(INT32_MAX)==B::from_signed(-1),"extreme right shifts");
    raises<std::length_error>([&]{(void)one.shift_right(INT32_MIN);},"extreme reversed left shift");
    std::cout<<(checks-failures)<<'/'<<checks<<" BigInteger checks passed\n";
    return failures ? 1 : 0;
}

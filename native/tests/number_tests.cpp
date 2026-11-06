// Exercise the libc fallback even on platforms that have floating from_chars.
#ifdef ABC_HAS_FLOAT_FROM_CHARS
#undef ABC_HAS_FLOAT_FROM_CHARS
#endif
#include "../src/number.hpp"
#include <array>
#include <limits>
#include <iostream>

int main() {
    unsigned failures=0,checks=0;
    const auto check=[&](bool passed,std::string_view text) {
        ++checks; if (!passed) { ++failures; std::cerr<<"FAIL: "<<text<<'\n'; }
    };
    const std::pair<std::string_view,double> accepted[]={
        {"0",0},{"-0",-0.0},{".5",0.5},{"5.",5},{"-3.25e+2",-325},
        {"1e-320",1e-320},{"-1e-320",-1e-320},
        {"4.9406564584124654e-324",std::numeric_limits<double>::denorm_min()},
        {"3e-324",std::numeric_limits<double>::denorm_min()},
        {"2.2250738585072014e-308",std::numeric_limits<double>::min()},
        {"1.7976931348623157e308",std::numeric_limits<double>::max()},
        {"0e999999999",0},{"-0e-999999999",-0.0}
    };
    for (const auto& [text,expected]:accepted) {
        double actual=123;
        check(abc::parse_real(text,actual) && actual==expected && std::signbit(actual)==std::signbit(expected),text);
    }
    for (const auto text:{"","-","+",".","-.",".e2","1e","1e+","1e-","+1"," 1","1 ","1\n",
        "1,5","nan","-nan","inf","infinity","0x1p0","0x1","1.2.3","1e2e3","1e9999","1e-9999","2e-324"}) {
        double actual=123; check(!abc::parse_real(text,actual),text);
    }
    const std::pair<std::string_view,float> singles[]={
        {"0",0.0F},{"-0",-0.0F},{"1.000000059604644775390625",1.0F},
        {"1.000000059604644775390626",std::nextafter(1.0F,2.0F)},
        {"1.40129846432481707092372958328991613e-45",std::numeric_limits<float>::denorm_min()},
        {"7.00649232162408535461864791644958066e-46",std::numeric_limits<float>::denorm_min()},
        {"7.00649232162408535461864791644958065e-46",0.0F},
        {"3.40282346638528859811704183484516925440e38",std::numeric_limits<float>::max()},
        {"1e-9999",0.0F},{"-1e-9999",-0.0F}
    };
    for(const auto& [text,expected]:singles){float actual=123;check(abc::parse_single(text,actual) && actual==expected && std::signbit(actual)==std::signbit(expected),text);}
    for(const auto text:{"","-","nan","inf","0x1p0","1e39","1e9999","+1"," 1","1 ","1e-","1,5"}){float actual=0;check(!abc::parse_single(text,actual),text);}
    const std::string embedded("1\0x",3);
    double actual=123; check(!abc::parse_real(embedded,actual),"Embedded NUL");
    std::cout<<(checks-failures)<<'/'<<checks<<" fallback floating checks passed\n";
    return failures ? 1 : 0;
}

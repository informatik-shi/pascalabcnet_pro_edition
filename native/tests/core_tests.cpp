#include "abc/core.hpp"

#include <iostream>
#include <limits>
#include <sstream>

namespace {
int failures = 0;
int checks = 0;

void require(bool condition, std::string_view description) {
    ++checks;
    if (!condition) { ++failures; std::cerr << "FAIL: " << description << '\n'; }
}

std::string run(std::string_view source, std::string input = "", abc::ExecutionOptions options = {}) {
    auto program = abc::compile(source);
    std::istringstream in(input);
    std::ostringstream out;
    abc::execute(program, in, out, options);
    return out.str();
}

void result(std::string_view source, std::string_view expected, std::string input = "") {
    try { require(run(source, std::move(input)) == expected, source); }
    catch (const std::exception& error) { require(false, std::string(source) + " -> " + error.what()); }
}

template<class F> void error(F action, std::string_view code) {
    try { action(); require(false, std::string("Expected ") + std::string(code)); }
    catch (const abc::Error& caught) { require(caught.code == code, std::string("Expected ") + std::string(code) + ", got " + caught.code); }
    catch (const std::exception& caught) { require(false, caught.what()); }
}

void compile_error(std::string_view source, std::string_view code) {
    error([&] { (void)abc::compile(source); }, code);
}

void runtime_error(std::string_view source, std::string_view code) {
    error([&] { (void)run(source); }, code);
}

void language() {
    result("begin end.", "");
    result("program Demo; begin writeln(1 + 2 * 3); end.", "7\n");
    result("BEGIN var N := 10; n := N + 2; WRITELN(n); END.", "12\n");
    result("var a,b: integer; begin a := 4; b := 5; writeln(a * b); end.", "20\n");
    result("begin var n: int64 := 2147483648; writeln(n + 1); end.", "2147483649\n");
    result("begin writeln(9007199254740993 + 2); end.", "9007199254740995\n");
    result("begin writeln(-2147483648, ' ', -9223372036854775808); end.", "-2147483648 -9223372036854775808\n");
    result("begin var x: real := 3; writeln(x / 2); writeln(5 div 2, ' ', -5 mod 2); end.", "1.5\n2 -1\n");
    result("begin writeln(1.25e2 + 0.5); end.", "125.5\n");
    result("begin writeln(1e-320 <> 0); end.", "True\n");
    result("begin writeln('It''s ' + 'Pascal'); writeln('', true, false); end.", "It's Pascal\nTrueFalse\n");
    result("begin writeln('Привет 🌍'); end.", "Привет 🌍\n");
    result("begin { outer (* inner *) } // line\n writeln(42); end.", "42\n");
    result("\xef\xbb\xbf" "begin\r\nwriteln(1);\r\nend.\r\n", "1\n");
    result("begin if true then writeln('yes') else writeln('no'); end.", "yes\n");
    result("begin if false then writeln('yes') else if true then writeln('no'); end.", "no\n");
    result("begin var i := 1; var total := 0; while i <= 10 do begin total := total + i; i := i + 1; end; writeln(total); end.", "55\n");
    result("begin var n := 1; begin var n := 2; writeln(n); end; writeln(n); end.", "2\n1\n");
    result("begin var i := 0; while i < 2 do begin var n: integer; n := n + 1; writeln(n); i := i + 1; end; end.", "1\n1\n");
    result("begin writeln(false and (1 div 0 = 0)); writeln(true or (1 div 0 = 0)); end.", "False\nTrue\n");
    result("begin writeln(not false, ' ', (2 < 3) and (4 >= 4), ' ', 'a' <> 'b'); end.", "True True True\n");
    result("begin var n: integer; readln(n); writeln(n * 2); end.", "42\n", " +21 \r\n");
    result("begin var s: string; readln(s); writeln(s); end.", "Привет мир\n", "Привет мир\n");
    result("begin var b: boolean; readln(b); writeln(not b); end.", "False\n", "TRUE\n");

    compile_error("begin var n := 10; n := 'text'; end.", "E_TYPE");
    compile_error("begin writeln('must not run'); if false then begin var n := 1; n := 'bad'; end; end.", "E_TYPE");
    compile_error("begin if false then writeln(missing); end.", "E_NAME");
    compile_error("begin var n := 1; var N := 2; end.", "E_DUPLICATE");
    compile_error("begin begin var n := 1; end; writeln(n); end.", "E_NAME");
    compile_error("begin if true then var n := 1; writeln(n); end.", "E_NAME");
    compile_error("begin if 1 then writeln(1); end.", "E_TYPE");
    compile_error("begin var n: integer := 2147483648; end.", "E_TYPE");
    compile_error("begin var x := 2.5 div 2; end.", "E_TYPE");
    compile_error("begin var a: bigint; end.", "E_UNSUPPORTED");
    compile_error("begin writeln(9223372036854775808); end.", "E_RANGE");
    compile_error("begin writeln(1e9999); end.", "E_RANGE");
    compile_error("begin writeln(1e-9999); end.", "E_RANGE");
    compile_error("begin writeln(1e); end.", "E_SYNTAX");
    compile_error("begin writeln('abc); end.", "E_SYNTAX");
    compile_error("begin (* comment", "E_SYNTAX");
    compile_error("begin {$Q-} writeln(1); end.", "E_UNSUPPORTED");
    compile_error("begin end. writeln(1)", "E_SYNTAX");
    compile_error("begin writeln('\xc0\xaf'); end.", "E_ENCODING");
    compile_error(std::string(1'048'577, ' '), "E_LIMIT");
    compile_error("begin writeln(" + std::string(300, '(') + "1" + std::string(300, ')') + "); end.", "E_LIMIT");
    std::string long_expression = "begin writeln(1";
    for (int i = 0; i < 300; ++i) long_expression += "+1";
    compile_error(long_expression + "); end.", "E_LIMIT");

    try { (void)abc::compile("begin\nvar x := 1;\nx := 'bad';\nend."); }
    catch (const abc::Error& caught) { require(caught.location.line == 3, "Diagnostic points to assignment source line"); }
}

void arithmetic() {
    for (const auto& source : {
        "begin writeln(2147483647 + 1); end.",
        "begin writeln(-2147483648 - 1); end.",
        "begin writeln(2147483647 * 2); end.",
        "begin var n := -2147483648; writeln(-n); end.",
        "begin writeln(-2147483648 div -1); end.",
        "begin writeln(9223372036854775807 + 1); end.",
        "begin writeln(-9223372036854775808 - 1); end.",
        "begin writeln(-9223372036854775808 * -1); end.",
        "begin writeln(9223372036854775807 * 2); end.",
        "begin writeln(-9223372036854775808 div -1); end.",
        "begin writeln(1e308 * 1e308); end."}) runtime_error(source, "E_OVERFLOW");
    result("begin writeln(-9223372036854775808 mod -1); end.", "0\n");
    result("begin writeln(-4611686018427387904 * 2); end.", "-9223372036854775808\n");
    result("begin writeln(-2147483648 * 0, ' ', -2 * -3); end.", "0 6\n");
    runtime_error("begin writeln(1 div 0); end.", "E_DIV_ZERO");
    runtime_error("begin writeln(1 mod 0); end.", "E_DIV_ZERO");
    runtime_error("begin writeln(1 / 0); end.", "E_DIV_ZERO");
    runtime_error("begin var n: integer; readln(n); end.", "E_INPUT");
    error([] { (void)run("begin var n: integer; readln(n); end.", "2147483648\n"); }, "E_INPUT");
    error([] { (void)run("begin var n: real; readln(n); end.", "NaN\n"); }, "E_INPUT");
    error([] { (void)run("begin var n: real; readln(n); end.", "++2\n"); }, "E_INPUT");
    error([] { (void)run("begin var s: string; readln(s); end.", "\xff\n"); }, "E_INPUT");
    abc::ExecutionOptions options;
    options.instruction_limit = 100;
    error([&] { (void)run("begin while true do begin end; end.", "", options); }, "E_LIMIT");
    options.cancelled = [] { return true; };
    error([&] { (void)run("begin writeln(1); end.", "", options); }, "E_CANCELLED");
    options = {};
    options.output_limit = 2;
    error([&] { (void)run("begin writeln('abc'); end.", "", options); }, "E_LIMIT");
    runtime_error("begin var s := 'a'; var n := 0; while n < 30 do begin s := s + s; n := n + 1; end; end.", "E_LIMIT");
}

void standard_library() {
    result("uses PABCSystem; begin writeln(Abs(-7), ' ', Abs(-7.5), ' ', Abs(-9223372036854775807)); end.", "7 7.5 9223372036854775807\n");
    result("begin var n: int64 := Sqr(50000); writeln(n); end.", "2500000000\n");
    result("begin writeln(Sqr(-2147483648), ' ', Sqr(3037000499)); end.", "4611686018427387904 9223372030926249001\n");
    result("begin writeln(Sign(-1), ' ', Sign(0), ' ', Sign(1.5), ' ', Sign(-9223372036854775808)); end.", "-1 0 1 -1\n");
    result("begin writeln(Min(3, -2), ' ', Max(1, 7, 3), ' ', Min(5, 3, 2, 4)); end.", "-2 7 2\n");
    result("begin writeln(Max(9007199254740993, 9007199254740992), ' ', Min(9007199254740993, 9007199254740992, 9007199254740994)); end.", "9007199254740993 9007199254740992\n");
    result("begin writeln(Min(2, 1.5), ' ', Max(1.5, 2, 3.0, 2.5)); end.", "1.5 3\n");
    result("begin writeln(Sqrt(81), ' ', Power(2, -3), ' ', Power(9, 0.5)); end.", "9 0.125 3\n");
    result("begin writeln(ISqrt(0), ' ', ISqrt(1), ' ', ISqrt(2), ' ', ISqrt(15), ' ', ISqrt(16), ' ', ISqrt(MaxInt64)); end.", "0 1 1 3 4 3037000499\n");
    result("begin writeln(Abs(Cbrt(-27) + 3) < 1e-12); end.", "True\n");
    result("begin writeln(Abs(Sin(Pi/2)-1)<1e-12, ' ', Abs(Cos(Pi)+1)<1e-12, ' ', Abs(Tan(Pi/4)-1)<1e-12); end.", "True True True\n");
    result("begin writeln(Sinh(0)=0, ' ', Cosh(0)=1, ' ', Tanh(0)=0); end.", "True True True\n");
    result("begin writeln(Abs(ArcSin(1)-Pi/2)<1e-12, ' ', Abs(ArcCos(-1)-Pi)<1e-12, ' ', Abs(ArcTan(1)-Pi/4)<1e-12); end.", "True True True\n");
    result("begin writeln(Abs(Exp(1)-E)<1e-12, ' ', Abs(Ln(E)-1)<1e-12, ' ', Log(1)=0); end.", "True True True\n");
    result("begin writeln(Abs(Log2(8)-3)<1e-12, ' ', Abs(Log10(100)-2)<1e-12, ' ', Abs(LogN(2,8)-3)<1e-12); end.", "True True True\n");
    result("begin writeln(Abs(DegToRad(180)-Pi)<1e-12, ' ', Abs(RadToDeg(Pi)-180)<1e-12); end.", "True True\n");
    result("begin writeln(Round(0.5), ' ', Round(1.5), ' ', Round(2.5), ' ', Round(-0.5), ' ', Round(-1.5), ' ', Round(-2.5)); end.", "0 2 2 0 -2 -2\n");
    result("begin writeln(Round(1.25,1), ' ', Round(1.35,1), ' ', Round(-1.25,1)); end.", "1.2 1.4 -1.2\n");
    result("begin writeln(Round(2147483647.4), ' ', Round(-2147483648.5)); end.", "2147483647 -2147483648\n");
    result("begin var n: integer := Trunc(-2.8); writeln(n, ' ', Floor(-2.8), ' ', Ceil(-2.8)); end.", "-2 -3 -2\n");
    result("begin writeln(Int(-2.75), ' ', Frac(-2.75), ' ', Trunc(-2147483648.9)); end.", "-2 -0.75 -2147483648\n");
    result("begin writeln(MaxInt, ' ', MaxInt64, ' ', MaxReal=MaxDouble, ' ', MinReal=MinDouble, ' ', MinReal>0); end.", "2147483647 9223372036854775807 True True True\n");
    result("begin writeln(Length(''), ' ', Length('Привет'), ' ', Length('🌍')); end.", "0 6 2\n");
    result("begin writeln(Copy('abcdef', 0, 3), ' ', Copy('abcdef', 4, 2147483647)); end.", "abc def\n");
    result("begin writeln(Copy('abc', 4, 1) = '', ' ', Copy('abc', 1, 0) = '', ' ', Copy('abc', 1, -1) = ''); end.", "True True True\n");
    result("begin writeln(Copy('Привет', 2, 3), ' ', Copy('🌍', 1, 2)); end.", "рив 🌍\n");
    result("begin writeln(Copy('🌍', 1, 1), ' ', Copy('🌍', 2, 1)); end.", "� �\n");
    result("begin writeln(Concat(Copy('🌍', 1, 1), Copy('🌍', 2, 1))); end.", "🌍\n");
    result("begin writeln(Pos('abc','abcabc'), ' ', PosEx('abc','abcabc',2), ' ', LastPos('abc','abcabc'), ' ', LastPos('abc','abcabc',5)); end.", "1 4 4 1\n");
    result("begin writeln(Pos('','abc'), ' ', Pos('x','abc'), ' ', Pos('a','abc',4), ' ', LastPos('aa','aaaa')); end.", "0 0 0 3\n");
    result("begin writeln(Pos('🌍','а🌍б'), ' ', Pos('б','а🌍б'), ' ', LastPos('abc','abcabc',2)); end.", "2 4 0\n");
    result("begin writeln(LastPos('a',''), ' ', LastPos('a','',0)); end.", "0 0\n");
    result("begin writeln(Concat('Привет, ', 'мир')); end.", "Привет, мир\n");
    result("begin var i := 10; Inc(i); Dec(i, 3); Inc(i, -2); writeln(i); end.", "6\n");
    result("begin var i: int64 := 2147483647; Inc(i); Dec(i, -1); writeln(i); end.", "2147483649\n");
    result("begin var i := 10; Inc(i, i); writeln(i); end.", "20\n");
    result("begin var Pi := 7; writeln(Pi); begin var Pi := 2; writeln(Pi); end; end.", "7\n2\n");
    compile_error("begin var n: integer := Sqr(5); end.", "E_TYPE");
    compile_error("begin writeln(Sqrt('bad')); end.", "E_TYPE");
    compile_error("begin writeln(Length(2)); end.", "E_TYPE");
    compile_error("begin writeln(Abs()); end.", "E_TYPE");
    compile_error("begin writeln(Abs(1,2)); end.", "E_TYPE");
    compile_error("begin writeln(Min(true,false)); end.", "E_TYPE");
    compile_error("begin var n := 2.0; Inc(n); end.", "E_TYPE");
    compile_error("begin var n := 2; Inc(n,1.5); end.", "E_TYPE");
    compile_error("begin var n := 2; Inc(n,2147483648); end.", "E_TYPE");
    compile_error("begin var Abs := 2; writeln(Abs(1)); end.", "E_TYPE");
    compile_error("begin writeln('must not run'); if false then writeln(UnknownFunction(1)); end.", "E_NAME");
    compile_error("uses GraphABC; begin end.", "E_UNSUPPORTED");
    std::string nested = "begin writeln(";
    for (int i = 0; i < 140; ++i) nested += "Abs(";
    nested += "1" + std::string(140, ')') + "); end.";
    compile_error(nested, "E_LIMIT");
    for (const auto& source : {"begin writeln(Abs(-2147483648)); end.",
        "begin writeln(Abs(-9223372036854775808)); end.", "begin writeln(Sqr(3037000500)); end.",
        "begin writeln(Round(2147483647.5)); end.", "begin writeln(Trunc(2147483648.0)); end.",
        "begin writeln(Floor(-2147483648.1)); end.", "begin writeln(Ceil(2147483647.1)); end.",
        "begin var i := MaxInt; Inc(i); end.", "begin var i := -2147483648; Dec(i); end.",
        "begin var i := MaxInt64; Inc(i); end."}) runtime_error(source, "E_OVERFLOW");
    for (const auto& source : {"begin writeln(Sqrt(-1)); end.", "begin writeln(Ln(0)); end.",
        "begin writeln(Log2(-1)); end.", "begin writeln(Log10(0)); end.", "begin writeln(ISqrt(-1)); end.",
        "begin writeln(LogN(1, 2)); end.", "begin writeln(LogN(2, 0)); end.",
        "begin writeln(ArcSin(2)); end.", "begin writeln(ArcCos(-2)); end.",
        "begin writeln(Power(-2, 0.5)); end."}) runtime_error(source, "E_DOMAIN");
    runtime_error("begin writeln(Exp(1000)); end.", "E_OVERFLOW");
    runtime_error("begin writeln(Round(1.2,16)); end.", "E_RANGE");
    runtime_error("begin writeln(Round(1.2,-1)); end.", "E_RANGE");
    runtime_error("begin writeln(Pos('a','abc',0)); end.", "E_RANGE");
    runtime_error("begin writeln(LastPos('a','abc',0)); end.", "E_RANGE");
    runtime_error("begin var s := 'a'; var n := 0; while n < 21 do begin s := Concat(s,s); Inc(n); end; end.", "E_LIMIT");
    try { (void)run("begin\n  Writeln(Sqrt(-1));\nend."); require(false, "Expected domain error"); }
    catch (const abc::Error& caught) {
        require(caught.location.line == 2 && caught.russian == "Аргумент находится вне области определения функции",
            "Standard library failures retain source location and Russian message");
    }
}

void text_library() {
    result("begin writeln('[',Trim('  abc  '),'] [',TrimLeft('  abc  '),'] [',TrimRight('  abc  '),']'); end.", "[abc] [abc  ] [  abc]\n");
    result("begin writeln(Length(Trim('   ')), ' ', Length(Trim(' ')), ' ', Length(Trim('\t'))); end.", "0 1 1\n");
    result("begin writeln(LeftStr('Привет',3), ' ', RightStr('Привет',3)); end.", "При вет\n");
    result("begin writeln(LeftStr('abc',-1)='', ' ', RightStr('abc',0)='', ' ', LeftStr('abc',100)='abc'); end.", "True True True\n");
    result("begin writeln(ReverseString('abcdef'), ' ', ReverseString('abcdef',2,3)); end.", "fedcba adcbef\n");
    result("begin writeln(ReverseString('',1,0)='', ' ', ReverseString('abc',4,0)='abc'); end.", "True True\n");
    result("begin var s := 'abc'; writeln(ReverseString(s), ' ', s); end.", "cba abc\n");
    result("begin writeln(ReverseString(ReverseString('🌍'))); end.", "🌍\n");
    result("begin writeln(ReverseString('🌍')); end.", "��\n");
    result("begin writeln(CompareStr('abc','abd')<0, ' ', CompareStr('abc','abc')=0, ' ', CompareStr('abcd','abc')>0); end.", "True True True\n");
    result("begin writeln(CompareStr('Я','я')<0, ' ', CompareStr('','')=0, ' ', CompareStr('','a')<0); end.", "True True True\n");
    result("begin writeln(IntToStr(-2147483648),' ',IntToStr(-9223372036854775808),' ',IntToStr(0)); end.", "-2147483648 -9223372036854775808 0\n");
    result("begin var n: integer := StrToInt(' +2147483647 '); writeln(n,' ',StrToInt('-2147483648')); end.", "2147483647 -2147483648\n");
    result("begin writeln(StrToInt(' 42 '),' ',StrToInt('　-42　'),' ',StrToInt('\t42\t')); end.", "42 -42 42\n");
    result("begin writeln(StrToInt64(' +9223372036854775807 '),' ',StrToInt64('-9223372036854775808')); end.", "9223372036854775807 -9223372036854775808\n");
    result("begin writeln(StrToInt('00000000000000000000000042'),' ',StrToInt64('-0')); end.", "42 0\n");
    result("begin writeln(StrToReal('1,234.5','.'),' ',StrToFloat(' +1.25e2 ')); end.", "1234.5 125\n");
    result("begin writeln(StrToReal('1,25',','),' ',StrToFloat('1::25','::')); end.", "1.25 1.25\n");
    result("begin writeln(StrToReal('.5'),' ',StrToReal('5.'),' ',StrToReal('1e-999'),' ',StrToReal('3e-324')>0); end.", "0.5 5 0 True\n");
    result("begin writeln(StrToReal('0e999999999')=0,' ',StrToReal('-1e-999')=0); end.", "True True\n");
    result("begin writeln(FloatToStr(1.25),' ',FloatToStr(-0.5),' ',FloatToStr(1000000.0)); end.", "1.25 -0.5 1000000\n");
    result("begin writeln(FloatToStr(0.0001),' ',FloatToStr(0.00001),' ',FloatToStr(1e16),' ',FloatToStr(1e17)); end.", "0.0001 1E-05 10000000000000000 1E+17\n");
    result("begin writeln(FloatToStr(-0.0),' ',FloatToStr(1.2345678901234567)); end.", "-0 1.2345678901234567\n");
    result("begin writeln(StrToInt(IntToStr(MaxInt))=MaxInt,' ',StrToInt64(IntToStr(MaxInt64))=MaxInt64,' ',StrToReal(FloatToStr(MinReal))=MinReal); end.", "True True True\n");
    result("begin var s: string; Readln(s); Writeln(StrToInt64(s)); end.", "42\n", std::string("42 \0\0\n", 6));
    result("begin var s: string; Readln(s); Writeln(StrToReal(s)); end.", "1.5\n", std::string("1.5 \0\n", 6));
    error([] { (void)run("begin var s: string; Readln(s); Writeln(StrToInt64(s)); end.", std::string("42\0 \n", 5)); }, "E_FORMAT");
    error([] { (void)run("begin var s: string; Readln(s); Writeln(StrToInt(s)); end.", std::string("42\0\n", 4)); }, "E_FORMAT");
    runtime_error("begin writeln(StrToInt('999999999999999999999x')); end.", "E_OVERFLOW");
    runtime_error("begin writeln(StrToInt64('999999999999999999999x')); end.", "E_FORMAT");
    for (const auto& source : {"begin writeln(StrToInt('')); end.", "begin writeln(StrToInt('+')); end.",
        "begin writeln(StrToInt('--1')); end.", "begin writeln(StrToInt('1 2')); end.",
        "begin writeln(StrToInt('1.0')); end.", "begin writeln(StrToInt64('9223372036854775807x')); end.",
        "begin writeln(StrToReal('1.2.3')); end.", "begin writeln(StrToReal('1e+')); end.",
        "begin writeln(StrToReal('1,,.2.3')); end.", "begin writeln(StrToReal('1,234.5')); end.",
        "begin writeln(StrToInt64(' 42 ')); end."}) runtime_error(source, "E_FORMAT");
    for (const auto& source : {"begin writeln(StrToInt('2147483648')); end.",
        "begin writeln(StrToInt('-2147483649')); end.", "begin writeln(StrToInt64('9223372036854775808')); end.",
        "begin writeln(StrToInt64('-9223372036854775809')); end.", "begin writeln(StrToReal('1e999')); end."}) runtime_error(source, "E_OVERFLOW");
    for (const auto& source : {"begin writeln(ReverseString('abc',0,1)); end.",
        "begin writeln(ReverseString('abc',1,-1)); end.", "begin writeln(ReverseString('abc',3,2)); end.",
        "begin writeln(ReverseString('',2,0)); end.", "begin writeln(StrToReal('1','')); end."}) runtime_error(source, "E_RANGE");
    compile_error("begin writeln(IntToStr(1.5)); end.", "E_TYPE");
    compile_error("begin var n: integer := StrToInt64('1'); end.", "E_TYPE");
    compile_error("begin writeln(Trim(1)); end.", "E_TYPE");
    compile_error("begin writeln(LeftStr('abc',1.5)); end.", "E_TYPE");
    try { (void)run("begin\n  Writeln(StrToInt('bad'));\nend."); require(false, "Expected conversion failure"); }
    catch (const abc::Error& caught) {
        require(caught.location.line == 2 && caught.russian == "Некорректная запись числа для типа integer",
            "String conversion retains its source line and Russian type-specific message");
    }
}

void mutable_strings() {
    result("begin var s := 'ab'; SetLength(s,5); writeln(Length(s),' [',s,']'); end.", "5 [ab   ]\n");
    result("begin var s: string; SetLength(s,3); writeln('[',s,']'); end.", "[   ]\n");
    result("begin var s := 'abc'; SetLength(s,1); writeln(s); SetLength(s,0); writeln(Length(s)); end.", "a\n0\n");
    result("begin var s := 'abc'; var original := s; SetLength(s,1); writeln(original,' ',s); end.", "abc a\n");
    result("begin var s := 'ab'; Insert('X',s,2); writeln(s); end.", "aXb\n");
    result("begin var s := 'ab'; Insert('X',s,-2147483648); Insert('Y',s,2147483647); writeln(s); end.", "XabY\n");
    result("begin var s := ''; Insert('abc',s,1); Insert('',s,2); writeln(s); end.", "abc\n");
    result("begin var s := 'abc'; Insert(s,s,2); writeln(s); end.", "aabcbc\n");
    result("begin var s := 'abc'; var t := s; Insert('x',s,1); writeln(t,' ',s); end.", "abc xabc\n");
    result("begin var s := 'abcdef'; Delete(s,2,3); writeln(s); end.", "aef\n");
    result("begin var s := 'abc'; Delete(s,0,1); Delete(s,-1,2); Delete(s,4,2); Delete(s,1,-1); Delete(s,1,0); writeln(s); end.", "abc\n");
    result("begin var s := 'abc'; Delete(s,2,2147483647); writeln(s); Delete(s,1,2147483647); writeln(Length(s)); end.", "a\n0\n");
    result("begin var s := ''; Delete(s,1,10); writeln(s=''); end.", "True\n");
    result("begin var s := 'Привет'; Insert('!',s,4); Delete(s,2,3); SetLength(s,2); writeln(s); end.", "Пв\n");
    result("begin var s := '🌍'; SetLength(s,1); writeln(Length(s),' ',s); end.", "1 �\n");
    result("begin var s := '🌍'; Insert('x',s,2); writeln(Length(s),' ',s); end.", "3 �x�\n");
    result("begin var s := '🌍'; Delete(s,1,1); writeln(Length(s),' ',s); end.", "1 �\n");
    result("begin var s := 'outer'; begin var s := 'inner'; SetLength(s,2); writeln(s); end; writeln(s); end.", "in\nouter\n");
    result("begin var s := 'x'; if true then SetLength(s,3) else Delete(s,1,1); writeln('[',s,']'); end.", "[x  ]\n");
    result("begin var s := ''; var i := 0; while i<3 do begin Insert(IntToStr(i),s,2147483647); Inc(i); end; writeln(s); end.", "012\n");
    result("begin var s := 'abc'; SetLength(s,Length(s)+2); Insert(Copy(s,1,1),s,2); Delete(s,Length(s),1); writeln('[',s,']'); end.", "[aabc ]\n");
    result("begin var s := 'x'; SetLength(s,1048576); writeln(Length(s)); Delete(s,1,1048576); writeln(Length(s)); end.", "1048576\n0\n");
    compile_error("begin SetLength('abc',2); end.", "E_TYPE");
    compile_error("begin var s := 'abc'; SetLength(Trim(s),2); end.", "E_TYPE");
    compile_error("begin var n := 1; SetLength(n,2); end.", "E_TYPE");
    compile_error("begin var s := 'abc'; SetLength(s,2.0); end.", "E_TYPE");
    compile_error("begin var s := 'abc'; SetLength(s,2147483648); end.", "E_TYPE");
    compile_error("begin var s := 'abc'; Insert(1,s,2); end.", "E_TYPE");
    compile_error("begin Insert('a','b',1); end.", "E_TYPE");
    compile_error("begin Delete('abc',1,1); end.", "E_TYPE");
    compile_error("begin var s := 'abc'; Delete(s,1,true); end.", "E_TYPE");
    compile_error("begin SetLength(); end.", "E_TYPE");
    compile_error("begin var s := 'abc'; Insert(s,s); end.", "E_TYPE");
    compile_error("begin var SetLength := 3; var s := 'abc'; SetLength(s,2); end.", "E_TYPE");
    compile_error("begin SetLength(missing,2); end.", "E_NAME");
    runtime_error("begin var s := 'abc'; SetLength(s,-1); end.", "E_RANGE");
    runtime_error("begin var s := 'abc'; SetLength(s,1048577); end.", "E_LIMIT");
    runtime_error("begin var s := ''; SetLength(s,1048576); Insert('x',s,1); end.", "E_LIMIT");
    try { (void)run("begin\n  var s := 'abc';\n  SetLength(s,-1);\nend."); require(false, "Expected string length error"); }
    catch (const abc::Error& caught) {
        require(caught.location.line == 3 && caught.russian == "Длина строки не должна быть отрицательной",
            "String mutation failure retains its source location and Russian explanation");
    }
    auto program = abc::compile("begin var s := 'abc'; Insert('x',s,2); end.");
    for (auto& instruction : program.code) if (instruction.op == abc::Op::call_builtin) instruction.type = abc::Type::integer;
    error([&] { abc::verify(program); }, "E_BYTECODE");
}

void library_bytecode() {
    auto program = abc::compile("begin writeln(Length('abc')); end.");
    std::size_t call = 0;
    while (program.code[call].op != abc::Op::call_builtin) ++call;
    const auto valid_call = program.code[call];
    program.code[call].operand = std::numeric_limits<std::size_t>::max();
    error([&] { abc::verify(program); }, "E_BYTECODE");
    program.code[call] = valid_call;
    program.code[call].type = abc::Type::real;
    error([&] { abc::verify(program); }, "E_BYTECODE");
    program.code[call] = valid_call;
    program.constants.front() = std::int32_t{3};
    program.code.front().type = abc::Type::integer;
    error([&] { abc::verify(program); }, "E_BYTECODE");
    program.code = {valid_call, {abc::Op::halt, abc::Type::invalid, 0, {}}};
    error([&] { std::istringstream in; std::ostringstream out; abc::execute(program, in, out); }, "E_BYTECODE");
    result("begin writeln(Pos('" + std::string(10000, 'a') + "b','" + std::string(100000, 'a') + "')); end.", "0\n");
    result("begin writeln(LastPos('" + std::string(10000, 'a') + "b','" + std::string(100000, 'a') + "')); end.", "0\n");
    program = abc::compile("begin writeln(Sign(1.0)); end.");
    program.constants.front() = std::numeric_limits<double>::quiet_NaN();
    error([&] { abc::verify(program); }, "E_BYTECODE");
    const std::u16string invalid(1, char16_t{0xd800});
    try { (void)abc::to_utf8(invalid); require(false, "Strict encoding must reject an unpaired surrogate"); }
    catch (const std::invalid_argument&) { require(true, "Strict encoding still rejects invalid UTF-16"); }
}

void bilingual_diagnostics() {
    try { (void)run("begin\n  var zero := 0;\n  Writeln(10 div zero);\nend."); require(false, "Expected division failure"); }
    catch (const abc::Error& caught) {
        require(caught.code == "E_DIV_ZERO" && caught.location.line == 3, "Runtime failure retains code and source line");
        const auto message = abc::format_diagnostic(caught, "Тест.pas");
        require(message.find("Тест.pas:3:") == 0 && message.find("EN: Division by zero") != std::string::npos
            && message.find("RU: Деление на ноль") != std::string::npos, "Both languages and Cyrillic filename survive formatting");
    }
    try { (void)run("begin var n: integer; readln(n); end.", "2147483648\n"); require(false, "Expected input failure"); }
    catch (const abc::Error& caught) {
        require(caught.russian == "Введённое значение не соответствует типу integer"
            && std::string(caught.what()) == "Input does not fit integer", "Input error retains declared type in both languages");
    }
    abc::ExecutionOptions options;
    options.instruction_limit = 10;
    try { (void)run("begin while true do begin end; end.", "", options); require(false, "Expected limit failure"); }
    catch (const abc::Error& caught) {
        require(caught.russian == "Превышен предел числа выполняемых инструкций", "Instruction limit has a Russian explanation");
    }
}

void verifier() {
    auto program = abc::compile("begin writeln(1 + 2); end.");
    program.code.front().type = abc::Type::real;
    error([&] { abc::verify(program); }, "E_BYTECODE");
    error([&] { std::istringstream in; std::ostringstream out; abc::execute(program, in, out); }, "E_BYTECODE");
    error([] { abc::verify({}); }, "E_BYTECODE");
    program = abc::compile("begin end.");
    program.code[0].op = static_cast<abc::Op>(999);
    error([&] { abc::verify(program); }, "E_BYTECODE");
    program.code = {{abc::Op::jump, abc::Type::invalid, 999, {}}};
    error([&] { abc::verify(program); }, "E_BYTECODE");
    program.code = {{abc::Op::print, abc::Type::integer, 0, {}}, {abc::Op::halt, abc::Type::invalid, 0, {}}};
    error([&] { abc::verify(program); }, "E_BYTECODE");
    program.constants = {true, std::int32_t{1}};
    program.code = {{abc::Op::push, abc::Type::boolean, 0, {}}, {abc::Op::jump_false, abc::Type::invalid, 3, {}},
        {abc::Op::push, abc::Type::integer, 1, {}}, {abc::Op::halt, abc::Type::invalid, 0, {}}};
    error([&] { abc::verify(program); }, "E_BYTECODE");
    // Invalid unreachable instructions must still be rejected.
    program.code = {{abc::Op::halt, abc::Type::invalid, 0, {}}, {abc::Op::load, abc::Type::integer, 999, {}}};
    error([&] { abc::verify(program); }, "E_BYTECODE");
    program.code = {{abc::Op::newline, abc::Type::invalid, 0, {}}};
    error([&] { abc::verify(program); }, "E_BYTECODE");
    program.constants = {std::int64_t{42}};
    program.code = {{abc::Op::push, abc::Type::int64, 0, {}}, {abc::Op::convert, abc::Type::int64, 0, {}},
        {abc::Op::print, abc::Type::int64, 0, {}}, {abc::Op::halt, abc::Type::invalid, 0, {}}};
    std::istringstream in;
    std::ostringstream out;
    abc::execute(program, in, out);
    require(out.str() == "42", "Verifier and runtime agree on identity conversion");
}
} // namespace

int main() {
    language();
    arithmetic();
    verifier();
    bilingual_diagnostics();
    standard_library();
    library_bytecode();
    text_library();
    mutable_strings();
    std::cout << (checks - failures) << '/' << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}

#include "abc/core.hpp"
#include "../src/linked.hpp"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <limits>
#include <sstream>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#ifdef __linux__
#include <sys/stat.h>
#endif

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

void nullable_strings() {
    result("begin var s:string:=nil; writeln('[',s,'] ',s=nil,' ',s='',' ',s='nil'); end.","[nil] True False False\n");
    result("begin var s:string:=nil; writeln(s<'',' ',s<='',' ',s>'',' ',s>='',' ',''>s,' ',''<>s); end.","True True False False True True\n");
    result("begin var s:string:=nil; writeln('[',s+'','] [','x'+s,'] [',nil+'x','] [',Concat(nil,nil),']'); end.","[] [x] [x] []\n");
    result("begin var s:string:=nil; writeln(Length(s),' [',Copy(s,1,2),'] [',LeftStr(s,2),'] [',RightStr(s,2),'] ',CompareStr(s,''),' ',CompareStr('',nil)); end.","0 [] [] [] 0 0\n");
    result("begin writeln(Pos(nil,nil),' ',LastPos(nil,nil),' ',Pos('',nil,0),' ',LastPos(nil,'abc',-1)); end.","0 0 0 0\n");
    result("begin writeln(StrToInt64(nil),' ',StrToReal(nil),' ',StrToFloat(nil,'.'),' ',FileExists(nil)); end.","0 0 0 False\n");
    result("begin var s:string:=nil; Delete(s,1,2); writeln(s=nil); Insert(nil,s,1); writeln(s=nil,' [',s,']'); s:=nil; SetLength(s,2); writeln('[',s,']'); end.","True\nFalse []\n[  ]\n");
    result("type Rec=record s:string; end; function F:string; begin end; begin var s:string; var r:Rec; var a:=new string[1]; writeln(s=nil,' ',r.s=nil,' ',a[0]=nil,' ',F=nil); end.","False False False False\n");
    result("begin var a:=new string[1]; SetLength(a,3); var b:array of string; SetLength(b,1); writeln(a[1]=nil,' ',b[0]=nil); end.","False False\n");
    result("uses Collections; begin var m:=new AssocArray<string,string>; writeln(m['absent'],' ',m['absent']=nil,' [',m.ToString,']'); m[nil]:='null'; m['']:='empty'; writeln(m[nil],' ',m['']); end.","nil True []\nnull empty\n");
    result("uses Collections; type Rec=record s:string; end; begin var m:=new AssocArray<string,Rec>; var r:=m['x']; writeln(r.s=nil); var d:=new DynArray<Rec>(1); r:=d[0]; writeln(r.s=nil); end.","True\nTrue\n");
    result("uses Collections; begin var d:=new DynArray<string>(1); writeln(d[0]=nil); d[0]:='kept'; d.Resize(0); d.Resize(1); writeln(d[0]); d.Resize(8); writeln(d[1]=nil); end.","True\nkept\nTrue\n");
    result("uses Collections; begin var q:=new Queue<string>; q.Enqueue(nil); writeln('[',q.ToString,'] ',q.Top=nil); var s:=new SimpleSet<string>; s.Add(nil); s.Add(''); s.Add(nil); writeln(s.Contains(nil),' ',s.Contains('')); s.Remove(nil); writeln(s.Contains(nil),' ',s.Contains('')); end.","[ ] True\nTrue True\nFalse True\n");
    result("begin var a:=Arr(nil,'b','','a'); Sort(a); writeln('[',a[0],'] [',a[1],'] ',a[2],' ',a[3]); SortDescending(a); writeln(a[0],' ',a[1],' [',a[2],'] [',a[3],']'); end.","[nil] [] a b\nb a [] [nil]\n");
    result("begin var l:=new List<string>; l.Add(nil); writeln(l.Contains(nil),' ',l.Contains('')); l.Add(''); writeln(l.Contains('')); end.","True False\nTrue\n");
    result("function F(s:string):string; begin Result:=s; end; procedure P(var s:string); begin s:=nil; end; begin var s:='x'; P(s); writeln(F(s)=nil); var a:=Arr('x'); P(a[0]); writeln(a[0]=nil); end.","True\nTrue\n");
    for (auto source : {"begin writeln(Trim(nil)); end.", "begin writeln(TrimLeft(nil)); end.",
         "begin writeln(TrimRight(nil)); end.", "begin writeln(ReverseString(nil)); end.",
         "begin writeln(ReverseString(nil,1,0)); end.", "begin writeln(StrToInt(nil)); end.",
         "begin writeln(Pos('x',nil)); end.", "begin writeln(LastPos('x',nil,1)); end.",
         "uses Collections; begin var s:=new Stack<string>; s.Push(nil); writeln(s.ToString); end.",
         "uses Collections; begin var d:=new DynArray<string>(1); writeln(d.ToString); end.",
         "uses Collections; begin var s:=new SimpleSet<string>; s.Add(nil); s.Print; end.",
         "uses Collections; begin var m:=new AssocArray<string,string>; m['x']:=nil; m.Println; end.",
         "uses Collections; begin var l:=new LinkedList<string>; l.AddLast(nil); writeln(l.ToString); end."})
        runtime_error(source,"E_NIL");
    runtime_error("begin Writeln(ReadAllText(nil)); end.","E_IO");
    runtime_error("begin WriteAllText(nil,'x'); end.","E_IO");
    runtime_error("begin Writeln(StrToReal(nil,'')); end.","E_RANGE");
    compile_error("begin var s:string:=nil; var n:integer:=s; end.","E_TYPE");
    compile_error("begin var n:=nil; end.","E_TYPE");
    compile_error("begin var a:=Arr(nil,nil); end.","E_TYPE");
    compile_error("begin var a:=Arr(nil,1); end.","E_TYPE");
    auto program=abc::compile("begin var s:string:=nil; writeln(s); end.");
    require(abc::value_type(abc::NilString{})==abc::Type::string,"Null string retains its static string type");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::push && instruction.type==abc::Type::string) { instruction.type=abc::Type::integer; break; }
    error([&] { abc::verify(program); },"E_BYTECODE");
    try { (void)run("begin\n var s:string:=nil;\n Writeln(Trim(s));\nend."); require(false,"Expected null string failure"); }
    catch (const abc::Error& caught) {
        require(caught.code=="E_NIL" && caught.location.line==3 && !caught.russian.empty(),"Null string error retains source line and both languages");
    }
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
    for (auto& instruction : program.code) if (instruction.op == abc::Op::mutate_string) instruction.type = abc::Type::integer;
    error([&] { abc::verify(program); }, "E_BYTECODE");
}

void language_foundations() {
    result("begin var a: array of integer; SetLength(a,3); a[0]:=10; a[2]:=20; writeln(Length(a),' ',a[1],' ',a[0]+a[2]); end.", "3 0 30\n");
    result("begin var a := Arr(1,2,3); var b := a; b[1]:=9; writeln(a[1]); SetLength(b,4); b[1]:=7; writeln(a[1],' ',b[3]); end.", "9\n9 0\n");
    result("begin var a := new real[2]; a[0]:=3; writeln(a.Length,' ',a[0]); end.", "2 3\n");
    result("begin var a := Arr(1,2.5); writeln(a[0]+a[1]); end.", "3.5\n");
    result("type Point = record x,y: integer; end; begin var p: Point; p.x:=4; p.y:=5; var q:=p; q.x:=9; writeln(p.x,' ',q.x,' ',p.y); end.", "4 9 5\n");
    result("type Point = record x: integer; end; begin var a: array of Point; SetLength(a,2); a[0].x:=7; writeln(a[0].x,' ',a[1].x); end.", "7 0\n");
    result("type Inner=record x: integer; end; Outer=record p: Inner; end; begin var a: Outer; a.p.x:=6; var b:=a; b.p.x:=8; writeln(a.p.x,' ',b.p.x); end.", "6 8\n");
    result("function Twice(x: integer): integer; begin Result:=x*2; end; begin writeln(Twice(21)); end.", "42\n");
    result("function F(n: integer): int64; begin if n<=1 then Exit(1); Result:=n*F(n-1); end; begin writeln(F(10)); end.", "3628800\n");
    result("function F(n: integer): integer; begin if n=0 then F:=1 else F:=n*F(n-1); end; begin writeln(F(5)); end.", "120\n");
    result("procedure Add(var n: integer; step: integer); begin n:=n+step; end; begin var n:=10; Add(n,5); writeln(n); end.", "15\n");
    result("procedure Bump(var x,y: integer); begin Inc(x); Inc(y); end; begin var n:=1; Bump(n,n); writeln(n); end.", "3\n");
    result("var g: integer; procedure Bump; begin Inc(g); end; begin Bump; Bump(); writeln(g); end.", "2\n");
    result("function Message: string; begin Result:='Привет'; end; begin writeln(Message); end.", "Привет\n");
    result("procedure Ignore; begin Exit; writeln('bad'); end; begin Ignore; writeln('ok'); end.", "ok\n");
    result("type P=record x: integer; end; procedure Change(p: P); begin p.x:=8; end; begin var p:P; p.x:=3; Change(p); writeln(p.x); end.", "3\n");
    result("type P=record x: integer; end; procedure Change(var p: P); begin p.x:=8; end; begin var p:P; Change(p); writeln(p.x); end.", "8\n");
    result("procedure Change(a: array of integer); begin a[0]:=8; SetLength(a,3); end; begin var a:=Arr(1); Change(a); writeln(a[0],' ',Length(a)); end.", "8 1\n");
    result("procedure Change(var a: array of integer); begin SetLength(a,3); end; begin var a:=Arr(1); Change(a); writeln(Length(a)); end.", "3\n");
    result("procedure Change(var n: integer); begin n:=9; end; begin var a:=Arr(1,2); Change(a[1]); writeln(a[1]); end.", "9\n");
    result("begin var xs := new List<integer>; xs.Add(3); xs.Add(5); xs[0]:=2; writeln(xs.Count,' ',xs.Contains(5)); xs.RemoveAt(0); writeln(xs[0]); xs.Clear(); writeln(xs.Count); end.", "2 True\n5\n0\n");
    compile_error("function F(x: integer): integer; begin Result:='bad'; end; begin end.", "E_TYPE");
    compile_error("procedure P(var x: integer); begin end; begin P(1); end.", "E_TYPE");
    compile_error("procedure P(var x: int64); begin end; begin var n:=1; P(n); end.", "E_TYPE");
    compile_error("begin var a:=Arr(1,2); a[0]:='bad'; end.", "E_TYPE");
    compile_error("type P=record x: integer; end; begin var p:P; p.missing:=1; end.", "E_NAME");
    compile_error("type P=record x: integer; end; Q=record x: integer; end; begin var p:P; var q:Q; p:=q; end.", "E_TYPE");
    runtime_error("begin var a:=Arr(1); writeln(a[-1]); end.", "E_BOUNDS");
    runtime_error("begin var a:=new integer[-1]; end.", "E_RANGE");
    runtime_error("procedure Loop; begin Loop; end; begin Loop; end.", "E_LIMIT");
}

void aggregate_semantics() {
    result("begin writeln('nil'); var a:=Arr('nil','value'); writeln(a[0]); end.", "nil\nnil\n");
    result("begin var a: array of integer; writeln(a=nil,' ',Length(a)); SetLength(a,0); writeln(a=nil,' ',High(a)); end.", "True 0\nFalse -1\n");
    result("begin var a:=Arr(3,1,2); var b:=Copy(a); Sort(a); SortDescending(b); writeln(a[0],a[1],a[2],' ',b[0],b[1],b[2]); FillByValue(a,9); writeln(a[2]); end.", "123 321\n9\n");
    result("type P=record x: integer; end; begin var a:=new P[2]; a[0].x:=3; var b:=Copy(a); b[0].x:=8; writeln(a[0].x,' ',b[0].x); end.", "3 8\n");
    result("type P=record x: integer; end; procedure Replace(var p:P; var x:integer); var q:P; begin q.x:=7; p:=q; x:=9; end; begin var p:P; Replace(p,p.x); writeln(p.x); end.", "9\n");
    result("type P=record x: integer; end; begin var p:P; p.x:=3; var xs:=new List<P>; xs.Add(p); p.x:=8; var q:=xs[0]; writeln(q.x,' ',xs.Contains(q),' ',xs.Contains(p)); end.", "3 True False\n");
    result("var calls:integer; function Index:integer; begin Inc(calls); Result:=0; end; begin var a:=Arr('abc'); SetLength(a[Index()],5); writeln(calls,' [',a[0],']'); end.", "1 [abc  ]\n");
    result("begin var a:=Arr(1,2); Inc(a[0],2); Dec(a[1]); Readln(a[1]); writeln(a[0],' ',a[1]); end.", "3 7\n", "7\n");
    result("function Step(var x:integer):integer; begin x:=10; Result:=2; end; begin var x:=1; Inc(x,Step(x)); writeln(x); end.", "12\n");
    result("var text:string; function Position:integer; begin text:='XYZ'; Result:=2; end; begin text:='abc'; Insert('!',text,Position()); writeln(text); end.", "X!YZ\n");
    result("begin var a:array of array of integer; SetLength(a,2); SetLength(a[0],3); a[0][1]:=7; writeln(a[0][1],' ',Length(a[1])); end.", "7 0\n");
    result("type R=record a,b:record x:integer; end; end; begin var r:R; r.a.x:=3; r.b:=r.a; writeln(r.b.x); end.", "3\n");
    runtime_error("begin var a: array of integer; writeln(a[0]); end.", "E_NIL");
    runtime_error("begin var xs:List<integer>; writeln(xs.Count); end.", "E_NIL");
    runtime_error("begin var a:=Arr(1); writeln(a[1]); end.", "E_BOUNDS");
    runtime_error("begin var a:=new integer[1048577]; end.", "E_LIMIT");
    compile_error("begin var a:=nil; end.", "E_TYPE");
    compile_error("begin writeln(nil); end.", "E_TYPE");
    compile_error("procedure P; begin end; begin var x:=P(); end.", "E_TYPE");
    compile_error("procedure P; begin Exit(1); end; begin end.", "E_TYPE");
    auto program=abc::compile("procedure P(var x:integer); begin x:=2; end; begin var x:=1; P(x); end.");
    program.routines[0].parameters[0].type=abc::Type::string;
    error([&] { abc::verify(program); }, "E_BYTECODE");
    program=abc::compile("type R=record x:integer; end; begin var r:R; end.");
    program.types[0].fields[0].type=static_cast<abc::Type>(64);
    error([&] { abc::verify(program); }, "E_BYTECODE");
}

void file_library() {
    namespace fs=std::filesystem;
    const auto folder=fs::temp_directory_path()/fs::path(std::u8string(u8"ABC native файлы ")+std::u8string{});
    const auto directory=fs::path(folder.native()+fs::path(std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())).native());
    if (!fs::create_directory(directory)) throw std::runtime_error("Cannot create unique file-test directory");
    struct Cleanup {
        fs::path path;
        fs::path root=fs::temp_directory_path();
        ~Cleanup() {
            std::error_code error;
            const auto parent=fs::weakly_canonical(root,error);
            if (error) return;
            const auto target=fs::weakly_canonical(path,error);
            if (!error && target.parent_path()==parent && target.filename().generic_u8string().starts_with(u8"ABC native "))
                fs::remove_all(target,error);
        }
    } cleanup{directory};
    auto quote=[](const fs::path& path) {
        const auto u8=path.generic_u8string(); std::string result="'";
        for (auto c:u8) { result+=static_cast<char>(c); if (c==u8'\'') result+='\''; }
        return result+"'";
    };
    const auto path=directory/fs::path(u8"Текст.txt"), lines=directory/"lines.txt";
    const auto file=quote(path), linefile=quote(lines);
    result("begin writeln(FileExists("+file+")); WriteAllText("+file+",'Привет 🌍'); writeln(FileExists("+file+"),' ',ReadAllText("+file+")); end.", "False\nTrue Привет 🌍\n");
    result("begin var s:string:=nil; WriteAllText("+file+",s); writeln(Length(ReadAllText("+file+"))); end.","0\n");
    result("begin var a:=new string[2]; a[0]:=nil; a[1]:='tail'; WriteAllLines("+linefile+",a); var b:=ReadAllLines("+linefile+"); writeln(Length(b),' [',b[0],'] ',b[1],' ',b[0]=nil); end.","2 [] tail False\n");
    result("begin WriteAllLines("+linefile+",Arr('alpha','beta')); var lines:=ReadAllLines("+linefile+"); writeln(Length(lines),' ',lines[0],' ',lines[1]); end.", "2 alpha beta\n");
    { std::ofstream out(path,std::ios::binary); out << "\xef\xbb\xbf" "a\r\nb\rc\n"; }
    result("begin var lines:=ReadAllLines("+file+"); writeln(Length(lines),' ',lines[0],lines[1],lines[2]); end.", "3 abc\n");
    { std::ofstream out(path,std::ios::binary); out << "\xff"; }
    runtime_error("begin writeln(ReadAllText("+file+")); end.", "E_ENCODING");
    runtime_error("begin writeln(ReadAllText("+quote(directory/"missing.txt")+")); end.", "E_IO");
    runtime_error("begin WriteAllText("+quote(directory/"missing"/"file.txt")+",'x'); end.", "E_IO");
    { std::ofstream out(path,std::ios::binary); out << std::string(1'048'577,'x'); }
    runtime_error("begin writeln(ReadAllText("+file+")); end.", "E_LIMIT");
    const auto home=fs::current_path();
    const auto base=quote(directory), child=quote(directory/fs::path(u8"Вложенная папка")), deep=quote(directory/fs::path(u8"Вложенная папка")/"deep");
    result("begin writeln(CreateDir("+deep+"),' ',CreateDir("+child+")); writeln(SetCurrentDir("+child+"),' ',GetDir=GetCurrentDir()); WriteAllText('local.txt','local'); writeln(FileExists('local.txt'),' ',ReadAllText('local.txt')); WriteAllLines('lines.txt',Arr('alpha','beta')); var lines:=ReadAllLines('lines.txt'); writeln(Length(lines),' ',lines[1]); end.","True True\nTrue True\nTrue local\n2 beta\n");
    require(fs::current_path()==home,"Program current directory does not change the host current directory");
    require(fs::exists(directory/fs::path(u8"Вложенная папка")/"local.txt"),"Relative writes use the program directory");
    result("begin ChDir("+child+"); writeln(SetCurrentDir('missing'),' ',FileExists('local.txt')); writeln(RemoveDir('.'),' ',DeleteFile('.'),' ',FileExists('.')); RmDir('deep'); writeln(DeleteFile('local.txt'),' ',DeleteFile('local.txt')); end.","False True\nFalse False False\nTrue False\n");
    result("begin ChDir("+base+"); writeln(RemoveDir('Вложенная папка'),' ',CreateDir('folder-as-file.txt'),' ',DeleteFile('folder-as-file.txt')); end.","False True False\n");
    result("begin ChDir("+base+"); RmDir('folder-as-file.txt'); ChDir('Вложенная папка'); writeln(DeleteFile('lines.txt')); ChDir('..'); writeln(RemoveDir('Вложенная папка'),' ',RemoveDir('Вложенная папка')); end.","True\nTrue False\n");
    require(fs::current_path()==home,"Nested ChDir operations remain local to one execution");
    result("begin writeln(CreateDir("+file+"),' ',SetCurrentDir("+file+"),' ',RemoveDir("+file+")); end.","False False False\n");
    result("begin writeln(CreateDir(nil),' ',RemoveDir(nil),' ',DeleteFile(nil),' ',SetCurrentDir(nil)); writeln(CreateDir(''),' ',RemoveDir(''),' ',SetCurrentDir('')); end.","False False False False\nFalse False False\n");
    runtime_error("begin MkDir(nil); end.","E_IO");
    runtime_error("begin ChDir("+quote(directory/"missing")+"); end.","E_IO");
    runtime_error("begin RmDir("+file+"); end.","E_IO");
    runtime_error("begin MkDir("+file+"); end.","E_IO");
    runtime_error("begin ChDir("+base+"); Writeln(1 div 0); end.","E_DIV_ZERO");
    require(fs::current_path()==home,"Runtime failure cannot leak a changed directory into the host");
    const auto selected=quote(directory/"selected-empty");
#ifdef _WIN32
    result("begin MkDir("+selected+"); ChDir("+selected+"); writeln(RemoveDir('.')); end.","False\n");
    result("begin writeln(RemoveDir("+selected+")); end.","True\n");
#else
    runtime_error("begin MkDir("+selected+"); ChDir("+selected+"); RmDir(GetDir); Writeln(GetDir); end.","E_IO");
#endif
    result("begin writeln(GetDir=GetCurrentDir()); end.","True\n");
    result("begin var p:string; Readln(p); writeln(CreateDir(p),' ',RemoveDir(p),' ',DeleteFile(p),' ',SetCurrentDir(p)); end.","False False False False\n",std::string("bad\0path\n",9));
    try { (void)run("begin\n RmDir("+quote(directory/"missing")+");\nend."); require(false,"Expected directory failure"); }
    catch (const abc::Error& caught) {
        require(caught.code=="E_IO" && caught.location.line==2 && std::string(caught.what())=="Directory operation failed"
            && caught.russian=="Не удалось выполнить операцию с каталогом","Directory error has source location and both languages");
    }
    compile_error("begin writeln(GetDir(1)); end.","E_TYPE");
    compile_error("begin MkDir(1); end.","E_TYPE");
    compile_error("begin var n:integer:=GetCurrentDir; end.","E_TYPE");
    compile_error("begin var b:boolean:=ChDir('x'); end.","E_TYPE");
    compile_error("begin var s:=ChangeFileNameExtension('x',1); end.","E_TYPE");
    compile_error("begin MkDir("+quote(directory/"must-not-exist")+"); if false then begin var n:integer:='bad'; end; end.","E_TYPE");
    require(!fs::exists(directory/"must-not-exist"),"All types are checked before directory effects");
    compile_error("begin WriteAllLines('file',Arr(1,2)); end.", "E_TYPE");
    compile_error("begin var x:=WriteAllText('file','x'); end.", "E_TYPE");

    const auto moves=quote(directory/"moves");
    result("begin MkDir("+moves+"); ChDir("+moves+"); WriteAllText('from.txt','source'); writeln(RenameFile('from.txt','renamed.txt'),' ',FileExists('from.txt'),' ',ReadAllText('renamed.txt')); writeln(RenameFile('renamed.txt','./renamed.txt')); end.","True False source\nTrue\n");
    result("begin ChDir("+moves+"); WriteAllText('occupied.txt','target'); writeln(RenameFile('renamed.txt','occupied.txt'),' ',ReadAllText('renamed.txt'),' ',ReadAllText('occupied.txt')); MkDir('occupied-dir'); writeln(RenameFile('renamed.txt','occupied-dir'),' ',ReadAllText('renamed.txt')); end.","False source target\nFalse source\n");
    result("begin ChDir("+moves+"); writeln(RenameFile('missing.txt','new.txt'),' ',RenameFile('missing.txt','missing.txt'),' ',RenameFile('renamed.txt','missing/target.txt')); writeln(FileExists('renamed.txt')); end.","False False False\nTrue\n");
    result("begin ChDir("+moves+"); MkDir('from-dir/child'); WriteAllText('from-dir/child/note.txt','inside'); writeln(RenameFile('from-dir','bad-dir')); writeln(RenameDirectory('from-dir','renamed-dir'),' ',ReadAllText('renamed-dir/child/note.txt')); writeln(RenameDirectory('renamed-dir','renamed-dir'),' ',RenameDirectory('renamed-dir','occupied-dir'),' ',RenameDirectory('renamed-dir','occupied.txt')); writeln(ReadAllText('renamed-dir/child/note.txt'),' ',ReadAllText('occupied.txt')); end.","False\nTrue inside\nFalse False False\ninside target\n");
    result("begin ChDir("+moves+"); writeln(RenameDirectory('renamed-dir','renamed-dir/child/subdir'),' ',RenameDirectory('missing-dir','new-dir'),' ',RenameDirectory('renamed-dir','missing-parent/new-dir')); writeln(RenameDirectory('renamed.txt','via-directory.txt'),' ',ReadAllText('via-directory.txt')); end.","False False False\nTrue source\n");
    result("begin ChDir("+moves+"); WriteAllText('имя''файла.txt','данные 🌍'); writeln(RenameFile('имя''файла.txt','новое имя.txt'),' ',ReadAllText('новое имя.txt')); MkDir('старое имя'); writeln(RenameDirectory('старое имя','новое имя')); end.","True данные 🌍\nTrue\n");
    result("begin ChDir("+moves+"); WriteAllText('case.txt','case'); MkDir('case-dir'); writeln(RenameFile('case.txt','CASE.txt'),' ',RenameDirectory('case-dir','CASE-DIR')); end.","True True\n");
    bool file_case=false, directory_case=false;
    for (const auto& entry:fs::directory_iterator(directory/"moves")) {
        if (entry.path().filename()=="CASE.txt") file_case=true;
        if (entry.path().filename()=="CASE-DIR") directory_case=true;
    }
    require(file_case && directory_case,"Case-only rename changes the stored file and directory names");
    result("begin writeln(RenameFile(nil,'x'),' ',RenameFile('x',nil),' ',RenameDirectory(nil,'x'),' ',RenameDirectory('x',nil)); writeln(RenameFile('','x'),' ',RenameFile('x',''),' ',RenameDirectory('','x'),' ',RenameDirectory('x','')); end.","False False False False\nFalse False False False\n");
    result("begin ChDir("+moves+"); var p:string; Readln(p); writeln(RenameFile('via-directory.txt',p),' ',RenameFile(p,'x'),' ',RenameDirectory('renamed-dir',p),' ',RenameDirectory(p,'x')); writeln(FileExists('via-directory.txt')); end.","False False False False\nTrue\n",std::string("bad\0path\n",9));
    const auto binary=directory/"moves"/"binary.bin", renamed_binary=directory/"moves"/"renamed.bin";
    const std::string binary_data=std::string("\0\xff\x80",3)+std::string(1'048'577,'x');
    { std::ofstream out(binary,std::ios::binary); out.write(binary_data.data(),static_cast<std::streamsize>(binary_data.size())); }
    result("begin writeln(RenameFile("+quote(binary)+","+quote(renamed_binary)+")); end.","True\n");
    { std::ifstream in(renamed_binary,std::ios::binary); const std::string moved((std::istreambuf_iterator<char>(in)),{});
      require(!fs::exists(binary) && moved==binary_data,"Rename preserves arbitrary binary bytes without the text IO limit"); }
    std::error_code link_error;
    { std::ofstream out(directory/"moves"/"hard-source.txt"); out << "hard"; }
    fs::create_hard_link(directory/"moves"/"hard-source.txt",directory/"moves"/"hard-link.txt",link_error);
    if (!link_error) {
#ifdef _WIN32
        result("begin ChDir("+moves+"); writeln(RenameFile('hard-source.txt','hard-link.txt')); end.","True\n");
        require(!fs::exists(directory/"moves"/"hard-source.txt") && fs::exists(directory/"moves"/"hard-link.txt"),"Windows removes the source name when moving onto the same file's hard link");
#else
        result("begin ChDir("+moves+"); writeln(RenameFile('hard-source.txt','hard-link.txt')); end.","True\n");
        require(fs::exists(directory/"moves"/"hard-source.txt") && fs::exists(directory/"moves"/"hard-link.txt"),"Unix same-inode rename leaves both hard links present");
#endif
    }
    fs::create_symlink("via-directory.txt",directory/"moves"/"link-a",link_error);
    if (!link_error) {
        fs::create_symlink("via-directory.txt",directory/"moves"/"link-b",link_error);
        if (!link_error) result("begin ChDir("+moves+"); writeln(RenameFile('link-a','link-b'),' ',RenameFile('link-a','moved-link'),' ',ReadAllText('moved-link')); end.","False True source\n");
        fs::create_symlink("absent-target",directory/"moves"/"broken-link",link_error);
        if (!link_error) result("begin ChDir("+moves+"); writeln(RenameFile('via-directory.txt','broken-link')); end.","False\n");
    }
    result("begin MkDir("+moves+"+'/selected/child'); ChDir("+moves+"+'/selected/child'); WriteAllText('inside.txt','selected'); end.","");
#ifdef _WIN32
    result("begin ChDir("+moves+"+'/selected/child'); writeln(RenameDirectory('.','../renamed-child'),' ',RenameDirectory('..','../../renamed-parent'),' ',FileExists('inside.txt')); end.","False False True\n");
    const auto locked_path=directory/"moves"/"via-directory.txt";
    const auto locked=::CreateFileW(locked_path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    require(locked!=INVALID_HANDLE_VALUE,"Open file without delete sharing for rename test");
    if (locked!=INVALID_HANDLE_VALUE) {
        result("begin ChDir("+moves+"); writeln(RenameFile('via-directory.txt','locked-destination'),' ',RenameDirectory('via-directory.txt','locked-destination')); end.","False False\n");
        ::CloseHandle(locked);
    }
#else
    result("begin ChDir("+moves+"+'/selected/child'); writeln(RenameDirectory('.','../renamed-child'),' ',FileExists('inside.txt')); writeln(RenameDirectory('..','../../renamed-parent'),' ',FileExists('inside.txt'),' ',SetCurrentDir(GetDir)); end.","True True\nTrue True True\n");
#endif
    require(fs::current_path()==home,"Renaming the program directory does not change the host cwd");
#ifdef __linux__
    // /dev/shm normally provides a second, writable filesystem in Linux CI.
    struct stat local_volume{}, shared_volume{};
    if (::stat(directory.c_str(),&local_volume)==0 && ::stat("/dev/shm",&shared_volume)==0
        && local_volume.st_dev!=shared_volume.st_dev) {
        const auto shared=fs::path("/dev/shm")/directory.filename();
        std::error_code shared_error;
        if (fs::create_directory(shared,shared_error) && !shared_error) {
            Cleanup cleanup_shared{shared,fs::path("/dev/shm")};
            const auto cross_source=directory/"cross-device.bin", cross_target=shared/"cross-device.bin";
            { std::ofstream out(cross_source,std::ios::binary); out.write(binary_data.data(),static_cast<std::streamsize>(binary_data.size())); }
            result("begin writeln(RenameFile("+quote(cross_source)+","+quote(cross_target)+")); end.","True\n");
            { std::ifstream in(cross_target,std::ios::binary); const std::string copied((std::istreambuf_iterator<char>(in)),{});
              require(!fs::exists(cross_source) && copied==binary_data,"Cross-filesystem rename copies binary bytes then removes the source"); }
            fs::create_directory(directory/"cross-directory");
            result("begin writeln(RenameDirectory("+quote(directory/"cross-directory")+","+quote(shared/"cross-directory")+")); end.","False\n");
            require(fs::exists(directory/"cross-directory") && !fs::exists(shared/"cross-directory"),"Cross-filesystem directory rename does not copy the tree");
            result("begin WriteAllText("+quote(cross_source)+",'source'); WriteAllText("+quote(cross_target)+",'target'); writeln(RenameFile("+quote(cross_source)+","+quote(cross_target)+"),' ',ReadAllText("+quote(cross_source)+"),' ',ReadAllText("+quote(cross_target)+")); end.","False source target\n");
        }
    }
#endif
    error([&] { (void)run("begin RenameFile("+quote(directory/"moves"/"via-directory.txt")+","+quote(directory/"moves"/"must-not-move")+"); var n:integer:='bad'; end."); },"E_TYPE");
    require(fs::exists(directory/"moves"/"via-directory.txt") && !fs::exists(directory/"moves"/"must-not-move"),"Type errors are rejected before rename effects");
    compile_error("begin Writeln(RenameFile('x',1)); end.","E_TYPE");
    compile_error("begin Writeln(RenameDirectory(1,'x')); end.","E_TYPE");
    compile_error("begin Writeln(RenameFile('x')); end.","E_TYPE");
    compile_error("begin var n:integer:=RenameDirectory('x','y'); end.","E_TYPE");
}

void file_extensions() {
    result("begin writeln(ChangeFileNameExtension('report.old.txt','dat'),' ',ChangeFileNameExtension('report','txt')); end.","report.old.dat report.txt\n");
    result("begin writeln('[',ChangeFileNameExtension('.hidden','.txt'),'] [',ChangeFileNameExtension('a.b.c',''),'] [',ChangeFileNameExtension('a.b.c',nil),']'); end.","[.txt] [a.b.] [a.b]\n");
    result("begin writeln('[',ChangeFileNameExtension('a/','.txt'),'] [',ChangeFileNameExtension('','.txt'),'] [',ChangeFileNameExtension(nil,'.txt'),']'); end.","[a/.txt] [] [nil]\n");
    result("begin writeln(ChangeFileNameExtension('dir.ext/file','.txt'),' ',ChangeFileNameExtension('dir.ext/file',nil)); end.","dir.ext/file.txt dir.ext/file\n");
    result("begin writeln(ChangeFileNameExtension('Каталог/Документ.старое','новое'),' ',ChangeFileNameExtension('nil','.txt')); end.","Каталог/Документ.новое nil.txt\n");
    result("begin writeln(ChangeFileNameExtension(nil,nil)=nil,' ',ChangeFileNameExtension('','')=''); end.","True True\n");
    runtime_error("begin var s:='x'; var i:=0; while i<20 do begin s:=s+s; Inc(i); end; Writeln(ChangeFileNameExtension(s,'.txt')); end.","E_LIMIT");
    auto program=abc::compile("begin Writeln(GetDir); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::call_builtin) { instruction.type=abc::Type::integer; break; }
    error([&] { abc::verify(program); },"E_BYTECODE");
}

void endpoint_collections() {
    result("uses Collections; begin var s:=new Stack<integer>; writeln(s.IsEmpty); s.Push(1); s.Push(2); writeln(s.Top,' ',s.Top(),' ',s.Pop(),' ',s.Pop,' ',s.IsEmpty()); s.Push(3); writeln(s.Pop); end.", "True\n2 2 2 1 True\n3\n");
    result("uses Collections; begin var q:=new Queue<string>(); writeln(q.IsEmpty()); q.Enqueue('first'); q.Enqueue('second'); writeln(q.Top,' ',q.Top(),' ',q.Dequeue,' ',q.Dequeue(),' ',q.IsEmpty); q.Enqueue('again'); writeln(q.Top); end.", "True\nfirst first first second True\nagain\n");
    result("uses Collections; type S=Stack<int64>; begin var s:=new S; s.Push(2147483648); s.Push(1); writeln(s.Pop,' ',s.Pop); end.", "1 2147483648\n");
    result("uses Collections; begin var s:=new Stack<integer>; var alias:=s; alias.Push(7); writeln(s=alias,' ',s.Pop); end.", "True 7\n");
    result("uses Collections; type P=record x:integer; end; begin var p:P; p.x:=3; var s:=new Stack<P>; s.Push(p); p.x:=9; var top:=s.Top; top.x:=8; var removed:=s.Pop; writeln(removed.x,' ',p.x,' ',s.IsEmpty); end.", "3 9 True\n");
    result("uses Collections; begin var a:=Arr(1); var q:=new Queue<array of integer>; q.Enqueue(a); a[0]:=7; var b:=q.Dequeue; writeln(b[0]); end.", "7\n");
    result("uses Collections; procedure Add(s:Stack<integer>); begin s.Push(4); end; procedure Replace(var s:Stack<integer>); begin s:=new Stack<integer>; s.Push(9); end; begin var s:=new Stack<integer>; Add(s); var alias:=s; Replace(s); writeln(s.Pop,' ',alias.Pop); end.", "9 4\n");
    result("uses Collections; begin var s:Stack<integer>; var q:Queue<string>; writeln(s=nil,' ',q=nil); s:=new Stack<integer>; q:=new Queue<string>; writeln(s=nil,' ',q=nil); end.", "True True\nFalse False\n");
    result("uses Collections; begin var s:=new Stack<integer>; var q:=new Queue<Stack<integer>>; s.Push(8); q.Enqueue(s); var popped:=q.Dequeue; writeln(popped.Pop,' ',s.IsEmpty); end.", "8 True\n");
    result("uses Collections; begin var s:=new Stack<integer>; writeln('[',s.ToString,']'); s.Push(1); s.Push(2); writeln('[',s.ToString(),']'); s.Print; s.Println(); var q:=new Queue<integer>; q.Enqueue(1); q.Enqueue(2); writeln('[',q.ToString,']'); q.Print; write('|'); q.Println; end.", "[]\n[2 1 ]\n2 1 \n2 1 \n[1 2 ]\n1 2 |1 2 \n");
    result("uses Collections; begin var q:=new Queue<string>; q.Enqueue('Привет 🌍'); q.Enqueue('nil'); writeln(q.ToString); var s:=new Stack<boolean>; s.Push(true); s.Push(false); writeln(s.ToString); end.", "Привет 🌍 nil \nFalse True \n");
    result("uses Collections; begin var q:=new Queue<integer>; var i:=0; var sum:=0; while i<50000 do begin q.Enqueue(i); sum:=sum+q.Dequeue; Inc(i); end; writeln(sum,' ',q.IsEmpty); end.", "1249975000 True\n");
    result("uses Collections; function Make:Queue<integer>; begin Result:=new Queue<integer>; Result.Enqueue(6); end; begin var a:=new Queue<integer>[2]; a[0]:=Make; writeln(a[0].Dequeue,' ',a[1]=nil); end.", "6 True\n");
    for (const auto* text : {
        "uses Collections; begin var s:=new Stack<integer>; s.Pop; end.",
        "uses Collections; begin var s:=new Stack<integer>; writeln(s.Top); end.",
        "uses Collections; begin var q:=new Queue<integer>; q.Dequeue; end.",
        "uses Collections; begin var q:=new Queue<integer>; writeln(q.Top); end."}) runtime_error(text,"E_EMPTY");
    runtime_error("uses Collections; begin var s:Stack<integer>; s.Push(1); end.","E_NIL");
    runtime_error("uses Collections; begin var q:Queue<integer>; writeln(q.IsEmpty); end.","E_NIL");
    compile_error("begin var s:=new Stack<integer>; end.","E_UNSUPPORTED");
    compile_error("uses Collections; begin var q:=new Queue<integer>; q.Enqueue('bad'); end.","E_TYPE");
    compile_error("uses Collections; begin var s:=new Stack<integer>; s.Push; end.","E_TYPE");
    compile_error("uses Collections; begin var q:=new Queue<integer>; q.Dequeue(1); end.","E_TYPE");
    compile_error("uses Collections; begin var q:=new Queue<integer>; q.Push(1); end.","E_NAME");
    compile_error("uses Collections; begin var s:=new Stack<integer>; s.Enqueue(1); end.","E_NAME");
    compile_error("uses Collections; begin var s:=new Stack<integer>; writeln(s[0]); end.","E_TYPE");
    compile_error("uses Collections; begin var q:=new Queue<integer>; writeln(Length(q)); end.","E_TYPE");
    compile_error("uses Collections; begin var s:=new Stack<integer>; Sort(s); end.","E_TYPE");
    compile_error("uses Collections; begin var q:=new Queue<integer>; writeln(q.Count); end.","E_NAME");
    compile_error("uses Collections; type P=record x:integer; end; begin var s:=new Stack<P>; s.Print; end.","E_UNSUPPORTED");
    compile_error("uses Collections; begin var q:=new Queue<integer>; var s:=new Stack<integer>; q:=s; end.","E_TYPE");
    for (const auto* name : {"Stack", "Queue"}) {
        try {
            (void)run(std::string("uses Collections; begin\n var c:=new ")+name+"<integer>;\n c.Top; end.");
            require(false,"Expected bilingual empty-collection error");
        } catch (const abc::Error& caught) {
            require(caught.code=="E_EMPTY" && caught.location.line==3
                && std::string(caught.what())==std::string(name)+" is empty"
                && caught.russian==(std::string(name)=="Stack" ? "Стек пуст" : "Очередь пуста"),"Empty collection retains source line and both languages");
        }
    }
    auto program=abc::compile("uses Collections; begin var s:=new Stack<integer>; s.Push(1); writeln(s.Pop); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::endpoint_call) instruction.operand=999;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("uses Collections; begin var q:=new Queue<integer>; writeln(q.IsEmpty); end.");
    program.types[0].kind=abc::CompoundKind::array;
    error([&]{abc::verify(program);},"E_BYTECODE");
    abc::ExecutionOptions limits; limits.allocation_limit=5;
    error([&]{(void)run("uses Collections; begin var q:=new Queue<integer>; var i:=0; while i<10 do begin q.Enqueue(i); q.Dequeue; Inc(i); end; end.","",limits);},"E_LIMIT");
}

void buffer_collections() {
    result("uses Collections; begin var d:=new DynArray<integer>; writeln(d.Count,' ',d.Capacity,' ',MIN_CAP,' ',INC_CAP_FACTOR); d.Add(7); d.Add(2); d.Insert(1,9); writeln(d[0],d.Elem[1],d[2],' ',d.Count,' ',d.Find(9),' ',d.Find(3)); d.Remove(1); writeln(d.ToString); end.","0 4 4 2\n792 3 1 -1\n7 2 \n");
    result("uses Collections; begin var d:=new DynArray<integer>(3); writeln(d.Count,' ',d.Capacity,' ',d[0]); d[0]:=1; d.Elem[1]:=2; d[2]:=3; d.Resize(1); d.Count:=3; writeln(d.ToString); d.Capacity:=12; d.Capacity:=2; d.Reserve(-1); writeln(d.Capacity); d.Resize(13); writeln(d.Count,' ',d.Capacity,' ',d[2],' ',d[12]); end.","3 10 0\n1 2 3 \n12\n13 26 3 0\n");
    result("uses Collections; begin var d:=new DynArray<integer>(3); d[2]:=9; d.Resize(1); d.Reserve(20); d.Resize(21); writeln(d[2],' ',d.Capacity); end.","0 42\n");
    result("uses Collections; begin var d:=new DynArray<integer>; d.Add(1); d.Add(2); d.Add(3); d.Remove(1); d.Resize(3); writeln(d.ToString); end.","1 3 3 \n");
    result("uses Collections; type D=DynArray<string>; begin var d:=new D(1); d[0]:='Привет'; var alias:=d; alias.Add('мир'); alias.Elem[0]:='Здравствуйте'; writeln(d.Count,' ',d.ToString); end.","2 Здравствуйте мир \n");
    result("uses Collections; type P=record x:integer; end; begin var d:=new DynArray<P>(1); var p:P; p.x:=7; d[0]:=p; p.x:=9; var q:=d[0]; q.x:=11; d.Insert(0,q); var a:=d[0]; var b:=d[1]; writeln(a.x,' ',b.x,' ',d.Find(p)); d.Remove(0); a:=d[0]; writeln(a.x); end.","11 7 -1\n7\n");
    result("uses Collections; begin var d:=new DynArray<array of integer>; var a:=Arr(1); d.Add(a); a[0]:=8; var b:=d[0]; writeln(b[0],' ',d.Find(a),' ',d.Find(Arr(8))); end.","8 0 -1\n");
    result("uses Collections; begin var s:=new SimpleSet<integer>; s.Add(3); s.Add(1); s.Add(3); s.Add(2); writeln('[',s.ToString,'] ',s.Contains(3),' ',s.Contains(9)); s.Remove(9); s.Remove(1); s.Add(1); writeln(s.ToString); end.","[3 1 2 ] True False\n3 2 1 \n");
    result("uses Collections; type P=record x:integer; end; begin var s:=new SimpleSet<P>; var p:P; p.x:=7; s.Add(p); p.x:=8; writeln(s.Contains(p)); p.x:=7; writeln(s.Contains(p)); s.Add(p); s.Remove(p); writeln(s.Contains(p)); end.","False\nTrue\nFalse\n");
    result("uses Collections; begin var s:=new SimpleSet<array of integer>; var a:=Arr(1); s.Add(a); s.Add(a); writeln(s.Contains(a),' ',s.Contains(Arr(1))); s.Remove(a); writeln(s.Contains(a)); end.","True False\nFalse\n");
    result("uses Collections; begin var s:=new SimpleSet<string>; var alias:=s; alias.Add('🌍'); alias.Add('🌍'); s.Print; write('|'); s.Println; s.Remove('🌍'); writeln('[',s.ToString,']'); var d:=new DynArray<boolean>; d.Add(true); d.Print; write('|'); d.Println; end.","🌍 |🌍 \n[]\nTrue |True \n");
    result("uses Collections; begin var d:DynArray<integer>; var s:SimpleSet<string>; writeln(d=nil,' ',s=nil); end.","True True\n");
    result("uses Collections; procedure Add(d:DynArray<integer>); begin d.Add(5); end; function Make:SimpleSet<integer>; begin Result:=new SimpleSet<integer>; Result.Add(9); end; begin var d:=new DynArray<integer>; Add(d); var s:=Make; writeln(d[0],' ',s.Contains(9)); end.","5 True\n");
    for (const auto* source : {
        "uses Collections; begin var d:=new DynArray<integer>; d.Insert(0,1); end.",
        "uses Collections; begin var d:=new DynArray<integer>(1); d.Insert(1,2); end.",
        "uses Collections; begin var d:=new DynArray<integer>; d.Remove(0); end.",
        "uses Collections; begin var d:=new DynArray<integer>; writeln(d[0]); end.",
        "uses Collections; begin var d:=new DynArray<integer>(1); d[-1]:=2; end."}) runtime_error(source,"E_BOUNDS");
    runtime_error("uses Collections; begin var d:=new DynArray<integer>(-1); end.","E_RANGE");
    runtime_error("uses Collections; begin var d:=new DynArray<integer>; d.Count:=-1; end.","E_RANGE");
    runtime_error("uses Collections; begin var d:=new DynArray<integer>(2147483647); end.","E_OVERFLOW");
    runtime_error("uses Collections; begin var d:=new DynArray<integer>; d.Capacity:=1048577; end.","E_LIMIT");
    runtime_error("uses Collections; begin var d:DynArray<integer>; d.Count:=1; end.","E_NIL");
    runtime_error("uses Collections; begin var s:SimpleSet<integer>; s.Contains(1); end.","E_NIL");
    compile_error("begin var d:=new DynArray<integer>; end.","E_UNSUPPORTED");
    compile_error("begin writeln(MIN_CAP); end.","E_NAME");
    compile_error("uses Collections; begin var d:=new DynArray<integer>; d.Add('bad'); end.","E_TYPE");
    compile_error("uses Collections; begin var d:=new DynArray<integer>; d.Count(); end.","E_TYPE");
    compile_error("uses Collections; begin var s:=new SimpleSet<integer>(1); end.","E_TYPE");
    compile_error("uses Collections; begin var s:=new Stack<integer>(1); end.","E_TYPE");
    compile_error("uses Collections; begin var s:=new SimpleSet<integer>; s.Remove('bad'); end.","E_TYPE");
    compile_error("uses Collections; begin var s:=new SimpleSet<integer>; writeln(s.Count); end.","E_NAME");
    compile_error("uses Collections; begin var s:=new SimpleSet<integer>; s[0]:=1; end.","E_TYPE");
    compile_error("uses Collections; procedure P(var x:integer); begin end; begin var d:=new DynArray<integer>(1); P(d[0]); end.","E_TYPE");
    compile_error("uses Collections; type P=record x:integer; end; begin var d:=new DynArray<P>(1); d[0].x:=1; end.","E_TYPE");
    compile_error("uses Collections; type P=record x:integer; end; begin var s:=new SimpleSet<P>; s.Print; end.","E_UNSUPPORTED");
    auto program=abc::compile("uses Collections; begin var d:=new DynArray<integer>(1); writeln(d[0]); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::buffer_call) instruction.operand=999;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("uses Collections; begin var s:=new SimpleSet<integer>; s.Add(1); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::buffer_call) instruction.operand=static_cast<std::size_t>(abc::BufferMethod::resize);
    error([&]{abc::verify(program);},"E_BYTECODE");
}

void association_collections() {
    result("uses Collections; begin var m:=new AssocArray<string,integer>; writeln(m['missing'],' [',m.ToString,']'); m['b']:=2; m.Elem['a']:=1; m['b']:=3; writeln(m['b'],' ',m.Elem['a']); write(m.ToString); end.","0 []\n3 1\nb 3\r\na 1\r\n");
    result("uses Collections; begin var m:=new AssocArray<integer,string>(); m.Print; m.Println; writeln('[',m.ToString,']'); m[7]:='Привет 🌍'; m.Print; write('|'); m.Println(); writeln(Length(m.ToString)); end.","[]\n7 Привет 🌍\r\n|7 Привет 🌍\r\n13\n");
    result("uses Collections; type M=AssocArray<integer,int64>; begin var a:=new M; var b:AssocArray<integer,int64>:=a; b[1]:=2147483648; writeln(a=b,' ',a[1]); end.","True 2147483648\n");
    result("uses Collections; begin var m:=new AssocArray<real,integer>; m[1]:=3; m[1.0]:=7; writeln(m[1]); end.","7\n");
    result("uses Collections; begin var m:=new AssocArray<boolean,string>; m[true]:='yes'; m[false]:='no'; writeln(m[false],' ',m[true]); end.","no yes\n");
    result("uses Collections; type P=record x:integer; end; begin var m:=new AssocArray<string,P>; var p:P; p.x:=7; m['p']:=p; p.x:=9; var q:=m['p']; q.x:=11; q:=m['p']; var missing:=m['missing']; missing.x:=4; var again:=m['missing']; writeln(q.x,' ',p.x,' ',again.x); end.","7 9 0\n");
    result("uses Collections; type P=record x:integer; end; begin var m:=new AssocArray<P,string>; var p:P; p.x:=7; m[p]:='first'; p.x:=8; writeln('[',m[p],']'); p.x:=7; m[p]:='updated'; writeln(m[p]); end.","[nil]\nupdated\n");
    result("uses Collections; begin var m:=new AssocArray<array of integer,string>; var a:=Arr(1); m[a]:='stored'; a[0]:=9; writeln(m[a],' [',m[Arr(9)],']'); m[nil]:='nil key'; writeln(m[nil]); end.","stored [nil]\nnil key\n");
    result("uses Collections; begin var m:=new AssocArray<string,array of integer>; var a:=Arr(1); m['a']:=a; a[0]:=8; var b:=m['a']; writeln(b[0],' ',m['missing']=nil); end.","8 True\n");
    result("uses Collections; begin var m:=new AssocArray<string,AssocArray<integer,string>>; var inner:=new AssocArray<integer,string>; inner[1]:='value'; m['inner']:=inner; var alias:=m['inner']; writeln(alias[1],' ',m['missing']=nil); end.","value True\n");
    result("uses Collections; var calls:integer; function Key:string; begin Inc(calls); Result:='key'; end; function V:integer; begin Inc(calls); Result:=9; end; begin var m:=new AssocArray<string,integer>; m[Key()]:=V(); writeln(calls,' ',m['key']); end.","2 9\n");
    result("uses Collections; type M=AssocArray<string,integer>; var m:M; function Update:integer; begin m['key']:=7; Result:=9; end; begin m:=new M; m['key']:=Update(); writeln(m['key']); end.","9\n");
    result("uses Collections; type M=AssocArray<string,integer>; procedure SetValue(m:M); begin m['x']:=4; end; procedure Replace(var m:M); begin m:=new M; m['x']:=9; end; begin var m:=new M; SetValue(m); var old:=m; Replace(m); writeln(old['x'],' ',m['x']); end.","4 9\n");
    result("uses Collections; begin var maps:=new AssocArray<string,integer>[2]; maps[0]:=new AssocArray<string,integer>; maps[0]['x']:=3; writeln(maps[0]['x'],' ',maps[1]=nil); end.","3 True\n");
    runtime_error("uses Collections; begin var m:AssocArray<string,integer>; writeln(m['x']); end.","E_NIL");
    runtime_error("uses Collections; begin var m:AssocArray<string,integer>; m['x']:=1; end.","E_NIL");
    compile_error("begin var m:=new AssocArray<string,integer>; end.","E_UNSUPPORTED");
    compile_error("uses Collections; begin var m:=new AssocArray<string,integer>(1); end.","E_TYPE");
    compile_error("uses Collections; begin var m:=new AssocArray<string,integer>; m[1]:=2; end.","E_TYPE");
    compile_error("uses Collections; begin var m:=new AssocArray<string,integer>; writeln(m[1]); end.","E_TYPE");
    compile_error("uses Collections; begin var m:=new AssocArray<string,integer>; m['x']:='bad'; end.","E_TYPE");
    compile_error("uses Collections; begin var a:=new AssocArray<string,integer>; var b:=new AssocArray<integer,integer>; a:=b; end.","E_TYPE");
    compile_error("uses Collections; begin var a:=new AssocArray<string,integer>; var b:=new AssocArray<string,string>; a:=b; end.","E_TYPE");
    compile_error("uses Collections; begin var m:=new AssocArray<string,integer>; m.ContainsKey('x'); end.","E_NAME");
    compile_error("uses Collections; begin var m:=new AssocArray<string,integer>; writeln(m.Count); end.","E_NAME");
    compile_error("uses Collections; begin var m:=new AssocArray<string,integer>; m.Print(1); end.","E_TYPE");
    compile_error("uses Collections; procedure P(var x:integer); begin end; begin var m:=new AssocArray<string,integer>; P(m['x']); end.","E_TYPE");
    compile_error("uses Collections; type P=record x:integer; end; begin var m:=new AssocArray<string,P>; m['x'].x:=1; end.","E_TYPE");
    compile_error("uses Collections; begin var m:=new AssocArray<array of integer,string>; m.Print; end.","E_UNSUPPORTED");
    compile_error("uses Collections; begin var m:=new AssocArray<string,array of integer>; m.ToString; end.","E_UNSUPPORTED");
    auto program=abc::compile("uses Collections; begin var m:=new AssocArray<string,integer>; m['x']:=1; end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::association_call) instruction.operand=999;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("uses Collections; begin var m:=new AssocArray<string,integer>; end.");
    program.types[0].key=abc::Type::unit;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program.types[0].key=static_cast<abc::Type>(64);
    error([&]{abc::verify(program);},"E_BYTECODE");
    abc::ExecutionOptions limits; limits.allocation_limit=3;
    error([&]{(void)run("uses Collections; begin var m:=new AssocArray<string,integer>; m['a']:=1; m['b']:=2; end.","",limits);},"E_LIMIT");
}

void linked_collections() {
    result("uses Collections; begin var l:=new LinkedList<integer>; writeln(l.First=nil,' ',l.Last=nil); l.AddFirst(2); l.AddFirst(1); l.AddLast(3); var n:=l.First; while n<>nil do begin write(n.Value); n:=n.Next; end; write(' '); n:=l.Last; while n<>nil do begin write(n.Value); n:=n.Prev; end; writeln(' ',l.First.Prev=nil,' ',l.Last.Next=nil); end.","True True\n123 321 True True\n");
    result("uses Collections; begin var l:=new LinkedList<integer>; l.AddLast(1); l.AddLast(3); l.AddBefore(l.Last,2); l.AddAfter(l.Last,4); l.AddBefore(l.First,0); l.AddAfter(l.First.Next,8); var n:=l.First; while n<>nil do begin write(n.Value); n:=n.Next; end; writeln; l.Remove(l.First.Next.Next); l.Remove(l.First); l.Remove(l.Last); n:=l.First; while n<>nil do begin write(n.Value); n:=n.Next; end; writeln; end.","018234\n123\n");
    result("uses Collections; begin var l:=new LinkedList<integer>; l.AddLast(1); l.AddLast(2); var old:=l.First; l.RemoveFirst; writeln(old.Value,' ',old.Next.Value,' ',l.First.Prev=nil); l.RemoveLast; writeln(l.First=nil,' ',l.Last=nil,' ',old.Next.Value); l.AddLast(3); writeln(l.First=l.Last); end.","1 2 True\nTrue True 2\nTrue\n");
    result("uses Collections; begin var l:=new LinkedList<integer>; l.AddLast(1); l.AddLast(2); l.AddLast(3); var removed:=l.First.Next; l.Remove(removed); removed.Value:=9; writeln(l.First.Next=l.Last,' ',removed.Prev=l.First,' ',removed.Next=l.Last,' ',removed.Value); end.","True True True 9\n");
    result("uses Collections; type N=LinkedListNode<integer>; begin var a:=new N(1,nil,nil); var b:=new N(2,a,nil); var c:=new N(3,nil,b); b.Value:=7; writeln(b.Prev=a,' ',a.Next=nil,' ',c.Next=b,' ',b.Value); end.","True True True 7\n");
    result("uses Collections; type P=record x:integer; end; begin var p:P; p.x:=7; var l:=new LinkedList<P>; l.AddLast(p); p.x:=9; var q:=l.First.Value; q.x:=11; var stored:=l.First.Value; writeln(stored.x,' ',p.x); l.First.Value:=q; stored:=l.First.Value; writeln(stored.x); end.","7 9\n11\n");
    result("uses Collections; begin var a:=Arr(1); var l:=new LinkedList<array of integer>; l.AddLast(a); a[0]:=8; var b:=l.First.Value; writeln(b[0]); end.","8\n");
    result("uses Collections; function Head:LinkedListNode<integer>; begin var l:=new LinkedList<integer>; l.AddLast(1); l.AddLast(2); Result:=l.First; end; begin var h:=Head; writeln(h.Value,' ',h.Next.Value); end.","1 2\n");
    result("uses Collections; begin var inner:=new LinkedList<integer>; var outer:=new LinkedList<LinkedList<integer>>; outer.AddLast(inner); var same:=outer.First.Value; same.AddLast(4); writeln(inner.First.Value); end.","4\n");
    result("uses Collections; begin var l:=new LinkedList<integer>; writeln('[',l.ToString,']'); l.AddLast(1); l.AddLast(2); l.AddLast(3); writeln('[',l.ToString(),']'); l.Print; write('|'); l.Println; end.","[]\n[3 ]\n3 |3 \n");
    result("uses Collections; begin var a:=new LinkedList<integer>; var b:=new LinkedList<integer>; a.AddLast(1); a.AddLast(2); b.AddLast(8); b.AddLast(9); a.AddBefore(b.Last,7); var n:=a.First; while n<>nil do begin write(n.Value); n:=n.Next; end; write(' '); n:=b.First; while n<>nil do begin write(n.Value); n:=n.Next; end; writeln; end.","12 879\n");
    for (const auto* source : {
        "uses Collections; begin var l:=new LinkedList<integer>; l.RemoveFirst; end.",
        "uses Collections; begin var l:=new LinkedList<integer>; l.RemoveLast; end."}) runtime_error(source,"E_EMPTY");
    for (const auto* source : {
        "uses Collections; begin var l:LinkedList<integer>; l.AddFirst(1); end.",
        "uses Collections; begin var l:=new LinkedList<integer>; l.Remove(nil); end.",
        "uses Collections; begin var l:=new LinkedList<integer>; l.AddBefore(nil,1); end.",
        "uses Collections; begin var n:LinkedListNode<integer>; writeln(n.Value); end.",
        "uses Collections; begin var n:LinkedListNode<integer>; n.Value:=1; end."}) runtime_error(source,"E_NIL");
    compile_error("begin var l:=new LinkedList<integer>; end.","E_UNSUPPORTED");
    compile_error("uses Collections; begin var n:=new LinkedListNode<integer>; end.","E_TYPE");
    compile_error("uses Collections; begin var n:=new LinkedListNode<integer>(1,nil); end.","E_TYPE");
    compile_error("uses Collections; begin var d:=new DynArray<integer>(1,2); end.","E_TYPE");
    compile_error("uses Collections; begin var l:=new LinkedList<integer>; l.AddLast('bad'); end.","E_TYPE");
    compile_error("uses Collections; begin var l:=new LinkedList<integer>; var n:=new LinkedListNode<string>('x',nil,nil); l.Remove(n); end.","E_TYPE");
    compile_error("uses Collections; begin var l:=new LinkedList<integer>; l.First:=nil; end.","E_TYPE");
    compile_error("uses Collections; begin var n:=new LinkedListNode<integer>(1,nil,nil); n.Next:=nil; end.","E_TYPE");
    compile_error("uses Collections; begin var l:=new LinkedList<integer>; l.First(); end.","E_TYPE");
    compile_error("uses Collections; begin var l:=new LinkedList<integer>; writeln(l.Count); end.","E_NAME");
    compile_error("uses Collections; type P=record x:integer; end; begin var l:=new LinkedList<P>; l.Print; end.","E_UNSUPPORTED");
    compile_error("uses Collections; procedure P(var x:integer); begin end; begin var l:=new LinkedList<integer>; l.AddLast(1); P(l.First.Value); end.","E_TYPE");
    const std::string cycle="uses Collections; begin var l:=new LinkedList<integer>; l.AddLast(1); l.AddLast(2); var n:=new LinkedListNode<integer>(4,l.Last,l.First); l.AddBefore(n,3); l.ToString; end.";
    abc::ExecutionOptions limits; limits.instruction_limit=200;
    error([&]{(void)run(cycle,"",limits);},"E_LIMIT");
    limits={}; int polls=0; limits.cancelled=[&]{return ++polls>100;};
    error([&]{(void)run(cycle,"",limits);},"E_CANCELLED");
    limits={}; limits.allocation_limit=4;
    error([&]{(void)run("uses Collections; begin var l:=new LinkedList<integer>; var i:=0; while i<10 do begin l.AddLast(i); l.RemoveFirst; Inc(i); end; end.","",limits);},"E_LIMIT");
    try { (void)run("uses Collections;\nbegin\n var l:=new LinkedList<integer>;\n l.RemoveFirst; end."); require(false,"Expected empty-list error"); }
    catch (const abc::Error& caught) {
        require(caught.code=="E_EMPTY" && caught.location.line==4 && std::string(caught.what())=="Linked list is empty" && caught.russian=="Связанный список пуст","Empty-list diagnostic retains both languages and position");
    }
    auto program=abc::compile("uses Collections; begin var l:=new LinkedList<integer>; l.AddLast(1); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::linked_call) instruction.operand=999;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("uses Collections; begin var l:=new LinkedList<integer>; end.");
    program.types[1].key=abc::Type::integer;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("uses Collections; begin var n:LinkedListNode<integer>; end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::make_default) instruction.operand=1;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("uses Collections; begin var l:=new LinkedList<integer>; end.");
    std::weak_ptr<abc::Aggregate> retained;
    {
        abc::Values values{program,16}; abc::LinkedRuntime runtime{values,{}};
        auto a=runtime.node(static_cast<abc::Type>(64),std::int32_t{1});
        auto b=runtime.node(static_cast<abc::Type>(64),std::int32_t{2},a,a);
        a->links->prev=b; a->links->next=b; retained=a;
    }
    require(retained.expired(),"Node arena releases cyclic links when execution ends");
}

void single_nodes() {
    result("uses Collections; begin var tail:=new SingleNode<integer>(3,nil); var head:=new SingleNode<integer>(1,tail); head.Next:=new SingleNode<integer>(2,tail); var n:=head; while n<>nil do begin write(n.Data); n:=n.Next; end; writeln; head.Next:=nil; writeln(head.Next=nil,' ',tail.Data); end.","123\nTrue 3\n");
    result("uses Collections; begin var n:=new SingleNode<string>('Привет 🌍',nil); var alias:=n; alias.Data:='изменено'; n.Next:=n; writeln(n.Data,' ',n.Next=n,' ',alias=n); n.Next:=nil; end.","изменено True True\n");
    result("uses Collections; type N=SingleNode<int64>; begin var n:=new N(2147483648,nil); var m:SingleNode<int64>:=n; m.Data:=1; writeln(n.Data); end.","1\n");
    result("uses Collections; type P=record x:integer; end; begin var p:P; p.x:=7; var n:=new SingleNode<P>(p,nil); p.x:=9; var q:=n.Data; q.x:=11; var stored:=n.Data; writeln(stored.x,' ',p.x); n.Data:=q; stored:=n.Data; writeln(stored.x); end.","7 9\n11\n");
    result("uses Collections; begin var a:=Arr(1); var n:=new SingleNode<array of integer>(a,nil); a[0]:=8; var b:=n.Data; writeln(b[0]); end.","8\n");
    result("uses Collections; begin var inner:=new SingleNode<integer>(4,nil); var outer:=new SingleNode<SingleNode<integer>>(inner,nil); var alias:=outer.Data; alias.Data:=5; writeln(inner.Data); end.","5\n");
    result("uses Collections; function Head:SingleNode<integer>; begin var t:=new SingleNode<integer>(2,nil); Result:=new SingleNode<integer>(1,t); end; begin var n:=Head; writeln(n.Data,' ',n.Next.Data); end.","1 2\n");
    result("uses Collections; begin var a:=new SingleNode<integer>[2]; a[0]:=new SingleNode<integer>(7,nil); writeln(a[0].Data,' ',a[1]=nil); end.","7 True\n");
    result("uses Collections; type N=SingleNode<integer>; var n:N; var calls:integer; function Receiver:N; begin Inc(calls); Result:=n; end; function Value:integer; begin Inc(calls); Result:=9; end; begin n:=new N(1,nil); Receiver().Data:=Value(); writeln(calls,' ',n.Data); end.","2 9\n");
    for (const auto* source : {
        "uses Collections; begin var n:SingleNode<integer>; writeln(n.Data); end.",
        "uses Collections; begin var n:SingleNode<integer>; n.Data:=1; end.",
        "uses Collections; begin var n:SingleNode<integer>; n.Next:=nil; end."}) runtime_error(source,"E_NIL");
    compile_error("begin var n:=new SingleNode<integer>(1,nil); end.","E_UNSUPPORTED");
    compile_error("uses Collections; begin var n:=new SingleNode<integer>; end.","E_TYPE");
    compile_error("uses Collections; begin var n:=new SingleNode<integer>(1,nil,nil); end.","E_TYPE");
    compile_error("uses Collections; begin var n:=new SingleNode<integer>(1,nil); n.Data:='bad'; end.","E_TYPE");
    compile_error("uses Collections; begin var n:=new SingleNode<integer>(1,nil); n.Next:=new SingleNode<string>('x',nil); end.","E_TYPE");
    compile_error("uses Collections; begin var n:=new SingleNode<integer>(1,nil); n.Next:=new LinkedListNode<integer>(2,nil,nil); end.","E_TYPE");
    compile_error("uses Collections; begin var n:=new SingleNode<integer>(1,nil); writeln(n.Value); end.","E_NAME");
    compile_error("uses Collections; begin var n:=new SingleNode<integer>(1,nil); writeln(n.Prev); end.","E_NAME");
    compile_error("uses Collections; begin var n:=new SingleNode<integer>(1,nil); n.Next(); end.","E_TYPE");
    compile_error("uses Collections; procedure P(var x:integer); begin end; begin var n:=new SingleNode<integer>(1,nil); P(n.Data); end.","E_TYPE");
    auto program=abc::compile("uses Collections; begin var n:=new SingleNode<integer>(1,nil); n.Next:=nil; end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::linked_call) instruction.operand=static_cast<std::size_t>(abc::LinkedMethod::prev);
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("uses Collections; begin var n:SingleNode<integer>; end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::make_default) instruction.operand=1;
    error([&]{abc::verify(program);},"E_BYTECODE");
    std::weak_ptr<abc::Aggregate> retained;
    {
        abc::Values values{program,16}; abc::LinkedRuntime runtime{values,{}};
        auto n=runtime.node(static_cast<abc::Type>(64),std::int32_t{1});
        n->links->next=n; retained=n;
    }
    require(retained.expired(),"Single-node self-reference releases with the execution arena");
    abc::ExecutionOptions limits; limits.instruction_limit=200;
    error([&]{(void)run("uses Collections; begin var n:=new SingleNode<integer>(1,nil); n.Next:=n; while n<>nil do n:=n.Next; end.","",limits);},"E_LIMIT");
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
    nullable_strings();
    language_foundations();
    aggregate_semantics();
    file_library();
    file_extensions();
    endpoint_collections();
    buffer_collections();
    association_collections();
    linked_collections();
    single_nodes();
    std::cout << (checks - failures) << '/' << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}

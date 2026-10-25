#include "abc/core.hpp"
#include "../src/linked.hpp"
#include "../src/random.hpp"

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

template<class F> void error(F action, std::string_view code, std::string_view source = {}) {
    const auto context=source.empty() ? std::string{} : " in "+std::string(source);
    try { action(); require(false, std::string("Expected ") + std::string(code)+context); }
    catch (const abc::Error& caught) { require(caught.code == code, std::string("Expected ") + std::string(code) + ", got " + caught.code+context); }
    catch (const std::exception& caught) { require(false, caught.what()); }
}

void compile_error(std::string_view source, std::string_view code) {
    error([&] { (void)abc::compile(source); }, code,source);
}

void runtime_error(std::string_view source, std::string_view code) {
    error([&] { (void)run(source); }, code,source);
}

void console_functions() {
    result("begin var a:=ReadInteger; var b:=ReadInt64(); var c:=ReadReal; var d:=ReadBoolean; Writeln(a,' ',b,' ',c,' ',d); Writeln('[',ReadString,']'); Writeln(ReadlnString()); end.","12 2147483648 1.25 True\n[ tail]\nnext\n","12 2147483648 1.25 TRUE tail\nnext\n");
    result("begin Writeln(ReadlnInteger,' ',ReadlnInt64(),' ',ReadlnReal,' ',ReadlnBoolean); end.","12 2147483648 1.5 False\n","12 ignored\n2147483648 ignored\n1.5 ignored\nFalse ignored\n");
    result("begin Writeln(Ord(ReadChar),' ',Ord(ReadlnChar())); Writeln(ReadString); Writeln(ReadlnString); end.","65 32\nfirst\nsecond\n","A tail\nfirst\nsecond\n");
    result("begin Writeln(ReadLexem,'|',ReadLexem()); Writeln('[',ReadString,']'); end.","one|два\n[ tail]\n","  one\tдва tail\n");
    result("begin Writeln('[',ReadString,'] [',ReadlnString,'] ',Ord(ReadChar),' ',Ord(ReadlnChar)); end.","[] [] 65535 65535\n");
    result("begin Writeln(ReadString,'|',ReadString,'|',ReadlnString); end.","one|two|three\n","one\rtwo\nthree\r\n");
    result("begin var n:=PABCSystem.ReadInteger(); var s:=PABCSystem.ReadString; Writeln(n,' [',s,']'); end.","4 [ tail]\n","4 tail\n");
    result("begin var n:=ReadInteger('Число:'); Writeln(n); var s:=ReadString('Строка:'); Writeln('[',s,']'); end.","Число: 7\nСтрока: [ rest]\n","7 rest\n");
    result("begin Writeln(ReadlnInteger(''),' ',ReadlnReal('Real:'),' ',ReadlnBoolean('Flag:')); end."," Real: Flag: 7 1.5 True\n","7 tail\n1.5 tail\nTrue tail\n");
    result("begin var p:string:=nil; var s:=ReadString(p); Writeln(s); end.","nil line\n","line\n");
    result("begin Writeln(Ord(ReadChar('Char:')),' ',Ord(ReadlnChar('End:'))); Writeln(ReadlnString('Next:')); end.","Char: End: 65 32\nNext: next\n","A ignored\nnext\n");
    result("function ReadInteger:integer; begin Result:=9; end; begin Writeln(ReadInteger,' ',PABCSystem.ReadInteger); end.","9 7\n","7\n");
    error([&]{(void)run("begin ReadLexem; end.");},"E_INPUT");
    error([&]{(void)run("begin ReadInteger; end.","no\n");},"E_INPUT");
    for (const auto source:{"begin ReadInteger(1); end.","begin ReadlnString(1); end.","begin ReadChar(true); end.","begin ReadLexem('prompt'); end.","begin ReadReal('a','b'); end.","begin var n:integer:=ReadInt64; end."}) compile_error(source,"E_TYPE");
    abc::ExecutionOptions options; options.output_limit=2;
    error([&]{(void)run("begin ReadInteger('abc'); end.","7\n",options);},"E_LIMIT");
    std::istringstream input; std::ostringstream output;
    auto program=abc::compile("begin Writeln('before ',1 div 0); end.");
    error([&]{abc::execute(program,input,output);},"E_DIV_ZERO");
    require(output.str().empty(),"Write arguments are evaluated before the first output");
    result("begin Writeln(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17); end.","1234567891011121314151617\n");
    program=abc::compile("begin Writeln(1); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::console_output) instruction.type=abc::Type::integer;
    error([&]{abc::verify(program);},"E_BYTECODE");
}

void console_input() {
    result("begin var a,b:integer; var x:real; var flag:boolean; Read(a,b,x,flag); Writeln(a,' ',b,' ',x,' ',flag); var s:string; Read(s); Writeln('[',s,']'); Readln; Readln(s); Writeln(s); end.","12 -3 1.5 True\n[ rest]\nnext\n","12 -3 1.5 TRUE rest\nnext\n");
    result("begin var a,b:integer; Readln(a,b); Writeln(a,' ',b); var s:string; Readln(s); Writeln(s); end.","1 2\nnext\n"," 1 2 ignored\nnext\n");
    result("begin var n:integer; Readln(n); Writeln(n); end.","42\n","\n  \t\n42 tail\n");
    result("begin var s:string; Read(s); Writeln('[',s,']'); Read(s); Writeln('[',s,']'); Readln(); Readln(s); Writeln(s); end.","[  first line]\n[]\nnext\n","  first line\r\nnext\r\n");
    result("begin Readln; Readln(); var n:integer; Readln(n); Writeln(n); end.","7\n","skip\nignore\n7\n");
    result("begin Readln; Readln(); var c:char; Read(c); var s:string; Read(s); Writeln(Ord(c),' ',s.Length,' ',Ord(s[1])); end.","65535 1 65535\n");
    result("begin var c,d:char; Read(c,d); Writeln(Ord(c),' ',Ord(d)); Readln; Readln(c); Writeln(Ord(c)); end.","55356 57101\n10\n","🌍\n\n");
    result("begin var c:char; Readln(c); Writeln(Ord(c)); var s:string; Readln(s); Writeln(s); end.","13\nnext\n","\r\nnext\n");
    result("begin var a:byte; var b:shortint; var c:smallint; var d:word; var e:longword; var f:uint64; var g:int64; Readln(a,b,c,d,e,f,g); Writeln(a,' ',b,' ',c,' ',d,' ',e,' ',f,' ',g); end.","255 -128 -32768 65535 4294967295 18446744073709551615 -9223372036854775808\n","255 -128 -32768 65535 4294967295 18446744073709551615 -9223372036854775808\n");
    result("var a:array of integer; function Index:integer; begin Writeln(a[0]); Result:=1; end; begin a:=Arr(0,0); Read(a[0],a[Index()]); Writeln(a[0],' ',a[1]); end.","9\n9 8\n","9 8\n");
    result("type R=record n:integer; end; begin var r:R; var a:=Arr(0); Readln(r.n,a[0]); Writeln(r.n,' ',a[0]); end.","8 9\n","8 9 tail\n");
    result("uses Arrays; begin var a:=new integer[3]; ReadArray(a); a.Writeln(','); var s:string; Readln(s); Writeln(s); end.","1,2,3\nnext\n","1 2\n3 tail\nnext\n");
    result("uses Arrays; begin var a:=new real[3]; Arrays.ReadArray(a); a.Writeln(','); end.","1.5,-2,30\n","1.5 -2 3e1\n");
    result("uses Arrays; begin var a:=new string[3]; ReadArray(a); WritelnArray(a,'|'); var s:string; Readln(s); Writeln(s); end.","one two||\nnext\n","one two\nnext\n");
    result("uses Arrays; begin var a:=new integer[0]; ReadArray(a); var n:integer; Readln(n); Writeln(n); end.","5\n","skip\n5\n");
    result("uses Arrays; begin var n:integer; Read(n); var a:=new integer[2]; ReadArray(a); Writeln(n,' ',a[0],' ',a[1]); end.","7 8 9\n","7 8 9 tail\n");
    result("uses Arrays; var a:array of integer; function Source:array of integer; begin Writeln('array'); Result:=a; end; begin a:=new integer[2]; ReadArray(Source()); a.Writeln; end.","array\n1 2\n","1 2\n");
    error([&]{(void)run("begin var l:=new List<integer>; l.Add(1); foreach var x in l do Read(l[0]); end.","9\n");},"E_MODIFIED");
    runtime_error("uses Arrays; begin var a:array of integer; ReadArray(a); end.","E_NIL");
    error([&]{(void)run("uses Arrays; begin var a:=new integer[2]; ReadArray(a); end.","1\n");},"E_INPUT");
    for (const auto input:{"256\n","-1\n","1.5\n","bad\n"}) error([&]{(void)run("begin var b:byte; Read(b); end.",input);},"E_INPUT");
    result("begin Read; Read(); Writeln('ok'); end.","ok\n");
    for (const auto source:{"begin var n:integer; Read(n,1); end.","begin var a:=Arr(1); Read(a); end.","begin for var i:=1 to 2 do Read(i); end.","begin foreach var c in 'ab' do Readln(c); end.","uses Arrays; begin var a:=new byte[2]; ReadArray(a); end.","uses Arrays; begin ReadArray(Arr(1),2); end."}) compile_error(source,"E_TYPE");
    compile_error("begin var a:=Arr(1); ReadArray(a); end.","E_NAME");
    std::istringstream input("9 8\n"); std::ostringstream output;
    auto program=abc::compile("var a:array of integer; function Bad:integer; begin Write(a[0]); Result:=0; end; begin a:=Arr(0); var b:array of integer; Read(a[0],b[Bad()]); end.");
    error([&]{abc::execute(program,input,output);},"E_NIL");
    require(output.str()=="9","Earlier console input occurs before evaluating a later receiver, matching source lowering");
    program=abc::compile("begin var n:integer; Readln(n); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::read_console) instruction.operand=0;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin var n:integer; Read(n); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::ref_local) instruction.op=abc::Op::load;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("uses Arrays; begin ReadArray(new integer[1]); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::read_array) instruction.operand=1;
    error([&]{abc::verify(program);},"E_BYTECODE");
    result("begin var a:integer; Readln(a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a); Writeln(a); end.","16\n","1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16\n");
    compile_error("begin var a:integer; Read(a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a); end.","E_TYPE");
    error([&]{(void)run("begin var s:string; Read(s); end.",std::string(1'048'577,'x'));},"E_LIMIT");
    error([&]{(void)run("begin var c:char; Read(c); end.","\xff\n");},"E_INPUT");
    abc::ExecutionOptions options; std::size_t polls=0; options.cancelled=[&]{return ++polls>8;};
    error([&]{(void)run("begin var n:integer; Read(n); end.",std::string(2048,' ')+'1'+"\n",options);},"E_CANCELLED");
}

void bitwise_operations() {
    result("begin Writeln(12 and 10,' ',12 or 10,' ',12 xor 10,' ',not 0,' ',not -1); Writeln(-8 shr 1,' ',1 shl 31,' ',1 shl 32,' ',1 shl -1,' ',-8 shr -1); end.","8 14 6 -1 0\n-4 -2147483648 1 -2147483648 -1\n");
    result("begin var a:int64:=1; Writeln(a shl 63,' ',a shl 64,' ',a shl 65,' ',(-9223372036854775808) shr 63); Writeln(not -9223372036854775808,' ',(not a) shr 1); end.","-9223372036854775808 1 2 -1\n9223372036854775807 -1\n");
    result("begin var u:longword:=MaxLongWord; var one:longword:=1; var mask:longword:=2147483648; var x:longword:=u and mask; Writeln(x,' ',not u,' ',u shr 31,' ',one shl 31,' ',one shl 32); var y:longword:=u xor u; Writeln(y); end.","2147483648 0 1 2147483648 1\n0\n");
    result("begin var u:uint64:=MaxUInt64; var one:uint64:=1; Writeln(u shr 63,' ',one shl 63,' ',one shl 64,' ',not u,' ',u and 255,' ',255 and u); Writeln(u xor u,' ',u or one); end.","1 9223372036854775808 1 0 255 255\n0 18446744073709551615\n");
    result("begin var a:byte:=240; var b:byte:=15; var c:byte:=a or b; var n:byte:=not a; Writeln(c,' ',n,' ',a and b,' ',a xor b,' ',a shl 8); end.","255 15 0 255 61440\n");
    result("begin var a:shortint:=-128; var b:shortint:=1; var n:shortint:=not a; var c:shortint:=a or b; Writeln(n,' ',c,' ',a shr 1); var s:smallint:=-32768; var t:smallint:=not s; var w:word:=65535; var z:word:=not w; Writeln(t,' ',z,' ',w shl 1); end.","127 -127 -64\n32767 0 131070\n");
    result("begin var a:byte:=240; var b:word:=3855; var w:word:=a or b; var s:shortint:=-1; var t:smallint:=s and a; Writeln(w,' ',t); end.","4095 240\n");
    result("begin Writeln(true xor false,' ',true xor true,' ',false xor false); Writeln(1+2 shl 2,' ',8 shr 1+1,' ',1 or 2 and 4,' ',not 1 and 3); end.","True False False\n9 5 1 2\n");
    result("var trace:string; function A:integer; begin trace:=trace+'A'; Result:=0; end; function B:integer; begin trace:=trace+'B'; Result:=2; end; function F:boolean; begin trace:=trace+'F'; Result:=true; end; begin Writeln(A() and B(),' ',trace); trace:=''; Writeln(true xor F(),' ',trace); trace:=''; Writeln(A() shl B(),' ',trace); end.","0 AB\nFalse F\n0 AB\n");
    result("begin var a:byte:=7; var b:byte:=not not a; Writeln(b); var shift:word:=32; Writeln(1 shl shift); end.","7\n1\n");
    for (const auto source:{"begin Writeln(1.0 and 2); end.","begin Writeln(true xor 1); end.","begin Writeln(not 'a'); end.","begin Writeln('a' shl 1); end.","begin var n:int64:=2; Writeln(1 shl n); end.","begin var n:integer:=2; var u:uint64:=3; Writeln(u and n); end.","begin Writeln(1 shr true); end."}) compile_error(source,"E_TYPE");
    auto program=abc::compile("begin Writeln(1 shl 2); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::shift_left) instruction.type=abc::Type::byte;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin Writeln(1 and 2); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::bit_and) instruction.type=abc::Type::real;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin Writeln(not 1); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::bit_not) instruction.operand=1;
    error([&]{abc::verify(program);},"E_BYTECODE");
}

void array_output() {
    result("uses Arrays; begin var a:=Arr(1,2,3); WriteArray(a); Writeln('|'); WritelnArray(a,';'); a.Write('/'); Writeln; a.Writeln; end.","1 2 3|\n1;2;3\n1/2/3\n1 2 3\n");
    result("uses Arrays; begin Arrays.WriteArray(Arr(true,false),','); Writeln; Arrays.WritelnArray(Arr('Я','A'),''); end.","True,False\nЯA\n");
    result("uses Arrays; begin var a:=new integer[0]; WriteArray(a,'x'); WritelnArray(a,'x'); a.Write; a.Writeln; var b:array of integer:=nil; WriteArray(b); WritelnArray(b); end.","\n\n\n");
    result("uses Arrays; begin var s:string:=nil; var a:=Arr('abc',s,''); WritelnArray(a,'|'); WritelnArray(Arr(1,2),s); end.","abc|nil|\n1nil2\n");
    result("uses Arrays; begin var b:byte:=255; var s:shortint:=-128; var w:word:=65535; var u:uint64:=MaxUInt64; WritelnArray(Arr(b,b)); WritelnArray(Arr(s,s)); WritelnArray(Arr(w)); WritelnArray(Arr(u)); WritelnArray(Arr(0.5,1.5)); end.","255 255\n-128 -128\n65535\n18446744073709551615\n0.5 1.5\n");
    result("uses Arrays; var a:array of integer; var trace:string; function Source:array of integer; begin trace:=trace+'A'; Result:=a; end; function Delim:string; begin trace:=trace+'D'; SetLength(a,0); Result:=','; end; begin a:=Arr(1,2); WritelnArray(Source(),Delim()); Writeln(trace,' ',a.Length); end.","1,2\nAD 0\n");
    result("uses Arrays; procedure WriteArray(a:array of integer); begin Writeln('own'); end; begin WriteArray(Arr(1)); Arrays.WritelnArray(Arr(2,3),','); end.","own\n2,3\n");
    runtime_error("uses Arrays; begin var a:array of integer; a.Write; end.","E_NIL");
    runtime_error("uses Arrays; begin var a:array of integer; a.Writeln; end.","E_NIL");
    compile_error("begin WriteArray(Arr(1)); end.","E_NAME");
    compile_error("uses Arrays; begin WritelnArray(1); end.","E_TYPE");
    compile_error("uses Arrays; begin WritelnArray(Arr(1),2); end.","E_TYPE");
    compile_error("uses Arrays; begin var l:=new List<integer>; WritelnArray(l); end.","E_TYPE");
    compile_error("uses Arrays; begin WritelnArray(Arr(1),' ',2); end.","E_TYPE");
    compile_error("uses Arrays; type R=record x:integer; end; begin var a:=new R[1]; WritelnArray(a); end.","E_UNSUPPORTED");
    compile_error("uses Arrays; begin var x:=WritelnArray(Arr(1)); end.","E_TYPE");
    abc::ExecutionOptions options; options.output_limit=4;
    error([&]{(void)run("uses Arrays; begin WritelnArray(Arr(1,2,3),'|'); end.","",options);},"E_LIMIT");
    auto program=abc::compile("uses Arrays; begin WritelnArray(Arr(1)); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::write_array) instruction.operand=4;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("uses Arrays; begin WritelnArray(Arr(1)); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::write_array) instruction.type=abc::Type::integer;
    error([&]{abc::verify(program);},"E_BYTECODE");
    std::size_t polls=0; options.output_limit=1'048'576; options.cancelled=[&]{return ++polls>8;};
    error([&]{(void)run("uses Arrays; begin WritelnArray(new integer[2000],''); end.","",options);},"E_CANCELLED");
}

void extended_math() {
    result("begin Writeln(GCD(0,0),' ',GCD(-84,30),' ',GCD(30,-84),' ',GCD(-17,0)); Writeln(LCM(-12,18),' ',LCM(0,0),' ',LCM(17,19)); end.","0 6 6 17\n36 0 323\n");
    result("begin var a:int64:=9223372036854775806; Writeln(GCD(a,6),' ',GCD(MaxInt64,0),' ',LCM(MaxInt64,1)); Writeln(LCM(-9223372036854775808,0),' ',LCM(0,-9223372036854775808)); end.","6 9223372036854775807 9223372036854775807\n0 0\n");
    result("begin var b:byte:=84; var w:word:=30; var u:longword:=4294967295; Writeln(GCD(b,w),' ',GCD(u,15),' ',LCM(b,w)); end.","6 15 420\n");
    result("begin Writeln(Hypot(3,4),' ',Hypot(-3,-4),' ',Hypot(0,0)); Writeln(Hypot(1e300,1e300)>1e300,' ',Hypot(1e-300,0)>0); end.","5 5 0\nTrue True\n");
    result("begin Writeln(Atan2(0,1),' ',Abs(Atan2(1,0)-Pi/2)<1e-15,' ',Abs(Atan2(-1,0)+Pi/2)<1e-15,' ',Abs(Atan2(0,-1)-Pi)<1e-15); end.","0 True True True\n");
    result("begin Writeln(Lerp(10,20,0),' ',Lerp(10,20,0.25),' ',Lerp(10,20,1),' ',Lerp(10,20,2)); Writeln(Remap(5,0,10,100,200),' ',Remap(5,10,0,0,100),' ',Remap(20,0,10,0,1)); end.","10 12.5 20 30\n150 50 2\n");
    result("begin Writeln(PABCSystem.GCD(84,30),' ',PABCSystem.LCM(6,8),' ',PABCSystem.Hypot(5,12),' ',PABCSystem.Lerp(0,10,0.5),' ',PABCSystem.Remap(1,0,2,10,20)); end.","6 24 13 5 15\n");
    result("begin Writeln(LCM(3037000499,3037000500)); end.","9223372033963249500\n");
    for (const auto source:{"begin GCD(-2147483648,0); end.","begin GCD(1,-2147483648); end.","begin GCD(-9223372036854775808,0); end.","begin LCM(-9223372036854775808,1); end.","begin LCM(1,-9223372036854775808); end.","begin LCM(MaxInt64,2); end.","begin LCM(3037000500,3037000501); end."}) runtime_error(source,"E_OVERFLOW");
    runtime_error("begin Remap(1,2,2,0,1); end.","E_DIV_ZERO");
    runtime_error("begin Hypot(MaxReal,MaxReal); end.","E_OVERFLOW");
    runtime_error("begin Lerp(-MaxReal,MaxReal,1.0); end.","E_OVERFLOW");
    for (const auto source:{"begin GCD(1.0,2.0); end.","begin GCD(MaxUInt64,1); end.","begin LCM(1.0,2); end.","begin var i:integer:=LCM(1,2); end.","begin var a:int64:=2; var i:integer:=GCD(a,2); end.","begin Atan2(1); end.","begin Hypot('abc',2); end.","begin Lerp(0,1,true); end.","begin Remap(1,0,2,10); end."}) compile_error(source,"E_TYPE");
    try { (void)run("begin\n GCD(-2147483648,0);\nend."); require(false,"Expected bilingual GCD overflow"); }
    catch (const abc::Error& e) { require(e.code=="E_OVERFLOW" && e.location.line==2 && !e.russian.empty(),"Math overflow preserves location and both languages"); }
}

void sequence_mutations() {
    result("begin var a:=Arr(1,2,3,4,5); var b:=a; Reverse(a); foreach var x in b do Write(x); Writeln; Reverse(a,1,3); foreach var x in a do Write(x); end.","54321\n52341");
    result("begin var a:=Arr(1,2); Reverse(a,2,0); Reverse(a,0,0); Writeln(a[0],a[1]); var b:=new integer[0]; Reverse(b); Reverse(b,0,0); end.","12\n");
    result("begin var s:string:='abcde'; var t:=s; Reverse(s,2,3); Writeln(s,' ',t); Reverse(s); Reverse(s,6,0); Writeln(s); end.","adcbe abcde\nebcda\n");
    result("begin var s:='a'#0'b'; Reverse(s); Writeln(s[1],' ',Ord(s[2]),' ',s[3]); end.","b 0 a\n");
    result("begin var s:=#55356#57101; Reverse(s); Writeln(Ord(s[1]),' ',Ord(s[2])); end.","57101 55356\n");
    result("type R=record s:string; end; begin var r:R; r.s:='abc'; PABCSystem.Reverse(r.s); Writeln(r.s); var a:=Arr('xy','zz'); Reverse(a[0]); Writeln(a[0]); end.","cba\nyx\n");
    result("begin var l:=new List<integer>; l.Add(1); l.Add(2); l.Add(3); Reverse(l); l.Reverse(1,2); foreach var x in l do Write(x); l.Reverse; foreach var x in l do Write(x); end.","312213");
    result("begin var a:=Arr(1,2,3,4,5); SetRandomSeed(1); Shuffle(a); foreach var x in a do Write(x); Writeln(' ',Random(100)); end.","43512 65\n");
    result("begin var a:=Arr(1,2,3,4,5); SetRandomSeed(1); var b:=a.Shuffle; Writeln(a=b); foreach var x in b do Write(x); end.","True\n43512");
    result("begin var l:=new List<integer>; for var i:=1 to 5 do l.Add(i); SetRandomSeed(1); var b:=l.Shuffle(); Writeln(l=b); foreach var x in l do Write(x); end.","True\n43512");
    result("begin SetRandomSeed(1); var a:=new integer[0]; Shuffle(a); var l:=new List<integer>; l.Add(7); foreach var x in l do Shuffle(l); Writeln(l[0],' ',Random(100)); end.","7 24\n");
    result("begin var a:=7; var b:=9; PABCSystem.Swap(a,b); Writeln(a,' ',b); Swap(a,a); Writeln(a); end.","9 7\n9\n");
    result("begin var a:=Arr(1,2); var b:=Arr(8,9); var old:=a; Swap(a,b); Writeln(a[0],' ',b=old); var same:=a.Swap(0,1); Writeln(same=a,' ',a[0],a[1]); end.","8 True\nTrue 98\n");
    result("type R=record x:integer; s:string; a:array of integer; end; begin var a:=new R[2]; a[0].x:=1; a[0].s:='one'; a[0].a:=Arr(5); a[1].x:=2; a[1].s:='two'; a[1].a:=Arr(6); Reverse(a); Writeln(a[0].x,' ',a[1].s); a[0].x:=8; Writeln(a[1].x); Swap(a[0],a[1]); Writeln(a[0].x,' ',a[1].s,' ',a[0].a[0]); end.","2 one\n1\n1 two 5\n");
    result("type R=record x:integer; end; var a:array of R; procedure Test(var x:integer); begin Reverse(a); Writeln(x); end; begin a:=new R[2]; a[0].x:=4; a[1].x:=8; Test(a[0].x); Writeln(a[0].x); end.","8\n8\n");
    result("var trace:string; function I:integer; begin trace:=trace+'I'; Result:=0; end; function J:integer; begin trace:=trace+'J'; Result:=1; end; begin var a:=Arr(2,4); Swap(a[I()],a[J()]); Writeln(trace,' ',a[0],a[1]); end.","IJ 42\n");
    result("var a:array of integer; var old:array of integer; function First:integer; begin Result:=0; end; function Last:integer; begin a:=Arr(7,8); Result:=1; end; begin a:=Arr(1,2); old:=a; Swap(a[First()],a[Last()]); Writeln(old[0],' ',a[1]); end.","2 8\n");
    result("var a:array of integer; function Start:integer; begin SetLength(a,0); Result:=1; end; begin a:=Arr(1,2,3,4); var old:=a; Reverse(a,Start(),2); foreach var x in old do Write(x); Writeln(' ',a.Length); end.","1324 0\n");
    result("var s:string; function Start:integer; begin s:='xyz'; Result:=1; end; begin s:='abc'; Reverse(s,Start(),2); Writeln(s); end.","yxz\n");
    result("begin var a:array of integer:=nil; var b:=Arr(4); Swap(a,b); Writeln(a[0],' ',b=nil); end.","4 True\n");
    for (const auto source:{"begin var a:array of integer; Reverse(a); end.","begin var l:List<integer>; Shuffle(l); end.","begin var s:string:=nil; Reverse(s); end.","begin var a:array of integer; a.Swap(0,0); end."}) runtime_error(source,"E_NIL");
    for (const auto source:{"begin var a:=Arr(1,2); Reverse(a,-1,0); end.","begin var a:=Arr(1,2); Reverse(a,0,-1); end.","begin var a:=Arr(1,2); Reverse(a,1,2147483647); end.","begin var a:=Arr(1,2); a.Swap(0,2); end."}) runtime_error(source,"E_BOUNDS");
    for (const auto source:{"begin var s:string:='abc'; Reverse(s,0,0); end.","begin var s:string:='abc'; Reverse(s,2,-1); end.","begin var s:string:='abc'; Reverse(s,-2147483648,0); end."}) runtime_error(source,"E_RANGE");
    runtime_error("begin var l:=new List<integer>; l.Add(1); foreach var x in l do Reverse(l,1,0); end.","E_MODIFIED");
    runtime_error("begin var l:=new List<integer>; l.Add(1); foreach var x in l do Swap(l[0],l[0]); end.","E_MODIFIED");
    runtime_error("begin var l:=new List<integer>; l.Add(1); l.Add(2); foreach var x in l do Shuffle(l); end.","E_MODIFIED");
    runtime_error("begin var l:=new List<string>; l.Add('ab'); foreach var x in l do Reverse(l[0]); end.","E_MODIFIED");
    for (const auto source:{"begin Reverse('abc'); end.","begin var a:=1; var b:int64:=2; Swap(a,b); end.","begin var a:=1; Swap(a,2); end.","begin for var i:=1 to 3 do Swap(i,i); end.","begin var s:string:='abc'; foreach var c in s do Reverse(c); end.","begin Shuffle('abc'); end.","begin var a:=Arr(1); Shuffle(a,1); end.","begin var a:=Arr(1); Reverse(a,1.5,0); end.","begin var a:=Arr(1); var b:=Reverse(a); end."}) compile_error(source,"E_TYPE");
    auto program=abc::compile("begin var a:=Arr(1); Reverse(a); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::sequence_mutate) instruction.operand=999;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin var s:string:='abc'; Reverse(s); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::sequence_mutate) instruction.operand=static_cast<std::size_t>(abc::SequenceMutation::shuffle);
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin var a:=1; var b:=2; Swap(a,b); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::swap_values) instruction.type=abc::Type::int64;
    error([&]{abc::verify(program);},"E_BYTECODE");
    abc::ExecutionOptions options; std::size_t polls=0;
    options.cancelled=[&]{return ++polls>8;};
    error([&]{(void)run("begin var a:=new integer[2000]; Reverse(a); end.","",options);},"E_CANCELLED");
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
    compile_error("begin var n: integer := 2147483648; end.", "E_RANGE");
    compile_error("begin var x := 2.5 div 2; end.", "E_TYPE");
    compile_error("begin var a: bigint; end.", "E_UNSUPPORTED");
    result("begin writeln(9223372036854775808); end.","9223372036854775808\n");
    compile_error("begin writeln(18446744073709551616); end.", "E_RANGE");
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
    result("function F(s:string):string; begin Result:=s; end; procedure P(var s:string); begin s:=nil; end; begin var s:string:='x'; P(s); writeln(F(s)=nil); var a:=new string[1]; a[0]:='x'; P(a[0]); writeln(a[0]=nil); end.","True\nTrue\n");
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

void structured_loops() {
    result("begin var sum:=0; for var i:=1 to 10 do sum:=sum+i; Writeln(sum); end.","55\n");
    result("begin for var i:=3 downto 1 do Write(i); Writeln; end.","321\n");
    result("begin var i:integer; for i:=1 to 3 do Write(i); Writeln(' ',i); for i:=7 to 2 do Write('bad'); Writeln(i); for i:=0 downto 2 do Write('bad'); Writeln(i); end.","123 3\n7\n0\n");
    result("begin var n:=3; var count:=0; for var i:=1 to n do begin n:=99; Inc(count); end; Writeln(count); end.","3\n");
    result("function Start:integer; begin Write('S'); Result:=1; end; function Finish:integer; begin Write('F'); Result:=2; end; begin for var i:=Start to Finish do Write(i); Write('|'); var j:integer; for j:=Start to Finish do Write(j); Writeln; end.","SF12|FS12\n");
    result("begin var i:=8; for var i:=i to i+1 do Write(i); Writeln(' ',i); for j:integer:=2 to j+2 do Write(j); Writeln; end.","89 8\n234\n");
    result("type Index=int64; begin for var i:Index:=2147483648 to 2147483650 do Writeln(i); end.","2147483648\n2147483649\n2147483650\n");
    result("begin for var i:=MaxInt-1 to MaxInt do Writeln(i); for var i:integer:=-2147483648 downto -2147483648 do begin Writeln(i); continue; end; end.","2147483646\n2147483647\n-2147483648\n");
    result("begin for var i:int64:=MaxInt64-1 to MaxInt64 do begin Writeln(i); continue; end; for var i:=-9223372036854775808 downto -9223372036854775808 do Writeln(i); end.","9223372036854775806\n9223372036854775807\n-9223372036854775808\n");
    result("begin for var i:=1 to 3 do begin for var j:=3 downto 1 do begin if j=2 then continue; Write(i,j,' '); end; end; Writeln; end.","13 11 23 21 33 31 \n");
    result("begin var n:=0; for var i:=1 to 3 do begin for var j:=1 to 5 do begin if j=3 then break; Inc(n); end; end; Writeln(n); end.","6\n");
    result("begin var i:=0; var sum:=0; while i<5 do begin Inc(i); if i mod 2=0 then continue; sum:=sum+i; end; Writeln(sum); while true do begin Inc(i); break; end; Writeln(i); end.","9\n6\n");
    result("begin var n:=0; repeat Inc(n); until true; Writeln(n); repeat Inc(n); until n=4; Writeln(n); repeat until true; end.","1\n4\n");
    result("var n,calls:integer; function Done:boolean; begin Inc(calls); Result:=n>=3; end; begin repeat Inc(n); continue; until Done; Writeln(n,' ',calls); end.","3 3\n");
    result("begin var n:=0; repeat Inc(n); if n=2 then break; until false; Writeln(n); repeat var m:=0; repeat Inc(m); until m=2; Inc(n); until n=4; Writeln(n); end.","2\n4\n");
    result("begin var n:=0; repeat Inc(n); var x:=n; until x=3; Writeln(n); end.","3\n");
    result("begin var n:=0; for var i:=1 to 3 do begin repeat Inc(n); break; until false; if i=2 then continue; Write(i); end; Writeln(' ',n); end.","13 3\n");
    result("begin var n:=0; for var i:=1 to 2 do begin var x:integer; Inc(x); Write(x); end; repeat var x:integer; Inc(x); Inc(n); Write(x); until n=2; Writeln; end.","1111\n");
    result("function Find(n:integer):integer; begin for var i:=1 to n do if i=3 then Exit(i); end; function Once:integer; begin repeat Exit(7); until false; end; begin Writeln(Find(5),' ',Find(1),' ',Once); end.","3 0 7\n");
    result("function Sum(n:integer):integer; begin for var i:=1 to n do Result:=Result+i; end; function Recur(n:integer):integer; begin Result:=1; for var i:=1 to n do Result:=Result+Recur(i-1); end; begin Writeln(Sum(5),' ',Recur(4)); end.","15 16\n");
    result("procedure Count(var n:integer); begin for n:=1 to 3 do Write(n); end; begin var n:=0; Count(n); Writeln(' ',n); end.","123 3\n");
    result("begin for var i:=1 to 2 do for var i:=1 to 2 do Write(i); Writeln; end.","1212\n");
    result("begin for var i:=1 to 3 do ; var n:=0; while n<1 do Inc(n); Writeln(n); end.","1\n");
    for (const auto* source:{"begin for var i:=1 to 2 do i:=9; end.","begin for var i:=1 to 2 do Inc(i); end.",
        "begin for var i:=1 to 2 do Dec(i); end.","begin for var i:=1 to 2 do Readln(i); end.",
        "begin var f:Text; for var i:=1 to 2 do Read(f,i); end.",
        "procedure P(var n:integer); begin Inc(n); end; begin for var i:=1 to 2 do P(i); end.",
        "begin var i:=0; for i:=1 to 2 do for i:=1 to 2 do Write(i); end.",
        "begin for var i:=1.5 to 2 do Write(i); end.",
        "begin var i:real; for i:=1 to 2 do Write(i); end.",
        "begin for var i:=1 to 1.5 do Write(i); end.","begin for var i:=1 to 2 do if true then i:=0; end.",
        "begin repeat until 1; end.","begin repeat until 'yes'; end.",
        "var i:integer; procedure P; begin for i:=1 to 2 do Write(i); end; begin P; end."}) compile_error(source,"E_TYPE");
    compile_error("begin for var i:=1 to 2 do Write(i); Writeln(i); end.","E_NAME");
    compile_error("begin for var i:=1 to 2147483648 do Write(i); end.","E_RANGE");
    compile_error("begin repeat var n:=1; until n=1; Writeln(n); end.","E_NAME");
    compile_error("begin for i:=1 to 2 do Write(i); end.","E_NAME");
    compile_error("begin for var i:=1 to 3 step 2 do Write(i); end.","E_UNSUPPORTED");
    compile_error("function F:integer; begin for Result:=1 to 3 do Write(Result); end; begin end.","E_UNSUPPORTED");
    for (const auto* source:{"begin break; end.","begin continue; end.","begin if true then break; end.",
        "procedure P; begin continue; end; begin while true do P; end.",
        "begin for var i:=1 2 do Write(i); end.","begin repeat Write(1); end.","begin repeat Write(1) Write(2); until true; end."}) compile_error(source,"E_SYNTAX");
    abc::ExecutionOptions limits; limits.instruction_limit=100;
    error([&]{(void)run("begin repeat continue; until false; end.","",limits);},"E_LIMIT");
    error([&]{(void)run("begin for var i:int64:=1 to MaxInt64 do continue; end.","",limits);},"E_LIMIT");
    limits={}; int polls=0; limits.cancelled=[&]{return ++polls>30;};
    error([&]{(void)run("begin repeat until false; end.","",limits);},"E_CANCELLED");
    limits={}; limits.output_limit=4;
    error([&]{(void)run("begin for var i:=1 to 8 do Write(i); end.","",limits);},"E_LIMIT");
    limits={}; limits.allocation_limit=1;
    error([&]{(void)run("begin for var i:=1 to 1 do Write(i); end.","",limits);},"E_LIMIT");
    auto program=abc::compile("begin for var i:int64:=1 to 3 do if i=2 then continue; end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::jump) { instruction.operand=program.code.size(); break; }
    error([&]{abc::verify(program);},"E_BYTECODE");
    std::istringstream input; std::ostringstream output;
    error([&]{abc::execute(abc::compile("begin Writeln('must not run'); for var i:=1 to 2 do Inc(i); end."),input,output);},"E_TYPE");
    require(output.str().empty(),"Readonly for controls are checked before any output");
    try { (void)run("begin\n repeat\n Writeln(1 div 0);\n until true;\nend."); require(false,"Expected repeat runtime error"); }
    catch (const abc::Error& caught) {
        require(caught.code=="E_DIV_ZERO" && caught.location.line==3 && caught.russian=="Деление на ноль", "Loop body errors retain source line and both languages");
    }
}

void foreach_loops() {
    result("begin var sum:=0; foreach var x in Arr(1,2,3,4) do sum:=sum+x; Writeln(sum); end.","10\n");
    result("begin var x:=99; foreach x in Arr(1,2,3) do Write(x); Writeln(' ',x); foreach x in new integer[0] do Write('bad'); Writeln(x); end.","123 3\n3\n");
    result("begin foreach var x:int64 in Arr(1,2) do Write(x,' '); foreach x:real in Arr(3,4) do Write(x,' '); Writeln; end.","1 2 3 4 \n");
    result("begin var x:=9; foreach var x in Arr(x,x+1) do Write(x,' '); Writeln(x); end.","9 10 9\n");
    result("begin foreach var x in Arr(true,false) do Writeln(x); foreach var s in Arr(nil,'текст','') do Writeln('[',s,']'); end.","True\nFalse\n[nil]\n[текст]\n[]\n");
    result("begin var a:=Arr(1,2,3); foreach var x in a do begin Write(x); if x=1 then a[1]:=9; end; Writeln; end.","193\n");
    result("begin var a:=Arr(1,2,3); foreach var x in a do begin Write(x); a:=Arr(99); end; Writeln(' ',a[0]); end.","123 99\n");
    result("begin var a:=Arr(1,2,3); foreach var x in a do begin Write(x); SetLength(a,0); end; Writeln(' ',Length(a)); end.","123 0\n");
    result("begin var a:=new integer[0]; foreach var x in a do Write('bad'); Writeln('empty'); end.","empty\n");
    result("begin var l:=new List<integer>; l.Add(1); l.Add(2); l.Add(3); foreach var x in l do Write(x); Writeln; end.","123\n");
    result("begin var l:=new List<integer>; foreach var x in l do Write('bad'); Writeln(l.Count); end.","0\n");
    result("begin var l:=new List<integer>; l.Add(1); l.Add(2); foreach var x in l do begin Write(x); l:=new List<integer>; end; Writeln(' ',l.Count); end.","12 0\n");
    result("var calls:integer; function Source:array of integer; begin Inc(calls); Result:=Arr(1,2,3); end; begin foreach var x in Source do Write(x); Writeln(' ',calls); end.","123 1\n");
    result("var calls:integer; function Source:List<integer>; begin Inc(calls); Result:=new List<integer>; Result.Add(7); Result.Add(8); end; begin foreach var x in Source do Write(x); Writeln(' ',calls); end.","78 1\n");
    result("begin foreach var i in Arr(1,2) do foreach var j in Arr(3,4) do Write(i,j,' '); Writeln; foreach var i in Arr(1,2) do foreach var i in Arr(5,6) do Write(i); Writeln; end.","13 14 23 24 \n5656\n");
    result("begin var n:=0; foreach var x in Arr(1,2,3,4,5) do begin if x mod 2=0 then continue; if x>3 then break; n:=n+x; end; Writeln(n); end.","4\n");
    result("begin var n:=0; for var i:=1 to 2 do begin foreach var x in Arr(1,2,3) do begin if x=2 then break; Inc(n); end; end; repeat foreach var x in Arr(1,2) do begin Inc(n); continue; end; until n=4; Writeln(n); end.","4\n");
    result("function Sum(a:array of integer):integer; begin foreach var x in a do Result:=Result+x; end; function Recur(n:integer):integer; begin if n=0 then Exit(0); foreach var x in Arr(n) do Result:=x+Recur(n-1); end; begin Writeln(Sum(Arr(4,5)),' ',Recur(4)); end.","9 10\n");
    result("function Find:integer; begin foreach var x in Arr(1,2,3) do if x=2 then Exit(x); end; begin Writeln(Find); end.","2\n");
    result("type Point=record x:integer; end; begin var p:Point; p.x:=3; var a:=Arr(p,p); foreach var item in a do begin var editable:=item; editable.x:=99; Writeln(item.x); a[1].x:=7; end; Writeln(a[0].x,' ',a[1].x); end.","3\n7\n3 7\n");
    result("type P=record x:integer; end; begin var p:P; p.x:=3; var l:=new List<P>; l.Add(p); foreach var item in l do begin var editable:=item; editable.x:=99; Writeln(item.x); end; Writeln(l[0].x); end.","3\n3\n");
    result("begin var row:=Arr(1); var a:=Arr(row); foreach var item in a do item[0]:=9; Writeln(row[0]); end.","9\n");
    result("begin var inner:=new List<integer>; var outer:=new List<List<integer>>; outer.Add(inner); foreach var item in outer do begin item.Add(7); item[0]:=8; end; Writeln(inner[0]); end.","8\n");
    result("uses Collections; begin var node:=new SingleNode<integer>(1,nil); foreach var item in Arr(node) do item.Data:=7; Writeln(node.Data); end.","7\n");
    result("procedure Sum(var total:integer; a:array of integer); begin var x:integer; foreach x in a do total:=total+x; end; begin var n:=0; Sum(n,Arr(1,2,3)); Writeln(n); end.","6\n");
    result("begin var l:=new List<integer>; l.Add(1); foreach var x in l do begin l.Add(2); break; end; Writeln(l.Count); end.","2\n");
    result("function F(l:List<integer>):integer; begin foreach var x in l do begin l.Clear; Exit(x); end; end; begin var l:=new List<integer>; l.Add(7); Writeln(F(l),' ',l.Count); end.","7 0\n");
    for (const auto* operation:{"l.Add(2)","l.RemoveAt(0)","l.Clear","l[0]:=l[0]","Inc(l[0])","Sort(l)","FillByValue(l,9)"})
        runtime_error(std::string("begin var l:=new List<integer>; l.Add(1); foreach var x in l do ")+operation+"; end.","E_MODIFIED");
    runtime_error("begin var l:=new List<integer>; l.Add(1); var alias:=l; foreach var x in l do begin alias.Clear; continue; end; end.","E_MODIFIED");
    runtime_error("begin var l:=new List<string>; l.Add('x'); foreach var s in l do SetLength(l[0],2); end.","E_MODIFIED");
    runtime_error("begin var l:=new List<array of integer>; l.Add(Arr(1)); foreach var item in l do SetLength(l[0],2); end.","E_MODIFIED");
    runtime_error("type P=record x:integer; end; begin var l:=new List<P>; var p:P; l.Add(p); foreach var item in l do l[0].x:=7; end.","E_MODIFIED");
    error([&]{(void)run("begin var l:=new List<integer>; l.Add(1); foreach var x in l do Readln(l[0]); end.","9\n");},"E_MODIFIED");
    runtime_error("begin var a:array of integer; foreach var x in a do Write(x); end.","E_NIL");
    runtime_error("begin var l:List<integer>; foreach var x in l do Write(x); end.","E_NIL");
    for (const auto* source:{"begin foreach var x in Arr(1,2) do x:=9; end.","begin foreach var x in Arr(1,2) do Inc(x); end.",
        "begin foreach var x in Arr(1) do Readln(x); end.",
        "procedure P(var n:integer); begin end; begin foreach var x in Arr(1) do P(x); end.",
        "begin foreach var x:integer in Arr(2147483648) do Write(x); end.",
        "begin var s:string; foreach s in Arr(1) do Write(s); end.",
        "begin foreach var x in Arr(1) do foreach x in Arr(2) do Write(x); end.",
        "begin foreach var x in Arr(1) do for x:=1 to 2 do Write(x); end.",
        "begin for var x:=1 to 2 do foreach x in Arr(1) do Write(x); end.",
        "begin foreach var row in Arr(Arr(1)) do row:=Arr(2); end.",
        "type P=record x:integer; end; begin foreach var p in new P[1] do p.x:=7; end.",
        "var x:integer; procedure P; begin foreach x in Arr(1) do Write(x); end; begin P; end."}) compile_error(source,"E_TYPE");
    compile_error("begin foreach x in Arr(1) do Write(x); end.","E_NAME");
    compile_error("begin foreach var x in Arr(1) do Write(x); Writeln(x); end.","E_NAME");
    for (const auto* source:{"begin foreach var x in 1 do Write(x); end.",
        "begin foreach var x in nil do Write(x); end.",
        "uses Collections; begin foreach var x in new Stack<integer> do Write(x); end.",
        "uses Collections; begin foreach var x in new DynArray<integer> do Write(x); end.",
        "begin foreach var (x,y) in Arr(1) do Write(x); end.",
        "begin foreach var x in Arr(1) index i do Write(x); end."}) compile_error(source,"E_UNSUPPORTED");
    abc::ExecutionOptions limits; limits.instruction_limit=60;
    error([&]{(void)run("begin foreach var x in new integer[100] do continue; end.","",limits);},"E_LIMIT");
    limits={}; int polls=0; limits.cancelled=[&]{return ++polls>30;};
    error([&]{(void)run("begin foreach var x in new integer[100] do continue; end.","",limits);},"E_CANCELLED");
    limits={}; limits.allocation_limit=3;
    error([&]{(void)run("begin foreach var x in Arr(1) do Write(x); end.","",limits);},"E_LIMIT");
    auto program=abc::compile("begin foreach var x in Arr(1) do Write(x); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::iteration_check) instruction.operand=1;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin foreach var x in Arr(1) do Write(x); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::iteration_version) instruction.type=abc::Type::integer;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin foreach var x in Arr(1) do Write(x); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::iteration_check) instruction.op=abc::Op::iteration_version;
    error([&]{abc::verify(program);},"E_BYTECODE");
    std::istringstream input; std::ostringstream output;
    error([&]{abc::execute(abc::compile("begin Writeln('must not run'); foreach var x in Arr(1) do Inc(x); end."),input,output);},"E_TYPE");
    require(output.str().empty(),"Foreach variable types and readonly status are checked before output");
    try { (void)run("begin\n var l:=new List<integer>; l.Add(1);\n foreach var x in l do l.Clear;\nend."); require(false,"Expected modified list diagnostic"); }
    catch (const abc::Error& caught) {
        require(caught.code=="E_MODIFIED" && caught.location.line==3 && caught.russian=="Коллекция изменена во время перебора",
            "Modified List foreach error carries both languages and source position");
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
    runtime_error("begin var s:string := 'a'; var n := 0; while n < 30 do begin s := s + s; n := n + 1; end; end.", "E_LIMIT");
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
    compile_error("begin var n := 2; Inc(n,2147483648); end.", "E_RANGE");
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
    runtime_error("begin var s:string := 'a'; var n := 0; while n < 21 do begin s := Concat(s,s); Inc(n); end; end.", "E_LIMIT");
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
    result("begin var s:string := 'x'; if true then SetLength(s,3) else Delete(s,1,1); writeln('[',s,']'); end.", "[x  ]\n");
    result("begin var s := ''; var i := 0; while i<3 do begin Insert(IntToStr(i),s,2147483647); Inc(i); end; writeln(s); end.", "012\n");
    result("begin var s := 'abc'; SetLength(s,Length(s)+2); Insert(Copy(s,1,1),s,2); Delete(s,Length(s),1); writeln('[',s,']'); end.", "[aabc ]\n");
    result("begin var s:string := 'x'; SetLength(s,1048576); writeln(Length(s)); Delete(s,1,1048576); writeln(Length(s)); end.", "1048576\n0\n");
    compile_error("begin SetLength('abc',2); end.", "E_TYPE");
    compile_error("begin var s := 'abc'; SetLength(Trim(s),2); end.", "E_TYPE");
    compile_error("begin var n := 1; SetLength(n,2); end.", "E_TYPE");
    compile_error("begin var s := 'abc'; SetLength(s,2.0); end.", "E_TYPE");
    compile_error("begin var s := 'abc'; SetLength(s,2147483648); end.", "E_RANGE");
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
    runtime_error("begin var s:string:='x'; var i:=0; while i<20 do begin s:=s+s; Inc(i); end; Writeln(ChangeFileNameExtension(s,'.txt')); end.","E_LIMIT");
    auto program=abc::compile("begin Writeln(GetDir); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::call_builtin) { instruction.type=abc::Type::integer; break; }
    error([&] { abc::verify(program); },"E_BYTECODE");
}

void text_files() {
    namespace fs=std::filesystem;
    const auto root=fs::temp_directory_path();
    const auto directory=root/fs::path("ABC native text "+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!fs::create_directory(directory)) throw std::runtime_error("Cannot create text-file test directory");
    struct Cleanup {
        fs::path path,root;
        ~Cleanup() {
            std::error_code error;
            const auto target=fs::weakly_canonical(path,error), parent=fs::weakly_canonical(root,error);
            if (!error && target.parent_path()==parent && target.filename().generic_u8string().starts_with(u8"ABC native text ")) fs::remove_all(target,error);
        }
    } cleanup{directory,root};
    auto quote=[](const fs::path& path, bool native=false) {
        const auto bytes=native ? path.u8string() : path.generic_u8string(); std::string result="'";
        for (auto c:bytes) { result+=static_cast<char>(c); if (c==u8'\'') result+='\''; }
        return result+"'";
    };
    const auto file=quote(directory/"text.txt"), other=quote(directory/"other.txt");
    const std::string unit_newline=
#ifdef _WIN32
        "\r\n";
#else
        "\n";
#endif
    result("uses Events,PointRect,FilesOperations; begin WriteStringToFile("+file+",'one'#13'two'#10'three'#13#10'four'); Write(ReadFileToEnd("+file+")); end.","one"+unit_newline+"two"+unit_newline+"three"+unit_newline+"four"+unit_newline);
    result("uses FilesOperations; begin FilesOperations.WriteStringToFile("+file+",'abc'); Write(FilesOperations.ReadFileToEnd("+file+")); end.","abc"+unit_newline);
    result("uses FilesOperations; begin WriteStringToFile("+file+",''); Writeln(ReadFileToEnd("+file+")=''); WriteStringToFile("+file+",#13#10); Write(ReadFileToEnd("+file+")); end.","True\n"+unit_newline);
    result("uses FilesOperations; begin var s:string:=nil; WriteStringToFile("+file+",s); Writeln(ReadAllText("+file+")); WriteStringToFile("+file+",'Я🌍'); Writeln(ReadAllText("+file+")); end.","nil\nЯ🌍\n");
    result("uses FilesOperations; function ReadFileToEnd(name:string):string; begin Result:='local'; end; begin WriteStringToFile("+file+",'native'); Writeln(ReadFileToEnd("+file+"),' ',Pos('native',FilesOperations.ReadFileToEnd("+file+"))=1); end.","local True\n");
    result("uses FilesOperations; type R=record ReadFileToEnd:string; end; begin var FilesOperations:R; FilesOperations.ReadFileToEnd:='field'; Writeln(FilesOperations.ReadFileToEnd); end.","field\n");
    result("uses FilesOperations; begin MkDir("+quote(directory/"unit-child")+"); ChDir("+quote(directory/"unit-child")+"); WriteStringToFile('relative.txt','local'); Writeln(ReadAllText('relative.txt')); end.","local\n");
    runtime_error("uses FilesOperations; begin Writeln(ReadFileToEnd("+quote(directory/"missing-unit.txt")+")); end.","E_IO");
    runtime_error("uses FilesOperations; begin WriteStringToFile(nil,'bad'); end.","E_IO");
    runtime_error("uses FilesOperations; begin Writeln(ReadFileToEnd(nil)); end.","E_IO");
    runtime_error("uses FilesOperations; begin WriteStringToFile("+file+",StringOfChar('Я',1048576)); end.","E_LIMIT");
    result("uses FilesOperations; begin WriteStringToFile("+file+",StringOfChar(#10,524288)); Writeln(Length(ReadFileToEnd("+file+"))); end.",std::to_string(524288*unit_newline.size())+"\n");
#ifdef _WIN32
    runtime_error("uses FilesOperations; begin WriteStringToFile("+file+",StringOfChar(#10,1048576)); ReadFileToEnd("+file+"); end.","E_LIMIT");
#else
    result("uses FilesOperations; begin WriteStringToFile("+file+",StringOfChar(#10,1048576)); Writeln(Length(ReadFileToEnd("+file+"))); end.","1048576\n");
#endif
    runtime_error("uses FilesOperations; begin WriteAllText("+file+",'x'); var files:=new Text[64]; for var i:=0 to 63 do files[i]:=OpenRead("+file+"); Writeln(ReadFileToEnd("+file+")); end.","E_LIMIT");
    result("uses FilesOperations; begin var name:="+quote(directory/fs::path(u8"Заметка 🌍.txt"))+"; WriteStringToFile(name,'Unicode'); Writeln(ReadAllText(name),' ',DeleteFile(name)); end.","Unicode True\n");
    try { (void)run("uses FilesOperations;\nbegin\nReadFileToEnd("+quote(directory/"absent-unit.txt")+");\nend."); require(false,"Expected FilesOperations IO diagnostic"); }
    catch (const abc::Error& caught) { require(caught.code=="E_IO" && caught.location.line==3 && !caught.russian.empty(),"FilesOperations retains call-site location and bilingual IO errors"); }
    result("begin Writeln(PABCSystem.Abs(-4),' ',PABCSystem.Chr(65)); end.","4 A\n");
    for (const auto* source:{"begin Writeln(ReadFileToEnd('x')); end.","begin WriteStringToFile('x','y'); end.",
        "begin Writeln(FilesOperations.ReadFileToEnd('x')); end.","uses Events; begin Writeln(Events.Abs(1)); end.",
        "uses FilesOperations; begin Writeln(FilesOperations.Abs(1)); end."}) compile_error(source,"E_NAME");
    for (const auto* source:{"uses FilesOperations; begin Writeln(ReadFileToEnd(1)); end.",
        "uses FilesOperations; begin WriteStringToFile('x',1); end.","uses FilesOperations; begin FilesOperations.WriteStringToFile('x'); end."}) compile_error(source,"E_TYPE");
    {
        std::istringstream input; std::ostringstream output;
        error([&]{abc::execute(abc::compile("begin Writeln('must not run'); WriteStringToFile('x','y'); end."),input,output);},"E_NAME");
        require(output.str().empty(),"Unit imports are checked before execution");
    }
    result("begin var f:=OpenWrite("+file+"); f.Write(' ',#13,#10,'Я','🌍'); f.Close; f.Reset; Writeln(Ord(ReadChar(f)),' ',Ord(f.ReadChar),' ',Ord(f.ReadChar),' ',f.ReadChar); Writeln(Ord(f.ReadChar),' ',Ord(f.ReadChar),' ',Ord(f.ReadChar),' ',f.Eof); f.Close; end.","32 13 10 Я\n55356 57101 65535 True\n");
    result("begin var f:=OpenWrite("+file+"); f.Writeln('ax'); f.Writeln('by'); f.Close; f.Reset; Writeln(ReadlnChar(f),' ',f.ReadlnChar,' ',f.Eof); f.Close; end.","a b True\n");
    result("begin var f:=OpenWrite("+file+"); f.Write('Яx 65535'); f.Close; f.Reset; var c:char; var w:word; Read(f,c); Writeln(c); Read(f,c); Read(f,w); Writeln(c,' ',w); f.Close; end.","Я\nx 65535\n");
    result("begin var f:=OpenWrite("+file+"); f.Write(#55356,#57101); f.Close; Writeln(ReadAllText("+file+")); end.","🌍\n");
    result("begin var f:=OpenWrite("+file+"); f.Writeln(byte(255),' ',shortint(-128),' ',smallint(-32768)); f.Close; f.Reset; var b:byte; var s:shortint; var n:smallint; Read(f,b,s,n); Writeln(b,' ',s,' ',n); f.Close; end.","255 -128 -32768\n");
    runtime_error("begin WriteAllText("+file+",'256'); var f:=OpenRead("+file+"); var b:byte; Read(f,b); end.","E_OVERFLOW");
    runtime_error("begin WriteAllText("+file+",'-129'); var f:=OpenRead("+file+"); var s:shortint; Read(f,s); end.","E_OVERFLOW");
    runtime_error("begin WriteAllText("+file+",'32768'); var f:=OpenRead("+file+"); var n:smallint; Read(f,n); end.","E_OVERFLOW");
    result("begin var f:=OpenWrite("+file+"); f.Writeln(MaxLongWord,' ',MaxUInt64); f.Close; f.Reset; var w:longword; var u:uint64; Read(f,w,u); Writeln(w,' ',u); f.Close; end.","4294967295 18446744073709551615\n");
    runtime_error("begin WriteAllText("+file+",'18446744073709551616'); var f:=OpenRead("+file+"); var u:uint64; Read(f,u); end.","E_OVERFLOW");
    runtime_error("begin WriteAllText("+file+",'-1'); var f:=OpenRead("+file+"); var u:uint64; Read(f,u); end.","E_OVERFLOW");
    runtime_error("begin WriteAllText("+file+",'4294967296'); var f:=OpenRead("+file+"); var w:longword; Read(f,w); end.","E_OVERFLOW");
    result("begin var f:Text; Assign(f,"+file+"); Rewrite(f); Writeln(f,12,' ',34,' tail'); Close(f); Reset(f); var a,b:integer; var s:string; Read(f,a,b); Read(f,s); Writeln(a,' ',b,' [',s,'] ',Eoln(f),' ',Eof(f)); Readln(f,s); Writeln('[',s,'] ',Eof(f)); Readln(f,s); Writeln('[',s,']'); Close(f); end.","12 34 [ tail] True False\n[] True\n[]\n");
    result("procedure Add(f:Text); begin Writeln(f,'добавлено 🌍'); end; begin var f:=OpenAppend("+file+"); var alias:=f; Add(alias); Close(alias); var g:=OpenRead("+file+"); Readln(g); Writeln(ReadString(g)); Close(g); end.","добавлено 🌍\n");
    result("begin var f:=new TextFile; Rewrite(f,"+file+"); Write(f,'abc'); Rewrite(f); Write(f,'X'); Close(f); Writeln(ReadAllText("+file+")); Rewrite(f); Write(f,'abcdef'); Flush(f); Rewrite(f); Write(f,'XY'); Close(f); Writeln(ReadAllText("+file+")); end.","abcX\nXYcdef\n");
    result("begin var f:Text; Write(f); Read(f); Flush(f); AssignFile(f,"+file+"); Rewrite(f); Writeln(f,1,' ',2147483648,' ',1.5,' ',True,' word'); CloseFile(f); Reset(f); Writeln(ReadInteger(f),' ',ReadInt64(f),' ',ReadReal(f),' ',ReadBoolean(f),' ',ReadLexem(f)); Writeln(SeekEoln(f)); Readln(f); Writeln(SeekEof(f)); Close(f); end.","1 2147483648 1.5 True word\nTrue\nTrue\n");
    result("begin var f:=OpenWrite("+file+"); Writeln(f,'a'); Writeln(f,'b'); Close(f); Reset(f); var s,t:string; Readln(f,s,t); Writeln('[',s,'] [',t,']'); Writeln(ReadlnString(f)); Close(f); end.","[a] []\nb\n");
    result("type Holder=record f:Text; end; begin var h:Holder; var a:=new Text[2]; Assign(h.f,"+file+"); Rewrite(h.f); Writeln(h.f,'record'); Close(h.f); Assign(a[1],"+other+"); Rewrite(a[1]); Writeln(a[1],'array'); Close(a[1]); Writeln(ReadAllLines("+file+")[0],' ',ReadAllLines("+other+")[0]); end.","record array\n");
    result("function Make:Text; begin Rewrite(Result,"+file+"); end; begin var f:=Make; Writeln(f,'result'); Close(f); Writeln(ReadAllLines("+file+")[0]); end.","result\n");
    result("begin var f:Text; Assign(f,"+file+"); MkDir("+quote(directory/"child")+"); ChDir("+quote(directory/"child")+"); Rewrite(f); Writeln(f,'bound'); Close(f); Writeln(ReadAllLines("+file+")[0],' ',FileExists('text.txt')); end.","bound False\n");
    result("begin var f:=OpenWrite("+file+"); Writeln(f,'automatic close'); end.","");
    result("begin Writeln(ReadAllLines("+file+")[0]); end.","automatic close\n");
    const auto path=directory/"text.txt";
    { std::ofstream out(path,std::ios::binary); out << "\xef\xbb\xbf" "first\rsecond\r\nthird\n"; }
    result("begin var f:=OpenRead("+file+"); while not Eof(f) do Writeln('[',ReadString(f),']'); Close(f); end.","[first]\n[second]\n[third]\n");
    { std::ofstream out(path,std::ios::binary); out << "  \t\r\n42  \t\n"; }
    result("begin var f:=OpenRead("+file+"); Writeln(SeekEoln(f),' ',Eof(f)); Readln(f); Writeln(ReadlnInteger(f),' ',Eof(f)); Close(f); end.","True False\n42 True\n");
    { std::ofstream out(path,std::ios::binary); out << "\xe3\x80\x80" "42 9223372036854775807 -1.5e2 FALSE"; }
    result("begin var f:=OpenRead("+file+"); var n:integer; var big:int64; var r:real; var b:boolean; Read(f,n,big,r,b); Writeln(n,' ',big,' ',r,' ',b); Close(f); end.","42 9223372036854775807 -150 False\n");
    { std::ofstream out(path,std::ios::binary); out.write("42\0\0\n",5); }
    result("begin var f:=OpenRead("+file+"); Writeln(ReadInteger(f)); Close(f); end.","42\n");
    { std::ofstream out(path,std::ios::binary); out << "2147483648"; }
    runtime_error("begin var f:=OpenRead("+file+"); var n:integer; Read(f,n); end.","E_OVERFLOW");
    { std::ofstream out(path,std::ios::binary); out << "999999999999999999999999x"; }
    runtime_error("begin var f:=OpenRead("+file+"); ReadInteger(f); end.","E_FORMAT");
    { std::ofstream out(path,std::ios::binary); out << "maybe"; }
    runtime_error("begin var f:=OpenRead("+file+"); var b:boolean; Readln(f,b); end.","E_FORMAT");
    { std::ofstream out(path,std::ios::binary); }
    runtime_error("begin var f:=OpenRead("+file+"); ReadInteger(f); end.","E_EOF");
    runtime_error("begin var f:Text; Reset(f); end.","E_IO");
    runtime_error("begin var f:Text; Writeln(f,1); end.","E_IO");
    runtime_error("begin var f:Text; Close(f); end.","E_IO");
    runtime_error("begin var f:=OpenWrite("+file+"); Writeln(f,1); Close(f); Close(f); end.","E_IO");
    runtime_error("begin var f:=OpenWrite("+file+"); Eof(f); end.","E_IO");
    runtime_error("begin var f:=OpenWrite("+file+"); var n:integer; Read(f,n); end.","E_IO");
    runtime_error("begin var f:=OpenRead("+file+"); Writeln(f,'bad'); end.","E_IO");
    { std::ofstream out(path,std::ios::binary); out << "\xff"; }
    runtime_error("begin var f:=OpenRead("+file+"); end.","E_ENCODING");
    { std::ofstream out(path,std::ios::binary); out << std::string(1'048'577,'x'); }
    runtime_error("begin var f:=OpenRead("+file+"); end.","E_LIMIT");
    runtime_error("begin var f:=OpenAppend("+file+"); end.","E_LIMIT");
    runtime_error("begin var f:=OpenWrite("+file+"); var s:string:='x'; var i:=0; while i<20 do begin s:=s+s; Inc(i); end; Write(f,s); Writeln(f); end.","E_LIMIT");
    result("begin var f:=OpenWrite("+file+"); var s:string:='a'; var i:=0; while i<11 do begin s:=s+s; Inc(i); end; Write(f,s); Flush(f); Close(f); Writeln(Length(ReadAllText("+file+"))); end.","2048\n");
    result("begin var f:=OpenWrite("+file+"); Write(f,Copy('🌍',1,1)); Write(f,Copy('🌍',2,1)); Close(f); Writeln(ReadAllText("+file+")); end.","🌍\n");
    runtime_error("begin var files:=new List<Text>; var i:=0; while i<65 do begin files.Add(OpenRead("+file+")); Inc(i); end; end.","E_LIMIT");
    result("begin var files:=new List<Text>; var i:=0; while i<200 do begin var f:=OpenRead("+file+"); Close(f); files.Add(f); Inc(i); end; Writeln(files.Count); end.","200\n");
    runtime_error("uses Collections; begin var m:=new AssocArray<string,Text>; Reset(m['absent']); end.","E_NIL");
    for (auto source:{"begin var f:Text; var n:integer:=f; end.","begin var f:Text; f:=nil; end.",
         "begin var f:Text; Writeln(f=f); end.","begin var f:Text; var n:integer; Read(f,1); end.",
         "begin var f:Text; Writeln(1,f); end.","begin var f:Text; Readln(1); end.",
         "begin var a:=new Text[2]; Sort(a); end.","begin var f:Text; Reset(f,1); end.",
         "begin var f:=new Text(1); end.","begin var f:Text; Read(f,f); end."}) compile_error(source,"E_TYPE");
    compile_error("uses Collections; begin var s:=new Stack<Text>; Writeln(s.ToString); end.","E_UNSUPPORTED");
    error([&]{ (void)run("begin var f:=OpenWrite("+other+"); Writeln(f,'must not run'); var n:integer:='bad'; end."); },"E_TYPE");
    result("begin Writeln(ReadAllLines("+other+")[0]); end.","array\n");
    auto program=abc::compile("begin var f:Text; Rewrite(f,'x'); Writeln(f,1); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::file_output) { instruction.type=abc::Type::integer; break; }
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin var f:Text; var n:integer; Readln(f,n); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::file_read) { instruction.type=abc::Type::real; break; }
    error([&]{abc::verify(program);},"E_BYTECODE");
    try { (void)run("begin\n var f:Text;\n Writeln(f,1);\nend."); require(false,"Expected file error"); }
    catch (const abc::Error& caught) {
        require(caught.code=="E_IO" && caught.location.line==3 && !caught.russian.empty(),"Text file error retains source line and both languages");
    }
    result("begin var f:Text; f.Write; f.Print(); f.Flush; end.","");
    result("begin var f:=OpenWrite("+file+"); f.Write(1,' ',2); f.Writeln; f.Print(3,4); f.Println('a','b'); f.Println(); f.Writeln('last'); f.Flush(); f.Close; f.Reset(); Writeln('[',f.ReadString,']'); Writeln('[',f.ReadlnString(),']'); Writeln('[',f.ReadlnString,']'); Writeln('[',f.ReadString(),'] ',f.Eof()); f.Close(); end.","[1 2]\n[3 4 a b]\n[]\n[last] True\n");
    result("begin var f:=OpenWrite("+file+"); f.Writeln(' 42 rest'); f.Writeln('1.5'); f.Writeln('TRUE'); f.Writeln('word tail'); f.Close; f.Reset; Writeln(f.ReadInteger,' [',f.ReadString,']'); Writeln(f.ReadlnReal(),' ',f.ReadlnBoolean); Writeln(f.ReadWord); Writeln('[',f.ReadlnString,'] ',f.SeekEof); f.Close; f.Append; f.Println('добавлено','🌍'); f.Close; f.Reset; f.Readln; f.Readln(); f.Readln; f.Readln; Writeln(f.ReadString); f.Close; end.","42 [ rest]\n1.5 True\nword\n[ tail] True\nдобавлено 🌍\n");
    result("begin var f:Text; Assign(f,"+quote(directory/"missing.txt")+"); Writeln(f.Name,' ',f.FullName()="+quote(directory/"missing.txt",true)+"); ChDir("+quote(directory/"child")+"); Writeln(f.FullName="+quote(directory/"missing.txt",true)+"); end.","missing.txt True\nTrue\n");
    { std::ofstream out(path,std::ios::binary); out << "12 rest\r\nnext\rtail\n"; }
    result("begin var f:=OpenRead("+file+"); Writeln(f.ReadInteger); Writeln('[',f.ReadToEnd(),']'); Writeln(f.Eof,' [',f.ReadToEnd,']'); f.Close; Writeln(f.Name); end.","12\n[ rest\r\nnext\rtail\n]\nTrue []\ntext.txt\n");
    result("begin var f:=OpenRead("+file+"); Writeln(f.ReadlnInteger,' ',f.Eoln); f.Readln; Writeln(f.ReadlnString,' ',f.SeekEoln()); f.Close; end.","12 False\ntail True\n");
    result("var calls:integer; var f:Text; function GetFile:Text; begin Inc(calls); Result:=f; end; function Change:integer; begin Write(f,'pre'); Result:=2; end; begin f:=OpenWrite("+file+"); GetFile().Print(1,Change); f.Close; Writeln(ReadAllText("+file+"),' ',calls); end.","pre1 2  1\n");
    result("var f,g:Text; function Change:integer; begin f:=g; Result:=2; end; begin f:=OpenWrite("+file+"); g:=OpenWrite("+other+"); var saved:=f; f.Println(1,Change); saved.Close; g.Close; Writeln(ReadAllLines("+file+")[0],' [',ReadAllText("+other+"),']'); end.","1 2 []\n");
    result("type Holder=record f:Text; end; begin var h:Holder; h.f:=OpenRead("+file+"); var a:=new Text[1]; a[0]:=h.f; Writeln(a[0].ReadInteger,' ',h.f.ReadlnInteger); a[0].Close; end.","1 2\n");
    result("begin var f:=OpenWrite("+file+"); var s:string:=nil; f.Write(1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6); f.Print(s,'',true,1.5); f.Close; Writeln(ReadAllText("+file+")); end.","1234567890123456nil  True 1.5 \n");
    for (const auto* source:{"begin var f:Text; f.Writeln(Arr(1)); end.","begin var f:Text; f.Print(f); end.",
        "begin var f:Text; var n:integer:=f.Write(1); end.","begin var f:Text; f.Readln(1); end.",
        "begin var f:Text; f.Reset('x'); end.","begin var f:Text; f.Name:='bad'; end.",
        "begin var f:Text; Writeln(f.FullName(1)); end.","begin var f:Text; f.ReadWord(1); end.",
        "begin var f:Text; f.Eof:=true; end."}) compile_error(source,"E_TYPE");
    for (const auto* method:{"Lines","ReadInt64","Missing"})
        compile_error(std::string("begin var f:Text; f.")+method+"; end.","E_NAME");
    runtime_error("begin var f:Text; Writeln(f.Name); end.","E_NIL");
    runtime_error("begin var f:Text; Writeln(f.FullName); end.","E_NIL");
    runtime_error("begin var f:Text; Writeln(f.ReadToEnd); end.","E_NIL");
    runtime_error("begin var f:=OpenWrite("+file+"); Writeln(f.ReadToEnd); end.","E_NIL");
    runtime_error("begin var f:=OpenRead("+file+"); f.Close; Writeln(f.ReadToEnd); end.","E_NIL");
    runtime_error("begin var f:=OpenRead("+file+"); f.Println(1); end.","E_IO");
    runtime_error("begin var f:Text; f.Writeln; end.","E_IO");
    runtime_error("uses Collections; begin var m:=new AssocArray<string,Text>; m['missing'].Close; end.","E_NIL");
    runtime_error("uses Collections; begin var m:=new AssocArray<string,Text>; m['missing'].Write; end.","E_NIL");
    runtime_error("begin var f:=OpenWrite("+file+"); var s:string:='x'; var i:=0; while i<20 do begin s:=s+s; Inc(i); end; f.Print(s); end.","E_LIMIT");
    result("begin var f:=OpenWrite("+other+"); f.Write('preserved'); f.Close; end.","");
    compile_error("begin var f:=OpenWrite("+other+"); f.Writeln('must not run'); f.ReadInteger(1); end.","E_TYPE");
    result("begin Writeln(ReadAllText("+other+")); end.","preserved\n");
    program=abc::compile("begin var f:Text; f.Println(1); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::file_output) instruction.operand=68;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin var f:Text; f.Print(1); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::file_output) instruction.type=abc::Type::integer;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin var f:Text; f.Print(1); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::file_output) instruction.operand=10;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin var f:Text; f.Print(1); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::push) { instruction.op=abc::Op::make_default; instruction.type=abc::Type::text_file; instruction.operand=0; }
    error([&]{abc::verify(program);},"E_BYTECODE");
    try { (void)run("begin\n var f:Text;\n Writeln(f.ReadToEnd);\nend."); require(false,"Expected Text method error"); }
    catch (const abc::Error& caught) {
        require(caught.code=="E_NIL" && caught.location.line==3 && caught.russian=="Нельзя обратиться к пустой ссылке nil",
            "Text member error retains both languages and source line");
    }
}

void text_file_changes() {
    namespace fs=std::filesystem;
    const auto root=fs::temp_directory_path();
    const auto directory=root/fs::path("ABC text changes "+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!fs::create_directory(directory)) throw std::runtime_error("Cannot create file-change test directory");
    struct Cleanup {
        fs::path path,root;
        ~Cleanup() {
            std::error_code error;
            const auto target=fs::weakly_canonical(path,error), parent=fs::weakly_canonical(root,error);
            if (!error && target.parent_path()==parent && target.filename().generic_u8string().starts_with(u8"ABC text changes ")) fs::remove_all(target,error);
        }
    } cleanup{directory,root};
    auto quote=[](const fs::path& path) {
        const auto bytes=path.generic_u8string(); std::string result="'";
        for (auto c:bytes) { result+=static_cast<char>(c); if (c==u8'\'') result+='\''; }
        return result+"'";
    };
    const auto source=directory/"source.txt", destination=directory/"destination.txt";
    const auto from=quote(source), to=quote(destination), folder=quote(directory);
    const auto prefix="begin WriteAllText("+from+",'source'); var f:Text; Assign(f,"+from+"); ";
    result(prefix+"Rename(f,"+to+"); Writeln(FileExists("+from+"),' ',FileExists("+to+"),' ',f.Name); f.Erase; Writeln(ReadAllText("+to+")); Assign(f,"+to+"); Erase(f); f.Erase(); Writeln(FileExists("+to+"),' ',f.Name); end.","False True source.txt\nsource\nFalse destination.txt\n");
    result(prefix+"f.Rename("+from+"); Writeln(ReadAllText("+from+")); f.Erase; end.","source\n");
    result("procedure MoveFile(f:Text; name:string); begin f.Rename(name); end; begin WriteAllText("+from+",'routines'); var f:Text; Assign(f,"+from+"); var alias:=f; MoveFile(alias,"+to+"); Writeln(f.Name,' ',alias.Name,' ',ReadAllText("+to+")); Assign(alias,"+to+"); f.Erase; end.","source.txt source.txt routines\n");
    result("var calls:integer; var f:Text; function GetFile:Text; begin Inc(calls); Result:=f; end; begin WriteAllText("+from+",'once'); Assign(f,"+from+"); GetFile().Rename("+to+"); Writeln(calls,' ',ReadAllText("+to+")); Assign(f,"+to+"); f.Erase; end.","1 once\n");
    fs::create_directory(directory/"child");
    result(prefix+"ChDir("+quote(directory/"child")+"); f.Rename('relative.txt'); Writeln(FileExists('relative.txt'),' ',FileExists("+from+"),' ',f.Name); Assign(f,'relative.txt'); f.Erase; end.","True False source.txt\n");
    for (const auto* operation:{"Erase(f)","f.Erase","Rename(f,'new.txt')","f.Rename('new.txt')"})
        runtime_error(std::string("begin var f:Text; ")+operation+"; end.","E_IO");
    runtime_error("uses Collections; begin var m:=new AssocArray<string,Text>; Erase(m['missing']); end.","E_NIL");
    runtime_error("uses Collections; begin var m:=new AssocArray<string,Text>; m['missing'].Rename('new.txt'); end.","E_NIL");
    runtime_error("begin var f:Text; Assign(f,"+from+"); f.Rename("+to+"); end.","E_IO");
    runtime_error(prefix+"WriteAllText("+to+",'occupied'); f.Rename("+to+"); end.","E_IO");
    result("begin Writeln(ReadAllText("+from+"),' ',ReadAllText("+to+")); end.","source occupied\n");
    for (const auto& target:{std::string("nil"),std::string("''"),folder,quote(directory/"absent"/"file.txt")})
        runtime_error(prefix+"Rename(f,"+target+"); end.","E_IO");
    runtime_error(prefix+"var bad:string; SetLength(bad,1); f.Rename(bad); end.","E_IO");
    runtime_error("begin var f:Text; Assign(f,"+folder+"); f.Erase; end.","E_IO");
    runtime_error("begin var f:Text; Assign(f,"+folder+"); f.Rename("+quote(directory/"folder-copy")+"); end.","E_IO");
    require(fs::is_directory(directory/"child"),"Erase and Rename never remove directories");
    { std::ofstream out(source,std::ios::binary); out << std::string(1'048'577,'\xff'); }
    result("begin DeleteFile("+to+"); var f:Text; Assign(f,"+from+"); f.Rename("+to+"); Writeln(FileExists("+from+"),' ',FileExists("+to+")); end.","False True\n");
    { std::ifstream input(destination,std::ios::binary); const std::string bytes((std::istreambuf_iterator<char>(input)),{});
      require(bytes==std::string(1'048'577,'\xff'),"Text Rename preserves binary files beyond the text IO limit"); }
    result("begin var f:Text; Assign(f,"+to+"); f.Erase; end.","");
    runtime_error(prefix+"f.Rename("+to+"); f.Reset; end.","E_IO");
    result("begin Writeln(ReadAllText("+to+")); DeleteFile("+to+"); end.","source\n");
    result(prefix+"var reader:=OpenRead("+from+"); Assign(reader,"+to+"); WriteAllText("+to+",'other'); reader.Erase; Writeln(FileExists("+from+"),' ',FileExists("+to+"),' ',reader.ReadToEnd); reader.Close; f.Erase; end.","True False source\n");
#ifdef _WIN32
    runtime_error(prefix+"var reader:=OpenRead("+from+"); f.Erase; end.","E_IO");
    runtime_error(prefix+"var reader:=OpenRead("+from+"); f.Rename("+to+"); end.","E_IO");
    result(prefix+"var reader:=OpenRead("+from+"); Writeln(DeleteFile("+from+"),' ',RenameFile("+from+","+to+")); reader.Close; f.Erase; end.","False False\n");
    runtime_error(prefix+"var reader:=OpenRead("+from+"); Assign(reader,"+to+"); f.Erase; end.","E_IO");
    result("begin var f:=OpenWrite("+from+"); WriteAllText("+to+",'other'); Assign(f,"+to+"); f.Rewrite; Writeln(DeleteFile("+from+"),' ',DeleteFile("+to+")); f.Close; Writeln(DeleteFile("+from+")); end.","False True\nTrue\n");
    result(prefix+"var reader:=OpenRead("+from+"); Writeln(RenameFile("+from+","+from+")); reader.Close; f.Erase; end.","True\n");
    result("begin WriteAllText("+quote(directory/"child"/"held.txt")+",'held'); var reader:=OpenRead("+quote(directory/"child"/"held.txt")+"); Writeln(RenameDirectory("+quote(directory/"child")+","+quote(directory/"moved-child")+")); reader.Close; Writeln(RenameDirectory("+quote(directory/"child")+","+quote(directory/"moved-child")+")); Writeln(RenameDirectory("+quote(directory/"moved-child")+","+quote(directory/"child")+")); end.","False\nTrue\nTrue\n");
    { std::ofstream out(source); out << "read only"; }
    require(::SetFileAttributesW(source.c_str(),FILE_ATTRIBUTE_READONLY)!=0,"Prepare readonly erase test");
    runtime_error("begin var f:Text; Assign(f,"+from+"); f.Erase; end.","E_IO");
    require(::SetFileAttributesW(source.c_str(),FILE_ATTRIBUTE_NORMAL)!=0,"Restore readonly test file");
#else
    result(prefix+"var reader:=OpenRead("+from+"); f.Erase; Writeln(FileExists("+from+"),' ',reader.ReadToEnd); reader.Close; end.","False source\n");
    result(prefix+"var reader:=OpenRead("+from+"); f.Rename("+to+"); Writeln(FileExists("+from+"),' ',ReadAllText("+to+"),' ',reader.ReadToEnd); reader.Close; Assign(f,"+to+"); f.Erase; end.","False source source\n");
    std::error_code link_error;
    fs::create_symlink(directory/"missing-target",directory/"dangling",link_error);
    if (!link_error) {
        result("begin var f:Text; Assign(f,"+quote(directory/"dangling")+"); f.Erase; end.","");
        require(!fs::is_symlink(fs::symlink_status(directory/"dangling")),"Erase removes dangling links without touching targets");
    }
#endif
    result("begin var f:Text; Assign(f,"+from+"); f.Erase; f.Erase; end.","");
    for (const auto* program:{"begin Erase('x'); end.","begin var f:Text; Erase(f,1); end.",
        "begin var f:Text; Rename(f,1); end.","begin var f:Text; f.Erase(1); end.",
        "begin var f:Text; f.Rename; end.","begin var f:Text; f.Rename('a','b'); end.",
        "begin var f:Text; var n:integer:=f.Erase; end."}) compile_error(program,"E_TYPE");
    result("begin WriteAllText("+from+",'preserved'); end.","");
    error([&]{(void)run("begin var f:Text; Assign(f,"+from+"); f.Erase; f.Rename(1); end.");},"E_TYPE");
    result("begin Writeln(ReadAllText("+from+")); DeleteFile("+from+"); end.","preserved\n");
    auto program=abc::compile("begin var f:Text; Rename(f,'x'); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::push) { instruction.type=abc::Type::integer; program.constants[instruction.operand]=std::int32_t{1}; }
    error([&]{abc::verify(program);},"E_BYTECODE");
    for (const auto* operation:{"f.Erase","f.Rename('new.txt')"}) {
        try { (void)run("begin\n var f:Text; Assign(f,"+folder+");\n "+operation+";\nend."); require(false,"Expected file-change error"); }
        catch (const abc::Error& caught) {
            require(caught.code=="E_IO" && caught.location.line==3 && !caught.russian.empty()
                && std::string(caught.what()).starts_with("Cannot "),"Erase and Rename report both languages and source line");
        }
    }
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
void random_library() {
    result("begin Randomize(1); Writeln(Random(100),' ',Random(100),' ',Random(100),' ',Random(100),' ',Random(100)); SetRandomSeed(1); Writeln(Random(100)); end.","24 11 46 77 65\n24\n");
    result("begin Randomize(1); Writeln(Abs(Random-0.24866858415709278)<1e-16); Randomize(0); Writeln(Random(100)); end.","True\n72\n");
    result("function Take:integer; begin Result:=Random(100); end; begin Randomize(1); Writeln(Take(),' ',Random(100),' ',Take()); PABCSystem.Randomize(1); Writeln(PABCSystem.Random(100)); end.","24 11 46\n24\n");
    result("begin Randomize(1); Writeln(Random(0),' ',Random(100)); Randomize(1); Writeln(Random(1),' ',Random(100)); Randomize(1); Writeln(Random(2,2),' ',Random(100)); Randomize(1); Writeln(Random('a','a'),' ',Random(100)); end.","0 11\n0 11\n2 11\na 11\n");
    result("begin Randomize(-1); var a:=Random(100); Randomize(1); Writeln(a=Random(100)); Randomize(-2147483648); var b:=Random(100); Randomize(MaxInt); Writeln(b=Random(100)); end.","True\nTrue\n");
    result("begin Randomize(1); Writeln(Random(-2147483648,2147483646)); Randomize(1); var a:=Random(3,1); Randomize(1); Writeln(a=Random(1,3)); end.","-534011720\nTrue\n");
    result("begin Randomize(1); Writeln(Random('Я','А')); Randomize(1); var n:=Ord(Random(#0,#65535)); Writeln(n<=65535); end.","З\nTrue\n");
    result("begin Randomize(1); var x:=Random(-5.0); Randomize(1); Writeln(x=Random(5.0)); Randomize(1); var y:=Random(5.0,1.0); Randomize(1); Writeln(y=Random(1.0,5.0)); Randomize(1); Writeln(Random(2.0,2.0),' ',Random(100)); end.","True\nTrue\n2 11\n");
    result("begin Randomize(1); Writeln(RandomReal(0,1),' ',RandomReal(0,1,3)); Randomize(1); Writeln(RandomReal(2.736,2.737)); Randomize(1); Writeln(RandomReal(10,0)); Randomize(1); var x:=RandomReal(2,4,-1); Randomize(1); Writeln(x=Random(2.0,4.0)); end.","0.25 0.111\n2.736\n0\nTrue\n");
    result("begin Randomize; var valid:=true; for var i:=1 to 200 do begin var n:=Random(-2147483648,2147483646); valid:=valid and (n>=-2147483648) and (n<=2147483646); var x:=Random; valid:=valid and (x>=0) and (x<1); end; Writeln(valid); Randomize(); Writeln(Random(1)); end.","True\n0\n");
    result("begin Randomize(1); var a:=new integer[3]; for var i:=0 to 2 do a[i]:=Random(100); Writeln(a[0],' ',a[1],' ',a[2]); end.","24 11 46\n");
    result("uses Arrays; begin Randomize(1); var a:=CreateRandomIntegerArray(3); Writeln(a[0],' ',a[1],' ',a[2]); Randomize(1); var b:=ArrRandomInteger(3); Writeln(b[0],' ',b[1],' ',b[2]); Randomize(1); FillByRandom(b); Writeln(b[0],' ',b[1],' ',b[2]); end.","24 11 46\n25 11 47\n24 11 46\n");
    result("uses Arrays; begin Randomize(1); var a:=Arrays.CreateRandomIntegerArray(2,10,12); Writeln(a[0],' ',a[1]); Randomize(1); Arrays.FillByRandom(a,12,10); Writeln(a[0],' ',a[1]); end.","10 10\n10 10\n");
    result("uses Arrays; begin Randomize(1); var a:=CreateRandomRealArray(2,10,20); Randomize(1); var x:=Random*10; var y:=Random*10; Writeln(a[0]=x,' ',a[1]=y,' ',a[0]<10); Randomize(1); FillByRandom(a,20,10); Writeln(a[0]<0); end.","True True True\nTrue\n");
    result("begin Randomize(1); var a:=ArrRandomReal(2); Writeln(a[0],' ',a[1]); Randomize(1); var b:=ArrRandomReal(2,3); Writeln(b[0],' ',b[1]); Randomize(1); var c:=PABCSystem.ArrRandomReal(2,0,1); Writeln(c[0],' ',c[1]); end.","2.49 1.11\n2.487 1.107\n0.25 0.11\n");
    result("begin Randomize(1); var a:=ArrRandom(3,1); Writeln(a[0],' ',a[1],' ',a[2]); Writeln(Length(ArrRandom),' ',Length(ArrRandomInteger),' ',Length(ArrRandomReal)); end.","25 12 47\n10 10 10\n");
    result("begin Randomize(1); var a:=new real[2]; a.FillRandom(10,20); Randomize(1); Writeln(a[0]=Random(10.0,20.0),' ',a[1]=Random(10.0,20.0)); var b:=new integer[2]; Randomize(1); FillRandom(b,0,99); Writeln(b[0],' ',b[1]); end.","True True\n24 11\n");
    result("uses Arrays; var calls:string; function N:integer; begin calls:=calls+'N'; Result:=2; end; function L:integer; begin calls:=calls+'L'; Result:=0; end; function H:integer; begin calls:=calls+'H'; Result:=99; end; begin Randomize(1); var a:=CreateRandomIntegerArray(N(),L(),H()); Writeln(calls,' ',a[0],' ',a[1]); end.","NLH 24 11\n");
    result("uses Arrays; var a:array of integer; function Low:integer; begin SetLength(a,0); Result:=0; end; begin a:=new integer[2]; var alias:=a; Randomize(1); FillByRandom(a,Low(),99); Writeln(Length(a),' ',alias[0],' ',alias[1]); end.","0 24 11\n");
    result("uses Arrays; begin Randomize(1); var a:=CreateRandomIntegerArray(0,0,MaxInt); FillByRandom(a,0,MaxInt); var b:=ArrRandomReal(0,0,1,99); Writeln(Length(a),' ',Length(b),' ',Random(100)); end.","0 0 24\n");
    for (const auto* source:{"uses Arrays; begin var a:=CreateRandomIntegerArray(-1); end.","begin var a:=ArrRandom(-1); end."}) runtime_error(source,"E_RANGE");
    runtime_error("uses Arrays; begin var a:array of integer; FillByRandom(a); end.","E_NIL");
    runtime_error("begin var a:array of real; a.FillRandom(0,1); end.","E_NIL");
    runtime_error("uses Arrays; begin var a:=CreateRandomIntegerArray(1,0,MaxInt); end.","E_OVERFLOW");
    compile_error("begin var a:=CreateRandomIntegerArray(3); end.","E_NAME");
    compile_error("begin var a:=Arr(1); FillByRandom(a); end.","E_NAME");
    compile_error("uses Arrays; begin var a:=new byte[2]; FillByRandom(a); end.","E_TYPE");
    compile_error("uses Arrays; begin var a:=new integer[2]; FillByRandom(a,1.5,2.5); end.","E_TYPE");
    compile_error("begin var a:=ArrRandomInteger(3,2); end.","E_TYPE");
    compile_error("begin var a:=ArrRandomReal(3,2.5); end.","E_TYPE");
    for (const auto* source:{"begin var n:integer:=Random; end.","begin var n:=Random(true); end.",
        "begin Randomize('seed'); end.","begin SetRandomSeed(); end.","begin Writeln(Random('ab','cd')); end.",
        "begin Writeln(RandomReal(0,1,'two')); end."}) compile_error(source,"E_TYPE");
    runtime_error("begin Randomize(1); Random(-1); end.","E_RANGE");
    runtime_error("begin Randomize(1); Random(0,MaxInt); end.","E_OVERFLOW");
    runtime_error("begin Randomize(1); RandomReal(0,1,16); end.","E_RANGE");
    runtime_error("begin Randomize(1); Random(-MaxReal,MaxReal); end.","E_OVERFLOW");
    {
        abc::RandomContext first,second; first.seed(1); second.seed(1);
        error([&]{(void)first.range(2,1,{});},"E_RANGE");
        require(first.unit({})==second.unit({}),"Rejected random bounds do not advance seeded state");
        first.seed(1); second.seed(1);
        require(first.range(0,0,{})==0 && first.unit({})!=second.unit({}),"Zero random bound consumes one compatibility sample");
    }
    auto program=abc::compile("begin var x:=Random; var n:=Random(10); var c:=Random('a','z'); end.");
    require(program.locals[0].type==abc::Type::real && program.locals[1].type==abc::Type::integer && program.locals[2].type==abc::Type::character,"Random overload result types are known before execution");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::call_builtin) { instruction.type=abc::Type::boolean; break; }
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("uses Arrays; begin var a:=CreateRandomIntegerArray(3); end.");
    require(program.types[0].kind==abc::CompoundKind::array && program.types[0].element==abc::Type::integer,"Random array element type is fixed before execution");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::random_array) instruction.operand=static_cast<std::size_t>(abc::RandomArrayMethod::rounded_real);
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("uses Arrays; begin var a:=new real[1]; FillByRandom(a); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::fill_random_array) instruction.operand=999;
    error([&]{abc::verify(program);},"E_BYTECODE");
    abc::ExecutionOptions limit; limit.allocation_limit=5;
    error([&]{(void)run("begin var a:=ArrRandom(20); end.","",limit);},"E_LIMIT");
    limit={}; int polled=0; limit.cancelled=[&]{return ++polled>10;};
    error([&]{(void)run("begin var a:=ArrRandom(2000); end.","",limit);},"E_CANCELLED");
    try { (void)run("begin\n Randomize(1);\n Random(-1);\nend."); require(false,"Expected random range diagnostic"); }
    catch (const abc::Error& caught) { require(caught.code=="E_RANGE" && caught.location.line==3 && caught.russian=="Некорректные границы генератора случайных чисел","Random errors carry source position and both languages"); }
    std::istringstream input; std::ostringstream output;
    error([&]{abc::execute(abc::compile("begin Writeln('must not run'); var n:integer:=Random; end."),input,output);},"E_TYPE");
    require(output.str().empty(),"Random overload result types are checked before any output");
}

void unsigned_integers() {
    result("begin var w:longword; var u:uint64; Writeln(w,' ',u); w:=4294967295; u:=18446744073709551615; Writeln(w,' ',u); end.","0 0\n4294967295 18446744073709551615\n");
    result("begin var w:=longword(4294967295); Writeln(w+1,' ',w-longword(1),' ',w*longword(2),' ',-w,' ',w div longword(2)); end.","4294967296 4294967294 8589934590 -4294967295 2147483647\n");
    result("begin var u:=MaxUInt64; Writeln(u+0,' ',u-1,' ',u div 2,' ',u mod 2); Writeln(u>18446744073709551614,' ',u=u,' ',uint64(0)*u); end.","18446744073709551615 18446744073709551614 9223372036854775807 1\nTrue True 0\n");
    result("begin Writeln(Sqr(MaxWord),' ',Sqr(MaxLongWord),' ',Sqr(uint64(4294967295))); Writeln(Abs(MaxUInt64),' ',Sign(MaxUInt64),' ',Ord(MaxUInt64)); end.","4294836225 18446744065119617025 18446744065119617025\n18446744073709551615 1 18446744073709551615\n");
    result("begin Writeln(Min(MaxUInt64,1),' ',Max(uint64(0),MaxUInt64,1),' ',Min(MaxLongWord,1),' ',Min(MaxLongWord,-1)); end.","1 18446744073709551615 1 -1\n");
    result("begin Writeln(Pred(MaxUInt64),' ',Succ(uint64(0)),' ',Succ(longword(4294967294)),' ',Pred(MaxLongWord)); Writeln(+MaxUInt64,' ',-uint64(0)); end.","18446744073709551614 1 4294967295 4294967294\n18446744073709551615 0\n");
    result("begin var u:=MaxUInt64; Dec(u); Inc(u,-1); Dec(u,-1); Writeln(u); u:=9223372036854775808; Inc(u,-9223372036854775808); Writeln(u); var w:longword:=2147483648; Inc(w,-2147483648); Writeln(w); end.","18446744073709551614\n0\n0\n");
    result("begin var w:longword:=MaxByte; var u:uint64:=w; var n:int64:=w; var r:real:=MaxUInt64; Writeln(w,' ',u,' ',n,' ',r>1e19); Writeln(byte(uint64(255)),' ',integer(longword(123)),' ',int64(MaxInt),' ',integer(true),' ',Chr(uint64(65))); end.","255 255 255 True\n255 123 2147483647 1 A\n");
    result("function Next(u:uint64):uint64; begin Result:=u+1; end; procedure Advance(var w:longword); begin Inc(w); end; begin var w:longword:=4294967294; Advance(w); Writeln(w,' ',Next(MaxInt64)); end.","4294967295 9223372036854775808\n");
    result("type R=record u:uint64; w:longword; end; begin var r:R; r.u:=MaxUInt64; r.w:=MaxLongWord; var copy:=r; Dec(copy.u); Dec(copy.w); Writeln(r.u,' ',copy.u,' ',r.w,' ',copy.w); end.","18446744073709551615 18446744073709551614 4294967295 4294967294\n");
    result("begin var a:=Arr(1,MaxUInt64); Writeln(a[0],' ',a[1]); var b:=Arr(-1,MaxLongWord); Writeln(b[0],' ',b[1]); var l:=new List<uint64>; l.Add(MaxUInt64); l.Add(0); Sort(l); Writeln(l[0],' ',l[1]); end.","1 18446744073709551615\n-1 4294967295\n0 18446744073709551615\n");
    result("uses Collections; begin var s:=new Stack<uint64>; s.Push(MaxUInt64); Writeln(s.Pop); var m:=new AssocArray<uint64,longword>; Writeln(m[MaxUInt64]); m[MaxUInt64]:=MaxLongWord; Writeln(m[MaxUInt64]); end.","18446744073709551615\n0\n4294967295\n");
    result("begin for var u:=MaxUInt64-1 to MaxUInt64 do Writeln(u); for var u:uint64:=0 downto 0 do Writeln(u); for var w:longword:=4294967294 to MaxLongWord do Writeln(w); end.","18446744073709551614\n18446744073709551615\n0\n4294967294\n4294967295\n");
    result("begin for var u:=18446744073709551615 to 18446744073709551615 do Writeln(u); for var w:longword:=0 downto 0 do Writeln(w); foreach var u:uint64 in Arr(MaxWord,word(1)) do Writeln(u); end.","18446744073709551615\n0\n65535\n1\n");
    result("begin var w:longword; var u:uint64; Readln(w); Readln(u); Writeln(w,' ',u); end.","4294967295 18446744073709551615\n","4294967295\n18446744073709551615\n");
    result("begin Writeln(MaxByte,' ',MaxShortInt,' ',MaxSmallInt,' ',MaxWord,' ',PABCSystem.MaxLongWord,' ',PABCSystem.MaxUInt64); Writeln(Ord(END_OF_LINE_SYMBOL),' ',__IS_SYSTEM_MODULE,' ',Pos('№',AllDelimiters)>0); end.","255 127 32767 65535 4294967295 18446744073709551615\n10 True True\n");
    result("function MaxByte:integer; begin Result:=7; end; begin Writeln(MaxByte,' ',PABCSystem.MaxByte); end.","7 255\n");
    compile_error("function MaxUInt64(x:integer):integer; begin Result:=x; end; begin Writeln(MaxUInt64); end.","E_TYPE");
    for (const auto* source:{"begin var u:uint64:=-1; end.","begin var w:longword:=4294967296; end.","begin var u:uint64:=18446744073709551616; end."}) compile_error(source,"E_RANGE");
    for (const auto* source:{"begin var n:int64:=1; var u:uint64:=n; end.","begin var u:=MaxUInt64; var n:int64:=u; end.",
        "begin var u:=MaxUInt64; var n:integer:=1; Writeln(u+n); end.","begin for var u:uint64:=0 to 1 do Inc(u); end.",
        "procedure P(var n:int64); begin end; begin var u:uint64; P(u); end."}) compile_error(source,"E_TYPE");
    for (const auto* source:{"begin Writeln(MaxUInt64+1); end.","begin Writeln(uint64(0)-1); end.",
        "begin Writeln(MaxUInt64*2); end.","begin Writeln(Sqr(uint64(4294967296))); end.",
        "begin Writeln(uint64(-1)); end.","begin Writeln(longword(-1)); end.","begin Writeln(longword(MaxUInt64)); end.",
        "begin Writeln(byte(MaxUInt64)); end.","begin Writeln(int64(MaxUInt64)); end.","begin Writeln(integer(MaxLongWord)); end.",
        "begin Writeln(-uint64(1)); end.","begin Writeln(Succ(MaxUInt64)); end.","begin Writeln(Pred(uint64(0))); end.",
        "begin var u:=MaxUInt64; Inc(u); end.","begin var u:uint64; Dec(u); end.",
        "begin var w:=MaxLongWord; Inc(w); end.","begin var w:longword; Dec(w); end."}) runtime_error(source,"E_OVERFLOW");
    runtime_error("begin Writeln(MaxUInt64 div 0); end.","E_DIV_ZERO");
    runtime_error("begin Writeln(MaxUInt64 mod 0); end.","E_DIV_ZERO");
    runtime_error("begin var l:=new List<uint64>; l.Add(1); foreach var u in l do Inc(l[0]); end.","E_MODIFIED");
    auto program=abc::compile("begin var u:=18446744073709551615; var w:=MaxLongWord; var sum:=w+w; var square:=Sqr(MaxWord); var least:=Min(u,1); end.");
    require(program.locals[0].type==abc::Type::uint64 && program.locals[1].type==abc::Type::longword && program.locals[2].type==abc::Type::int64 && program.locals[3].type==abc::Type::uint64 && program.locals[4].type==abc::Type::uint64,"Unsigned literals, arithmetic, Sqr(word) and Min results have exact static types");
    program=abc::compile("begin var u:=MaxUInt64; Writeln(u-1); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::subtract) instruction.type=abc::Type::longword;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin var u:uint64; Inc(u); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::increment) instruction.type=abc::Type::longword;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin var b:boolean; Inc(b); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::increment) instruction.type=abc::Type::integer;
    error([&]{abc::verify(program);},"E_BYTECODE");
}

void small_integers() {
    result("begin Writeln(Succ(false),' ',Succ(true),' ',Pred(false),' ',Pred(true)); var b:boolean; Inc(b); Writeln(b); Inc(b); Writeln(b); Dec(b); Writeln(b); Dec(b); Writeln(b); end.","True False True False\nTrue\nFalse\nTrue\nFalse\n");
    result("begin Writeln(false<true,' ',false<=false,' ',true>false,' ',true>=true); for var b:=false to true do Write(Ord(b)); Write(' '); for var b:=true downto false do Write(Ord(b)); Writeln; for var b:=true to false do Write('bad'); for var b:=false downto true do Write('bad'); end.","True True True True\n01 10\n");
    result("begin for var b:=true to true do Writeln(b); for var b:=false downto false do Writeln(b); var count:=0; for var b:=false to true do begin Inc(count); if not b then continue; Write(b); end; Writeln(' ',count); end.","True\nFalse\nTrue 2\n");
    result("type R=record b:boolean; end; procedure Flip(var b:boolean); begin Dec(b); end; begin var r:R; Flip(r.b); var a:=Arr(true,false); Inc(a[0]); Dec(a[1]); Writeln(r.b,' ',a[0],' ',a[1]); end.","True False True\n");
    runtime_error("begin var l:=new List<boolean>; l.Add(true); foreach var b in l do Dec(l[0]); end.","E_MODIFIED");
    compile_error("begin var b:boolean; Inc(b,1); end.","E_TYPE");
    compile_error("begin var b:boolean; Dec(b,false); end.","E_TYPE");
    compile_error("begin for var b:=false to true do Inc(b); end.","E_TYPE");
    result("begin var b:byte; var s:shortint; var n:smallint; Writeln(b,' ',s,' ',n); b:=255; s:=-128; n:=-32768; Writeln(b,' ',s,' ',n); end.","0 0 0\n255 -128 -32768\n");
    result("begin var b:=byte(255); var s:=shortint(-128); var n:=smallint(-32768); Writeln(b+1,' ',s-1,' ',n-1,' ',-b,' ',b div 2,' ',s mod 3); end.","256 -129 -32769 -255 127 -2\n");
    result("procedure P(var b:byte; var s:shortint; var n:smallint; var w:word); begin Inc(b); Inc(s); Inc(n); Inc(w); end; begin var b:=byte(1); var pb:=+b; var ps:=+shortint(-2); var pn:=+smallint(-3); var pw:=+word(4); P(pb,ps,pn,pw); Writeln(b,' ',pb,' ',ps,' ',pn,' ',pw); end.","1 2 -1 -2 5\n");
    result("begin var b:=byte(65); var w:word:=b; var s:smallint:=b; var i:integer:=b; var n:int64:=b; var r:real:=b; Writeln(w,' ',s,' ',i,' ',n,' ',r,' ',Chr(b)); end.","65 65 65 65 65 A\n");
    result("begin var s:=shortint(-12); var n:smallint:=s; var i:integer:=n; var big:int64:=n; var r:real:=n; Writeln(s,' ',n,' ',i,' ',big,' ',r,' ',IntToStr(s)); end.","-12 -12 -12 -12 -12 -12\n");
    result("begin Writeln(Abs(shortint(-127)),' ',Abs(smallint(-32767)),' ',Abs(byte(255)),' ',Abs(word(65535))); Writeln(Sign(shortint(-1)),' ',Sign(byte(0)),' ',Sign(word(65535))); Writeln(Sqr(byte(255)),' ',Sqr(shortint(-128)),' ',Sqr(smallint(-32768))); end.","127 32767 255 65535\n-1 0 1\n65025 16384 1073741824\n");
    result("begin Writeln(Min(byte(1),byte(2)),' ',Max(shortint(-1),shortint(1)),' ',Min(smallint(-32768),smallint(1)),' ',Max(word(65535),word(1))); end.","1 1 -32768 65535\n");
    result("begin var b:=byte(1); var s:=shortint(-1); var n:=smallint(20); Inc(b,2); Dec(s,-2); Inc(n,-5); Writeln(b,' ',s,' ',n); Writeln(Ord(b),' ',Ord(s),' ',Ord(n)); end.","3 1 15\n3 1 15\n");
    result("begin Writeln(Succ(byte(254)),' ',Pred(byte(1)),' ',Succ(shortint(-128)),' ',Pred(shortint(127)),' ',Succ(smallint(32766)),' ',Pred(smallint(-32767))); end.","255 0 -127 126 32767 -32768\n");
    result("begin Writeln(byte('A'),' ',shortint(word(127)),' ',smallint(byte(255)),' ',byte(2147483648-2147483648)); end.","65 127 255 0\n");
    result("begin var a:=Arr(byte(1),shortint(-2)); Writeln(a[0],' ',a[1]); var b:=Arr(word(65535),smallint(-3)); Writeln(b[0],' ',b[1]); end.","1 -2\n65535 -3\n");
    result("type R=record b:byte; s:shortint; n:smallint; end; begin var r:R; r.b:=255; r.s:=-128; r.n:=-32768; var copy:=r; Dec(copy.b); Inc(copy.s); Inc(copy.n); Writeln(r.b,' ',r.s,' ',r.n,' ',copy.b,' ',copy.s,' ',copy.n); end.","255 -128 -32768 254 -127 -32767\n");
    result("procedure P(var b:byte; var s:shortint; var n:smallint); begin Inc(b); Dec(s); Inc(n); end; function F(b:byte):byte; begin Result:=Succ(b); end; begin var b:=byte(1); var s:=shortint(0); var n:=smallint(4); P(b,s,n); Writeln(b,' ',s,' ',n,' ',F(b)); end.","2 -1 5 3\n");
    result("uses Collections; begin var a:=new byte[2]; Writeln(a[0]); a[1]:=255; var l:=new List<shortint>; l.Add(-128); l.Add(127); Sort(l); Writeln(a[1],' ',l[0],' ',l[1]); var m:=new AssocArray<byte,smallint>; Writeln(m[byte(1)]); m[byte(1)]:=-32768; Writeln(m[byte(1)]); end.","0\n255 -128 127\n0\n-32768\n");
    result("uses Collections; begin var s:=new Stack<byte>; s.Push(255); s.Push(1); Writeln(s.ToString); var q:=new Queue<shortint>; q.Enqueue(-128); Writeln(q.Dequeue); var d:=new DynArray<smallint>(1); Writeln(d[0]); d[0]:=-32768; Writeln(d.ToString); end.","1 255 \n-128\n0\n-32768 \n");
    result("begin for var b:byte:=254 to 255 do Writeln(b); for var s:shortint:=-127 downto -128 do Writeln(s); for var n:smallint:=32766 to 32767 do Writeln(n); end.","254\n255\n-127\n-128\n32766\n32767\n");
    result("begin for var b:byte:=0 downto 0 do Writeln(b); for var s:shortint:=127 to 127 do Writeln(s); for var n:smallint:=-32768 downto -32768 do Writeln(n); end.","0\n127\n-32768\n");
    result("begin foreach var n:smallint in Arr(byte(2),byte(3)) do Write(n); foreach var n:integer in Arr(shortint(-1),shortint(1)) do Write(n); Writeln; var b:=byte(2); Writeln('X'*b,' ',b*'Y'); end.","23-11\nXX YY\n");
    result("begin var b:byte; var s:shortint; var n:smallint; Readln(b); Readln(s); Readln(n); Writeln(b,' ',s,' ',n); end.","255 -128 -32768\n","255\n-128\n-32768\n");
    for (const auto* source:{"begin var n:byte:=-1; end.","begin var n:byte:=256; end.",
        "begin var n:shortint:=-129; end.","begin var n:shortint:=128; end.",
        "begin var n:smallint:=-32769; end.","begin var n:smallint:=32768; end.",
        "begin var n:byte:=9223372036854775807; end."}) compile_error(source,"E_RANGE");
    for (const auto* source:{"begin var b:byte; var s:shortint:=b; end.","begin var s:shortint; var b:byte:=s; end.",
        "begin var i:integer:=1; var b:byte:=i; end.","procedure P(var n:integer); begin end; begin var b:byte; P(b); end.",
        "begin foreach var b:byte in Arr(word(1)) do Write(b); end.",
        "begin for var b:byte:=0 to 1 do Inc(b); end.","begin var a:=new byte[1]; a[0]:='X'; end."}) compile_error(source,"E_TYPE");
    for (const auto* source:{"begin Writeln(byte(-1)); end.","begin Writeln(byte(256)); end.",
        "begin Writeln(shortint(-129)); end.","begin Writeln(shortint(128)); end.",
        "begin Writeln(smallint(-32769)); end.","begin Writeln(smallint(32768)); end.",
        "begin Writeln(byte(word(65535))); end.","begin Writeln(byte(9223372036854775807)); end.",
        "begin Writeln(Succ(byte(255))); end.","begin Writeln(Pred(byte(0))); end.",
        "begin Writeln(Succ(shortint(127))); end.","begin Writeln(Pred(shortint(-128))); end.",
        "begin Writeln(Succ(smallint(32767))); end.","begin Writeln(Pred(smallint(-32768))); end.",
        "begin var b:byte:=255; Inc(b); end.","begin var b:byte; Dec(b); end.",
        "begin var s:shortint:=-128; Dec(s); end.","begin var s:shortint:=127; Inc(s); end.",
        "begin var n:smallint:=-32768; Dec(n); end.","begin var n:smallint:=32767; Inc(n); end.",
        "begin var b:=byte(1); Inc(b,2147483647); end."}) runtime_error(source,"E_OVERFLOW");
    runtime_error("begin Writeln(Abs(shortint(-128))); end.","E_OVERFLOW");
    runtime_error("begin Writeln(Abs(smallint(-32768))); end.","E_OVERFLOW");
    result("begin Writeln(Sqr(word(65535))); end.","4294836225\n");
    compile_error("uses FilesOperations; begin Writeln(FilesOperations.Sqr(word(1))); end.","E_NAME");
    runtime_error("begin var l:=new List<byte>; l.Add(1); foreach var b in l do Inc(l[0]); end.","E_MODIFIED");
    error([&]{(void)run("begin var b:byte; Readln(b); end.","256\n");},"E_INPUT");
    error([&]{(void)run("begin var s:shortint; Readln(s); end.","-129\n");},"E_INPUT");
    auto program=abc::compile("begin var b:=byte(1); var s:=shortint(2); var n:=smallint(3); var o:=Ord(b); var a:=Arr(b,s); end.");
    require(program.locals[0].type==abc::Type::byte && program.locals[1].type==abc::Type::shortint && program.locals[2].type==abc::Type::smallint && program.locals[3].type==abc::Type::byte,"Small integer types and Ord results remain statically distinct");
    require(program.types[0].element==abc::Type::smallint,"Array inference joins byte and shortint into smallint");
    program=abc::compile("begin var a:=Abs(byte(2)); var s:=Sqr(byte(2)); var m:=Min(shortint(1),shortint(2)); var w:=Max(word(1),word(2)); end.");
    require(program.locals[0].type==abc::Type::byte && program.locals[1].type==abc::Type::integer && program.locals[2].type==abc::Type::shortint && program.locals[3].type==abc::Type::word,"Small integer math overloads preserve declared source result types");
    program=abc::compile("begin var b:byte:=2; Inc(b); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::increment) instruction.type=abc::Type::shortint;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin Writeln(byte(2)+byte(3)); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::add) instruction.type=abc::Type::byte;
    error([&]{abc::verify(program);},"E_BYTECODE");
}

void characters() {
    result("begin var c:char; var w:word; Writeln(Ord(c),' ',w); c:='Я'; w:=Ord(c); Writeln(c,' ',w,' ',w+1); end.","0 0\nЯ 1071 1072\n");
    result("begin var c:='A'; Writeln(Ord(c),' ',Succ(c),' ',Pred(c),' ',Chr(90),' ',ChrUnicode(1071)); end.","65 B @ Z Я\n");
    result("begin Writeln(OrdUnicode(#55296),' ',Ord(#65535),' ',Ord(True),' ',Ord(False),' ',Ord(2147483648)); end.","55296 65535 1 0 2147483648\n");
    result("begin Writeln(#65+'B',' ',#0=#0,' ',#10<#13,' ',#65535>#0); Writeln('a'+'b',' ','a'+'bc',' ','ab'+'c',' ',nil+'x',' ','x'+nil); end.","AB True True True\nab abc abc x x\n");
    result("begin var w:word:=65535; Writeln(w,' ',Ord(w),' ',Pred(w),' ',word(12),' ',char(w)); end.","65535 65535 65534 12 ￿\n");
    result("begin var w:word:=65; var a:integer:=w; var b:int64:=w; var r:real:=w; Writeln(a,' ',b,' ',r,' ',Chr(w),' ',word('Я')); Writeln(-w,' ',w div 2,' ',Abs(w)); end.","65 65 65 A 1071\n-65 32 65\n");
    result("begin var c:='a'; Inc(c); Inc(c,2); Dec(c); Dec(c,-1); Writeln(c); Inc(c,-2); Writeln(c); var w:word:=4; Dec(w,3); Inc(w,2); Writeln(w); end.","d\nb\n3\n");
    result("function F(c:char):char; begin Result:=Succ(c); end; procedure P(var c:char); begin Inc(c); end; begin var c:='Я'; P(c); Writeln(F(c)); end.","б\n");
    result("type Pair=record c:char; w:word; end; begin var a:=new Pair[1]; a[0].c:='X'; a[0].w:=Ord('Y'); var b:=a[0]; Inc(b.c); Inc(b.w); Writeln(a[0].c,' ',a[0].w,' ',b.c,' ',b.w); end.","X 89 Y 90\n");
    result("uses Collections; begin var s:=new Stack<char>; s.Push('A'); s.Push('Я'); Writeln(s.ToString); Writeln(s.Pop,' ',s.Top); var m:=new AssocArray<char,word>; Writeln(m['x']); m['x']:=Ord('Z'); Writeln(m['x']); end.","Я A \nЯ A\n0\n90\n");
    result("begin var a:=Arr('z','a','Я'); Sort(a); Writeln(a[0],a[1],a[2]); var l:=new List<char>; l.Add('b'); l.Add('a'); Sort(l); Writeln(l.Contains('b'),' ',l[0]); end.","azЯ\nTrue a\n");
    result("begin var s:string:='Яx'; Writeln(s[1],' ',Ord(s[2])); var copy:=s; s[1]:='A'; Writeln(s,' ',copy); end.","Я 120\nAx Яx\n");
    result("type R=record s:string; end; begin var r:R; r.s:='ab'; r.s[2]:='Я'; var a:=new string[1]; a[0]:=r.s; a[0][1]:='X'; Writeln(r.s,' ',a[0]); end.","aЯ XЯ\n");
    result("begin var l:=new List<string>; l.Add('ab'); l[0][1]:='X'; Writeln(l[0]); end.","Xb\n");
    result("begin var s:='abcd'; var i:=0; foreach var c in s do begin Inc(i); s:='new'; if c='b' then continue; Write(c); if i=3 then break; end; Writeln(' ',s); end.","ac new\n");
    result("var calls:integer; function F:string; begin Inc(calls); Result:='аб'; end; begin foreach var c in F() do Write(c); Writeln(' ',calls); end.","аб 1\n");
    result("begin foreach var c:string in 'аб' do Writeln(c+'!'); foreach var c in '🌍' do Writeln(Ord(c)); end.","а!\nб!\n55356\n57101\n");
    result("begin var s:string; foreach var c in s do Write('bad'); var c:char; foreach c in 'ab' do Write(c); Writeln(c); end.","abb\n");
    result("begin for var c:='a' to 'c' do Write(c); Write(' '); for var c:='в' downto 'а' do Write(c); Writeln; for var c:=#65535 to #65535 do Writeln(Ord(c)); for var c:=#0 downto #0 do Writeln(Ord(c)); end.","abc вба\n65535\n0\n");
    result("begin for var w:word:=65534 to 65535 do Writeln(w); for var w:word:=0 downto 0 do Writeln(w); end.","65534\n65535\n0\n");
    result("begin var c:char; Readln(c); Writeln(c); var w:word; Readln(w); Writeln(w); end."," \n65535\n"," word\n65535\n");
    for (const auto* source:{"begin var c:char:=12; end.","begin var c:char:='ab'; end.",
        "begin var c:='a'; c:='ab'; end.","begin var c:char; c:=nil; end.",
        "begin var c:='a'; Writeln(c div 2); end.","begin var s:='ab'; s[1]:='XY'; end.",
        "begin var s:='ab'; s[1]:=65; end.","begin foreach var c in 'ab' do c:='x'; end.",
        "begin for var c:='a' to 'b' do Inc(c); end.","begin foreach var s in Arr('ab') do s[1]:='x'; end.",
        "procedure P(var c:char); begin end; begin var s:='ab'; P(s[1]); end."}) compile_error(source,"E_TYPE");
    for (const auto* source:{"begin Writeln(#65536); end.","begin Writeln(#99999999999999999999999); end.",
        "begin var w:word:=65536; end."}) compile_error(source,"E_RANGE");
    compile_error("begin Writeln(#); end.","E_SYNTAX");
    for (const auto* source:{"begin Writeln(Chr(-1)); end.","begin Writeln(Chr(65536)); end.","begin Writeln(word(65536)); end."}) runtime_error(source,"E_OVERFLOW");
    for (const auto* source:{"begin Writeln(Succ(#65535)); end.","begin Writeln(Pred(#0)); end.",
        "begin var c:=#65535; Inc(c); end.","begin var c:=#0; Dec(c); end.",
        "begin var c:='a'; Inc(c,2147483647); end.","begin var w:word; Dec(w); end.",
        "begin var w:word:=65535; Inc(w); end.","begin Writeln(Succ(word(65535))); end."}) runtime_error(source,"E_OVERFLOW");
    for (const auto* source:{"begin var s:='ab'; Writeln(s[0]); end.","begin var s:='ab'; s[3]:='X'; end.",
        "begin var s:='ab'; Writeln(s[-2147483648]); end."}) runtime_error(source,"E_BOUNDS");
    runtime_error("begin var s:string:=nil; Writeln(s[1]); end.","E_NIL");
    runtime_error("begin var s:string:=nil; s[1]:='X'; end.","E_NIL");
    runtime_error("begin var s:string:=nil; foreach var c in s do Write(c); end.","E_NIL");
    runtime_error("begin var l:=new List<string>; l.Add('ab'); foreach var s in l do l[0][1]:='X'; end.","E_MODIFIED");
    auto program=abc::compile("begin var c:='x'; var w:=Ord(c); end.");
    require(program.locals[0].type==abc::Type::character && program.locals[1].type==abc::Type::word,"Character and Ord result types are statically known");
    program=abc::compile("begin var s:='ab'; s[1]:='X'; end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::set_character) instruction.type=abc::Type::integer;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin Writeln('ab'[1]); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::get_index) instruction.operand=2;
    error([&]{abc::verify(program);},"E_BYTECODE");
    try { (void)run("begin\nvar c:=#65535;\nInc(c);\nend."); require(false,"Expected character overflow"); }
    catch (const abc::Error& caught) { require(caught.code=="E_OVERFLOW" && caught.location.line==3 && !caught.russian.empty(),"Character overflow retains source location and both languages"); }
    result("begin Writeln('A'#66'Я'); Writeln(('a'#0'b').Length,' ',Ord(('a'#0'b')[2])); Writeln(#55356#57101); end.","ABЯ\n3 0\n🌍\n");
    result("begin var s:=#65''; s:='abc'; Writeln(s.Length,' ',''.Length,' ',('a'+'b').Length); end.","3 0 2\n");
    result("begin Writeln(StringOfChar('Я',3),' ',StringOfChar(#0,2).Length); Writeln('ab'*3,' ',3*'Я',' ',('ab'*0).Length,' ',('a'*(-1)).Length); end.","ЯЯЯ 2\nababab ЯЯЯ 0 0\n");
    result("begin var s:string:=nil; Writeln((s*2147483647).Length,' ',(''*2147483647).Length,' ',('a'*(-2147483648)).Length); end.","0 0 0\n");
    result("var calls:string; function S:string; begin calls:=calls+'S'; Result:='x'; end; function N:integer; begin calls:=calls+'N'; Result:=2; end; begin Writeln(S()*N(),' ',calls); calls:=''; Writeln(N()*S(),' ',calls); end.","xx SN\nxx NS\n");
    result("begin Writeln('7'.ToDigit,' ','0'.ToDigit(),' ',ToDigit('9')); end.","7 0 9\n");
    result("begin var n:word:=3; Writeln('a'*n,' ',n*'b'); end.","aaa bbb\n");
    runtime_error("begin Writeln(StringOfChar('X',-1)); end.","E_RANGE");
    runtime_error("begin Writeln(StringOfChar('X',1048577)); end.","E_LIMIT");
    runtime_error("begin Writeln('ab'*2147483647); end.","E_LIMIT");
    runtime_error("begin Writeln(2147483647*'X'); end.","E_LIMIT");
    runtime_error("begin var s:string:=nil; Writeln(s.Length); end.","E_NIL");
    runtime_error("begin Writeln('Я'.ToDigit); end.","E_FORMAT");
    runtime_error("begin Writeln('١'.ToDigit); end.","E_FORMAT");
    compile_error("begin Writeln(StringOfChar('ab',2)); end.","E_TYPE");
    compile_error("begin Writeln('a'*2.0); end.","E_TYPE");
    compile_error("begin Writeln('7'.ToDigit(1)); end.","E_TYPE");
    compile_error("begin Writeln('abc'.Length()); end.","E_TYPE");
    program=abc::compile("begin var text:='abc'; foreach var c in text do Write(c); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::iteration_check) instruction.type=abc::Type::character;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin var text:='abc'; text[1]:='X'; end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::set_character) instruction.operand=1;
    error([&]{abc::verify(program);},"E_BYTECODE");
    abc::ExecutionOptions limits; limits.instruction_limit=30;
    error([&]{(void)run("begin foreach var c in StringOfChar('a',100) do continue; end.","",limits);},"E_LIMIT");
}
} // namespace

int main() {
    language();
    structured_loops();
    foreach_loops();
    characters();
    small_integers();
    unsigned_integers();
    random_library();
    extended_math();
    array_output();
    bitwise_operations();
    console_input();
    console_functions();
    sequence_mutations();
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
    text_files();
    text_file_changes();
    endpoint_collections();
    buffer_collections();
    association_collections();
    linked_collections();
    single_nodes();
    std::cout << (checks - failures) << '/' << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}

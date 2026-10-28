#include "abc/core.hpp"
#include "../src/linked.hpp"
#include "../src/random.hpp"
#include "../src/school.hpp"
#include "../src/runtime_values.hpp"
#include "../src/sequences.hpp"
#include "../src/real_range.hpp"

#include <iostream>
#include <array>
#include <algorithm>
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

void big_integer_library() {
    result("begin var a:BigInteger:=123456789012345678901234567890bi; Writeln(a+10,' ',a*2); Writeln((-11bi) div 2,' ',(-11bi) mod 2,' ',1bi/2,' ',5bi/2bi); Writeln((1bi shl 80) shr 76,' ',(-9bi) shr 1,' ',not 0bi); end.","123456789012345678901234567900 246913578024691357802469135780\n-5 -1 0.5 2\n16 -5 -1\n");
    result("begin var n:BigInteger:=1; for var i:=2 to 100 do n:=n*i; Writeln(n); var a:=BigInteger.Parse(' +000123 '); Writeln(a,' ',new BigInteger(-1.9),' ',BigInteger.Create(MaxUInt64)); end.","93326215443944152681699238856266700490715968264381621468592963895217599993229915608941463976156518286253697920827223758251185210916864000000000000000000000000\n123 -1 18446744073709551615\n");
    result("begin Writeln(BigInteger.Zero.IsZero,' ',BigInteger.One.IsOne,' ',BigInteger.MinusOne.Sign,' ',(42bi).IsEven); var a:BigInteger; Writeln(a,' ',new BigInteger()); Writeln(BigInteger.Abs(-7),' ',BigInteger.Pow(2,100),' ',GCD(MaxUInt64,1)); Writeln(LCM(-4bi,6bi),' ',ISqrt(15241578750190521bi),' ',Sqr(123456789bi)); end.","True True -1 True\n0 0\n7 1267650600228229401496703205376 1\n12 123456789 15241578750190521\n");
    result("begin var a:BigInteger:=17; Writeln(BigInteger.TryParse('bad',a),' ',a); Writeln(BigInteger.TryParse(nil,a),' ',a); Writeln(BigInteger.TryParse('-42',a),' ',a); var s:string; Str(a,s); Writeln(s); var code:integer; Val('x',a,code); Writeln(a,' ',code); Writeln('12345678901234567890'.ToBigInteger); end.","False 0\nFalse 0\nTrue -42\n-42\n0 1\n12345678901234567890\n");
    result("begin var a:=new List<BigInteger>; a.Add(1bi shl 80); a.Add(-2bi); Writeln(a.Contains(1208925819614629174706176bi),' ',a); var b:=Arr(1bi,MaxUInt64,-7); Sort(b); Writeln(b); Writeln((123bi,'ab')); Writeln(FormatValue(42bi,4)); end.","True [1208925819614629174706176,-2]\n[-7,1,18446744073709551615]\n(123,ab)\n  42\n");
    result("begin Writeln(int64(-9223372036854775808bi),' ',uint64(18446744073709551615bi),' ',word(65535bi),' ',char(1071bi)); Writeln(real(9007199254740995bi)=9007199254740994.0,' ',real(MaxUInt64)=18446744073709551616.0); var r:real:=Power(2,3); Writeln(r); end.","-9223372036854775808 18446744073709551615 65535 Я\nTrue True\n8\n");
    result("begin var a:BigInteger:=7; Writeln(TryRead(a),' ',a); Writeln(TryRead(a),' ',a); Writeln(ReadBigInteger); end.","False 7\nTrue 12345678901234567890\n-42\n","bad 12345678901234567890 -42");
    result("begin var a:BigInteger; Readln(a); Writeln(a); end.","123456789012345678901234567890\n","123456789012345678901234567890\n");
    result("function R:real; begin Write('R'); Result:=1; end; begin Writeln((4bi)/R); end.","R4\n");
    const auto program=abc::compile("function R:real; begin Write('R'); Result:=1; end; begin Writeln((1bi shl 2000)/R); end.");
    std::istringstream input; std::ostringstream output;
    error([&]{abc::execute(program,input,output);},"E_OVERFLOW");
    require(output.str()=="R","BigInteger mixed division converts inside the operator after both arguments evaluate");
    for (const auto source:{"begin var a:integer:=1bi; end.","begin var a:BigInteger:=1.5; end.","begin BigInteger(True); end.",
        "begin BigInteger('ab'); end.","begin var a:real:=1bi; end.","begin BigInteger.One(); end.",
        "begin var a:=1bi; a.IsEven(); end.","begin var a:=0; BigInteger.TryParse('1',a); end."}) compile_error(source,"E_TYPE");
    compile_error("begin var a:=1.5bi; end.","E_SYNTAX");
    for (const auto source:{"begin integer(2147483648bi); end.","begin uint64(-1bi); end.","begin char(65536bi); end.",
        "begin real(1bi shl 2000); end."}) runtime_error(source,"E_OVERFLOW");
    runtime_error("begin Writeln(1bi div 0); end.","E_DIV_ZERO");
    runtime_error("begin Power(2bi,-1); end.","E_RANGE");
    runtime_error("begin BigInteger.Parse('17x'); end.","E_FORMAT");
    runtime_error("begin BigInteger.Parse(nil); end.","E_NIL");
    runtime_error("begin Writeln(1bi shl 65536); end.","E_LIMIT");
    auto invalid=abc::compile("begin var a:=1bi+2bi; end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::add) instruction.type=abc::Type::int64;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    abc::ExecutionOptions options; options.allocation_limit=1;
    error([&]{(void)run("begin Writeln(1bi shl 100); end.","",options);},"E_LIMIT");
    options={}; options.instruction_limit=100;
    error([&]{(void)run("begin var a:=1bi shl 1000; Writeln(a*a); end.","",options);},"E_LIMIT");
    options={}; std::size_t polls=0; options.cancelled=[&]{return ++polls>20;};
    error([&]{(void)run("begin BigInteger.Parse(StringOfChar('0',10000)+'1'); end.","",options);},"E_CANCELLED");
}

void school_swap_substrings() {
    result("uses School; begin var s:string:='cat dog cat'; SwapSubstr(s,'cat','dog'); Writeln(s); School.SwapSubstr(s,'cat','dog'); Writeln(s); s.SwapSubstr('cat','dog'); Writeln(s); end.","dog cat dog\ncat dog cat\ndog cat dog\n");
    result("uses School; begin var s:string:='aaa aa'; SwapSubstr(s,'aa','a'); Writeln(s); s:='aaa aa'; SwapSubstr(s,'a','aa'); Writeln(s); end.","aaa a\naaaaaa aaaa\n");
    result("uses School; begin var s:string:='a'; SwapSubstr(s,'','a'); Writeln(s); s:='a'; SwapSubstr(s,'a',''); Writeln(s); s:=''; SwapSubstr(s,'','x'); Writeln(s.Length); end.","aaa\na\n0\n");
    result("uses School; begin var s:string:='Яя Я'; SwapSubstr(s,'Я','я'); Writeln(s); s:='a'#0'b'; SwapSubstr(s,#0,'b'); Writeln(Ord(s[2]),' ',Ord(s[3])); s:=#55356#57101; SwapSubstr(s,#55356,#57101); Writeln(Ord(s[1]),' ',Ord(s[2])); end.","яЯ я\n98 0\n57101 55356\n");
    result("uses School; begin var s:string:='abc'; SwapSubstr(s,'missing',nil); SwapSubstr(s,'missing','missing'); Writeln(s); SwapSubstr(s,'a','missing'); Writeln(s); s:='a.b z'; SwapSubstr(s,'a.b','z'); Writeln(s); end.","abc\nabc\nz a.b\n");
    result("uses School; var a:array of string; function Index:integer; begin Write('I'); Result:=0; end; function First:string; begin Write('F'); a[0]:='ab'; Result:='a'; end; function Second:string; begin Write('S'); Result:='b'; end; begin a:=Arr('old'); SwapSubstr(a[Index],First,Second); Writeln('|',a[0]); end.","IFS|ba\n");
    result("uses School; function Side:string; begin Write('S'); Result:='b'; end; begin var s:string:='abc'; SwapSubstr(s,'missing',Side); Writeln(s); end.","Sabc\n");
    result("uses School; begin var a:=new List<string>; a.Add('abc'); foreach var s in a do a[0].SwapSubstr('missing','a'); Writeln(a); end.","[abc]\n");
    result("uses School; begin var s:string:='a^ b^'; SwapSubstr(s,'a^','b^'); Writeln(s); for var i:=1 to 500 do SwapSubstr(s,'a^','b^'); Writeln(s); end.","a^ b^\na^ b^\n");
    runtime_error("uses School; begin var s:string:='a'; SwapSubstr(s,'a','a'); end.","E_KEY");
    runtime_error("uses School; begin var s:string:=''; SwapSubstr(s,'',''); end.","E_KEY");
    runtime_error("uses School; begin var s:string:='a.b acb z'; SwapSubstr(s,'a.b','z'); end.","E_KEY");
    runtime_error("uses School; begin var s:string:='[x'; SwapSubstr(s,'[','x'); end.","E_REGEX");
    for (const auto source:{"uses School; begin var s:string:=nil; SwapSubstr(s,'a','b'); end.",
        "uses School; begin var s:string:='ab'; SwapSubstr(s,nil,'b'); end.",
        "uses School; begin var s:string:='ab'; SwapSubstr(s,'a',nil); end."}) runtime_error(source,"E_NIL");
    runtime_error("uses School; begin var a:=new List<string>; a.Add('ab'); foreach var s in a do SwapSubstr(a[0],'a','b'); end.","E_MODIFIED");
    runtime_error("uses School; begin var a:=new List<string>; a.Add('a^ b^'); foreach var s in a do SwapSubstr(a[0],'a^','b^'); end.","E_MODIFIED");
    for (const auto source:{"uses School; begin SwapSubstr('ab','a','b'); end.",
        "uses School; begin Writeln(('ab').SwapSubstr('a','b')); end.","uses School; begin var s:string:='ab'; s.SwapSubstr('a'); end.",
        "uses School; begin var s:string:='ab'; SwapSubstr(s,'a',True); end.",
        "uses School; begin foreach var s in Arr('ab') do s.SwapSubstr('a','b'); end."}) compile_error(source,"E_TYPE");
    compile_error("begin var s:string:='ab'; SwapSubstr(s,'a','b'); end.","E_NAME");
    auto invalid=abc::compile("uses School; begin var s:string:='ab'; SwapSubstr(s,'a','b'); end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::swap_substrings) instruction.operand=1;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    invalid=abc::compile("uses School; begin var s:string:='ab'; SwapSubstr(s,'a','b'); end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::swap_substrings) instruction.type=abc::Type::integer;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    abc::ExecutionOptions options; options.allocation_limit=100;
    error([&]{(void)run("uses School; begin var s:string:='ab'; SwapSubstr(s,'a','b'); end.","",options);},"E_LIMIT");
    options={}; options.instruction_limit=2000;
    error([&]{(void)run("uses School; begin var s:string:='(a+)+$ '+StringOfChar('a',100)+'! Z'; SwapSubstr(s,'(a+)+$','Z'); end.","",options);},"E_LIMIT");
    options={}; std::size_t polls=0; options.cancelled=[&]{return ++polls>12;};
    error([&]{(void)run("uses School; begin var s:string:='(a+)+$ '+StringOfChar('a',100)+'! Z'; SwapSubstr(s,'(a+)+$','Z'); end.","",options);},"E_CANCELLED");
    try { (void)run("uses School; begin\nvar s:string:='[x';\nSwapSubstr(s,'[','x'); end."); require(false,"Expected regex error"); }
    catch (const abc::Error& caught) { require(caught.code=="E_REGEX" && caught.location.line==3 && std::string_view(caught.what())=="Invalid regular expression" && caught.russian=="Некорректное регулярное выражение","Regex diagnostic retains location and both languages"); }
    abc::Program empty; abc::Values values{empty,1'048'576};
    auto receiver=values.cell(abc::Type::string,std::u16string{u"a.b acb z"});
    error([&]{abc::school_swap_substr(values,receiver,std::u16string{u"a.b"},std::u16string{u"z"},{});},"E_KEY");
    require(abc::string_text(receiver->value)==u"a.b acb z","SwapSubstr writes only after all matches succeed");
    receiver->value=std::u16string{u"[x"};
    error([&]{abc::school_swap_substr(values,receiver,std::u16string{u"["},std::u16string{u"x"},{});},"E_REGEX");
    require(abc::string_text(receiver->value)==u"[x","Regex compilation failure leaves the receiver unchanged");
    receiver->value=std::u16string{u"ab"};
    const auto remaining=values.remaining;
    for (unsigned i=0;i<1000;++i) abc::school_swap_substr(values,receiver,std::u16string{u"a"},std::u16string{u"b"},{});
    require(abc::string_text(receiver->value)==u"ab" && values.remaining==remaining,"Regex temporary buffers do not cumulatively consume the value budget");
    receiver->value=std::u16string(10'000,u'a');
    error([&]{abc::school_swap_substr(values,receiver,std::u16string{},std::u16string(10'000,u'a'),{});},"E_LIMIT");
    std::vector<std::string> texts{""};
    for (unsigned length=1;length<=4;++length) for (unsigned bits=0;bits<(1U<<length);++bits) {
        std::string text;
        for (unsigned i=0;i<length;++i) text+=((bits>>i)&1U) ? 'b' : 'a';
        texts.push_back(std::move(text));
    }
    const std::vector<std::string> keys{"","a","b","aa","ab","ba","bb"};
    const auto array=[](const std::vector<std::string>& items) {
        std::string source="Arr(";
        for (const auto& item:items) { if (source!="Arr(") source+=","; source+="'"+item+"'+''"; }
        return source+")";
    };
    std::string expected;
    for (const auto& text:texts) for (const auto& first:keys) for (const auto& second:keys) {
        if (first==second) continue;
        if (text.find(first)==std::string::npos || text.find(second)==std::string::npos) { expected+=text+'\n'; continue; }
        std::string swapped;
        for (std::size_t i=0;i<=text.size();) {
            const auto tail=std::string_view(text).substr(i);
            const auto* key=tail.starts_with(first) ? &first : tail.starts_with(second) ? &second : nullptr;
            if (key) {
                swapped+=(key==&first ? second : first);
                if (!key->empty()) { i+=key->size(); continue; }
            }
            if (i==text.size()) break;
            swapped+=text[i++];
        }
        expected+=swapped+'\n';
    }
    result("uses School; begin var texts:="+array(texts)+"; var keys:="+array(keys)+"; foreach var text in texts do foreach var first in keys do foreach var second in keys do if first<>second then begin var s:string:=text; SwapSubstr(s,first,second); Writeln(s); end; end.",expected);
}

void regex_search_library() {
    result("begin Writeln(IsMatch('abc123','[0-9]+'),' ',Regex.IsMatch('abc','[0-9]+'),' ',PABCSystem.IsMatch('abc','^a')); Writeln('abc123'.MatchValue('[0-9]+'),' ',MatchValue('abc','x').Length); Writeln('Яя'.IsMatch('^Я'),' ',PABCSystem.MatchValue('a123b456','[0-9]+')); end.","True False True\n123 0\nTrue 123\n");
    result("begin Writeln(IsMatch('',''),' ',IsMatch('abc',''),' ',MatchValue('abc','').Length); Writeln(MatchValue('aaab','a+'),' ',MatchValue('ab','(a|ab)')); Writeln(IsMatch('Я','\\p{L}'),' ',IsMatch('١','\\d')); end.","True True 0\naaa a\nTrue True\n");
    result("begin Writeln(IsMatch('A','(?i)a'),' ',IsMatch('a'#13'b','a.b'),' ',IsMatch('a'#10'b','a.b')); Writeln(MatchValue('a'#0'b',#0).Length,' ',Ord(MatchValue(#55356#57101,'.')[1])); end.","True True False\n1 55356\n");
    result("var order:string; function Text:string; begin order:=order+'T'; Result:='abc123'; end; function Pattern:string; begin order:=order+'P'; Result:='[0-9]+'; end; begin Writeln(Text().MatchValue(Pattern()),' ',order); end.","123 TP\n");
    result("function IsMatch(s,p:string):boolean; begin Result:=False; end; begin Writeln(IsMatch('a','a'),' ',PABCSystem.IsMatch('a','a'),' ',Regex.IsMatch('a','a')); end.","False True True\n");
    result("type R=record IsMatch:integer; end; begin var Regex:R; Regex.IsMatch:=7; Writeln(Regex.IsMatch); end.","7\n");
    for (const auto source:{"begin IsMatch(nil,'a'); end.","begin MatchValue('abc',nil); end.",
        "begin var s:string:=nil; Writeln(s.IsMatch('a')); end.","begin var s:string:=nil; Writeln(s.MatchValue('a')); end."}) runtime_error(source,"E_NIL");
    for (const auto source:{"begin IsMatch('abc','['); end.","begin MatchValue('abc','['); end.",
        "begin IsMatch(nil,'['); end.","begin MatchValue(nil,'['); end.","begin IsMatch('abc','(*UTF)a'); end."}) runtime_error(source,"E_REGEX");
    for (const auto source:{"begin IsMatch(1,'a'); end.","begin IsMatch('ab'); end.","begin MatchValue('ab','a',0); end.",
        "begin Writeln('ab'.IsMatch('a',0)); end.","begin Regex.IsMatch('ab','a',0); end."}) compile_error(source,"E_TYPE");
    abc::ExecutionOptions options; options.allocation_limit=100;
    error([&]{(void)run("begin IsMatch('a','a'); end.","",options);},"E_LIMIT");
    options={}; options.instruction_limit=2000;
    error([&]{(void)run("begin IsMatch(StringOfChar('a',100)+'!','(a+)+$'); end.","",options);},"E_LIMIT");
    options={}; std::size_t polls=0; options.cancelled=[&]{return ++polls>12;};
    error([&]{(void)run("begin MatchValue(StringOfChar('a',100)+'!','(a+)+$'); end.","",options);},"E_CANCELLED");
    auto invalid=abc::compile("begin Writeln(IsMatch('ab','a')); end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::call_builtin) instruction.type=abc::Type::string;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
}

void sequence_to_list_library() {
    result("begin var a:=Arr(1,2); var l:List<integer>:=ToList(a); l[0]:=9; l.Add(3); Writeln(a,' ',l,' ',l.ToArray()); var b:=l.ToList; l[1]:=8; Writeln(b,' ',l); Writeln(PABCSystem.ToList(new integer[0]).Count); end.","[1,2] [9,2,3] [9,2,3]\n[9,2,3] [9,8,3]\n0\n");
    result("begin var a:=Arr(2bi,1bi shl 80); var s:sequence of BigInteger:=a; var l:=s.ToList(); Writeln(l,' ',l.Contains(2bi)); l.Clear; Writeln(s.ToArray()); end.","[2,1208925819614629174706176] True\n[2,1208925819614629174706176]\n");
    result("type R=record n:integer; a:array of integer; end; begin var r:R; r.n:=1; r.a:=Arr(7); var a:=Arr(r); var l:=a.ToList(); l[0].n:=9; l[0].a[0]:=8; Writeln(a[0].n,' ',l[0].n,' ',a[0].a[0]); end.","1 9 8\n");
    result("begin var a:=new List<integer>; a.Add(1); foreach var x in a do begin var b:=a.ToList(); b[0]:=8; b.Add(9); Writeln(b); end; Writeln(a); end.","[8,9]\n[1]\n");
    result("begin var a:=Arr(1,2); var s:sequence of integer:=a; a[1]:=7; Writeln(s.ToList); var l:=Arr('ab','cd').ToList; Writeln(l); end.","[1,7]\n[ab,cd]\n");
    result("uses School; begin var ip:=new CalcIP('10.0.0.1',30); var s:=ip.GenAddr; var a:=s.ToList; Writeln(a[0].Value,' ',a[1].Value); a[0].Value:='1.2.3.4'; Writeln(s.ToList()[0].Value); end.","10.0.0.1 10.0.0.2\n10.0.0.1\n");
    result("function ToList(a:array of integer):integer; begin Result:=7; end; begin var a:=Arr(1,2); Writeln(ToList(a),' ',PABCSystem.ToList(a)); end.","7 [1,2]\n");
    for (const auto source:{"begin var a:array of integer; ToList(a); end.","begin var a:List<integer>:=nil; a.ToList(); end.",
        "begin var a:sequence of integer:=nil; a.ToList(); end."}) runtime_error(source,"E_NIL");
    runtime_error("begin var a:=Arr(1,2).ToList(); foreach var x in a do a[0]:=9; end.","E_MODIFIED");
    runtime_error("type R=record n:integer; end; begin var r:R; var a:=Arr(r).ToList(); foreach var x in a do a[0].n:=9; end.","E_MODIFIED");
    for (const auto source:{"begin ToList(nil); end.","begin ToList(1); end.","begin ToList(Arr(1),2); end.",
        "begin var a:List<real>:=Arr(1).ToList(); end.","begin var a:=new integer[1,2]; a.ToList(); end.",
        "begin var a:=Arr(1); a.ToList(1); end."}) compile_error(source,"E_TYPE");
    abc::ExecutionOptions options; options.allocation_limit=8;
    error([&]{(void)run("begin var a:=Arr(1,2,3,4); Writeln(a.ToList()); end.","",options);},"E_LIMIT");
    options={}; options.instruction_limit=40;
    error([&]{(void)run("uses School; begin var ip:=new CalcIP('10.0.0.1',16); ip.GenAddr.ToList(); end.","",options);},"E_LIMIT");
    options={}; std::size_t polls=0; options.cancelled=[&]{return ++polls>4;};
    error([&]{(void)run("uses School; begin var ip:=new CalcIP('10.0.0.1',16); ip.GenAddr.ToList(); end.","",options);},"E_CANCELLED");
    auto invalid=abc::compile("begin var a:=Arr(1).ToList; Writeln(a.ToArray()); end.");
    std::size_t array=0;
    for (const auto& instruction:invalid.code) if (instruction.op==abc::Op::sequence_array) array=instruction.operand;
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::sequence_list) instruction.operand=array;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    invalid=abc::compile("begin Writeln(Arr(1).ToList()); end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::sequence_list) instruction.operand=0;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    const auto types=abc::compile("begin var a:=new integer[0]; var s:sequence of integer:=a; var l:=s.ToList(); end.");
    const auto find=[&](abc::CompoundKind kind) {
        for (std::size_t i=0;i<types.types.size();++i) if (types.types[i].kind==kind) return static_cast<abc::Type>(64+i);
        throw std::runtime_error("Missing materialization test type");
    };
    abc::Values values{types,1'048'576};
    const auto source_array=values.array(find(abc::CompoundKind::array),1024);
    const auto view=abc::sequence_create(values,find(abc::CompoundKind::sequence),source_array,abc::SequenceProvider::array);
    options={}; options.instruction_limit=20;
    error([&]{(void)abc::sequence_collect(values,view,find(abc::CompoundKind::list),options);},"E_LIMIT");
    options={}; options.cancelled=[]{return true;};
    error([&]{(void)abc::sequence_collect(values,view,find(abc::CompoundKind::list),options);},"E_CANCELLED");
}

void sequence_queries_library() {
    result("begin var a:=Arr(4,7,9); var s:sequence of integer:=a; Writeln(a.Any,' ',Any(s),' ',Any(new integer[0])); Writeln(s.First(),' ',s.Last,' ',s.ElementAt(1)); Writeln(First(a),' ',PABCSystem.Last(a),' ',ElementAt(a,2)); Writeln(s.Contains(7),' ',Contains(a,8),' ',a.Contains(9)); end.","True True False\n4 9 7\n4 9 9\nTrue False True\n");
    result("begin var a:=Arr(1,2).ToList(); Writeln(a.First,' ',a.LastOrDefault(),' ',a.ElementAtOrDefault(2)); Writeln(a.Contains(2),' ',Contains(a,1)); a.Clear(); Writeln(a.Any(),' ',a.FirstOrDefault,' ',a.LastOrDefault); end.","1 2 0\nTrue True\nFalse 0 0\n");
    result("begin var a:=new string[0]; Writeln(a.FirstOrDefault=nil,' ',a.LastOrDefault=nil,' ',a.ElementAtOrDefault(-1)=nil); var b:=new string[1]; Writeln(b.First=nil,' ',b.Contains(nil)); b[0]:=nil; Writeln(b.First=nil,' ',b.Contains(nil)); Writeln((new char[0]).FirstOrDefault= #0,' ',(new boolean[0]).LastOrDefault); Writeln((new BigInteger[0]).FirstOrDefault,' ',(new uint64[0]).LastOrDefault); end.","True True True\nFalse False\nTrue True\nTrue False\n0 0\n");
    result("type R=record x:integer; s:string; end; begin var r:R; r.x:=7; var a:=Arr(r); var copy:=a.First; copy.x:=8; Writeln(a.First.x,' ',a.Contains(r),' ',Contains(a,copy)); var d: R:=(new R[0]).FirstOrDefault; Writeln(d.x,' ',d.s=nil); end.","7 True False\n0 True\n");
    result("begin var a:=Arr(1bi,1bi shl 80); var s:sequence of BigInteger:=a; Writeln(s.First,' ',s.Last); Writeln(s.Contains(1),' ',s.Contains(1bi shl 80),' ',s.Contains(2)); var t:=Arr((1,'ab'),(2,'cd')); Writeln(t.First.Item1,' ',t.Contains((2,'cd'))); end.","1 1208925819614629174706176\nTrue True False\n1 True\n");
    result("uses School; begin var ip:=new CalcIP('10.0.0.1',30); var s:=ip.GenAddr; Writeln(s.Any,' ',s.First.Value,' ',s.Last.Value,' ',s.ElementAt(1).Value); Writeln(s.ElementAtOrDefault(2)=nil,' ',s.Contains(nil)); var z:=new CalcIP('10.0.0.1',31); Writeln(z.GenAddr.Any,' ',z.GenAddr.FirstOrDefault=nil); end.","True 10.0.0.1 10.0.0.2 10.0.0.2\nTrue False\nFalse True\n");
    result("var a:array of integer; function Source:sequence of integer; begin Write('S'); Result:=a; end; function Index:integer; begin Write('I'); a[1]:=7; a:=Arr(9); Result:=1; end; begin a:=Arr(1,2); Writeln(ElementAt(Source,Index),' ',a); end.","SI7 [9]\n");
    result("var a:List<integer>; function Target:integer; begin Write('T'); a.Add(2); Result:=2; end; begin a:=new List<integer>; a.Add(1); Writeln(Contains(a,Target),' ',a.Count); end.","TTrue 2\n");
    result("function First(a:array of integer):integer; begin Result:=99; end; begin var a:=Arr(7,9); Writeln(First(a),' ',PABCSystem.First(a),' ',a.First); end.","99 7 7\n");
    result("begin var a:=Arr(Arr(1)); var first:=a.First; first[0]:=7; Writeln(a[0][0]); end.","7\n");
    for (const auto source:{"begin First(new integer[0]); end.","begin Last(new integer[0]); end.",
        "begin var a:sequence of integer:=new integer[0]; a.First(); end."}) runtime_error(source,"E_EMPTY");
    for (const auto source:{"begin ElementAt(Arr(1),-1); end.","begin ElementAt(Arr(1),1); end.",
        "uses School; begin var ip:=new CalcIP('10.0.0.1',30); ip.GenAddr.ElementAt(2); end."}) runtime_error(source,"E_BOUNDS");
    for (const auto method:{"Any()","First()","Last()","FirstOrDefault()","LastOrDefault()","ElementAt(-1)","ElementAtOrDefault(-1)","Contains(1)"})
        runtime_error(std::string("begin var a:sequence of integer:=nil; a.")+method+"; end.","E_NIL");
    for (const auto source:{"begin Any(nil); end.","begin Any(Arr(1),True); end.","begin First(1); end.",
        "begin var a:=new integer[1,2]; First(a); end.","begin ElementAt(Arr(1),1.5); end.",
        "begin Contains(Arr(1),'ab'); end.","begin FirstOrDefault(Arr(1),7); end.",
        "begin var a:array of real; var x:integer:=a.First; end.","begin Writeln(Arr(1).Any(2)); end."}) compile_error(source,"E_TYPE");
    abc::ExecutionOptions options; options.instruction_limit=1000; options.allocation_limit=300;
    require(run("uses School; begin var ip:=new CalcIP('128.0.0.1',2); Writeln(ip.GenAddr.Any,' ',ip.GenAddr.First.Value,' ',ip.GenAddr.ElementAt(2).Value); end.","",options)=="True 128.0.0.1 128.0.0.3\n","Any/First/ElementAt stop within a huge generator without materializing it");
    error([&]{(void)run("uses School; begin var ip:=new CalcIP('128.0.0.1',2); ip.GenAddr.Last; end.","",options);},"E_LIMIT");
    auto invalid=abc::compile("begin Writeln(Arr(1).First); end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::sequence_query) instruction.operand=255;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    invalid=abc::compile("begin Writeln(Arr(1).First); end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::sequence_query) instruction.type=abc::Type::integer;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    invalid=abc::compile("begin Writeln(Arr(1).First); end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::sequence_query) instruction.operand=static_cast<std::size_t>(abc::SequenceQuery::contains);
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    const auto program=abc::compile("begin var a:=Arr(1,2); var s:sequence of integer:=a; var l:=a.ToList(); Writeln(s.First); end.");
    const auto find=[&](abc::CompoundKind kind) {
        for (std::size_t i=0;i<program.types.size();++i) if (program.types[i].kind==kind) return static_cast<abc::Type>(64+i);
        throw std::runtime_error("Missing query test type");
    };
    abc::Values values{program,1'048'576};
    const auto array=values.array(find(abc::CompoundKind::array),1024);
    const auto view=abc::sequence_create(values,find(abc::CompoundKind::sequence),array,abc::SequenceProvider::array);
    options={}; options.instruction_limit=1;
    require(std::get<bool>(abc::sequence_query(values,view,abc::SequenceQuery::contains,std::int32_t{0},options)),"Contains returns immediately on the first match");
    require(std::get<std::int32_t>(abc::sequence_query(values,view,abc::SequenceQuery::last,{},options))==0,"Last uses direct access for array views");
    error([&]{(void)abc::sequence_query(values,view,abc::SequenceQuery::contains,std::int32_t{1},options);},"E_LIMIT");
    options={}; options.cancelled=[]{return true;};
    error([&]{(void)abc::sequence_query(values,view,abc::SequenceQuery::contains,std::int32_t{1},options);},"E_CANCELLED");
    const auto list=values.array(find(abc::CompoundKind::list),1);
    values.own_list_cell(list->items[0],list);
    const auto list_view=abc::sequence_create(values,find(abc::CompoundKind::sequence),list,abc::SequenceProvider::list);
    options.cancelled=[&]{values.write(list->items[0],std::int32_t{7}); return false;};
    error([&]{(void)abc::sequence_query(values,list_view,abc::SequenceQuery::first,{},options);},"E_MODIFIED");
    try { (void)run("begin\nvar a:=new integer[0];\nFirst(a); end."); require(false,"Expected empty sequence diagnostic"); }
    catch (const abc::Error& caught) { require(caught.code=="E_EMPTY" && caught.location.line==3 && std::string_view(caught.what())=="Sequence contains no elements" && caught.russian=="Последовательность не содержит элементов","Empty sequence diagnostic carries location and both languages"); }
}

void sequence_single_library() {
    result("begin var a:=Arr(7); var s:sequence of integer:=a; Writeln(a.Single,' ',Single(s),' ',PABCSystem.SingleOrDefault(a)); var l:=a.ToList(); Writeln(l.SingleOrDefault()); l.Clear(); Writeln(l.SingleOrDefault,' ',(new string[0]).SingleOrDefault=nil); end.","7 7 7\n7\n0 True\n");
    result("type R=record x:integer; s:string; end; begin var r:R; r.x:=7; var a:=Arr(r); var b:=a.Single; b.x:=9; Writeln(a[0].x,' ',b.x); var d: R:=(new R[0]).SingleOrDefault; Writeln(d.x,' ',d.s=nil); var n:=new string[1]; n[0]:=nil; Writeln(n.Single=nil); end.","7 9\n0 True\nTrue\n");
    result("begin Writeln(Arr(1bi shl 80).Single,' ',(new BigInteger[0]).SingleOrDefault); Writeln(Arr((7,'ab')).Single.Item2); end.","1208925819614629174706176 0\nab\n");
    result("uses School; begin var ip:=new CalcIP('10.0.0.1',30); ip.Broadcast.Value:=ip.Network.Value; Writeln(ip.GenAddrBin.Single.Length,' ',SingleOrDefault(ip.GenAddrBin)=BinFormat(int64(ip.Network.Value10),4,'')); var z:=new CalcIP('10.0.0.1',31); Writeln(z.GenAddr.SingleOrDefault=nil); end.","32 True\nTrue\n");
    runtime_error("begin Single(new integer[0]); end.","E_EMPTY");
    for (const auto source:{"begin Single(Arr(1,2)); end.","begin SingleOrDefault(Arr(1,2)); end.",
        "begin var a:=Arr(1,2).ToList(); a.Single; end.",
        "uses School; begin var ip:=new CalcIP('10.0.0.1',30); ip.GenAddr.SingleOrDefault; end."}) runtime_error(source,"E_MANY");
    for (const auto source:{"begin var a:sequence of integer:=nil; a.Single; end.","begin var a:array of integer; SingleOrDefault(a); end."}) runtime_error(source,"E_NIL");
    for (const auto source:{"begin Single(nil); end.","begin SingleOrDefault(Arr(1),99); end.",
        "begin Writeln(Arr(1).Single(1)); end.","begin Single(new integer[1,1]); end."}) compile_error(source,"E_TYPE");
    abc::ExecutionOptions options; options.instruction_limit=1000; options.allocation_limit=300;
    error([&]{(void)run("uses School; begin var ip:=new CalcIP('128.0.0.1',2); ip.GenAddr.Single; end.","",options);},"E_MANY");
    try { (void)run("begin\nvar a:=Arr(1,2);\na.SingleOrDefault; end."); require(false,"Expected multiple sequence diagnostic"); }
    catch (const abc::Error& caught) { require(caught.code=="E_MANY" && caught.location.line==3 && std::string_view(caught.what())=="Sequence contains more than one element" && caught.russian=="Последовательность содержит более одного элемента","Single diagnostic retains location and both languages"); }
}

void sequence_factories_library() {
    result("begin var a:=Arr(1,2); var s:=Seq(a); var view:sequence of integer:=a; a[0]:=9; Writeln(s.ToArray,' ',view.ToArray); Writeln(PABCSystem.Seq(3,4,5).ToList); Writeln(Seq(new integer[0]).Any); end.","[1,2] [9,2]\n[3,4,5]\nFalse\n");
    result("begin Writeln(Seq(byte(1),integer(2),int64(3)).ToArray); var a:=Arr(1); var b:=Arr(2); var nested:=Seq(a,b); a[0]:=7; Writeln(nested.ToArray); Writeln(Seq((1,'ab'),(2,'cd')).First.Item2); var s:string:=nil; Writeln(Seq(s,'ab').First=nil); end.","[1,2,3]\n[[7],[2]]\nab\nTrue\n");
    result("type R=record x:integer; a:array of integer; end; begin var r:R; r.x:=1; r.a:=Arr(7); var a:=Arr(r); var s:=Seq(a); a[0].x:=9; var copy:=s.First; copy.x:=4; copy.a[0]:=8; Writeln(s.First.x,' ',a[0].x,' ',s.First.a[0]); end.","1 9 8\n");
    result("begin Writeln(SeqFill(3,7).ToArray,' ',PABCSystem.SeqFill(0,7).Any); var s:=SeqFill(2,'ab'); foreach var x in s do Write(x); Writeln; foreach var x in s do Write(x); Writeln; end.","[7,7,7] False\nabab\nabab\n");
    result("type R=record x:integer; a:array of integer; end; begin var r:R; r.x:=1; r.a:=Arr(7); var s:=SeqFill(2,r); r.x:=9; var copy:=s.First; copy.x:=8; copy.a[0]:=4; Writeln(s.First.x,' ',s.Last.x,' ',s.Last.a[0]); var a:=s.ToArray; a[0].x:=5; Writeln(a[1].x,' ',s.First.x); end.","1 1 4\n1 1\n");
    result("begin var a:=Arr(1); var s:=SeqFill(2,a); a[0]:=7; Writeln(s.First=s.Last,' ',s.ToArray); var text:string:=nil; Writeln(SeqFill(1,text).Single=nil,' ',SeqFill(0,'ab').FirstOrDefault=nil); Writeln(SeqFill(2,1bi shl 80).ToArray); end.","True [[7],[7]]\nTrue True\n[1208925819614629174706176,1208925819614629174706176]\n");
    result("uses School; begin var a:=new Addr32('10.0.0.1'); var s:=SeqFill(2,a); a.Value:='10.0.0.7'; Writeln(s.First=s.Last,' ',s.First.Value); end.","True 10.0.0.7\n");
    result("begin var matrix:=new integer[1,1]; var s:=Seq(matrix); matrix[0,0]:=7; Writeln(s.First[0,0]); var l:=new List<integer>; l.Add(1); Writeln(Seq(l).First=l); end.","7\nTrue\n");
    result("function CountValue:integer; begin Write('N'); Result:=2; end; function ElementValue:integer; begin Write('V'); Result:=7; end; begin Writeln(SeqFill(CountValue,ElementValue).ToArray); end.","NV[7,7]\n");
    result("function Seq(n:integer):integer; begin Result:=99; end; begin Writeln(Seq(1),' ',PABCSystem.Seq(1).First); end.","99 1\n");
    for (const auto source:{"begin Seq(); end.","begin Seq(nil); end.","begin SeqFill(1,nil); end.",
        "begin SeqFill(1.5,7); end.","begin SeqFill(1); end.","begin Seq(1,True); end.",
        "begin var a:sequence of real:=Seq(1,2); end."}) compile_error(source,"E_TYPE");
    runtime_error("begin var a:array of integer; Seq(a); end.","E_NIL");
    runtime_error("begin SeqFill(-1,7); end.","E_RANGE");
    const auto negative=abc::compile("function ElementValue:integer; begin Write('V'); Result:=7; end; begin SeqFill(-1,ElementValue); end.");
    std::istringstream input; std::ostringstream output;
    error([&]{abc::execute(negative,input,output);},"E_RANGE");
    require(output.str()=="V","SeqFill evaluates its element argument before validating a negative count");
    abc::ExecutionOptions options; options.allocation_limit=20; options.instruction_limit=200;
    require(run("begin var s:=SeqFill(MaxInt,7); Writeln(s.First,' ',s.Any); foreach var x in s do begin Writeln(x); break; end; end.","",options)=="7 True\n7\n","SeqFill retains a single value for a huge lazy sequence");
    error([&]{(void)run("begin SeqFill(MaxInt,7).ToArray; end.","",options);},"E_LIMIT");
    options={}; options.instruction_limit=100;
    error([&]{(void)run("begin Count(SeqFill(1000,7)); end.","",options);},"E_LIMIT");
    std::string large="begin var s:=PABCSystem.Seq(";
    for (unsigned i=0;i<256;++i) { if (i) large+=","; large+=std::to_string(i); }
    large+="); Writeln(s.Count,' ',s.Last); end."; result(large,"256 255\n");
    auto invalid=abc::compile("begin var s:=Seq(1,2); end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::sequence_snapshot) instruction.operand=0;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    invalid=abc::compile("begin var l:=new List<integer>; var s:sequence of integer:=l; end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::sequence_wrap) instruction.op=abc::Op::sequence_snapshot;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    invalid=abc::compile("begin var s:=SeqFill(2,7); end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::sequence_repeat) instruction.operand=1;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    invalid=abc::compile("begin var s:=SeqFill(2,7); end.");
    invalid.constants[0]=std::int64_t{2}; invalid.code[0].type=abc::Type::int64;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    const auto program=abc::compile("begin var a:=new integer[0]; var s:=Seq(a); end.");
    const auto find=[&](abc::CompoundKind kind) {
        for (std::size_t i=0;i<program.types.size();++i) if (program.types[i].kind==kind) return static_cast<abc::Type>(64+i);
        throw std::runtime_error("Missing factory test type");
    };
    abc::Values values{program,1'048'576}; const auto array=values.array(find(abc::CompoundKind::array),1024);
    options={}; options.instruction_limit=10;
    error([&]{(void)abc::sequence_snapshot(values,find(abc::CompoundKind::sequence),array,options);},"E_LIMIT");
    options={}; options.cancelled=[]{return true;};
    error([&]{(void)abc::sequence_snapshot(values,find(abc::CompoundKind::sequence),array,options);},"E_CANCELLED");
    values.remaining=2;
    error([&]{(void)abc::sequence_snapshot(values,find(abc::CompoundKind::sequence),array,{});},"E_LIMIT");
    try { (void)run("begin\nSeqFill(-1,7);\nend."); require(false,"Expected sequence count diagnostic"); }
    catch (const abc::Error& caught) { require(caught.code=="E_RANGE" && caught.location.line==2 && std::string_view(caught.what())=="Sequence count cannot be negative" && caught.russian=="Число элементов последовательности не может быть отрицательным","SeqFill count error retains source position and both languages"); }
}

void sequence_ranges_library() {
    result("begin Writeln(Range(1,4).ToArray,' ',PABCSystem.Range(byte(2),word(3)).ToList); Writeln(Range(3,1).Any,' ',Range(2,2).Single); foreach var c in Range('а','в') do Write(c); Writeln; Writeln(Range('a','c').Contains('b')); end.","[1,2,3,4] [2,3]\nFalse 2\nабв\nTrue\n");
    result("begin Writeln(Range(not(MaxInt),not(MaxInt)+1).ToArray,' ',Range(MaxInt,MaxInt).Single); Writeln(Ord(Range(#65534,#65535).Last),' ',Range('z','a').Count); Writeln(Range(3,1)=Range(9,2),' ',Range(1,2)=Range(1,2)); end.","[-2147483648,-2147483647] 2147483647\n65535 0\nTrue False\n");
    result("function FirstBound:integer; begin Write('A'); Result:=3; end; function LastBound:integer; begin Write('B'); Result:=5; end; begin Writeln(Range(FirstBound,LastBound).ToArray); end.","AB[3,4,5]\n");
    result("begin var s:=Range(1,2); foreach var x in s do foreach var y in s do Write(x,y,';'); Writeln; var a:=s.ToArray; a[0]:=9; Writeln(s.First,' ',s.Last); end.","11;12;21;22;\n1 2\n");
    result("uses School; begin Writeln(Range(-3,4).MinMax); end.","(-3,4)\n");
    result("function Range(a,b:integer):integer; begin Result:=99; end; begin Writeln(Range(1,2),' ',PABCSystem.Range(1,2).ToArray); end.","99 [1,2]\n");
    for (const auto source:{"begin Range(0,MaxInt); end.","begin Range(not(MaxInt),0); end.","begin Range(not(MaxInt),MaxInt); end."}) runtime_error(source,"E_OVERFLOW");
    runtime_error("begin Range(1,2).ElementAt(2); end.","E_BOUNDS");
    for (const auto source:{"begin Range(1,3,'x'); end.","begin Range(1bi,2.5); end.","begin Range(1.0,2.0); end.",
        "begin Range('ab','cd'); end.","begin Range(1); end.","begin Range(nil,nil); end.",
        "begin var s:sequence of real:=Range(1,2); end."}) compile_error(source,"E_TYPE");
    abc::ExecutionOptions options; options.allocation_limit=20; options.instruction_limit=200;
    require(run("begin var s:=Range(1,MaxInt); Writeln(s.First,' ',s.Any); foreach var x in s do begin Writeln(x); break; end; end.","",options)=="1 True\n1\n","Large Range is lazy and supports early termination with a constant allocation budget");
    error([&]{(void)run("begin Range(1,MaxInt).ToArray; end.","",options);},"E_LIMIT");
    options={}; options.instruction_limit=100;
    error([&]{(void)run("begin Count(Range(1,1000)); end.","",options);},"E_LIMIT");
    auto invalid=abc::compile("begin var s:=Range(1,2); end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::sequence_range) instruction.operand=2;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    invalid=abc::compile("begin var s:=Range(1,2); var r:=SeqFill(2,1.5); end.");
    abc::Type reals=abc::Type::invalid;
    for (std::size_t i=0;i<invalid.types.size();++i) if (invalid.types[i].kind==abc::CompoundKind::sequence && invalid.types[i].element==abc::Type::real) reals=static_cast<abc::Type>(64+i);
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::sequence_range) instruction.type=reals;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
}

void stepped_ranges_library() {
    result("begin Writeln(Range(1,8,3).ToArray,' ',Range(8,1,-3).ToList); Writeln(PABCSystem.Range(byte(1),word(5),smallint(2)).ToArray); Writeln(Range(1,5,-1).Any,' ',Range(5,1,1).Count); end.","[1,4,7] [8,5,2]\n[1,3,5]\nFalse 0\n");
    result("begin Writeln(Range('а','ж',2).ToArray,' ',Range('z','t',-2).ToList); Writeln(Ord(Range(#65535,#0,-65535).Last),' ',Range('a','z',MaxInt).First); end.","[а,в,д,ж] [z,x,v,t]\n0 a\n");
    result("begin Writeln(Range(not(MaxInt),0,MaxInt).ToArray); Writeln(Range(MaxInt,0,not(MaxInt)).ToArray); Writeln(Range(7,7,1).Single,' ',Range(7,7,-1).Single); end.","[-2147483648,-1]\n[2147483647]\n7 7\n");
    result("function A:integer; begin Write('A'); Result:=1; end; function B:integer; begin Write('B'); Result:=6; end; function S:integer; begin Write('S'); Result:=2; end; begin Writeln(Range(A,B,S).ToArray); end.","ABS[1,3,5]\n");
    result("begin var s:=Range(1,3,2); foreach var x in s do foreach var y in s do Write(x,y,';'); Writeln; Writeln(s.ToArray,' ',s.ToArray); end.","11;13;31;33;\n[1,3] [1,3]\n");
    result("begin var s:=Range(1,2,0); Writeln('created'); Writeln(s.ElementAtOrDefault(-1)); end.","created\n0\n");
    result("begin var s:=Range(MaxInt,MaxInt,1); Writeln(s.First,' ',s.Any,' ',s.Contains(MaxInt),' ',s.ElementAt(0)); foreach var x in s do begin Writeln(x); break; end; end.","2147483647 True True 2147483647\n2147483647\n");
    result("begin foreach var x in Range(not(MaxInt),not(MaxInt),-1) do begin Writeln(x); break; end; foreach var x in Range('a','a',MaxInt) do begin Writeln(x); break; end; end.","-2147483648\na\n");
    result("begin var n:=0; foreach var x in Range(5,0,-2) do begin if x=3 then continue; n:=n+x; end; Writeln(n); end.","6\n");
    result("uses School; begin Writeln(Range(-5,5,3).MinMax); end.","(-5,4)\n");
    for (const auto suffix:{"Any","First","FirstOrDefault","Last","Single","SingleOrDefault","Count","ToArray","ToList","Contains(1)","ElementAt(0)"})
        runtime_error("begin Range(1,2,0)."+std::string(suffix)+"; end.","E_RANGE");
    runtime_error("begin foreach var x in Range(2,1,0) do Writeln(x); end.","E_RANGE");
    runtime_error("uses School; begin Range(1,2,0).MinMax; end.","E_RANGE");
    for (const auto suffix:{"Last","LastOrDefault","Single","SingleOrDefault","Count","ToArray","ToList","Contains(0)","ElementAt(1)","ElementAtOrDefault(1)"})
        runtime_error("begin Range(MaxInt,MaxInt,1)."+std::string(suffix)+"; end.","E_OVERFLOW");
    runtime_error("uses School; begin Range(MaxInt,MaxInt,1).MinMax; end.","E_OVERFLOW");
    runtime_error("begin Range('a','a',MaxInt).Last; end.","E_OVERFLOW");
    runtime_error("begin Range(not(MaxInt),not(MaxInt),-1).Count; end.","E_OVERFLOW");
    runtime_error("begin Range(MaxInt,not(MaxInt),not(MaxInt)).ToArray; end.","E_OVERFLOW");
    runtime_error("begin Range(MaxInt-1,MaxInt,1).Single; end.","E_MANY");
    for (const auto source:{"begin Range(1,3,True); end.","begin Range(1,3,nil); end.","begin Range(1,3,1,1); end.","begin Range('ab','cd',1); end.","begin Range(1bi,3bi,1.5); end."}) compile_error(source,"E_TYPE");
    for (const auto source:{"begin foreach var x in Range(MaxInt,MaxInt,1) do Write(x); end.","begin foreach var x in Range(MaxInt,MaxInt,1) do begin Write(x); continue; end; end."}) {
        auto p=abc::compile(source); std::istringstream in; std::ostringstream out;
        error([&]{abc::execute(p,in,out);},"E_OVERFLOW");
        require(out.str()=="2147483647","Final Range value is observable before overflow on resume");
    }
    abc::ExecutionOptions options; options.allocation_limit=20; options.instruction_limit=200;
    require(run("begin var s:=Range(not(MaxInt),MaxInt,1); Writeln(s.First,' ',s.Any); foreach var x in s do begin Writeln(x); break; end; end.","",options)=="-2147483648 True\n-2147483648\n","Full-width stepped Range keeps a constant allocation budget and a 64-bit length");
    error([&]{(void)run("begin Range(not(MaxInt),MaxInt,1).ToArray; end.","",options);},"E_LIMIT");
    options={}; options.instruction_limit=100;
    error([&]{(void)run("begin Range(1,1000,1).Last; end.","",options);},"E_LIMIT");
    try { (void)run("begin\nvar s:=Range(1,2,0);\ns.Any;\nend."); require(false,"Expected zero-step diagnostic"); }
    catch (const abc::Error& caught) { require(caught.code=="E_RANGE" && caught.location.line==3 && std::string_view(caught.what())=="Sequence step cannot be zero" && caught.russian=="Шаг последовательности не может быть равен нулю","Range zero-step error is deferred to enumeration with both languages"); }
    auto invalid=abc::compile("begin foreach var x in Range(1,2,1) do Writeln(x); end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::sequence_resume) instruction.operand=1;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    invalid=abc::compile("begin var s:=Range(1,2,1); end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::sequence_range) instruction.operand=0;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    // Independent small-domain corpus: integer arithmetic gives exact expected values.
    for (int a=-3;a<=3;++a) for (int b=-3;b<=3;++b) for (int step:{-3,-1,1,3}) {
        std::string expected="["; bool comma=false;
        for (int x=a;step>0 ? x<=b : x>=b;x+=step) { if (comma) expected+=","; comma=true; expected+=std::to_string(x); }
        expected+="]\n";
        result("begin Writeln(Range("+std::to_string(a)+","+std::to_string(b)+","+std::to_string(step)+").ToArray); end.",expected);
    }
}

void big_ranges_library() {
    result("begin var s:sequence of BigInteger:=Range(2bi,5bi); Writeln(s.ToArray,' ',PABCSystem.Range(8bi,2bi,-3bi).ToList); Writeln(Range(3bi,1bi).Any,' ',Range(1bi,3bi,-1bi).Count,' ',Range(7bi,7bi).Single); end.","[2,3,4,5] [8,5,2]\nFalse 0 7\n");
    result("begin Writeln(Range(1,5bi,word(2)).ToArray,' ',Range(byte(5),1,-2bi).ToArray); Writeln(Range(int64(9223372036854775806),MaxInt64,1bi).ToArray); Writeln(Range(MaxUInt64,18446744073709551617bi).ToArray); end.","[1,3,5] [5,3,1]\n[9223372036854775806,9223372036854775807]\n[18446744073709551615,18446744073709551616,18446744073709551617]\n");
    result("begin var start:=1000000000000000000000000000000bi; var s:=Range(start,start+6,2); Writeln(s.ToArray); Writeln(s.First=start,' ',s.Last=start+6,' ',s.ElementAt(2)=start+4,' ',s.Contains(start+2)); Writeln(s.Count,' ',s.ElementAtOrDefault(9),' ',Range(1bi,0bi).FirstOrDefault); end.","[1000000000000000000000000000000,1000000000000000000000000000002,1000000000000000000000000000004,1000000000000000000000000000006]\nTrue True True True\n4 0 0\n");
    result("function A:BigInteger; begin Write('A'); Result:=1; end; function B:BigInteger; begin Write('B'); Result:=6; end; function S:BigInteger; begin Write('S'); Result:=2; end; begin Writeln(Range(A,B,S).ToArray); end.","ABS[1,3,5]\n");
    result("begin var s:=Range(1bi,3bi); foreach var x in s do foreach var y in s do Write(x,y,';'); Writeln; Writeln(s.ToArray,' ',s.ToArray); var a:=s.ToArray; a[0]:=9; Writeln(s.First); end.","11;12;13;21;22;23;31;32;33;\n[1,2,3] [1,2,3]\n1\n");
    result("begin var a:=1bi; var b:=5bi; var step:=2bi; var s:=Range(a,b,step); a:=9; b:=0; step:=0; Writeln(s.ToArray); var r:=Range(1bi,2bi,0bi); Writeln('created ',r.ElementAtOrDefault(-1)); end.","[1,3,5]\ncreated 0\n");
    result("uses School; begin Writeln(Range(-10bi,10bi,3bi).MinMax,' ',Range(1bi,0bi).MinMax); end.","(-10,8) (0,0)\n");
    result("begin var n:=0bi; foreach var x in Range(5bi,0bi,-2bi) do begin if x=3bi then continue; n:=n+x; end; Writeln(n); end.","6\n");
    result("begin var p:=1bi shl 65535; var s:=Range(-p,p,1bi); Writeln(s.First= -p,' ',s.Any,' ',s.ElementAt(1)= -p+1); foreach var x in s do begin Writeln(x= -p); break; end; end.","True True True\nTrue\n");
    result("begin var p:=1bi shl 65535; var s:=Range(p,p,p); Writeln(s.First=p,' ',s.Contains(p)); foreach var x in s do begin Writeln(x=p); break; end; end.","True True\nTrue\n");
    for (const auto suffix:{"Any","First","Last","Single","Count","ToArray","ToList","Contains(1bi)","ElementAt(0)"})
        runtime_error("begin Range(1bi,2bi,0bi)."+std::string(suffix)+"; end.","E_RANGE");
    runtime_error("begin foreach var x in Range(2bi,1bi,0bi) do Writeln(x); end.","E_RANGE");
    runtime_error("uses School; begin Range(1bi,2bi,0bi).MinMax; end.","E_RANGE");
    runtime_error("begin Range(1bi,3bi).Single; end.","E_MANY");
    runtime_error("begin Range(3bi,1bi).First; end.","E_EMPTY");
    runtime_error("begin Range(1bi,3bi).ElementAt(3); end.","E_BOUNDS");
    runtime_error("begin Range(1bi,3bi).ElementAt(-1); end.","E_BOUNDS");
    for (const auto suffix:{"Last","Single","Count","ToArray","ToList","Contains(0bi)","ElementAt(1)"})
        runtime_error("begin var p:=1bi shl 65535; Range(p,p,p)."+std::string(suffix)+"; end.","E_LIMIT");
    for (const auto source:{"begin Range(1bi,3bi,True); end.","begin Range(1bi,3bi,1.5); end.","begin var s:sequence of integer:=Range(1bi,3bi); end.","begin var s:sequence of real:=Range(1bi,3bi); end."}) compile_error(source,"E_TYPE");
    abc::ExecutionOptions options; options.allocation_limit=64; options.instruction_limit=500;
    require(run("begin var s:=Range(0bi,1bi shl 100); Writeln(s.First,' ',s.Any); foreach var x in s do begin Writeln(x); break; end; end.","",options)=="0 True\n0\n","BigInteger Range with more than uint64 elements starts with bounded memory");
    error([&]{(void)run("begin Range(0bi,1bi shl 100).Count; end.","",options);},"E_LIMIT");
    error([&]{(void)run("begin Range(0bi,1bi shl 100).ToList; end.","",options);},"E_LIMIT");
    const auto p=abc::compile("begin var s:=Range(1bi,3bi); end.");
    abc::Type type=abc::Type::invalid;
    for (std::size_t i=0;i<p.types.size();++i) if (p.types[i].kind==abc::CompoundKind::sequence) type=static_cast<abc::Type>(64+i);
    abc::Values values{p,1000}; const auto a=abc::BigInteger::from_signed(1),b=abc::BigInteger::from_signed(3);
    const auto s=abc::sequence_range_big(values,type,a,b,a),cursor=abc::sequence_begin(values,s);
    require(!abc::sequence_known_length(cursor,{}).has_value(),"BigInteger Range does not precompute or narrow its length");
    options={}; options.cancelled=[]{return true;};
    error([&]{(void)abc::sequence_resume(values,cursor,0,options);},"E_CANCELLED");
    options={};
    require(abc::sequence_resume(values,cursor,0,options) && abc::sequence_resume(values,cursor,0,options),"Cursor resume is idempotent at the same position");
    require(std::get<abc::BigInteger>(abc::sequence_at(values,cursor,0,options))==a,"Repeated resume does not advance BigInteger values");
    require(abc::sequence_resume(values,cursor,1,options) && std::get<abc::BigInteger>(abc::sequence_at(values,cursor,1,options))==abc::BigInteger::from_signed(2),"BigInteger cursor advances exactly once");
    error([&]{(void)abc::sequence_resume(values,cursor,3,options);},"E_BYTECODE");
    error([&]{(void)abc::sequence_resume(values,cursor,0,options);},"E_BYTECODE");
    auto invalid=abc::compile("begin var s:=Range(1bi,2bi,1bi); end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::sequence_range) instruction.operand=0;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    // Exercise every direction against a fixed-integer independent corpus.
    for (int first=-3;first<=3;++first) for (int last=-3;last<=3;++last) for (int step:{-3,-1,1,3}) {
        std::string expected="["; bool comma=false;
        for (int x=first;step>0 ? x<=last : x>=last;x+=step) { if (comma) expected+=","; comma=true; expected+=std::to_string(x); }
        expected+="]\n";
        result("begin Writeln(Range("+std::to_string(first)+"bi,"+std::to_string(last)+"bi,"+std::to_string(step)+"bi).ToArray); end.",expected);
    }
}

void partition_points_library() {
    result("begin var s:sequence of real:=PartitionPoints(0,1,4); Writeln(s.ToArray,' ',s.Count); Writeln(Linspace(1,0,5).ToList); Writeln(PABCSystem.Linspace(2,2,3).ToArray,' ',PABCSystem.PartitionPoints(3,1,2).ToArray); end.","[0,0.25,0.5,0.75,1] 5\n[1,0.75,0.5,0.25,0]\n[2,2,2] [3,2,1]\n");
    result("begin var s:=Linspace(0,1,11); Writeln(s.Count,' ',s.Last<>1,' ',s.ElementAt(1)=0.1); var a:=1e16; var b:=a+4; var p:=PartitionPoints(a,b,4); Writeln(p.Count,' ',p.Last=a,' ',p.Last<>b); end.","11 True True\n5 True True\n");
    result("begin var s:=PartitionPoints(1,3,2); foreach var x in s do foreach var y in s do Write(x,':',y,';'); Writeln; Writeln(s.ToArray,' ',s.ToArray); var a:=s.ToArray; a[0]:=9; Writeln(s.First); end.","1:1;1:2;1:3;2:1;2:2;2:3;3:1;3:2;3:3;\n[1,2,3] [1,2,3]\n1\n");
    result("uses School; begin Writeln(PartitionPoints(-1,1,4).MinMax,' ',Linspace(1,1,2).Contains(1.0)); end.","(-1,1) True\n");
    result("function A:real; begin Write('A'); Result:=0; end; function B:real; begin Write('B'); Result:=1; end; function N:integer; begin Write('N'); Result:=2; end; begin Writeln(PartitionPoints(A,B,N).ToArray); Writeln(Linspace(A,B,N).ToArray); end.","ABN[0,0.5,1]\nABN[0,1]\n");
    result("begin var p:=PartitionPoints(0,1,0); var s:=Linspace(0,1,1); Writeln('created ',p.ElementAtOrDefault(-1),' ',s.ElementAtOrDefault(-1)); end.","created 0 0\n");
    result("begin var s:=PartitionPoints(0,1e308,1); Writeln(s.First,' ',s.Any,' ',s.Contains(1e308),' ',s.ElementAt(1)=1e308); foreach var x in s do begin Writeln(x); break; end; end.","0 True True True\n0\n");
    for (const auto suffix:{"Any","First","Last","Single","Count","ToArray","ToList","Contains(0.0)","ElementAt(0)"}) {
        runtime_error("begin PartitionPoints(0,1,0)."+std::string(suffix)+"; end.","E_RANGE");
        runtime_error("begin Linspace(0,1,1)."+std::string(suffix)+"; end.","E_RANGE");
    }
    runtime_error("begin PartitionPoints(0,1,-1).Any; end.","E_RANGE");
    runtime_error("begin Linspace(0,1,0).Any; end.","E_RANGE");
    runtime_error("begin Linspace(0,1,not(MaxInt)); end.","E_OVERFLOW");
    runtime_error("begin foreach var x in PartitionPoints(0,1,0) do Writeln(x); end.","E_RANGE");
    runtime_error("uses School; begin PartitionPoints(0,1,0).MinMax; end.","E_RANGE");
    for (const auto suffix:{"Last","Single","Count","ToArray","ToList","Contains(-1.0)","ElementAt(2)"})
        runtime_error("begin PartitionPoints(0,1e308,1)."+std::string(suffix)+"; end.",std::string_view(suffix)=="Single" ? "E_MANY" : "E_OVERFLOW");
    runtime_error("begin PartitionPoints(-1e308,1e308,1).First; end.","E_OVERFLOW");
    for (const auto source:{"begin PartitionPoints(0,1,2.0); end.","begin Linspace(0,1,2bi); end.","begin PartitionPoints(0bi,1,2); end.","begin Linspace(0,1); end.","begin var s:sequence of integer:=Linspace(0,1,2); end."}) compile_error(source,"E_TYPE");
    abc::ExecutionOptions options; options.allocation_limit=64; options.instruction_limit=500;
    require(run("begin var s:=PartitionPoints(0,1,MaxInt); Writeln(s.First,' ',s.Any); foreach var x in s do begin Writeln(x); break; end; end.","",options)=="0 True\n0\n","PartitionPoints supports n+1 above int32 without materialization");
    error([&]{(void)run("begin PartitionPoints(0,1,MaxInt).ToArray; end.","",options);},"E_LIMIT");
    error([&]{(void)run("begin Linspace(0,1,1000).Last; end.","",options);},"E_LIMIT");
    auto p=abc::compile("begin var s:=PartitionPoints(0,1,2); end.");
    for (auto& instruction:p.code) if (instruction.op==abc::Op::sequence_partition) instruction.operand=2;
    error([&]{abc::verify(p);},"E_BYTECODE");
    p=abc::compile("begin var s:=Linspace(0,1,2); var a:=Range(1,2); end.");
    abc::Type integer_sequence=abc::Type::invalid;
    for (std::size_t i=0;i<p.types.size();++i) if (p.types[i].kind==abc::CompoundKind::sequence && p.types[i].element==abc::Type::integer) integer_sequence=static_cast<abc::Type>(64+i);
    for (auto& instruction:p.code) if (instruction.op==abc::Op::sequence_partition) instruction.type=integer_sequence;
    error([&]{abc::verify(p);},"E_BYTECODE");
    try { (void)run("begin\nvar s:=PartitionPoints(0,1,0);\ns.Any;\nend."); require(false,"Expected partition count diagnostic"); }
    catch (const abc::Error& caught) { require(caught.code=="E_RANGE" && caught.location.line==3 && std::string_view(caught.what())=="Partition count must be positive" && caught.russian=="Число частей разбиения должно быть положительным","Partition count validation is deferred with both languages and the consumer position"); }
}

void real_ranges_library() {
    result("begin var s:sequence of real:=Range(0,1,0.25); Writeln(s.ToArray,' ',s.Count); Writeln(PABCSystem.Range(1.0,0,-0.25).ToList); Writeln(Range(int64(1),3,int64(1)).ToArray); end.","[0,0.25,0.5,0.75,1] 5\n[1,0.75,0.5,0.25,0]\n[1,2,3]\n");
    result("begin var s:=Range(0,1,0.1); Writeln(s.Count,' ',s.Last=1,' ',s.ToArray[10]=1,' ',s.Contains(1.0),' ',s.ElementAt(3)=3*0.1); Writeln(Range(1,0,-0.1).Last=0); Writeln(Range(0,1,0.3).Last=3*0.3); end.","11 True True True True\nTrue\nTrue\n");
    result("begin Writeln(Range(0,1,0.4).Count,' ',Range(0,1,0.4).Last=0.8); Writeln(Range(0,1,0.6).Count,' ',Range(0,1,0.6).Last=0.6); Writeln(Range(0,1,2.0).Single,' ',Range(0,1,3.0).Single); Writeln(Range(3,0,0.5).Any,' ',Range(0,3,-0.5).Count); end.","3 True\n2 True\n0 0\nFalse 0\n");
    result("begin Writeln(Range(0,1,0.500000000000005).Last=1,' ',Range(0,1,0.500000000000006).Count); Writeln(Range(0,1,0.499999999999995).Last=1,' ',Range(0,1,0.499999999999994).Last<>1); end.","True 2\nTrue True\n");
    result("begin var a:=1e14; var s:=Range(a,a+1,0.01); Writeln(s.Count,' ',s.ElementAt(5)=a+5*0.01,' ',s.Last=a+1); Writeln(Range(0,7.92281625142643e28,7.92281625142643e28).Count); end.","101 True True\n2\n");
    result("begin var s:=Range(0,1,0.5); foreach var x in s do foreach var y in s do Write(x,':',y,';'); Writeln; var a:=s.ToArray; a[0]:=9; Writeln(s.ToList,' ',s.ToArray); end.","0:0;0:0.5;0:1;0.5:0;0.5:0.5;0.5:1;1:0;1:0.5;1:1;\n[0,0.5,1] [0,0.5,1]\n");
    result("uses School; begin Writeln(Range(-1,1,0.5).MinMax); var s:=Range(1,2,0.0); Writeln('created ',s.ElementAtOrDefault(-1)); end.","(-1,1)\ncreated 0\n");
    result("function A:real; begin Write('A'); Result:=0; end; function B:real; begin Write('B'); Result:=1; end; function S:real; begin Write('S'); Result:=0.5; end; begin Writeln(Range(A,B,S).ToArray); end.","ABS[0,0.5,1]\n");
    result("begin Writeln(Range(1e40,1e40,1e40).Single=1e40,' ',Range(1e40,0,1e40).Any); end.","True False\n");
    for (const auto suffix:{"Any","First","Last","Count","ToArray","ToList","Contains(0.0)","ElementAt(0)","SingleOrDefault"}) runtime_error("begin Range(0,1,0.0)."+std::string(suffix)+"; end.","E_RANGE");
    runtime_error("begin foreach var x in Range(0,1,0.0) do Writeln(x); end.","E_RANGE");
    runtime_error("uses School; begin Range(0,1,0.0).MinMax; end.","E_RANGE");
    for (const auto source:{"begin Range(0,1,1e-29).Any; end.","begin Range(0.0,1e-29,1e-30).First; end.","begin Range(1e16,1e16+2,1.0).First; end.","begin Range(0.0,1e28,1e-28).First; end."}) runtime_error(source,"E_DIV_ZERO");
    for (const auto source:{"begin Range(0,1e29,1.0).First; end.","begin Range(-7e28,7e28,1e28).First; end.","begin Range(0,1,1e29).First; end.","begin Range(0,1e29,1e-99).First; end."}) runtime_error(source,"E_OVERFLOW");
    runtime_error("begin Range(0.0,2147483648,1).First; end.","E_OVERFLOW");
    runtime_error("begin Range(0,1,0.5).Single; end.","E_MANY");
    runtime_error("begin Range(1,0,0.5).First; end.","E_EMPTY");
    runtime_error("begin Range(0,1,0.5).ElementAt(3); end.","E_BOUNDS");
    for (const auto source:{"begin Range(0.0,1.0); end.","begin Range(0.0,1.0,True); end.","begin Range(0bi,1,0.5); end.","begin var s:sequence of integer:=Range(0,1,0.5); end."}) compile_error(source,"E_TYPE");
    abc::ExecutionOptions options; options.allocation_limit=512; options.instruction_limit=10000;
    require(run("begin var s:=Range(0.0,MaxInt,1); Writeln(s.First,' ',s.Any); foreach var x in s do begin Writeln(x); break; end; end.","",options)=="0 True\n0\n","Real Range supports more than int32 elements with bounded planning and early exits");
    error([&]{(void)run("begin Range(0.0,MaxInt,1).ToArray; end.","",options);},"E_LIMIT");
    options.allocation_limit=64;
    error([&]{(void)run("begin Range(0,1,0.5).First; end.","",options);},"E_LIMIT");
    const auto p=abc::compile("begin end."); abc::Values values{p,300}; options={};
    const auto plan=abc::real_range_plan(values,0,1,0.4,options);
    require(plan.prefix==2 && plan.count==3 && !plan.snap,"Decimal reciprocal uses midpoint-to-even rounding");
    require(values.remaining==300,"Decimal planning releases its bounded scratch allocation");
    options.cancelled=[]{return true;};
    error([&]{(void)abc::real_range_plan(values,0,1,0.5,options);},"E_CANCELLED");
    require(values.remaining==300,"Decimal planning releases scratch after cancellation");
    options={}; options.instruction_limit=1;
    error([&]{(void)abc::real_range_plan(values,0,1,0.5,options);},"E_LIMIT");
    require(values.remaining==300,"Decimal planning releases scratch after a work limit");
    try { (void)run("begin\nvar s:=Range(0,1e29,1.0);\ns.Any;\nend."); require(false,"Expected Decimal overflow"); }
    catch (const abc::Error& caught) { require(caught.code=="E_OVERFLOW" && caught.location.line==3 && std::string_view(caught.what())=="Decimal overflow" && caught.russian=="Переполнение десятичного числа","Decimal overflow is deferred and bilingual at the consumer position"); }
    auto invalid=abc::compile("begin var s:=Range(0,1,0.5); end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::sequence_range) instruction.operand=0;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    // Exact quarter-unit corpus, with independent fixed-integer expected values.
    const auto quarter=[](int n){
        const auto magnitude=n<0 ? -n : n; const std::array<std::string_view,4> suffix={"",".25",".5",".75"};
        return (n<0 ? std::string{"-"} : std::string{})+std::to_string(magnitude/4)+std::string(suffix[magnitude%4]);
    };
    for (int a=-4;a<=4;++a) for (int b=-4;b<=4;++b) for (int step:{-3,-1,1,3}) {
        std::string expected="["; bool comma=false;
        for (int x=a;step>0 ? x<=b : x>=b;x+=step) { if (comma) expected+=","; comma=true; expected+=quarter(x); }
        expected+="]\n";
        result("begin Writeln(Range("+quarter(a)+","+quarter(b)+","+quarter(step)+").ToArray); end.",expected);
    }
}

void school_minmax_library() {
    result("uses School; begin Writeln(MinMax(Arr(4,-7,9,2)),' ',School.MinMax(Arr(int64(-9223372036854775808),MaxInt64))); var a:=new List<real>; a.Add(1.5); a.Add(-2.25); Writeln(a.MinMax); Writeln(MinMax(new integer[0]),' ',MinMax(new int64[0])); var r:=MinMax(new real[0]); Writeln(r.Item1>1e308,' ',r.Item2< -1e308); end.","(-7,9) (-9223372036854775808,9223372036854775807)\n(-2.25,1.5)\n(2147483647,-2147483648) (9223372036854775807,-9223372036854775808)\nTrue True\n");
    result("uses School; function MinMax(a:array of integer):integer; begin Result:=7; end; begin var a:=Arr(4,2,8); Writeln(MinMax(a),' ',School.MinMax(a),' ',a.MinMax()); end.","7 (2,8) (2,8)\n");
    result("uses School; begin SetRandomSeed(1); MinMax(Arr(3,1,2)); Writeln(Random(100)); end.","24\n");
    std::string literal="Arr("; std::int32_t low=INT32_MAX,high=INT32_MIN; std::uint32_t state=17;
    for (unsigned i=0;i<1000;++i) {
        state=1664525U*state+1013904223U; const auto number=static_cast<std::int32_t>(state%200001)-100000;
        if (number<low) low=number;
        if (number>high) high=number;
        if (i) literal+=",";
        literal+=std::to_string(number);
    }
    literal+=")";
    result("uses School; begin Writeln(MinMax("+literal+")); end.","("+std::to_string(low)+","+std::to_string(high)+")\n");
    runtime_error("uses School; begin var a:array of integer; MinMax(a); end.","E_NIL");
    runtime_error("uses School; begin var a:List<int64>:=nil; MinMax(a); end.","E_NIL");
    for (const auto source:{"uses School; begin MinMax(new byte[0]); end.","uses School; begin MinMax(new integer[1,2]); end.",
        "uses School; begin MinMax(Arr('ab')); end.","uses School; begin MinMax(nil); end.",
        "uses School; begin MinMax(Arr(1),Arr(2)); end.","uses School; begin var a:=Arr(1); a.MinMax(1); end.",
        "uses School; begin var t:(integer,integer):=MinMax(Arr(int64(1))); end."}) compile_error(source,"E_TYPE");
    compile_error("begin MinMax(Arr(1)); end.","E_NAME");
    auto program=abc::compile("uses School; begin MinMax(Arr(1,2)); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::school_minmax) instruction.operand=64;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("uses School; begin MinMax(Arr(1,2)); end.");
    program.types.back().fields[1].type=abc::Type::int64;
    error([&]{abc::verify(program);},"E_BYTECODE");
    abc::ExecutionOptions options; options.instruction_limit=100;
    error([&]{(void)run("uses School; begin MinMax(new integer[10000]); end.","",options);},"E_LIMIT");
    options={}; std::size_t polls=0; options.cancelled=[&]{return ++polls>20;};
    error([&]{(void)run("uses School; begin MinMax(new integer[10000]); end.","",options);},"E_CANCELLED");
    options={}; options.allocation_limit=1;
    error([&]{(void)run("uses School; begin MinMax(new integer[0]); end.","",options);},"E_LIMIT");
}

void tuple_library() {
    result("uses School; type Pair=(int64,string); function Data:Pair; begin Result:=(42,'answer'); end; begin var p:=Data; Writeln(p,' ',p[0],' ',p.Item2); Writeln(Rec(1,'ab',True)); Writeln(НОДНОК(-18,30),' ',School.НОДНОК(-9223372036854775808,0)); end.","(42,answer) 42 answer\n(1,ab,True)\n(-6,90) (-9223372036854775808,0)\n");
    result("begin var a:(integer,integer); var b:(integer,integer):=nil; Writeln(a=nil,' ',a=b,' ',a=(0,0)); a:=(1,2); b:=a; a:=(3,4); Writeln(a,' ',b,' ',b=(1,2),' ',b<>(1,3)); end.","True True False\n(3,4) (1,2) True True\n");
    result("type R=record x:integer; a:array of integer; end; begin var r:R; r.x:=1; r.a:=Arr(2); var t:=(r,True); r.x:=9; r.a[0]:=7; Writeln(t); var c:=t.Item1; c.x:=3; Writeln(t.Item1.x,' ',c.x); t.Item1.a[0]:=8; Writeln(r.a[0]); end.","((1,[7]),True)\n1 3\n8\n");
    result("begin var a:=new List<(integer,string)>; a.Add((1,'ab')); Writeln(a.Contains((1,'ab')),' ',a.Contains((2,'ab')),' ',a); var t:=((1,2),Arr(3)); var u:=((1,2),t.Item2); Writeln(t=u); end.","True False [(1,ab)]\nTrue\n");
    result("type T=(integer,integer,integer,integer,integer,integer,integer); begin var t:=new T(1,2,3,4,5,6,7); Writeln(t,' ',t[6]); var a:=new (integer,integer)[2]; Writeln(a); a[0]:=(1,2); Writeln(a); end.","(1,2,3,4,5,6,7) 7\n[nil,nil]\n[(1,2),nil]\n");
    result("function N(x:integer):integer; begin Write(x); Result:=x; end; begin var t:=(N(1),N(2),N(3)); Writeln('|',t); end.","123|(1,2,3)\n");
    result("type T=(integer,integer); procedure Change(var t:T); begin t:=(3,4); end; begin var t:=(1,2); var u:=t; Change(t); Writeln(t,' ',u); end.","(3,4) (1,2)\n");
    result("function Rec(a,b:integer):integer; begin Result:=7; end; begin Writeln(Rec(1,2),' ',PABCSystem.Rec(1,2)); end.","7 (1,2)\n");
    for (const auto source:{"begin var t:=(1,2); t.Item1:=3; end.","begin var t:=(1,2); t[0]:=3; end.",
        "begin var t:=(1,2); Inc(t.Item1); end.","type T=(integer,integer); begin var t:=new T; end.",
        "begin var t:=(1,2); var i:=0; Writeln(t[i]); end.","begin var t:=(nil,1); end.",
        "begin var t:(int64,int64); var p:=(1,2); t:=p; end.","uses School; begin НОДНОК(1.5,2); end.",
        "uses School; begin var t:(integer,integer):=НОДНОК(2,4); end."}) compile_error(source,"E_TYPE");
    compile_error("begin var t:=(1,2); Writeln(t[2]); end.","E_RANGE");
    compile_error("begin var t:=(1,2,3,4,5,6,7,8); end.","E_UNSUPPORTED");
    compile_error("begin НОДНОК(2,4); end.","E_NAME");
    runtime_error("begin var t:(integer,string); Writeln(t.Item1); end.","E_NIL");
    runtime_error("uses School; begin НОДНОК(0,0); end.","E_DIV_ZERO");
    runtime_error("uses School; begin НОДНОК(-9223372036854775808,-1); end.","E_OVERFLOW");
    auto program=abc::compile("begin var t:=(1,2); Writeln(t.Item1); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::get_field) instruction.op=abc::Op::ref_field;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin var t:=(1,2); end.");
    program.types[0].fields[0].name="wrong";
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin var t:=(1,2); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::tuple_literal) instruction.op=abc::Op::school_pair;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin var t:=(1,2); end.");
    program.code.push_back({abc::Op::make_default,static_cast<abc::Type>(64),1,{}});
    error([&]{abc::verify(program);},"E_BYTECODE");
    abc::ExecutionOptions options; options.allocation_limit=1;
    error([&]{(void)run("begin Writeln((1,2)); end.","",options);},"E_LIMIT");
}

void unicode_character_library() {
    result("begin Writeln(LowerCase('Ё'),' ',UpperCase('я'),' ',UpCase('ς'),' ',LowCase('Ä')); Writeln('Ж'.ToLower,' ','ж'.ToUpper(),' ','Я'.Code); Writeln(Code(#65535)); end.","ё Я Σ ä\nж Ж 1071\n65535\n");
    result("begin Writeln('Я'.IsLetter,' ','字'.IsLetter,' ','ª'.IsLetter,' ','Ⅳ'.IsLetter,' ',#55296.IsLetter); Writeln('١'.IsDigit,' ','９'.IsDigit,' ','²'.IsDigit,' ','0'.IsDigit); Writeln('ß'.IsLower,' ','ǅ'.IsUpper,' ','Σ'.IsUpper,' ','中'.IsLower); end.","True True True False False\nTrue True False True\nTrue False True False\n");
    result("begin var s:string:='ЯЁÄΣ𐐀'; Writeln(LowerCase(s),' ',s.ToLower()); s:='яёäς𐐨'; Writeln(UpperCase(s),' ',s.ToUpper); Writeln(Length(UpperCase('ß'+'')),' ',UpperCase('ß'+'')); end.","яёäσ𐐨 яёäσ𐐨\nЯЁÄΣ𐐀 ЯЁÄΣ𐐀\n1 ß\n");
    result("begin Writeln(Ord(LowerCase(#55296)),' ',Ord(UpperCase(#56320))); var s:=#55296'Я'#56320; var t:=s.ToLower; Writeln(Length(t),' ',Ord(t[1]),' ',t[2],' ',Ord(t[3])); Writeln(LowerCase('')='',' ',UpperCase('')=''); end.","55296 56320\n3 55296 я 56320\nTrue True\n");
    result("function UpperCase(c:char):char; begin Result:='X'; end; begin Writeln(UpperCase('я'),' ',PABCSystem.UpperCase('я'),' ','я'.ToUpper); end.","X Я Я\n");
    runtime_error("begin var s:string; s:=nil; LowerCase(s); end.","E_NIL");
    runtime_error("begin var s:string:=nil; s.ToUpper; end.","E_NIL");
    runtime_error("begin var c:='١'; c.ToDigit; end.","E_FORMAT");
    for (const auto source:{"begin IsLetter('ab'); end.","begin IsDigit(1); end.","begin var c:char:=LowerCase('ab'); end.",
        "begin var s:='ab'; s.ToUpper(1); end.","begin var c:='я'; c.IsUpper(1); end."}) compile_error(source,"E_TYPE");
    std::string expected;
    for (char c='A';c<='Z';++c) expected+=char(c+32);
    result("begin for var c:='A' to 'Z' do Write(c.ToLower); end.",expected);
    abc::ExecutionOptions options; options.instruction_limit=100;
    error([&]{(void)run("begin LowerCase(StringOfChar('A',10000)); end.","",options);},"E_LIMIT");
    options={}; std::size_t polls=0; options.cancelled=[&]{return ++polls>20;};
    std::string supplementary="x"; for (unsigned i=0;i<10000;++i) supplementary+="𐐨";
    error([&]{(void)run("begin UpperCase('"+supplementary+"'); end.","",options);},"E_CANCELLED");
}

void unicode_identifiers_and_school_aliases() {
    result("program Проверка; type Запись=record Поле:integer; end; var ДАННЫЕ:Запись; function Удвоить(Число:integer):integer; begin Result:=число*2; end; begin данные.поле:=21; Writeln(УДВОИТЬ(Данные.ПОЛЕ)); end.","42\n");
    result("begin var ЁЖ:=7; var Äpfel:=3; var ΔΕΛΤΑ:=5; var 数字2:=9; Writeln(ёж,' ',äPFEL,' ',δελτα,' ',数字2); end.","7 3 5 9\n");
    result("uses School; begin Writeln(НОД(18,30),' ',нок(18,30),' ',School.НоД(-18,30),' ',School.НоК(-18,30)); Writeln(НОД(-9223372036854775808,2),' ',НОК(0,-9223372036854775808)); end.","6 90 6 90\n2 0\n");
    result("uses School; function НОД(a,b:integer):integer; begin Result:=7; end; begin Writeln(НОД(18,30),' ',School.НОД(18,30)); end.","7 6\n");
    result("uses School; begin Writeln(School.Dec('ſ',36),' ',School.Dec('ı',36),' ',School.Dec('ſı',36)); end.","28 18 1026\n");
    std::string expected;
    for (int a= -12;a<=12;++a) for (int b= -12;b<=12;++b) {
        auto left=a,right=b;
        while (right) { const auto rem=left%right; left=right; right=rem; }
        const auto gcd=left<0 ? -left : left;
        expected+=std::to_string(gcd)+" "+std::to_string(gcd ? (a<0 ? -a : a)/gcd*(b<0 ? -b : b) : 0)+"\n";
    }
    result("uses School; begin for var a:=-12 to 12 do for var b:=-12 to 12 do if (a<>0) or (b<>0) then Writeln(НОД(a,b),' ',НОК(a,b)) else Writeln(НОД(a,b),' ',0); end.",expected);
    compile_error("begin var СЧЁТ:=1; var счёт:=2; end.","E_DUPLICATE");
    compile_error("type Р=record Ё:integer; ё:integer; end; begin end.","E_DUPLICATE");
    compile_error("begin var á:=1; end.","E_SYNTAX");
    compile_error("begin var 𐐀:=1; end.","E_SYNTAX");
    compile_error("begin var a١:=1; end.","E_SYNTAX");
    compile_error("begin НОД(2,4); end.","E_NAME");
    compile_error("uses School; begin НОК(1.5,2); end.","E_TYPE");
    compile_error("uses School; begin var n:integer:=НОД(2,4); end.","E_TYPE");
    runtime_error("uses School; begin НОК(0,0); end.","E_DIV_ZERO");
    for (const auto source:{"uses School; begin НОД(-9223372036854775808,0); end.",
        "uses School; begin НОД(-9223372036854775808,-1); end.",
        "uses School; begin НОК(-9223372036854775808,2); end.","uses School; begin НОК(MaxInt64,2); end."}) runtime_error(source,"E_OVERFLOW");
    for (const auto& source:{std::string{"begin var ноль:=0; Writeln(1 div ноль); end."},std::string{"begin Writeln('я🌍'); неизвестное; end."}}) {
        const auto offset=source.find(source.find("div")!=std::string::npos ? "div" : "неизвестное");
        try { (void)run(source); require(false,"Expected a located Unicode diagnostic"); }
        catch (const abc::Error& caught) {
            require(caught.location.offset==offset && caught.location.column==abc::from_utf8(source.substr(0,offset)).size()+1,
                "Diagnostics retain byte offsets and count UTF-16 columns after Cyrillic and supplementary text");
        }
    }
}

void school_fold_library() {
    result("uses School; begin var a:=Arr(int64(18),30,42); var g:int64:=GCD(a); Writeln(g,' ',LCM(a),' ',GCD(18,30,42)); Writeln(School.GCD(18,30),' ',School.LCM(4,6),' ',PABCSystem.GCD(18,30)); end.","6 630 6\n6 12 6\n");
    result("uses School; begin var narrow:integer:=GCD(18,30); var wide:int64:=School.GCD(18,30); Writeln(narrow,' ',wide); Writeln(GCD(-7),' ',LCM(-7),' ',School.GCD(-9223372036854775808),' ',School.LCM(-9223372036854775808)); end.","6 6\n-7 -7 -9223372036854775808 -9223372036854775808\n");
    result("uses School; begin Writeln(School.LCM(-4,6),' ',School.LCM(4,-6),' ',School.LCM(-4,-6),' ',LCM(2,3,4)); Writeln(School.GCD(-4,-6),' ',GCD(0,0),' ',LCM(0,0),' ',LCM(int64(0),-9223372036854775808)); end.","-12 12 -12 12\n2 0 0 0\n");
    result("uses School; function GCD(n:integer):integer; begin Result:=7; end; begin Writeln(GCD(12),' ',School.GCD(12),' ',PABCSystem.GCD(12,18)); end.","7 12 6\n");
    result("uses School; function N(x:integer):int64; begin Write(x); Result:=x; end; begin Writeln('|',GCD(N(2),N(4),N(6))); end.","246|2\n");
    result("uses School; begin SetRandomSeed(1); GCD(2,4,6); LCM(2,3,4); Writeln(Random(100)); end.","24\n");
    for (const auto source:{"uses School; begin GCD(); end.","uses School; begin LCM(); end.",
        "uses School; begin School.GCD(nil); end.","uses School; begin var a:array of int64; LCM(a); end.",
        "uses School; begin GCD(new int64[0]); end."}) runtime_error(source,"E_RANGE");
    runtime_error("uses School; begin School.LCM(0,0); end.","E_DIV_ZERO");
    for (const auto source:{"uses School; begin School.LCM(MaxInt64,MaxInt64); end.",
        "uses School; begin School.LCM(0,-9223372036854775808); end.",
        "uses School; begin School.GCD(-9223372036854775808,0); end."}) runtime_error(source,"E_OVERFLOW");
    result("uses School; begin Writeln(LCM(MaxInt64,MaxInt64)); end.","9223372036854775807\n");
    for (const auto source:{"uses School; begin GCD(Arr(2,4)); end.","uses School; begin LCM(new List<int64>); end.",
        "uses School; begin GCD(1.5); end.",
        "uses School; begin var n:integer:=School.GCD(2,4); end.","begin GCD(2,4,6); end."}) compile_error(source,"E_TYPE");
    auto program=abc::compile("uses School; begin GCD(2,4,6); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::school_fold) instruction.operand=2;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("uses School; begin GCD(2,4,6); end.");
    program.types[0].element=abc::Type::integer;
    error([&]{abc::verify(program);},"E_BYTECODE");
    abc::ExecutionOptions options; options.instruction_limit=100;
    error([&]{(void)run("uses School; begin GCD(new int64[10000]); end.","",options);},"E_LIMIT");
    options={}; std::size_t polls=0; options.cancelled=[&]{return ++polls>20;};
    error([&]{(void)run("uses School; begin GCD(new int64[10000]); end.","",options);},"E_CANCELLED");
    std::string many="School.GCD(1";
    for (unsigned i=1;i<4096;++i) many+=",1";
    result("uses School; begin Writeln("+many+")); end.","1\n");
    compile_error("uses School; begin "+many+",1); end.","E_SYNTAX");
}

void school_text_library() {
    std::vector<std::string> texts{""}; std::size_t start=0;
    for (unsigned length=1;length<=4;++length) {
        const auto end=texts.size();
        for (auto i=start;i<end;++i) for (char c:std::string_view{"abc"}) texts.push_back(texts[i]+c);
        start=end;
    }
    const std::vector<std::string> mappings(texts.begin(),texts.begin()+13);
    const auto array=[](const std::vector<std::string>& words) {
        std::string result="Arr(";
        for (std::size_t i=0;i<words.size();++i) { if (i) result+=","; result+="'"+words[i]+"'"; }
        return result+")";
    };
    std::string expected;
    for (const auto& text:texts) for (const auto& source:mappings) for (const auto& target:mappings) {
        auto sequential=text,last=text; std::string translated;
        for (std::size_t i=0;i<source.size();++i) {
            std::string next;
            for (char c:sequential) if (c!=source[i]) next+=c; else if (i<target.size()) next+=target[i];
            sequential=std::move(next);
        }
        for (char c:text) { const auto i=source.find(c); if (i==std::string::npos) translated+=c; else if (i<target.size()) translated+=target[i]; }
        if (!source.empty()) { const auto i=last.rfind(source); if (i!=std::string::npos) last.replace(i,source.size(),target); }
        expected+=sequential+"|"+translated+"|"+last+"\n";
    }
    result("uses School; begin var texts:="+array(texts)+"; var sources:="+array(mappings)+"; var targets:="+array(mappings)+"; foreach var text in texts do foreach var source in sources do foreach var target in targets do begin var last:=text; ReplaceLast(last,source,target); Writeln(ReplaceMany(text,source,target),'|',Translate(text,source,target),'|',last); end; end.",expected);
    result("uses School; begin Writeln(ReplaceMany('абба','аб','ба'),' ',Translate('абба','аб','ба')); var s:='ааа'; School.ReplaceLast(s,'аа','X'); Writeln(s); Writeln('aaa'.ReplaceMany(Arr('aa'),Arr('X'+''))); end.","аааа бааб\nаX\nXa\n");
    result("uses School; begin var a:=new List<string>; a.Add('ab'); a.Add('X'); Writeln(ReplaceMany('abab',a,Arr('X'+'','Y'))); Writeln('abcabc'.ReplaceMany(Arr('ab','c'),Arr('X'+''))); Writeln(ReplaceMany('aa',Arr('a'+''),Arr('ba'))); end.","YY\nXX\nbaba\n");
    result("uses School; begin var sources:=new List<string>; sources.Add('ab'); var targets:=new List<string>; targets.Add(nil); Writeln(ReplaceMany('ababC',sources,targets)); Writeln(ReplaceMany('abc',new string[0],targets)); end.","C\nabc\n");
    result("uses School; begin var s:string:=ReplaceMany(nil,'',''); Writeln(s=nil,' ',Length(s)); Writeln(Translate('',nil,nil)='',' ',Translate('abc','x',nil)); Writeln(Translate('ab','aa','Z'),' ',Translate('ab','ab','X')); end.","False 0\nTrue abc\nZb X\n");
    result("uses School; begin var s:string:=nil; ReplaceLast(s,nil,nil); Writeln(s=nil); s:='abc'; ReplaceLast(s,'x','y'); Writeln(s); ReplaceLast(s,s,'X'); Writeln(s); end.","True\nabc\nX\n");
    result("uses School; var a:array of string; function Index:integer; begin Write('I'); Result:=0; end; function Source:string; begin Write('S'); a[0]:='bbb'; Result:='bb'; end; function Target:string; begin Write('T'); Result:='X'; end; begin a:=Arr('aaa'); ReplaceLast(a[Index],Source,Target); Writeln('|',a[0]); end.","IST|bX\n");
    result("uses School; begin var a:=new List<string>; a.Add('abc'); foreach var s in a do ReplaceLast(a[0],'zz','X'); Writeln(a); end.","[abc]\n");
    result("uses School; begin Writeln(School.ToInts64('2147483648 -9223372036854775808 9223372036854775807')); Writeln(' 1'#9'2'#10'3'#13'4 '.ToInts64); Writeln(ToInts64(''),' ',ToInts64('  '),' ',ToInts64(#11'42'#11)); end.","[2147483648,-9223372036854775808,9223372036854775807]\n[1,2,3,4]\n[] [] [42]\n");
    result("uses School; function B:boolean; begin Write('B'); Result:=False; end; begin Writeln(Imp(False,False),' ',False.Imp(True),' ',True.Imp(False),' ',School.Imp(True,True)); Writeln(False.Imp(B)); end.","True True False True\nBTrue\n");
    result("uses School; function Translate(s,a,b:string):string; begin Result:='own'; end; begin Writeln(Translate('ab','ab','bc'),' ',School.Translate('ab','ab','bc'),' ','ab'.Translate('ab','bc')); end.","own bc bc\n");
    for (const auto source:{"uses School; begin ReplaceMany('',nil,''); end.","uses School; begin ReplaceMany('','',nil); end.",
        "uses School; begin Translate(nil,'',''); end.","uses School; begin Translate('a',nil,''); end.","uses School; begin Translate('a','a',nil); end.",
        "uses School; begin var s:string:=nil; ReplaceLast(s,'a','b'); end.","uses School; begin ToInts64(nil); end.",
        "uses School; begin var a:array of string:=nil; ReplaceMany('',new string[0],a); end.",
        "uses School; begin var a:=new List<string>; a.Add(nil); ReplaceMany('',a,Arr('X'+'')); end.",
        "uses School; begin ReplaceMany('',Arr('a'+''),nil); end."}) runtime_error(source,"E_NIL");
    runtime_error("uses School; begin ReplaceMany('',Arr(''),Arr('X'+'')); end.","E_RANGE");
    runtime_error("uses School; begin ToInts64('9223372036854775808'); end.","E_OVERFLOW");
    runtime_error("uses School; begin ToInts64('1'#160'2'); end.","E_FORMAT");
    runtime_error("uses School; begin var a:=new List<string>; a.Add('abc'); foreach var s in a do ReplaceLast(a[0],'a','a'); end.","E_MODIFIED");
    runtime_error("uses School; begin ReplaceMany('aa',Arr('a'+''),Arr(StringOfChar('X',1048576))); end.","E_LIMIT");
    runtime_error("uses School; begin var s:='ab'; ReplaceLast(s,'a',StringOfChar('X',1048576)); end.","E_LIMIT");
    for (const auto source:{"begin ReplaceMany('a','b','c'); end.","begin Translate('a','b','c'); end.","begin var s:='a'; ReplaceLast(s,'a','b'); end.","begin ToInts64('1'); end.","begin Imp(True,False); end."}) compile_error(source,"E_NAME");
    for (const auto source:{"uses School; begin ReplaceLast('abc','a','b'); end.","uses School; begin foreach var s in Arr('aa') do ReplaceLast(s,'a','b'); end.",
        "uses School; begin ReplaceMany('a',Arr(1),Arr('b')); end.","uses School; begin ReplaceMany('a',Arr('a'),'b'); end.",
        "uses School; begin ToInts64('1',2); end.","uses School; begin var a:array of integer:=ToInts64('1'); end.","uses School; begin Imp(1,2); end."}) compile_error(source,"E_TYPE");
    auto program=abc::compile("uses School; begin var s:='abc'; ReplaceLast(s,'a','b'); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::replace_last) instruction.operand=1;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("uses School; begin ReplaceMany('abc',Arr('a'+''),Arr('b'+'')); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::replace_many) instruction.operand=static_cast<std::size_t>(abc::Type::string);
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("uses School; begin ToInts64('1'); end.");
    program.types[0].element=abc::Type::integer;
    error([&]{abc::verify(program);},"E_BYTECODE");
    abc::ExecutionOptions options; options.allocation_limit=5;
    error([&]{(void)run("uses School; begin ToInts64('1 2 3 4 5 6'); end.","",options);},"E_LIMIT");
    options={}; options.instruction_limit=50'000;
    require(run("uses School; begin Writeln(Length(ReplaceMany(StringOfChar('a',10000)+'b',Arr(StringOfChar('a',5000)+'c'),Arr('X'+'')))); end.","",options)=="10001\n","Substring replacement scans repetitive text within linear work budget");
    options={}; std::size_t polls=0; options.cancelled=[&]{return ++polls>20;};
    error([&]{(void)run("uses School; begin Translate(StringOfChar('a',10000),'a','b'); end.","",options);},"E_CANCELLED");
    std::string wide_array="Arr(0";
    for (unsigned i=1;i<4096;++i) wide_array+=",0";
    result("begin Writeln(Length("+wide_array+"))); end.","4096\n");
    result("begin var flag:=True; while flag do begin Writeln(Length("+wide_array+"))); flag:=False; end; end.","4096\n");
    compile_error("begin "+wide_array+",0); end.","E_SYNTAX");
    abc::Program invalid_merge;
    invalid_merge.constants={true,std::int32_t{0},0.0};
    invalid_merge.code={{abc::Op::push,abc::Type::boolean,0,{}},{abc::Op::jump_false,abc::Type::boolean,4,{}},
        {abc::Op::push,abc::Type::integer,1,{}},{abc::Op::jump,abc::Type::invalid,5,{}},{abc::Op::push,abc::Type::real,2,{}},
        {abc::Op::drop,abc::Type::integer,0,{}},{abc::Op::halt,abc::Type::invalid,0,{}}};
    error([&]{abc::verify(invalid_merge);},"E_BYTECODE");
}

void school_factor_library() {
    const auto list=[](const std::vector<std::int64_t>& numbers) {
        std::string text="[";
        for (std::size_t i=0;i<numbers.size();++i) { if (i) text+=","; text+=std::to_string(numbers[i]); }
        return text+"]";
    };
    std::string expected;
    for (std::int64_t n= -2;n<=1000;++n) {
        std::vector<std::int64_t> factors,unique,divisors;
        auto remaining=n;
        for (std::int64_t d=2;remaining>=2 && d<=remaining/d;++d) while (remaining%d==0) { factors.push_back(d); remaining/=d; }
        if (remaining>=2) factors.push_back(remaining);
        for (const auto d:factors) if (unique.empty() || unique.back()!=d) unique.push_back(d);
        if (n<2) divisors.push_back(1);
        else for (std::int64_t d=1;d<=n;++d) if (n%d==0) divisors.push_back(d);
        expected+=list(factors)+"|"+list(unique)+"|"+std::to_string(factors.size())+"|"+std::to_string(unique.size())+"|"+list(divisors)+"|"+std::to_string(divisors.size())+"\n";
    }
    for (const auto wide:{false,true}) {
        const std::string argument=wide ? "int64(n)" : "n";
        result("uses School; begin for var n:=-2 to 1000 do Writeln(PrimeFactors("+argument+"),'|',PrimeDivisors("+argument+"),'|',PrimeFactorsCount("+argument+"),'|',PrimeDivisorsCount("+argument+"),'|',Divisors("+argument+"),'|',DivisorsCount("+argument+")); end.",expected);
    }
    result("uses School; begin Writeln(Factorize(360),' ',School.PrimeFactors(360),' ',(360).PrimeDivisors); Writeln((360).PrimeFactorsCount,' ',(360).PrimeDivisorsCount,' ',(360).DivisorsCount); Writeln(Divizors(12),' ',(12).Divizors,' ',int64(12).Divisors); end.","[2,2,2,3,3,5] [2,2,2,3,3,5] [2,3,5]\n6 3 24\n[1,2,3,4,6,12] [1,2,3,4,6,12] [1,2,3,4,6,12]\n");
    result("uses School; begin var a:List<int64>:=Factorize(longword(4294967295)); var b:List<integer>:=byte(12).Factorize; Writeln(a,' ',b); Writeln(int64(600851475143).Factorize); Writeln(PrimeFactors(MaxInt64),' ',PrimeFactorsCount(MaxInt64),' ',PrimeDivisorsCount(MaxInt64),' ',DivisorsCount(MaxInt64)); end.","[3,5,17,257,65537] [2,2,3]\n[71,839,1471,6857]\n[7,7,73,127,337,92737,649657] 7 6 96\n");
    result("uses School; begin Writeln(Factorize(4611686014132420609)); Writeln(Factorize(9223372036854775783)); Writeln(Divisors(9223372036854775783)); end.","[2147483647,2147483647]\n[9223372036854775783]\n[1,9223372036854775783]\n");
    result("uses School; begin Writeln(Factorize(1000000016000000063)); Writeln(DivisorsCount(1000000016000000063)); end.","[1000000007,1000000009]\n4\n");
    {
        const auto prime=[](std::int64_t n) { for (std::int64_t d=2;d<=n/d;++d) if (n%d==0) return false; return n>=2; };
        constexpr std::int64_t p=3037000493,q=3037000453;
        require(prime(p) && prime(q),"Independent trial division proves both near-limit factors prime");
        result("uses School; begin Writeln(Factorize("+std::to_string(p*q)+")); end.","[3037000453,3037000493]\n");
    }
    result("uses School; begin Writeln(Factorize(-2147483648),' ',Factorize(int64(-2147483648))); Writeln(Divisors(-2147483648),' ',Divisors(int64(-2147483648))); end.","[] []\n[1] [1]\n");
    result("uses School; begin var a:=FirstPrimes(4794); a[0]:=17; Writeln(Factorize(4),' ',Factorize(int64(4)),' ',Divisors(4)); a.Clear; Writeln(Divisors(12),' ',DivisorsCount(12)); end.","[4] [4] [1,4]\n[1,12] 2\n");
    result("uses School; function Factorize(n:integer):integer; begin Result:=7; end; begin Writeln(Factorize(12),' ',School.Factorize(12),' ',(12).Factorize); end.","7 [2,2,3] [2,2,3]\n");
    for (const auto source:{"uses School; begin Factorize(-2147483649); end.","uses School; begin PrimeDivisorsCount(-9223372036854775808); end.","uses School; begin Divisors(-2147483649); end."}) runtime_error(source,"E_OVERFLOW");
    runtime_error("uses School; begin var a:=FirstPrimes(4794); a.Clear; Factorize(2); end.","E_BOUNDS");
    runtime_error("uses School; begin var a:=FirstPrimes(4794); a[0]:=0; Factorize(4); end.","E_DIV_ZERO");
    runtime_error("uses School; begin var a:=FirstPrimes(4794); a[0]:=1; PrimeFactorsCount(4); end.","E_LIMIT");
    runtime_error("uses School; begin var a:=Factorize(12); foreach var n in a do a[0]:=9; end.","E_MODIFIED");
    runtime_error("uses School; begin Factorize(4611686022722355201); end.","E_FACTOR");
    runtime_error("uses School; begin PrimeFactorsCount(4611686022722355201); end.","E_FACTOR");
    for (const auto source:{"begin Factorize(12); end.","begin PrimeFactors(12); end.","begin Divisors(12); end."}) compile_error(source,"E_NAME");
    for (const auto source:{"uses School; begin Factorize(1.5); end.","uses School; begin Divisors(1,2); end.",
        "uses School; begin var n:uint64:=12; Factorize(n); end.","uses School; begin Divizors(int64(12)); end.",
        "uses School; begin var a:List<integer>:=Factorize(int64(12)); end.","uses School; begin var a:List<int64>:=Factorize(12); end."}) compile_error(source,"E_TYPE");
    auto program=abc::compile("uses School; begin var a:=Factorize(int64(12)); end.");
    program.types[0].element=abc::Type::integer;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("uses School; begin FirstPrimes(1); end.");
    program.types.push_back(program.types[0]);
    program.code.push_back({abc::Op::school_list,static_cast<abc::Type>(65),static_cast<std::size_t>(abc::SchoolList::first_primes),{}});
    error([&]{abc::verify(program);},"E_BYTECODE");
    abc::ExecutionOptions options; options.allocation_limit=30;
    error([&]{(void)run("uses School; begin Factorize(4611686018427387904); end.","",options);},"E_LIMIT");
    error([&]{(void)run("uses School; begin DivisorsCount(4611686018427387904); end.","",options);},"E_LIMIT");
    require(run("uses School; begin Writeln(PrimeFactorsCount(4611686018427387904)); end.","",options)=="62\n","Wide factor count does not allocate a result list");
    options={}; std::size_t polls=0; options.cancelled=[&]{return ++polls>30;};
    error([&]{(void)run("uses School; begin Factorize(1000000016000000063); end.","",options);},"E_CANCELLED");
    options={}; options.instruction_limit=10;
    error([&]{(void)run("uses School; begin Factorize(4611686018427387904); end.","",options);},"E_LIMIT");
}

void school_library() {
    result("uses School; begin Writeln(Digits(0),' ',Digits(-7),' ',Digits(-9223372036854775808)); Writeln(Digits(255,16),' ',School.Digits(7,2),' ',(35).Digits(36)); Writeln(Digits(123,1),' ',Digits(123,MaxInt)); end.","[0] [] []\n[15,15] [1,1,1] [35]\n[] [123]\n");
    result("uses School; begin var d:List<integer>:=Digits(12345); d[0]:=9; Writeln(d,' ',d.DigitsToInt64); var e:=new List<integer>; Writeln(e.DigitsToInt64(-2147483648)); e.Add(-1); e.Add(2); Writeln(DigitsToInt64(e)); end.","[9,2,3,4,5] 92345\n0\n-8\n");
    result("uses School; begin var d:=new List<integer>; d.Add(-2); d.Add(-1); Writeln(d.DigitsToInt64(0),' ',d.DigitsToInt64(1)); d[0]:=-3; d[1]:=-4; Writeln(d.DigitsToInt64(-2)); Writeln(DigitsToInt64(Digits(123456789012345678))); end.","-1 -3\n2\n123456789012345678\n");
    result("uses School; var d:List<integer>; function Base:integer; begin d[0]:=3; Result:=10; end; begin d:=Digits(12); Writeln(DigitsToInt64(d,Base)); end.","32\n");
    result("uses School; function Digits(n:integer):integer; begin Result:=7; end; begin Writeln(Digits(12),' ',School.Digits(12),' ',(12).Digits); end.","7 [1,2] [1,2]\n");
    result("uses School; begin Writeln(Primes(-10),' ',Primes(0),' ',Primes(1),' ',Primes(2)); Writeln(Primes(20),' ',Primes(20,2)); Writeln(Primes(-100,1),' ',Primes(-100,3)); Writeln(Primes(2147483600,MaxInt)); end.","[] [] [] [2]\n[2,3,5,7,11,13,17,19] [2,3,5,7,11,13,17,19]\n[] [2,3]\n[2147483629,2147483647]\n");
    result("uses School; begin Writeln(FirstPrimes(-1),' ',FirstPrimes(0),' ',School.FirstPrimes(5)); var a:=FirstPrimes(4794); Writeln(a.Count,' ',a[4793]); a[0]:=17; Writeln(FirstPrimes(3)); var b:=FirstPrimes(4794); Writeln(a=b,' ',a.Count,' ',b.Count,' ',b[4794]); b.Clear; Writeln(FirstPrimes(0)); end.","[] [] [2,3,5,7,11]\n4794 46351\n[17,3,5]\nTrue 4795 4795 46351\n[]\n");
    result("uses School; begin var a:=FirstPrimes(3); a[0]:=99; Writeln(FirstPrimes(3)); var b:=FirstPrimes(10000); Writeln(b.Count,' ',b[b.Count-1]); end.","[2,3,5]\n10000 104729\n");
    result("uses School; begin var a:=FirstPrimes(4794); a[0]:=17; Writeln(IsPrime(4),' ',IsPrime(int64(9))); a.Clear; Writeln(IsPrime(4)); end.","True True\nTrue\n");
    runtime_error("uses School; begin var a:=FirstPrimes(4794); a[0]:=0; IsPrime(4); end.","E_DIV_ZERO");
    runtime_error("uses School; begin var a:=Digits(123); foreach var n in a do a[0]:=9; end.","E_MODIFIED");
    runtime_error("uses School; begin var d:List<integer>:=nil; DigitsToInt64(d,1); end.","E_NIL");
    runtime_error("uses School; begin var d:=Digits(255,16); DigitsToInt64(d); end.","E_RANGE");
    runtime_error("uses School; begin DigitsToInt64(Digits(MaxInt64)); end.","E_OVERFLOW");
    runtime_error("uses School; begin FirstPrimes(-2); end.","E_BOUNDS");
    runtime_error("uses School; begin Primes(MaxInt); end.","E_LIMIT");
    runtime_error("uses School; begin var a:=FirstPrimes(4794); a.Clear; Primes(10,20); end.","E_BOUNDS");
    runtime_error("uses School; begin var a:=FirstPrimes(4794); foreach var n in a do FirstPrimes(4794); end.","E_MODIFIED");
    for (const auto source:{"begin Digits(12); end.","begin Primes(20); end.","begin FirstPrimes(3); end."}) compile_error(source,"E_NAME");
    for (const auto source:{"uses School; begin Digits(1.5); end.","uses School; begin DigitsToInt64(Arr(1,2)); end.",
        "uses School; begin var d:=new List<int64>; DigitsToInt64(d); end.","uses School; begin Primes(20.0); end.",
        "uses School; begin FirstPrimes(1,2); end."}) compile_error(source,"E_TYPE");
    {
        auto program=abc::compile("uses School; begin var d:=Digits(12); end.");
        for (auto& instruction:program.code) if (instruction.op==abc::Op::school_list) instruction.operand=99;
        error([&]{abc::verify(program);},"E_BYTECODE");
        program=abc::compile("uses School; begin var d:=Digits(12); end.");
        for (auto& instruction:program.code) if (instruction.op==abc::Op::school_list) instruction.operand=static_cast<std::size_t>(abc::SchoolList::primes);
        error([&]{abc::verify(program);},"E_BYTECODE");
        program=abc::compile("uses School; begin var d:=Digits(12); end.");
        program.types[0].element=abc::Type::real;
        error([&]{abc::verify(program);},"E_BYTECODE");
        abc::ExecutionOptions options; options.allocation_limit=20;
        error([&]{(void)run("uses School; begin Primes(100); end.","",options);},"E_LIMIT");
        options.allocation_limit=1'048'576; std::size_t polls=0; options.cancelled=[&]{return ++polls>1000;};
        error([&]{(void)run("uses School; begin FirstPrimes(10000); end.","",options);},"E_CANCELLED");
        options.cancelled={}; options.instruction_limit=1'000'000;
        error([&]{(void)run("uses School; begin var a:=FirstPrimes(4794); a[0]:=1; FirstPrimes(4794); end.","",options);},"E_LIMIT");
    }
    const auto reference=[](std::uint64_t n) {
        if (n<2) return false;
        if (!(n&1U)) return n==2;
        for (std::uint64_t d=3;d<=n/d;d+=2) if (n%d==0) return false;
        return true;
    };
    for (const auto& [first,last]:{std::pair<std::int32_t,std::int32_t>{-5,50},{46300,46600},{92600,92800},{INT32_MAX-300,INT32_MAX}}) {
        std::string expected;
        for (std::int64_t n=std::max(first,2);n<=last;++n) if (reference(static_cast<std::uint64_t>(n))) expected+=std::to_string(n)+",";
        result("uses School; begin foreach var n in Primes("+std::to_string(first)+","+std::to_string(last)+") do Write(n,','); end.",expected);
    }
    {
        const auto program=abc::compile("uses School; begin var a:=FirstPrimes(4794); Writeln(a.Count); a.Clear; end.");
        for (int execution=0;execution<2;++execution) {
            std::istringstream input; std::ostringstream output; abc::execute(program,input,output);
            require(output.str()=="4794\n","School cache is owned by an execution");
        }
    }
    for (const auto& [first,last]:{std::pair<std::uint64_t,std::uint64_t>{0,5000},{2147483648ULL,2147484648ULL}}) {
        std::vector<bool> sieve;
        if (first==0) {
            sieve.resize(static_cast<std::size_t>(last)+1,true); sieve[0]=sieve[1]=false;
            for (std::uint64_t d=2;d<=last/d;++d) if (sieve[static_cast<std::size_t>(d)])
                for (auto multiple=d*d;multiple<=last;multiple+=d) sieve[static_cast<std::size_t>(multiple)]=false;
        }
        std::string expected;
        for (auto n=first;n<=last;++n) if (first==0 ? sieve[static_cast<std::size_t>(n)] : reference(n)) expected+=std::to_string(n)+',';
        result("uses School; begin for var n:int64:="+std::to_string(first)+" to "+std::to_string(last)+" do if IsPrime(n) then Write(n,','); end.",expected);
    }
    {
        std::uint64_t p=3037000499ULL; while (!reference(p)) --p;
        auto q=p-2; while (!reference(q)) --q;
        // The high prime is independently listed at t5k.org/curios/page.php?number_id=10747.
        result("uses School; begin Writeln(IsPrime(9223372036854775783),' ',IsPrime("+std::to_string(p*q)+")); end.","True False\n");
    }
    result("uses School; begin Writeln(IsPrime(-7),' ',IsPrime(0),' ',IsPrime(1),' ',IsPrime(2),' ',IsPrime(7),' ',IsPrime(9)); var b:byte:=7; Writeln(b.IsPrime,' ',(81).IsPrime); end.","False False False True True False\nTrue False\n");
    result("uses School; begin Writeln(IsPrime(2147483647),' ',School.IsPrime(4294967297),' ',IsPrime(2305843009213693951),' ',IsPrime(MaxInt64),' ',IsPrime(3825123056546413051)); Writeln(IsPrime(4611686014132420609)); end.","True False True False False\nFalse\n");
    result("uses School; begin SetRandomSeed(1); IsPrime(2305843009213693951); Writeln(Random(100)); end.","24\n");
    result("uses School; function IsPrime(n:integer):boolean; begin Result:=false; end; begin Writeln(IsPrime(7),' ',School.IsPrime(7),' ',(7).IsPrime); end.","False True True\n");
    runtime_error("uses School; begin var n:int64:=-2147483649; IsPrime(n); end.","E_OVERFLOW");
    compile_error("begin Writeln(IsPrime(7)); end.","E_NAME");
    compile_error("uses School; begin IsPrime(7.0); end.","E_TYPE");
    compile_error("uses School; begin var n:uint64:=7; IsPrime(n); end.","E_TYPE");
    {
        abc::ExecutionOptions options; std::size_t polls=0; options.cancelled=[&]{return ++polls>15;};
        error([&]{(void)run("uses School; begin IsPrime(2305843009213693951); end.","",options);},"E_CANCELLED");
    }
    result("uses School; begin Writeln(Bin(0),' ',Bin(-10),' ',Oct(511),' ',Hex(255)); Writeln(School.Dec('ff',16),' ',School.Dec('',2),' ',Dec('1010',2)); Writeln(ToBase(-8,2),' ',ToBase(0,36),' ',ToBase(35,36),' ',ToBase(MaxInt64,16)); end.","0 1010 777 FF\n255 0 10\n0 0 Z 7FFFFFFFFFFFFFFF\n");
    result("uses School; begin Writeln((255).ToBase(16),' ',(81).PerfectSquare); School.Dec('ff',16); Dec('ff',16); Dec('F',16); var n:=7; Dec(n); Writeln(n); end.","FF 9\n6\n");
    result("uses School; begin Writeln(BinFormat(0),'|',BinFormat(1,2),'|',BinFormat(-5,-1)); Writeln(BinFormat(256,0,':')); Writeln(BinFormat(1,2,nil)); end.","00000000|00000000 00000001|00000101\n00000001:00000000\n0000000000000001\n");
    result("uses School; begin Writeln(BinFormat(-9223372036854775808,0,'')); Writeln(BinFormat(MaxInt64,0,'')); PrintDelimDefault:='|'; Writeln(BinFormat(256)); end.","1000000000000000000000000000000000000000000000000000000000000000\n0111111111111111111111111111111111111111111111111111111111111111\n00000001 00000000\n");
    result("uses School; begin Writeln(PerfectSquare(-4),' ',PerfectSquare(0),' ',PerfectSquare(1),' ',PerfectSquare(81),' ',PerfectSquare(82),' ',PerfectSquare(MaxInt64)); Writeln(PerfectSquare(9223372030926249001)); end.","-1 0 1 9 -1 -1\n3037000499\n");
    result("uses School; begin Writeln(Abs(SinDegrees(30)-0.5)<1e-12,' ',Abs(CosDegrees(60)-0.5)<1e-12,' ',Abs(TanDegrees(45)-1)<1e-12); Writeln(School.SinDegrees(0),' ',School.CosDegrees(0)); end.","True True True\n0 1\n");
    result("uses School; function Bin(n:integer):string; begin Result:='own'; end; begin Writeln(Bin(7),' ',School.Bin(7)); end.","own 111\n");
    runtime_error("uses School; begin School.Dec(nil,1); end.","E_RANGE");
    runtime_error("uses School; begin School.Dec(nil,16); end.","E_NIL");
    runtime_error("uses School; begin School.Dec('2',2); end.","E_FORMAT");
    runtime_error("uses School; begin School.Dec(' 1',10); end.","E_FORMAT");
    runtime_error("uses School; begin School.Dec('9223372036854775808Z',10); end.","E_FORMAT");
    runtime_error("uses School; begin School.Dec('9223372036854775808',10); end.","E_OVERFLOW");
    runtime_error("uses School; begin ToBase(-7,1); end.","E_RANGE");
    runtime_error("uses School; begin Bin(-9223372036854775808); end.","E_OVERFLOW");
    runtime_error("uses School; begin BinFormat(1,2147483647); end.","E_LIMIT");
    runtime_error("uses School; begin BinFormat(1,2,StringOfChar('x',1048576)); end.","E_LIMIT");
    runtime_error("uses School; begin SinDegrees(1e308); end.","E_OVERFLOW");
    compile_error("begin Writeln(Bin(7)); end.","E_NAME");
    compile_error("begin Writeln((81).PerfectSquare); end.","E_TYPE");
    compile_error("uses School; begin Bin(1.5); end.","E_TYPE");
    result("uses School; begin var u:uint64:=1; Writeln(ToBase(u,2)); end.","1\n");
    compile_error("uses School; begin BinFormat(1,' '); end.","E_TYPE");
    abc::ExecutionOptions options; std::size_t polls=0; options.cancelled=[&]{return ++polls>30;};
    error([&]{(void)run("uses School; begin School.Dec(StringOfChar('0',10000)+'1',2); end.","",options);},"E_CANCELLED");
    polls=0;
    error([&]{(void)run("uses School; begin BinFormat(1,10000,''); end.","",options);},"E_CANCELLED");
}

void collection_sequence_views() {
    result("begin var a:=Arr(1,2); var s:sequence of integer:=a; Writeln(s=a,' ',a=s); var n:array of integer; s:=n; Writeln(s=n); foreach var row:sequence of integer in Arr(Arr(7),Arr(8)) do Writeln(row.ToArray); end.","True True\nTrue\n[7]\n[8]\n");
    result("begin var a:=Arr(1,2); var s:sequence of integer:=a; a[0]:=9; a:=Arr(3); foreach var x in s do Write(x,' '); Writeln('|',Count(s),' ',PABCSystem.ToArray(s)); end.","9 2 |2 [9,2]\n");
    result("begin var a:=Arr(1,2); var s:sequence of integer:=a; foreach var x in s do begin Writeln(x); a[1]:=9; end; var b:=s.ToArray; a[0]:=7; Writeln(b); end.","1\n9\n[1,9]\n");
    result("begin var a:=new List<integer>; a.Add(1); var s:sequence of integer:=a; a.Add(2); Writeln(s.Count,' ',ToArray(a)); foreach var x in s do Write(x); Writeln; end.","2 [1,2]\n12\n");
    result("begin var a:=Arr(1,2); var s:sequence of integer:=a; var t:sequence of integer:=a; Writeln(s=t); t:=Arr(1,2); Writeln(s=t); var n:array of integer; s:=n; var l:List<integer>; t:=l; Writeln(s=nil,' ',t=nil); end.","True\nFalse\nTrue True\n");
    result("type R=record x:integer; end; function Change(r:R):integer; begin r.x:=99; Result:=r.x; end; begin var a:=new R[1]; a[0].x:=7; var s:sequence of R:=a; var b:=s.ToArray; b[0].x:=9; foreach var r in s do Writeln(Change(r)); Writeln(a[0].x,' ',b[0].x); end.","99\n7 9\n");
    result("uses School; begin var s:sequence of BigInteger:=Arr(-2bi,18446744073709551616bi,7bi); Writeln(s.MinMax,' ',MinMax(s)); var l:=new List<integer>; l.Add(7); l.Add(-3); var q:sequence of integer:=l; Writeln(q.MinMax); var e:sequence of BigInteger:=new BigInteger[0]; Writeln(e.MinMax); end.","(-2,18446744073709551616) (-2,18446744073709551616)\n(-3,7)\n(0,0)\n");
    result("function Count(a:array of integer):integer; begin Result:=7; end; begin Writeln(Count(Arr(1,2)),' ',PABCSystem.Count(Arr(1,2))); end.","7 2\n");
    for (const auto source:{"begin var l:=new List<integer>; l.Add(1); l.Add(2); var s:sequence of integer:=l; foreach var x in s do l[0]:=9; end.","begin var l:=new List<integer>; l.Add(1); var s:sequence of integer:=l; foreach var x in s do l.Clear; end."}) runtime_error(source,"E_MODIFIED");
    runtime_error("begin var a:array of integer; var s:sequence of integer:=a; foreach var x in s do Writeln(x); end.","E_NIL");
    runtime_error("uses School; begin var a:array of integer; var s:sequence of integer:=a; MinMax(s); end.","E_NIL");
    for (const auto source:{"begin var s:sequence of int64:=Arr(1,2); end.","begin var s:sequence of integer:=new integer[1,2]; end.","begin Count(new integer[1,2]); end.","begin var a:=new List<real>; var s:sequence of integer:=a; end."}) compile_error(source,"E_TYPE");
    auto invalid=abc::compile("begin var s:sequence of integer:=Arr(1,2); end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::sequence_wrap) instruction.operand=0;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    abc::ExecutionOptions options; options.instruction_limit=100; options.allocation_limit=2100;
    require(run("type R=record x:integer; end; begin var s:sequence of R:=new R[1000]; Writeln(s.Count); end.","",options)=="1000\n","Array sequence Count uses the source collection count without copying record elements");
}

void lazy_ip_sequences() {
    result("uses School; begin var c:=new CalcIP('128.0.0.1',2); var s:sequence of Addr32:=c.GenAddr; foreach var a in s do begin Writeln(a.Value); break; end; end.","128.0.0.1\n");
    result("uses School; begin var c:=new CalcIP('10.0.0.1',30); c.Network.Value:='0.0.0.0'; c.Broadcast.Value:='255.255.255.255'; var n:=0; foreach var s in c.GenAddrBin do begin Writeln(s); Inc(n); if n=2 then break; end; end.","00000000000000000000000000000000\n00000000000000000000000000000001\n");
    result("uses School; function Make:sequence of Addr32; begin var c:=new CalcIP('10.0.0.1',30); Result:=c.GenAddr; end; procedure First(s:sequence of Addr32); begin foreach var x in s do begin Writeln(x.Value); break; end; end; begin var s:=Make; First(s); Writeln(s.Count(),' ',s.ToArray[1].Value); end.","10.0.0.1\n2 10.0.0.2\n");
    result("uses School; begin var c:=new CalcIP('10.0.0.1',30); var s:=c.GenAddr; c.Network.Value:='10.0.0.10'; foreach var a in s do begin Writeln(a.Value); c.Network.Value:='10.0.0.20'; end; foreach var a in s do begin Writeln(a.Value); break; end; end.","10.0.0.11\n10.0.0.22\n10.0.0.21\n");
    result("uses School; begin var c:=new CalcIP('10.0.0.1',30); var s:=c.GenAddr; foreach var a in s do foreach var b in s do Writeln(a.Value,' ',b.Value); Writeln(s.ToArray[0]=s.ToArray[0]); end.","10.0.0.1 10.0.0.1\n10.0.0.1 10.0.0.2\n10.0.0.2 10.0.0.1\n10.0.0.2 10.0.0.2\nFalse\n");
    result("uses School; begin var c:=new CalcIP('10.0.0.1',30); var s:=c.GenAddrBin; var n:=0; foreach var bits in s do begin Writeln(bits); c.Network.Value:='10.0.0.10'; c.Broadcast.Value:='10.0.0.11'; Inc(n); end; Writeln(n); foreach var bits in s do begin Writeln(bits); break; end; end.","00001010000000000000000000000000\n00001010000000000000000000000001\n00001010000000000000000000000010\n00001010000000000000000000000011\n4\n00001010000000000000000000001010\n");
    result("begin var s:sequence of integer; Writeln(s=nil); end.","True\n");
    for (const auto source:{"begin var s:sequence of integer; foreach var x in s do Writeln(x); end.","begin var s:sequence of integer; s.Count; end.","begin var s:sequence of integer; s.ToArray; end."}) runtime_error(source,"E_NIL");
    runtime_error("uses School; begin var c:=new CalcIP('10.0.0.1',30); var s:=c.GenAddr; c.Network.Value:='255.255.255.255'; s.Count; end.","E_OVERFLOW");
    for (const auto source:{"uses School; begin var c:=new CalcIP('10.0.0.1',30); var s:sequence of string:=c.GenAddr; end.","uses School; begin var c:=new CalcIP('10.0.0.1',30); var a:array of Addr32:=c.GenAddr; end.","begin var s:sequence of integer; s[0]:=1; end.","begin var s:sequence of integer; Writeln(s[0]); end.","begin var s:=new sequence of integer; end."}) compile_error(source,"E_TYPE");
    auto invalid=abc::compile("uses School; begin var c:=new CalcIP('10.0.0.1',30); foreach var a in c.GenAddr do Writeln(a.Value); end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::get_index) instruction.op=abc::Op::ref_index;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    invalid=abc::compile("uses School; begin var c:=new CalcIP('10.0.0.1',30); c.GenAddr.ToArray; end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::sequence_array) instruction.operand=64;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
}

void school_ip_classes() {
    std::string expected;
    std::uint32_t state=123456789;
    std::string samples="uses School; begin var a:=new Addr32('0.0.0.0'); var numbers:=Arr(";
    for (unsigned i=0;i<64;++i) {
        state=1664525U*state+1013904223U;
        if (i) samples+=",";
        samples+="longword("+std::to_string(state)+")";
        for (int byte=3;byte>=0;--byte) { if (byte!=3) expected+="."; expected+=std::to_string((state>>(byte*8))&255U); }
        expected+=" "+std::to_string(state)+"\n";
    }
    samples+="); foreach var n in numbers do begin var b:=a+n; Writeln(b.Value,' ',b.Value10); end; end.";
    result(samples,expected);
    result("uses School; begin var a:=new Addr32('192.168.1.1'); Writeln(a.Value,' ',a.Value10,' ',a.Value2); var m:=new Addr32(24); Writeln((a and m).Value,' ',(a or not m).Value,' ',(a+1).Value); Writeln(a.Value); end.","192.168.1.1 3232235777 11000000101010000000000100000001\n192.168.1.0 192.168.1.255 192.168.1.2\n192.168.1.1\n");
    result("uses School; begin var a:=new Addr32('00000001000000100000001100000100'); Writeln(a.Value,' ',a.Value10); var b:=a; b.Value:='001.002.003.004'; Writeln(a.Value,' ',b.Value2,' ',a=b); Writeln(a=new Addr32('1.2.3.4')); Writeln(a.AddrFormat(a.Value2)); end.","1.2.3.4 16909060\n001.002.003.004 00000001000000100000001100000100 True\nFalse\n00000001.00000010.00000011.00000100\n");
    result("uses School; begin var a:Addr32; Writeln(a=nil); a:=new Addr32; Writeln(a.Value10,'|',a.Value,'|',a.Value2); a.Value:='0.0.0.0'; Writeln(a); end.","True\n0||\n0.0.0.0,       00000000.00000000.00000000.00000000\n");
    result("uses School; procedure Change(a:Addr32); begin a.Value:='10.0.0.1'; end; begin var a:=new Addr32(8); Change(a); var list:=new List<Addr32>; list.Add(a); foreach var x in list do x.Value:='10.0.0.2'; Writeln(a.Value,' ',list.Contains(a)); var t:=(a,7); t.Item1.Value:='10.0.0.3'; Writeln(list[0].Value); end.","10.0.0.2 True\n10.0.0.3\n");
    result("uses School; begin var c:=new CalcIP('192.168.1.129',24); var b:byte:=c.Bitmask; Writeln(c.Address.Value,' /',b,' ',c.Hosts); Writeln(c.Netmask.Value,' ',c.Wildcard.Value); Writeln(c.Network.Value,' ',c.Broadcast.Value); Writeln(c.Hostmin.Value,' ',c.Hostmax.Value); end.","192.168.1.129 /24 254\n255.255.255.0 0.0.0.255\n192.168.1.0 192.168.1.255\n192.168.1.1 192.168.1.254\n");
    result("uses School; begin var c:=new CalcIP('10.0.0.1','255.255.255.252'); foreach var a in c.GenAddr do Write(a.Value,'|'); Writeln; foreach var s in c.GenAddrBin() do Writeln(s); c.Address.Value:='10.1.2.3'; Writeln(c.Address.Value,' ',c.Network.Value); end.","10.0.0.1|10.0.0.2|\n00001010000000000000000000000000\n00001010000000000000000000000001\n00001010000000000000000000000010\n00001010000000000000000000000011\n10.1.2.3 10.0.0.0\n");
    result("uses School; type Address=Addr32; R=record a:Addr32; end; begin var r:R; r.a:=new Address('255.255.255.255'); Writeln(r.a.Value10); var c:=CalcIP('10.0.0.2',31); Writeln(c.Hosts,' ',c.Hostmin.Value,' ',c.Hostmax.Value,' ',c.GenAddr.Length,' ',c.GenAddrBin.Length); end.","4294967295\n0 10.0.0.3 10.0.0.2 0 0\n");
    result("uses School; begin var c:=new CalcIP; Writeln(c.Address=nil,' ',c.Bitmask,' ',c.Hosts); var a:=Addr32(31); Writeln(a.Value); end.","True 0 0\n255.255.255.254\n");
    for (const auto source:{"uses School; begin var a:=new Addr32('256.1.2.3'); end.","uses School; begin var a:=new Addr32('1.2.3'); end.","uses School; begin var a:=new Addr32(' 1.2.3.4'); end.","uses School; begin var a:=new Addr32('1010'); end.","uses School; begin var c:=new CalcIP('10.0.0.1','255.0.255.0'); end.","uses School; begin var c:=new CalcIP('10.0.0.1','255.255.255.255'); end."}) runtime_error(source,"E_FORMAT");
    for (const auto source:{"uses School; begin var a:=new Addr32(0); end.","uses School; begin var a:=new Addr32(32); end.","uses School; begin var c:=new CalcIP('10.0.0.1',0); end.","uses School; begin var c:=new CalcIP('10.0.0.1',32); end."}) runtime_error(source,"E_RANGE");
    for (const auto source:{"uses School; begin var a:Addr32; Writeln(a.Value); end.","uses School; begin var a:=new Addr32(nil); end.","uses School; begin var a:=new Addr32(24); a.Value:=nil; end.","uses School; begin var a:Addr32; var b:=not a; end."}) runtime_error(source,"E_NIL");
    runtime_error("uses School; begin var a:=new Addr32('255.255.255.255'); var b:=a+1; end.","E_OVERFLOW");
    runtime_error("uses School; begin var c:=new CalcIP('10.0.0.1',1); end.","E_OVERFLOW");
    runtime_error("uses School; begin var a:=new Addr32; Writeln(a.ToString); end.","E_BOUNDS");
    runtime_error("uses School; begin var a:=new Addr32(24); a.AddrFormat('short'); end.","E_BOUNDS");
    runtime_error("uses School; begin var c:=new CalcIP('10.0.0.1',2); c.GenAddr.ToArray; end.","E_LIMIT");
    for (const auto source:{"uses School; begin var a:=new Addr32(24); a.Value2:='x'; end.","uses School; begin var c:=new CalcIP; c.Hosts:=7; end.","uses School; begin var a:=new Addr32(24); a.Value(); end.","uses School; begin var a:=new Addr32(True); end.","uses School; begin var a:=new Addr32(24); Read(a.Value); end.","uses School; begin var a:=new Addr32(24); var b:=1+a; end."}) compile_error(source,"E_TYPE");
    compile_error("begin var a:Addr32; end.","E_UNSUPPORTED");
    compile_error("uses School; begin var a:=new Addr32; a.addr10:=7; end.","E_NAME");
    auto invalid=abc::compile("uses School; begin var a:=new Addr32(24); end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::school_ip) instruction.operand=255;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    invalid=abc::compile("uses School; begin var a:=new Addr32(24); end."); invalid.types[0].fields[2].type=abc::Type::integer;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    invalid=abc::compile("uses School; begin var a:=new Addr32(24); end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::school_ip) instruction.operand|=256;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    invalid=abc::compile("uses School; begin var a:Addr32; end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::make_default) instruction.operand=1;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    invalid=abc::compile("uses School; begin var c:=new CalcIP('10.0.0.1',30); c.GenAddr; end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::school_ip && (instruction.operand>>8)) instruction.operand=(instruction.operand&255U)|(static_cast<std::size_t>(abc::Type::string)<<8);
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    const auto report=run("uses School; begin var c:=new CalcIP('10.0.0.1',30); Write(c.ToString); end.");
    require(report.starts_with("IP адрес (Address)                  | 10.0.0.1        | 00001010.00000000.00000000.000000  01"),"CalcIP.ToString retains labels, alignment and network/host bit separator");
    require(report.find("Префикс маски подсети (Bitmask)     | /30")!=std::string::npos && report.find("Доступно адресов для хостов (Hosts) | 2")!=std::string::npos && std::count(report.begin(),report.end(),'\n')==9,"CalcIP.ToString emits all nine source report rows");
    const auto program=abc::compile("uses School; function Text:string; begin Write('T'); Result:='1.2.3.4'; end; begin var a:Addr32; a.Value:=Text; end.");
    std::istringstream input; std::ostringstream output;
    error([&]{abc::execute(program,input,output);},"E_NIL"); require(output.str()=="T","Addr32 setter evaluates its value before the nil receiver call fails");
    abc::ExecutionOptions options; options.instruction_limit=50;
    error([&]{(void)run("uses School; begin var c:=new CalcIP('10.0.0.1',18); c.GenAddr.ToArray; end.","",options);},"E_LIMIT");
    options={}; std::size_t polls=0; options.cancelled=[&]{return ++polls>30;};
    error([&]{(void)run("uses School; begin var c:=new CalcIP('10.0.0.1',18); c.GenAddrBin.ToArray; end.","",options);},"E_CANCELLED");
}

void string_value_methods() {
    result("begin var s:='ab'; Writeln('[',s.PadLeft(5),'] [',s.PadRight(5,'0'),'] [',s.PadLeft(0),']'); Writeln(s,' ',s.PadLeft(4,'X').PadRight(6,'Y')); Writeln(''.PadLeft(3,'z')); end.","[   ab] [ab000] [ab]\nab XXabYY\nzzz\n");
    result("begin var s:='abcde'; Writeln(s.Substring(2),' ',s.Substring(1,3),'|',s.Substring(5),'|',s.Substring(5,0)); Writeln(s.Remove(2),' ',s.Remove(1,3),'|',s.Remove(5,0)); Writeln(s.Insert(0,'X'),' ',s.Insert(5,'Y'),' ',s.Insert(2,'')); Writeln(s); end.","cde bcd||\nab ae|abcde\nXabcde abcdeY abcde\nabcde\n");
    result("begin var s:='Я🌍'; Writeln(s.Length,' ',s.PadLeft(4,'0'),' ',s.Substring(1,2)); Writeln(Ord(s.Substring(1,1)[1]),' ',Ord(s.Substring(2)[1])); Writeln(Ord(''.PadRight(2,#0)[2])); end.","3 0Я🌍 🌍\n55356 57101\n0\n");
    result("begin var a:=new List<string>; a.Add('ab'); foreach var s in a do Writeln(a[0].Remove(0,0)); Writeln(a); var t:=('ab',7); Writeln(t.Item1.Insert(1,'X')); end.","ab\n[ab]\naXb\n");
    result("var s:string; function Receiver:string; begin Write('R'); Result:=s; end; function Position:integer; begin Write('P'); s:='changed'; Result:=1; end; function Text:string; begin Write('T'); Result:='X'; end; begin s:='ab'; Writeln(Receiver.Insert(Position,Text),' ',s); end.","RPTaXb changed\n");
    for (const auto source:{"begin var s:string:=nil; s.PadLeft(-1); end.","begin var s:string:=nil; s.Substring(-1); end.","begin var s:='ab'; s.Insert(-1,nil); end."}) runtime_error(source,"E_NIL");
    runtime_error("begin var s:='ab'; s.PadRight(-1); end.","E_RANGE");
    runtime_error("begin var s:='ab'; s.PadLeft(2147483647); end.","E_LIMIT");
    runtime_error("begin var s:='ab'; s.Insert(0,StringOfChar('x',1048576)); end.","E_LIMIT");
    for (const auto source:{"begin var s:='ab'; s.Substring(-1); end.","begin var s:='ab'; s.Substring(3,0); end.","begin var s:='ab'; s.Substring(1,2147483647); end.","begin var s:='ab'; s.Remove(0,-1); end.","begin var s:='ab'; s.Remove(3,0); end.","begin var s:='ab'; s.Insert(3,''); end."}) runtime_error(source,"E_BOUNDS");
    for (const auto source:{"begin var s:='ab'; s.PadLeft(3,'xx'); end.","begin var s:='ab'; s.Substring(1.5); end.","begin var s:='ab'; s.Insert(0,7); end.","begin var s:='ab'; s.Remove(); end.","begin PadLeft('ab',3); end."}) compile_error(source,source==std::string_view{"begin PadLeft('ab',3); end."} ? "E_NAME" : "E_TYPE");
}

void school_truth_tables() {
    result("uses School; function F(a,b:boolean):boolean; begin Result:=a xor b; end; begin var t:=TrueTable(F); Writeln(t.RowCount,' ',t.ColCount); TrueTablePrint(t); end.","4 3\n a b F\n -----\n 0 0 0\n 0 1 1\n 1 0 1\n 1 1 0\n");
    for (unsigned arity=2;arity<=5;++arity) {
        std::string parameters,expression;
        for (unsigned i=0;i<arity;++i) { if (i) { parameters+=","; expression+=" xor "; } const auto name=std::string(1,static_cast<char>('a'+i)); parameters+=name; expression+=name; }
        const auto source="uses School; function F("+parameters+":boolean):boolean; begin Result:="+expression+"; end; begin var t:=School.TrueTable(F); foreach var x in t do Write(Ord(x)); end.";
        std::string expected;
        for (unsigned row=0;row<(1U<<arity);++row) {
            unsigned parity=0;
            for (unsigned column=0;column<arity;++column) { const auto bit=(row>>(arity-column-1))&1U; expected+=char('0'+bit); parity^=bit; }
            expected+=char('0'+parity);
        }
        result(source,expected);
    }
    result("uses School; function F(a:array of boolean):boolean; begin Result:=True; foreach var v in a do Result:=Result and v; end; begin TrueTablePrint(TrueTable(3,F),1,'xyz'); TrueTablePrint(TrueTable(0,F)); end."," x y z F\n -------\n 1 1 1 1\n F\n -\n 1\n");
    result("uses School; var calls:integer; var first:array of boolean; function F(a:array of boolean):boolean; begin Inc(calls); if calls=1 then first:=a; a[0]:=True; Result:=a[1]; end; function Count:integer; begin Write('N'); Result:=2; end; begin var t:=TrueTable(Count,F); Writeln(calls,' ',first,' ',t[0,0],' ',t[1,0],' ',t[1,2]); end.","N4 [True,False] False False True\n");
    result("uses School; function F(a,b:boolean):boolean; begin Write(Ord(a),Ord(b),'|'); Result:=a; end; function Build:array[,] of boolean; begin Result:=TrueTable(F); end; begin var t:=Build; Writeln(t[3,2]); end.","00|01|10|11|True\n");
    result("uses School; var level:integer; function F(a,b:boolean):boolean; begin if level=0 then begin Inc(level); var t:=TrueTable(F); Dec(level); Result:=t[3,2]; end else Result:=a and b; end; begin Writeln(TrueTable(F)[0,2]); end.","True\n");
    for (const auto source:{"uses School; function F(a:integer;b:boolean):boolean; begin Result:=b; end; begin TrueTable(F); end.","uses School; function F(var a:boolean;b:boolean):boolean; begin Result:=a; end; begin TrueTable(F); end.","uses School; function F(a,b:boolean):integer; begin Result:=1; end; begin TrueTable(F); end.","uses School; function F(a:boolean):boolean; begin Result:=a; end; begin TrueTable(F); end.","uses School; function F(a,b:boolean):boolean; begin Result:=a; end; begin TrueTable(2,F); end.","uses School; function F(var a:array of boolean):boolean; begin Result:=True; end; begin TrueTable(2,F); end.","uses School; begin TrueTable(2,True); end."}) compile_error(source,"E_TYPE");
    compile_error("function F(a,b:boolean):boolean; begin Result:=a; end; begin TrueTable(F); end.","E_NAME");
    const std::string prefix="uses School; function F(a:array of boolean):boolean; begin Result:=True; end; begin TrueTable(";
    runtime_error(prefix+"-1,F); end.","E_RANGE"); runtime_error(prefix+"31,F); end.","E_RANGE"); runtime_error(prefix+"30,F); end.","E_LIMIT");
    auto invalid=abc::compile(prefix+"1,F); end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::school_table_new) instruction.operand=1;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    abc::ExecutionOptions options; options.instruction_limit=100;
    error([&]{(void)run(prefix+"10,F); end.","",options);},"E_LIMIT");
}

void school_truth_table_print() {
    const std::string table="var a:=new boolean[4,3]; a[1,1]:=True; a[1,2]:=True; a[2,0]:=True; a[2,2]:=True; a[3,0]:=True; a[3,1]:=True;";
    result("uses School; begin "+table+" TrueTablePrint(a); end."," a b F\n -----\n 0 0 0\n 0 1 1\n 1 0 1\n 1 1 0\n");
    result("uses School; begin "+table+" School.TrueTablePrint(a,0,'XYZ'); end."," X Y F\n -----\n 0 0 0\n 1 1 0\n");
    result("uses School; begin "+table+" TrueTablePrint(a,1,'x'); end."," x   F\n -----\n 0 1 1\n 1 0 1\n");
    result("uses School; begin TrueTablePrint(new boolean[1,1],-7); TrueTablePrint(new boolean[0,3]); end."," F\n -\n 0\n a b F\n -----\n");
    result("uses School; begin PrintDelimDefault:='|'; TrueTablePrint(new boolean[0,3],2,'x'); PrintDelimDefault:=nil; TrueTablePrint(new boolean[0,3]); end."," x| |F\n -----\n anilbnilF\n -----\n");
    result("uses School; begin PrintDelimDefault:=''; TrueTablePrint(new boolean[0,3],2,'🌍x'); end."," 🌍F\n -----\n");
    result("uses School; var m:array[,] of boolean; function Filter:integer; begin Write('F'); SetLength(m,0,0); Result:=2; end; function Names:string; begin Write('N'); Result:='xy'; end; begin m:=new boolean[1,2]; TrueTablePrint(m,Filter,Names); Writeln(m.Length); end.","FN x F\n ---\n 0 0\n0\n");
    result("uses School; procedure TrueTablePrint(a:array[,] of boolean); begin Write('own'); end; begin TrueTablePrint(new boolean[0,1]); School.TrueTablePrint(new boolean[0,1]); end.","own F\n -\n");
    for (const auto source:{"uses School; begin TrueTablePrint(nil); end.","uses School; begin TrueTablePrint(new boolean[0,2],2,nil); end."}) runtime_error(source,"E_NIL");
    runtime_error("uses School; begin TrueTablePrint(new boolean[0,0]); end.","E_RANGE");
    runtime_error("uses School; begin TrueTablePrint(new boolean[0,1048578]); end.","E_LIMIT");
    compile_error("begin TrueTablePrint(new boolean[0,2]); end.","E_NAME");
    for (const auto source:{"uses School; begin TrueTablePrint(); end.","uses School; begin TrueTablePrint(Arr(False)); end.","uses School; begin TrueTablePrint(new integer[2,3]); end.","uses School; begin TrueTablePrint(new boolean[0,2],True); end.","uses School; begin TrueTablePrint(new boolean[0,2],2,7); end.","uses School; begin var x:=TrueTablePrint(new boolean[0,2]); end."}) compile_error(source,"E_TYPE");
    auto invalid=abc::compile("uses School; begin TrueTablePrint(new boolean[0,2]); end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::school_table_print) instruction.operand=1;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    invalid=abc::compile("uses School; begin TrueTablePrint(new boolean[0,2]); end.");
    invalid.types[0].element=abc::Type::integer;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
    abc::ExecutionOptions options; options.instruction_limit=50;
    error([&]{(void)run("uses School; begin TrueTablePrint(new boolean[100,2],1); end.","",options);},"E_LIMIT");
    options={}; options.output_limit=10;
    error([&]{(void)run("uses School; begin TrueTablePrint(new boolean[0,3]); end.","",options);},"E_LIMIT");
    options={}; std::size_t polls=0; options.cancelled=[&]{return ++polls>30;};
    error([&]{(void)run("uses School; begin TrueTablePrint(new boolean[10000,2],1); end.","",options);},"E_CANCELLED");
}

void school_big_integer_library() {
    result("uses School; begin var u:uint64:=1; Writeln(u.Digits,' ',GCD(u)); end.","[1] 1\n");
    result("uses School; begin Writeln(Bin(-1024bi),' ',Oct(18446744073709551616bi),' ',Hex(-18446744073709551616bi)); Writeln(ToBase(18446744073709551616bi,16),' ',(-42bi).ToBase(2)); Writeln(ToBase('18446744073709551616',16),'|',ToBase('bad',16),'|',ToBase(nil,2)); end.","10000000000 2000000000000000000000 10000000000000000\n10000000000000000 0\n10000000000000000||\n");
    result("uses School; begin Writeln(DecBig('FFFFFFFFFFFFFFFFFFFFFFFF',16)); Writeln(DecBig('',2),' ',DecBig('ſ',36),' ',DecBig('ı',36)); Writeln(BinFormat(256bi,3,':')); Writeln(BinFormat(-5bi,-1),'|',BinFormat(0bi,2,nil)); end.","79228162514264337593543950335\n0 28 18\n00000000:00000001:00000000\n00000101|0000000000000000\n");
    result("uses School; begin var n:=18446744073709551616bi; Writeln(Digits(n),' ',n.Digits(16)); Writeln(n.Digits.DigitsToBigInteger=n); Writeln(Digits(-1bi),' ',Digits(0bi),' ',Digits(12bi,1)); Writeln(Digits(n,2147483647)); end.","[1,8,4,4,6,7,4,4,0,7,3,7,0,9,5,5,1,6,1,6] [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]\nTrue\n[] [0] []\n[4,8,4]\n");
    result("uses School; begin var a:=new List<integer>; Writeln(DigitsToBigInteger(a,0)); a.Add(-1); a.Add(-2); Writeln(a.DigitsToBigInteger(10)); a.Clear; a.Add(-3); Writeln(a.DigitsToBigInteger(-2)); end.","0\n-12\n-3\n");
    result("uses School; begin Writeln(School.GCD(Arr(-18bi,30bi,42bi))); Writeln(School.LCM(-4bi,6bi),' ',School.LCM(4bi,-6bi),' ',School.LCM(-4bi,-6bi)); Writeln(School.GCD(Arr(-42bi)),' ',School.LCM(Arr(-42bi))); Writeln(School.LCM(18446744073709551616bi,3bi)); end.","6\n-12 12 -12\n-42 -42\n55340232221128654848\n");
    result("uses School; begin Writeln(MinMax(Arr(2bi,-7bi,18446744073709551616bi))); var a:=new List<BigInteger>; Writeln(a.MinMax); a.Add(-3bi); a.Add(-1bi); Writeln(a.MinMax); end.","(-7,18446744073709551616)\n(0,0)\n(-3,-1)\n");
    for (const auto source:{"uses School; begin DecBig('2',2); end.","uses School; begin DecBig('-1',10); end.","uses School; begin DecBig(' 1',10); end."}) runtime_error(source,"E_FORMAT");
    runtime_error("uses School; begin var a:=new List<integer>; a.Add(2); DigitsToBigInteger(a,2); end.","E_RANGE");
    runtime_error("uses School; begin ToBase(nil,1); end.","E_RANGE");
    runtime_error("uses School; begin DecBig(nil,1); end.","E_RANGE");
    runtime_error("uses School; begin DecBig(nil,2); end.","E_NIL");
    runtime_error("uses School; begin var a:List<integer>; DigitsToBigInteger(a); end.","E_NIL");
    runtime_error("uses School; begin School.LCM(0bi,0bi); end.","E_DIV_ZERO");
    runtime_error("uses School; begin School.GCD(new BigInteger[0]); end.","E_RANGE");
    runtime_error("uses School; begin BinFormat(1bi,2147483647); end.","E_LIMIT");
    runtime_error("uses School; begin DecBig(StringOfChar('0',65536),2); end.","E_LIMIT");
    compile_error("uses School; begin DigitsToBigInteger(Arr(1,2)); end.","E_TYPE");
    compile_error("uses School; begin var a:integer:=DecBig('12',10); end.","E_TYPE");
    abc::ExecutionOptions options; std::size_t polls=0; options.cancelled=[&]{return ++polls>30;};
    error([&]{(void)run("uses School; begin BinFormat(1bi,10000,''); end.","",options);},"E_CANCELLED");
    options={}; options.allocation_limit=5;
    error([&]{(void)run("uses School; begin DecBig('FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF',16); end.","",options);},"E_LIMIT");
    options={}; options.instruction_limit=50;
    error([&]{(void)run("uses School; begin DecBig(StringOfChar('0',1000),2); end.","",options);},"E_LIMIT");
    auto invalid=abc::compile("uses School; begin DecBig('10',2).Digits.DigitsToBigInteger; end.");
    for (auto& instruction:invalid.code) if (instruction.op==abc::Op::school_list) instruction.type=abc::Type::big_integer;
    error([&]{abc::verify(invalid);},"E_BYTECODE");
}

void string_arrays() {
    result("begin Writeln(' a  b '.ToWords); Writeln(ToWords('a,b;c',',',';')); Writeln('a,b;c'.ToWords(',;')); end.","[a,b]\n[a,b,c]\n[a,b,c]\n");
    result("begin var s:='a'#9'b'#8195'c'; Writeln(s.ToWords.Length); Writeln(s.ToWords('')); var d:array of char; Writeln(s.ToWords(d)); d:=Arr(#9,#8195); Writeln(PABCSystem.ToWords(s,d)); end.","1\n[a,b,c]\n[a,b,c]\n[a,b,c]\n");
    result("begin Writeln('a,b;c'.ToWords(Arr(',',';'))); Writeln(''.ToWords.Length); var d:=new char[0]; Writeln(' a  b '.ToWords(d)); end.","[a,b,c]\n0\n[a,b]\n");
    result("begin var s:='a'#13#10#10'b'#13; var a:=s.ToLines; Writeln(a.Length); foreach var x in a do Writeln('[',x,']'); var b:=''.ToLines; Writeln(b.Length,' [',b[0],']'); end.","4\n[a]\n[]\n[b]\n[]\n1 []\n");
    result("begin var a:='a'#8232'b'; Writeln(a.ToLines.Length,' ',a.ToLines[0].Length); end.","1 3\n");
    result("begin Writeln(' 12 -3'#9'7 '.ToIntegers); Writeln(ToIntegers('12abc',1)); Writeln('1 2 tail'.ToIntegers(2)); Writeln(''.ToIntegers.Length); end.","[12,-3,7]\n[12]\n[1,2]\n0\n");
    result("begin Writeln('-21474836480'.ToIntegers); Writeln('1.5 -2e1'#9'0.25'.ToReals); Writeln(ToReals('1,5 2,25',',')); Writeln(' '#9#10#13.ToReals.Length); end.","[-2147483648,0]\n[1.5,-20,0.25]\n[1.5,2.25]\n0\n");
    result("begin var s:string:=nil; Writeln(s.ToIntegers(0)); end.","[]\n");
    result("var trace:string; function S:string; begin trace:=trace+'S'; Result:='a,b'; end; function D:string; begin trace:=trace+'D'; Result:=','; end; begin Writeln(S().ToWords(D()),' ',trace); end.","[a,b] SD\n");
    for (const auto* name:{"ToWords","ToLines","ToIntegers","ToReals"})
        runtime_error(std::string("begin var s:string:=nil; s.")+name+"; end.","E_NIL");
    runtime_error("begin var s:='a b'; var d:string:=nil; s.ToWords(d); end.","E_NIL");
    runtime_error("begin var s:='12abc'; s.ToIntegers; end.","E_FORMAT");
    runtime_error("begin var s:='   '; s.ToIntegers; end.","E_FORMAT");
    runtime_error("begin var s:='12'; s.ToIntegers(2); end.","E_FORMAT");
    runtime_error("begin var s:string:=nil; s.ToIntegers(-1); end.","E_RANGE");
    runtime_error("begin var s:string:=nil; s.ToIntegers(2147483647); end.","E_LIMIT");
    runtime_error("begin var s:string:=''; s.ToReals(''); end.","E_RANGE");
    runtime_error("begin var s:string:=nil; s.ToReals(''); end.","E_RANGE");
    runtime_error("begin var s:='1'#11'2'; s.ToReals; end.","E_FORMAT");
    for (const auto source:{"begin var s:='a b'; s.ToWords(nil); end.","begin var s:='a b'; s.ToWords(Arr(1,2)); end.",
        "begin var s:='a b'; s.ToWords(new char[1,2]); end.","begin var s:='a b'; s.ToWords(',;','/'); end.",
        "begin var s:='a b'; s.ToLines(1); end.","begin var s:='1 2'; s.ToIntegers(2.0); end.","begin var s:='1 2'; s.ToReals(2); end."}) compile_error(source,"E_TYPE");
    auto program=abc::compile("begin var a:='a b'.ToWords; end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::string_array) instruction.operand=99;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin var a:='a b'.ToWords; end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::string_array) instruction.operand=static_cast<std::size_t>(abc::StringArray::words_array);
    error([&]{abc::verify(program);},"E_BYTECODE");
    abc::ExecutionOptions options; options.allocation_limit=5;
    error([&]{(void)run("begin var a:='a b c d e f'.ToWords; end.","",options);},"E_LIMIT");
    options.allocation_limit=1'048'576; std::size_t polls=0; options.cancelled=[&]{return ++polls>30;};
    error([&]{(void)run("begin var a:=StringOfChar(' ',10000).ToWords; end.","",options);},"E_CANCELLED");
}

void string_cursor_input() {
    result("begin Writeln('bad'.ToInteger(7),' ','42'.ToInteger(7),' ',ToInteger('bad',8)); Writeln('bad'.ToReal(7.5),' ','1.5'.ToReal(7),' ',ToReal('bad',8)); var s:string:=nil; Writeln(s.ToReal(9)); end.","7 42 8\n7.5 1.5 8\n9\n");
    result("var calls:integer; function F:integer; begin Inc(calls); Result:=7; end; begin Writeln('42'.ToInteger(F()),' ',calls); end.","42 1\n");
    runtime_error("begin var s:string:=nil; s.ToInteger(7); end.","E_NIL");
    result("begin var s:='12 3.5 word'; var p:=0; Writeln(s.ReadInteger(p),' ',p); Writeln(s.ReadReal(p),' ',p); Writeln(s.ReadWord(p),' ',p); end.","12 2\n3.5 6\nword 11\n");
    result("begin var s:='12abc'; var p:=0; Writeln(ReadInteger(s,p),' ',p); Writeln(PABCSystem.ReadWord(s,p),' ',p); end.","12 2\nabc 5\n");
    result("begin Writeln('42'.ToInteger,' ',ToInteger('7'),' ',PABCSystem.ToInteger('8')); Writeln('1.5'.ToReal,' ','1,5'.ToReal(','),' ',ToReal('1,234.5','.')); end.","42 7 8\n1.5 1.5 1234.5\n");
    result("begin Writeln('42'.IsInteger,' ','bad'.IsInteger(),' ','1.5'.IsReal,' ','1,5'.IsReal()); var s:string:=nil; Writeln(s.IsReal,' ',StrToReal(s)); end.","True False True False\nFalse 0\n");
    result("function ToInteger(s:string):integer; begin Result:=99; end; begin Writeln(ToInteger('7'),' ','42'.ToInteger,' ',PABCSystem.ToInteger('7')); end.","99 42 7\n");
    result("begin var s:=#8195'🌍 x'; var p:=0; Writeln(s.ReadWord(p),' ',p); Writeln(s.ReadWord(p),' ',p); end.","🌍 3\nx 5\n");
    runtime_error("begin var s:string:=nil; s.ToReal; end.","E_NIL");
    runtime_error("begin var s:string:=nil; s.IsInteger; end.","E_NIL");
    runtime_error("begin ToReal('bad'); end.","E_FORMAT");
    runtime_error("begin var s:='bad'; var p:=0; s.ReadReal(p); end.","E_FORMAT");
    runtime_error("begin var s:='12'; var p:=-1; s.ReadInteger(p); end.","E_BOUNDS");
    runtime_error("begin var s:='12'; var p:=2147483647; s.ReadInteger(p); end.","E_OVERFLOW");
    runtime_error("begin var s:='12'; var p:=3; s.ReadWord(p); end.","E_BOUNDS");
    runtime_error("begin var s:=''; var l:=new List<integer>; l.Add(0); foreach var x in l do s.ReadWord(l[0]); end.","E_MODIFIED");
    result("begin var s:=''; var p:=0; Writeln('[',s.ReadWord(p),'] ',p); end.","[] 0\n");
    for (const auto source:{"begin var s:='12'; var p:int64; s.ReadInteger(p); end.","begin var s:='12'; s.ReadWord(0); end.",
        "begin var s:='12'; for var p:=0 to 1 do s.ReadInteger(p); end.","begin Writeln('12'.IsInteger(1)); end.",
        "begin Writeln('1.5'.ToReal(true)); end.","begin Writeln('12'.ToInteger(true)); end."}) compile_error(source,"E_TYPE");
    result("begin var p:=1; Writeln(ReadIntegerFromString('  -42,7',p),' ',p); Writeln('[',ReadWordFromString('  -42,7',p),'] ',p); end.","-42 6\n[,7] 8\n");
    result("begin var p:=1; Writeln(PABCSystem.ReadIntegerFromString('12abc',p),' ',p); Writeln(ReadWordFromString('12abc',p),' ',p); end.","12 3\nabc 6\n");
    result("begin var p:=1; var s:='-21474836480'; Writeln(ReadIntegerFromString(s,p),' ',p); Writeln(ReadIntegerFromString(s,p),' ',p); end.","-2147483648 12\n0 13\n");
    result("begin var p:=1; var s:='1.5 tail'; Writeln(ReadRealFromString(s,p),' ',p); Writeln(StringIsEmpty(s,p),' ',p); Writeln(ReadWordFromString(s,p),' ',p); Writeln(StringIsEmpty(s,p),' ',p); end.","1.5 4\nFalse 5\ntail 9\nTrue 9\n");
    result("begin var p:=1; var s:=#8195'🌍 x'; Writeln(ReadWordFromString(s,p),' ',p); Writeln(StringIsEmpty(s,p),' ',p); Writeln(ReadWordFromString(s,p),' ',p); Writeln(StringIsEmpty(s,p),' ',p); end.","🌍 4\nFalse 5\nx 6\nTrue 6\n");
    result("begin var p:=1; Writeln('[',ReadWordFromString('',p),'] ',p); p:=2147483647; Writeln(StringIsEmpty('',p),' ',p); end.","[] 1\nTrue 2147483647\n");
    result("begin var p:=1; var n:=41; Writeln(TryReadIntegerFromString('bad 7',p,n),' ',p,' ',n); p:=5; Writeln(TryReadIntegerFromString('bad 7',p,n),' ',p,' ',n); end.","False 1 41\nTrue 6 7\n");
    result("begin var p:=1; var n:real:=7; Writeln(TryReadRealFromString('bad 1.5',p,n),' ',p,' ',n); p:=5; Writeln(TryReadRealFromString('bad 1.5',p,n),' ',p,' ',n); end.","False 1 0\nTrue 8 1.5\n");
    result("begin var p:=1; var n:=7; Writeln(TryReadIntegerFromString('-21474836480',p,n),' ',p,' ',n); Writeln(TryReadIntegerFromString('',p,n),' ',p,' ',n); end.","False 1 7\nFalse 1 7\n");
    result("begin var p:=1; Writeln(TryReadIntegerFromString('42',p,p),' ',p); p:=1; Writeln(TryReadIntegerFromString('bad',p,p),' ',p); end.","True 42\nFalse 1\n");
    result("begin var l:=new List<integer>; l.Add(2); foreach var x in l do Writeln(StringIsEmpty('a',l[0]),' ',x); end.","True 2\n");
    result("begin var p:=1; var l:=new List<integer>; l.Add(7); foreach var x in l do Writeln(TryReadIntegerFromString('bad',p,l[0]),' ',x,' ',p); end.","False 7 1\n");
    runtime_error("begin var l:=new List<integer>; l.Add(1); var n:integer; foreach var x in l do TryReadIntegerFromString('bad',l[0],n); end.","E_MODIFIED");
    runtime_error("begin var p:=1; var l:=new List<real>; l.Add(7); foreach var x in l do TryReadRealFromString('bad',p,l[0]); end.","E_MODIFIED");
    runtime_error("begin var p:=1; ReadIntegerFromString('2147483648',p); end.","E_OVERFLOW");
    runtime_error("begin var p:=1; ReadIntegerFromString('x',p); end.","E_FORMAT");
    runtime_error("begin var p:=100; ReadIntegerFromString('x',p); end.","E_FORMAT");
    runtime_error("begin var p:=100; ReadWordFromString('x',p); end.","E_BOUNDS");
    runtime_error("begin var p:=0; StringIsEmpty('',p); end.","E_BOUNDS");
    runtime_error("begin var p:=0; ReadIntegerFromString('',p); end.","E_BOUNDS");
    runtime_error("begin var p:=1; ReadRealFromString('x',p); end.","E_FORMAT");
    for (const auto* name:{"ReadWordFromString","ReadIntegerFromString","ReadRealFromString","StringIsEmpty"})
        runtime_error(std::string("begin var p:=1; ")+name+"(nil,p); end.","E_NIL");
    for (const auto source:{"begin ReadWordFromString('x',1); end.","begin var p:int64; ReadWordFromString('x',p); end.",
        "begin var p:integer; var n:real; TryReadIntegerFromString('1',p,n); end.","begin var p,n:integer; TryReadRealFromString('1',p,n); end.",
        "begin var p:integer; TryReadIntegerFromString('1',p,2); end.","begin for var p:=1 to 2 do ReadIntegerFromString('1',p); end."}) compile_error(source,"E_TYPE");
    auto program=abc::compile("begin var p:=1; Writeln(ReadIntegerFromString('1',p)); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::string_read) instruction.operand=99;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin var p:=1; Writeln(ReadIntegerFromString('1',p)); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::string_read) instruction.type=abc::Type::real;
    error([&]{abc::verify(program);},"E_BYTECODE");
    abc::ExecutionOptions options; std::size_t polls=0; options.cancelled=[&]{return ++polls>20;};
    error([&]{(void)run("begin var p:=1; ReadIntegerFromString(StringOfChar(' ',10000)+'1',p); end.","",options);},"E_CANCELLED");
}

void string_conversions() {
    result("begin var n:=7; var r:real:=7; Writeln('bad'.TryToInteger(n),' ',n); Writeln('bad'.TryToReal(r),' ',r); Writeln('42'.TryToInteger(n),' ',n); end.","False 7\nFalse 0\nTrue 42\n");
    compile_error("begin var s:='42'; for var n:=1 to 2 do s.TryToInteger(n); end.","E_TYPE");
    result("begin var s:string; Str(MaxUInt64,s); Writeln(s); Str(1.5,s); Writeln(s); Str('Я',s); Writeln(s); Str(nil,s); Writeln(s=nil); Str(1.5:7:2,s); Writeln('[',s,']'); end.","18446744073709551615\n1.5\nЯ\nTrue\n[   1.50]\n");
    result("begin var s:string; var b:byte:=255; Str(b,s); Writeln(s); PABCSystem.Str(-2147483648,s); Writeln(s); end.","255\n-2147483648\n");
    result("var a:array of string; function Idx:integer; begin a[0]:='after'; Result:=1; end; begin a:=Arr('before',''); Str(a[0],a[Idx()]); Writeln(a[0],' ',a[1]); end.","after before\n");
    result("begin var n:=7; var e:=9; Val('bad',n,e); Writeln(n,' ',e); Val('2147483648',n,e); Writeln(n,' ',e); Val(' -2147483648 ',n,e); Writeln(n,' ',e); end.","7 1\n7 1\n-2147483648 0\n");
    result("begin var n:=7; Val('42',n,n); Writeln(n); Val('bad',n,n); Writeln(n); end.","0\n1\n");
    result("begin var n:=7; Writeln(TryStrToInt(#8195'42'#160,n),' ',n); Writeln(TryStrToInt('bad',n),' ',n); Writeln(TryStrToInt('2'#0,n),' ',n); end.","True 42\nFalse 42\nFalse 42\n");
    result("begin var n:int64:=7; Writeln(TryStrToInt64(#160'42',n),' ',n); Writeln(TryStrToInt64(' +42 '#0,n),' ',n); Writeln(TryStrToInt64(nil,n),' ',n); end.","False 0\nTrue 42\nFalse 0\n");
    result("begin var n:real:=7; Writeln(TryStrToReal('1.5e2',n),' ',n); Writeln(TryStrToFloat('1,234',n),' ',n); Writeln(TryStrToReal(nil,n),' ',n); Writeln(PABCSystem.TryStrToFloat('1e-999',n),' ',n); end.","True 150\nFalse 0\nFalse 0\nTrue 0\n");
    for (const auto& [type,edge,bad]:{std::array<std::string,3>{"byte","255","256"},{"shortint","-128","-129"},
        {"smallint","-32768","32768"},{"word","65535","65536"},{"longword","4294967295","4294967296"},
        {"int64","9223372036854775807","9223372036854775808"},{"uint64","18446744073709551615","18446744073709551616"}})
        result("begin var n:"+type+"; var e:integer; Val('"+edge+"',n,e); Writeln(n,' ',e); Val('"+bad+"',n,e); Writeln(n,' ',e); end.",edge+" 0\n0 1\n");
    result("begin var n:uint64; var e:integer; Val('-000',n,e); Writeln(n,' ',e); Val('-1',n,e); Writeln(n,' ',e); Val('42 '#0,n,e); Writeln(n,' ',e); end.","0 0\n0 1\n42 0\n");
    result("begin var n:real:=7; var e:integer; Val('bad',n,e); Writeln(n,' ',e); Val(' .5 ',n,e); Writeln(n,' ',e); Val(nil,n,e); Writeln(n,' ',e); end.","0 1\n0.5 0\n0 1\n");
    result("begin var l:=new List<integer>; l.Add(7); foreach var x in l do Writeln(TryStrToInt('bad',l[0]),' ',x); end.","False 7\n");
    runtime_error("begin var l:=new List<int64>; l.Add(7); foreach var x in l do TryStrToInt64('bad',l[0]); end.","E_MODIFIED");
    runtime_error("begin var n:integer; TryStrToInt(nil,n); end.","E_NIL");
    runtime_error("begin var n,e:integer; Val(nil,n,e); end.","E_NIL");
    for (const auto source:{"begin var s:string; Str(true,s); end.","begin var s:string; Str(Arr(1),s); end.",
        "begin var s:string; Str(1,1); end.","begin var n:integer; Str(1,n); end.","begin var n:integer; TryStrToInt64('1',n); end.",
        "begin var n:int64; TryStrToReal('1',n); end.","begin TryStrToInt('1',2); end.","begin var n:boolean; var e:integer; Val('1',n,e); end.",
        "begin var n:integer; var e:int64; Val('1',n,e); end.","begin var n:integer; Val('1',n,2); end.",
        "begin for var n:=1 to 2 do TryStrToInt('1',n); end.","begin var n:integer; for var e:=1 to 2 do Val('1',n,e); end."}) compile_error(source,"E_TYPE");
    for (const auto op:{abc::Op::convert_string,abc::Op::try_parse,abc::Op::val}) {
        const auto source=op==abc::Op::convert_string ? "begin var s:string; Str(1,s); end." : op==abc::Op::try_parse
            ? "begin var n:integer; TryStrToInt('1',n); end." : "begin var n,e:integer; Val('1',n,e); end.";
        auto program=abc::compile(source);
        for (auto& instruction:program.code) if (instruction.op==op) instruction.operand=1;
        error([&]{abc::verify(program);},"E_BYTECODE");
    }
}

void try_console_read() {
    {
        struct BrokenInput:std::streambuf { int_type underflow() override {throw std::ios_base::failure("input failure");} } buffer;
        std::istream failed(&buffer); failed.exceptions(std::ios::badbit); std::ostringstream output;
        abc::execute(abc::compile("begin var n:=5; Writeln(TryRead(n),' ',n); end."),failed,output);
        require(output.str()=="False 5\n","TryRead catches host input failures and preserves the target");
        struct BrokenOutput:std::streambuf { std::streamsize xsputn(const char*,std::streamsize) override {return 0;} } output_buffer;
        std::ostream broken(&output_buffer); broken.exceptions(std::ios::badbit|std::ios::failbit); std::istringstream input("7");
        abc::execute(abc::compile("begin var n:integer; if TryRead(n,'N') then Writeln(1 div 0); end."),input,broken);
        require(input.tellg()==std::streampos{0},"A caught prompt write failure does not consume input");
    }
    result("begin var n:=41; Writeln(TryRead(n),' ',n); Writeln(TryRead(n),' ',n); Writeln('[',ReadlnString,']'); Writeln(TryRead(n),' ',n); end.","False 41\nTrue 9\n[ tail]\nFalse 9\n","bad 9 tail\n");
    result("begin var n:real:=2.5; Writeln(TryRead(n),' ',n); Writeln(TryRead(n),' ',n); end.","False 2.5\nTrue 150\n","NaN 1.5e2\n");
    result("begin var b:boolean:=true; Writeln(TryRead(b),' ',b); Writeln(TryRead(b),' ',b); Writeln(TryRead(b),' ',b); end.","False True\nTrue False\nTrue True\n","1 false TRUE\n");
    for (const auto& [type,edge,bad]:{std::array<std::string,3>{"byte","255","256"},{"shortint","-128","-129"},
        {"smallint","-32768","32768"},{"word","65535","65536"},{"integer","2147483647","2147483648"},
        {"longword","4294967295","4294967296"},{"int64","9223372036854775807","9223372036854775808"},
        {"uint64","18446744073709551615","18446744073709551616"}})
        result("begin var n:"+type+"; Writeln(PABCSystem.TryRead(n),' ',n); Writeln(TryRead(n),' ',n); end.","True "+edge+"\nFalse "+edge+"\n",edge+" "+bad);
    result("begin PrintDelimDefault:='|'; var n:=1; Writeln(TryRead(n,'N'),' ',n); Writeln(TryRead(n,''),' ',n); Writeln(TryRead(n,nil),' ',n); end.","N|True 7\nTrue 8\nnil|False 8\n","7 8 bad");
    result("var a:array of integer; var calls:integer; function Idx:integer; begin Inc(calls); Result:=0; end; function Msg:string; begin SetLength(a,0); Result:=''; end; begin a:=Arr(1); var old:=a; Writeln(TryRead(a[Idx()],Msg()),' ',a.Length,' ',old[0],' ',calls); end.","True 0 7 1\n","7");
    result("begin var l:=new List<integer>; l.Add(1); foreach var x in l do Writeln(TryRead(l[0]),' ',x); end.","False 1\n","bad");
    result("begin var n:=1; Writeln(TryRead(n),' ',n); Writeln(TryRead(n),' ',n); end.","False 1\nTrue 7\n","\xff\n7");
    runtime_error("begin var a:array of integer; TryRead(a[0],'N'); end.","E_NIL");
    runtime_error("function Msg:string; begin var n:=1 div 0; Result:=''; end; begin var n:integer; TryRead(n,Msg()); end.","E_DIV_ZERO");
    error([&]{(void)run("begin var l:=new List<integer>; l.Add(1); foreach var x in l do TryRead(l[0]); end.","7");},"E_MODIFIED");
    for (const auto source:{"begin TryRead(); end.","begin TryRead(1); end.","begin var s:string; TryRead(s); end.","begin var c:char; TryRead(c); end.",
        "begin var n:integer; TryRead(n,1); end.","begin var n:integer; TryRead(n,'',2); end.","begin for var n:=1 to 2 do TryRead(n); end."}) compile_error(source,"E_TYPE");
    auto program=abc::compile("begin var n:integer; TryRead(n); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::try_read) instruction.type=abc::Type::string;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin var n:integer; TryRead(n); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::try_read) instruction.operand=1;
    error([&]{abc::verify(program);},"E_BYTECODE");
    abc::ExecutionOptions options; options.output_limit=1;
    error([&]{(void)run("begin var n:integer; TryRead(n,'N'); end.","7",options);},"E_LIMIT");
    options.output_limit=1'048'576; std::size_t polls=0; options.cancelled=[&]{return ++polls>10;};
    error([&]{(void)run("begin var n:integer; TryRead(n); end.",std::string(1000,'1'),options);},"E_CANCELLED");
}

void structural_output() {
    result("begin Print(1,2); Println(3,4); Print; Println; Println(); end.","1 2 3 4\n\n\n");
    result("begin PrintDelimDefault:='|'; Print(1,2); Println(3,4); PrintDelimDefault:=''; Print(5,6); Println(7,8); end.","1|2|3|4\n5678\n");
    result("begin PrintDelimDefault:=nil; Print(1); Println(2,3); Println(4); end.","1nil2nil3\n4\n");
    result("procedure Change(var s:string); begin s:='/'; end; begin Change(PrintDelimDefault); Println(1,2); PABCSystem.PrintDelimDefault:=';'; PABCSystem.Print(3,4); PABCSystem.Println; end.","1/2\n3;4;\n");
    result("procedure Say; begin PrintDelimDefault:='|'; Println(1,2); var n:=7; Writeln(n); end; begin Say; Println(3,4); end.","1|2\n7\n3|4\n");
    result("var PrintDelimDefault:string:='local'; begin Println(1,2); Writeln(PrintDelimDefault); PABCSystem.PrintDelimDefault:='!'; Println(3,4); end.","1 2\nlocal\n3!4\n");
    result("procedure Print(x:integer); begin Writeln('own',x); end; begin Print(7); PABCSystem.Print(8); PABCSystem.Println(); end.","own7\n8 \n");
    result("begin Println(Arr(1,2),new integer[0,3]); Println(1.5:5:1,7:3); end.","[1,2] [[]]\n  1.5   7\n");
    result("begin PrintDelimDefault:='|'; var n:=ReadInteger('N'); Println(n,n); end.","N|7|7\n","7\n");
    result("begin PrintDelimDefault:=nil; var n:=ReadInteger(''); Writeln(n); end.","nil7\n","7\n");
    result("function Change:integer; begin PrintDelimDefault:='|'; Result:=2; end; begin Println(PrintDelimDefault,Change()); end."," |2\n");
    result("begin PrintDelimDefault:=#57101''; Print(#55356); PrintDelimDefault:=''; Println; end.","🌍\n");
    result("var x:integer:=ReadInteger('first'); begin PrintDelimDefault:='|'; Println(x,ReadInteger('second')); end.","first second|1|2\n","1\n2\n");
    runtime_error("begin PrintDelimDefault:=StringOfChar('x',1048576); Print(1); end.","E_LIMIT");
    compile_error("begin PrintDelimDefault:=1; end.","E_TYPE");
    compile_error("begin PrintDelimDefault(); end.","E_NAME");
    compile_error("begin var n:=Println(1); end.","E_TYPE");
    {
        const auto program=abc::compile("begin Print(1); end."); std::istringstream in; std::ostringstream a,b;
        abc::execute(program,in,a); abc::execute(program,in,b);
        require(a.str()=="1 " && b.str()=="1 ","Print delimiter state is initialized for every execution");
        auto invalid=program; invalid.print_delimiter=invalid.locals.size();
        error([&]{abc::verify(invalid);},"E_BYTECODE");
        invalid=program; invalid.locals[*invalid.print_delimiter].type=abc::Type::integer;
        error([&]{abc::verify(invalid);},"E_BYTECODE");
        invalid=program; invalid.locals[*invalid.print_delimiter].global=false;
        error([&]{abc::verify(invalid);},"E_BYTECODE");
        invalid=program; for (auto& instruction:invalid.code) if (instruction.op==abc::Op::print_output) instruction.operand=4;
        error([&]{abc::verify(invalid);},"E_BYTECODE");
    }
    result("begin Writeln('[',7:3,'] [',1.5:6:2,'] [','Я':3,'] [',true:6,'] [',nil:4,']'); end.","[  7] [  1.50] [  Я] [  True] [ nil]\n");
    result("begin Writeln('[','🌍':4,'] [',MaxUInt64:0,'] [',1.5:-6:-2,']'); end.","[  🌍] [18446744073709551615] [1.50  ]\n");
    result("type R=record x:integer; end; begin var r:R; r.x:=7; Writeln('[',r:5,'] [',Arr(1,2):7,']'); end.","[  (7)] [  [1,2]]\n");
    result("var trace:string; function V:real; begin trace:=trace+'V'; Result:=1.5; end; function W:integer; begin trace:=trace+'W'; Result:=5; end; function D:integer; begin trace:=trace+'D'; Result:=1; end; begin Writeln(V():W():D(),'|',trace); end.","  1.5|VWD\n");
    result("function FormatValue(x,w:integer):string; begin Result:='own'; end; begin Writeln(FormatValue(7,3),'|',7:3); end.","own|  7\n");
    result("var a:array of integer; function Change:integer; begin a[0]:=7; Result:=2; end; begin a:=Arr(1); Writeln(a:0,' ',Change()); end.","[1] 2\n");
    compile_error("begin Writeln('never',1:0:2); end.","E_TYPE");
    compile_error("begin Writeln('x':0:2); end.","E_TYPE");
    compile_error("begin Writeln(1:1.5); end.","E_TYPE");
    compile_error("begin var n:integer; Read(n:3); end.","E_SYNTAX");
    compile_error("begin ObjectToString(1:3); end.","E_SYNTAX");
    runtime_error("begin Writeln(1:-1); end.","E_RANGE");
    runtime_error("begin Writeln(1.5:0:-2147483648); end.","E_OVERFLOW");
    {
        std::istringstream in; std::ostringstream out;
        error([&]{abc::execute(abc::compile("begin Writeln('must not print',1:-1); end."),in,out);},"E_RANGE");
        require(out.str().empty(),"Formatted values are evaluated before the enclosing output call");
    }
    result("type R=record x:integer; end; begin var r:R; r.x:=7; Writeln(r,'|',Arr(1,2),'|',nil); end.","(7)|[1,2]|nil\n");
    result("var a:array of integer; function Change:integer; begin a[0]:=7; Result:=2; end; begin a:=Arr(1); Writeln(a,' ',Change()); end.","[7] 2\n");
    result("type R=record x:integer; end; var r:R; function Change:integer; begin r.x:=7; Result:=2; end; begin r.x:=1; Writeln(r,' ',Change()); end.","(1) 2\n");
    compile_error("uses Collections; begin Writeln('never'); Writeln(new Stack<integer>); end.","E_TYPE");
    result("begin Writeln(ObjectToString(7),'|',ObjectToString(true),'|',ObjectToString(nil),'|',PABCSystem.ObjectToString('🌍')); end.","7|True|nil|🌍\n");
    result("type R=record x:integer; s:string; a:array of integer; end; begin var r:R; r.x:=4; r.s:='Я'; r.a:=Arr(1,2); Writeln(ObjectToString(r)); Writeln('[',FormatValue(r,16),']'); end.","(4,Я,[1,2])\n[     (4,Я,[1,2])]\n");
    result("type E=record end; type R=record e:E; b:boolean; end; begin var r:R; Writeln(ObjectToString(r)); end.","((),False)\n");
    result("begin var a:array of integer; var l:List<integer>; Writeln(ObjectToString(a),'|',ObjectToString(l),'|',ObjectToString(new integer[0])); Writeln(FormatValue(nil,5)); end.","nil|nil|[]\n  nil\n");
    result("begin var m:=new integer[2,2]; m[0,1]:=7; Writeln(ObjectToString(m)); Writeln(ObjectToString(new integer[0,5]),'|',ObjectToString(new integer[3,0])); end.","[[0,7],[0,0]]\n[[]]|[[]]\n");
    result("begin var a:=Arr(Arr(1,2),Arr(3)); Writeln(ObjectToString(a)); var l:=new List<string>; l.Add(nil); l.Add('x'); Writeln(ObjectToString(l),' ',l.Count()); end.","[[1,2],[3]]\n[nil,x] 2\n");
    result("uses Arrays; type R=record x:integer; end; begin var a:=new R[2]; a[0].x:=7; WritelnArray(a,'|'); var m:=new R[1,2]; m[0,1].x:=8; WriteMatrix(m,4); m.Write('/'); end.","(7)|(0)\n (0)  (8) \n(0)/(8)/\n");
    result("uses Arrays; begin WritelnArray(Arr(Arr(1,2),Arr(3)),'|'); end.","[1,2]|[3]\n");
    result("begin Writeln(PABCSystem.FormatValue(Arr(1,2),7)); end.","  [1,2]\n");
    for (const auto count:{99,100,101,102}) {
        const auto expected=[&](int n,bool trim) { std::string text="["; for (int i=0;i<n;++i) { if (i) text+=','; text+='0'; } if (trim) text+=",..."; return text+"]\n"; };
        result("begin Writeln(ObjectToString(new integer["+std::to_string(count)+"])); end.",expected(count>100 ? 99 : count,count>100));
        result("begin var l:=new List<integer>; for var i:=1 to "+std::to_string(count)+" do l.Add(0); Writeln(ObjectToString(l)); end.",expected(count>101 ? 100 : count,count>101));
    }
    result("begin var a:=new integer[101,1]; var s:=ObjectToString(a); Writeln(s.Length,' ',s[s.Length-3],s[s.Length-2],s[s.Length-1]); end.","401 ...\n");
    runtime_error("begin FormatValue(Arr(1),-1); end.","E_RANGE");
    runtime_error("begin FormatValue(Arr(1),2147483647); end.","E_LIMIT");
    runtime_error("begin ObjectToString(Arr(StringOfChar('x',600000),StringOfChar('y',600000))); end.","E_LIMIT");
    compile_error("begin ObjectToString(1,2); end.","E_TYPE");
    compile_error("begin FormatValue(Arr(1),0,2); end.","E_TYPE");
    compile_error("uses Collections; begin var s:=new Stack<integer>; ObjectToString(s); end.","E_UNSUPPORTED");
    compile_error("type R=record f:Text; end; begin var r:R; ObjectToString(r); end.","E_UNSUPPORTED");
    auto program=abc::compile("begin Writeln(ObjectToString(Arr(1))); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::object_text) instruction.operand=2;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin Writeln(ObjectToString(Arr(1))); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::object_text) instruction.type=abc::Type::text_file;
    error([&]{abc::verify(program);},"E_BYTECODE");
}

void rectangular_arrays() {
    result("uses Arrays; begin var m:=new integer[1,2]; Arrays.FillByValue(m,3); var c:=PABCSystem.Copy(m); Writeln(PABCSystem.High(c),' ',c[0,1]); var a:=Arr(3,1,2); Arrays.Sort(a); Arrays.SortDescending(a); foreach var x in a do Write(x); end.","0 3\n321");
    compile_error("uses Arrays; begin var l:=new List<integer>; Arrays.Sort(l); end.","E_TYPE");
    result("uses Arrays; begin var m:=new integer[2,2]; m[0,0]:=1; m[0,1]:=20; m[1,0]:=-3; m[1,1]:=4; WriteMatrix(m,3); m.Write('|'); m.Writeln('/'); end.","  1  20 \n -3   4 \n1|20|\n-3|4|\n1/20/\n-3/4/\n\n");
    result("uses Arrays; begin var m:=new real[1,2]; m[0,0]:=1.5; m[0,1]:=2.25; WriteMatrix(m); Arrays.WriteMatrix(m,6,1); var a:=new real[1,1]; a[0,0]:=1.5; WriteMatrix(a,-6,-2); end.","1.50 2.25 \n   1.5    2.2 \n1.50   \n");
    result("uses Arrays; begin var m:=new string[1,2]; m[0,0]:=nil; m[0,1]:='Я'; WriteMatrix(m,4); var a:=new integer[3,0]; WriteMatrix(a,-1); a.Writeln; end."," nil    Я \n\n\n\n\n\n\n\n");
    result("begin Writeln('[',FormatValue(7,3),'] [',FormatValue(-7,1),'] [',FormatValue('🌍',4),']'); Writeln(FormatValue(MaxUInt64,2),' ',FormatValue(true,0)); Writeln(FormatValue(2.5,0,0),' ',FormatValue(3.5,0,0),' [',FormatValue(1.5,-6,-2),']'); end.","[  7] [-7] [  🌍]\n18446744073709551615 True\n2 4 [1.50  ]\n");
    result("uses Arrays; var m:array[,] of integer; var trace:string; function Width:integer; begin trace:=trace+'W'; SetLength(m,0,0); Result:=0; end; begin m:=new integer[1,2]; m[0,1]:=2; var old:=m; WriteMatrix(m,Width()); Writeln(trace,' ',m.Length,' ',old.Length); end.","0 2 \nW 0 2\n");
    result("uses Arrays; begin var m:=new real[0,2]; WriteMatrix(m,0,-2147483648); end.","");
    runtime_error("uses Arrays; begin var m:array[,] of integer; WriteMatrix(m); end.","E_NIL");
    runtime_error("uses Arrays; begin WriteMatrix(new integer[1,1],-1); end.","E_RANGE");
    runtime_error("begin FormatValue(1,-1); end.","E_RANGE");
    runtime_error("begin FormatValue(1.5,0,-2147483648); end.","E_OVERFLOW");
    runtime_error("begin FormatValue(1.5,0,2147483647); end.","E_LIMIT");
    compile_error("begin WriteMatrix(new integer[1,2]); end.","E_NAME");
    compile_error("uses Arrays; begin WriteMatrix(new integer[1,2],0,2); end.","E_TYPE");
    compile_error("uses Arrays; begin WriteMatrix(Arr(1,2)); end.","E_TYPE");
    abc::ExecutionOptions limits; std::size_t polls=0; limits.cancelled=[&]{return ++polls>20;};
    error([&]{(void)run("uses Arrays; begin WriteMatrix(new integer[20000,0]); end.","",limits);},"E_CANCELLED");
    auto invalid_output=abc::compile("uses Arrays; begin WriteMatrix(new integer[1,2]); end.");
    for (auto& instruction:invalid_output.code) if (instruction.op==abc::Op::write_matrix) instruction.operand=1;
    error([&]{abc::verify(invalid_output);},"E_BYTECODE");
    result("uses Arrays; begin SetRandomSeed(1); var a:=CreateRandomIntegerMatrix(2,3); foreach var x in a do Write(x,'|'); Writeln(' ',a.RowCount,' ',a.ColCount()); SetRandomSeed(1); var b:=Arrays.CreateRandomRealMatrix(1,2,5,7); Writeln(Round(b[0,0],2),' ',Round(b[0,1],2)); end.","25|11|47|77|66|43| 2 3\n0.5 0.22\n");
    result("uses Arrays; begin var a:=new integer[1,3]; SetRandomSeed(1); FillByRandom(a); foreach var x in a do Write(x,'|'); SetRandomSeed(1); a.FillRandom(10,12); Writeln(' ',a[0,0],' ',a[0,1],' ',a[0,2]); end.","25|11|47| 10 10 11\n");
    result("begin SetRandomSeed(1); var a:=MatrRandom(1,3); foreach var x in a do Write(x,'|'); Writeln; SetRandomSeed(1); var b:=PABCSystem.MatrRandomInteger(1,2,10,12); Writeln(b[0,0],' ',b[0,1]); end.","25|11|47|\n10 10\n");
    result("begin SetRandomSeed(1); var a:=MatrRandomReal(1,2,0,1); Writeln(a[0,0],' ',a[0,1]); var b:=MatrRandom; var c:=MatrRandomInteger(1); Writeln(RowCount(b),' ',ColCount(b),' ',c.RowCount,' ',c.ColCount); end.","0.25 0.11\n5 5 1 5\n");
    result("begin SetRandomSeed(1); var a:=MatrRandomReal(1,1,5); Writeln(a[0,0]); var b:=new real[1,2]; SetRandomSeed(1); b.FillRandom(5.0,7.0); Writeln(Round(b[0,0],2),' ',Round(b[0,1],2)); end.","6.24\n5.5 5.22\n");
    result("begin SetRandomSeed(1); var a:=MatrRandom(0,2,0,MaxInt); var b:=MatrRandomReal(1,0,0,1,16); Writeln(a.Length,' ',b.ColCount,' ',Random(100)); end.","0 0 24\n");
    result("begin var a:=new integer[1,2]; a[0,1]:=7; var b:=a.Copy; b[0,1]:=9; Writeln(a[0,1],' ',b.High,' ',b.ColCount); var c:=Arr(3,1,2); c.Sort; c.SortDescending(); foreach var x in c do Write(x); end.","7 0 2\n321");
    runtime_error("begin var m:=MatrRandom(-1,2); end.","E_RANGE");
    runtime_error("begin var m:=MatrRandom(1,1,0,MaxInt); end.","E_OVERFLOW");
    runtime_error("begin var m:=MatrRandomReal(1,1,0,1,16); end.","E_RANGE");
    compile_error("begin CreateRandomIntegerMatrix(1,2); end.","E_NAME");
    compile_error("uses Arrays; begin CreateRandomIntegerMatrix(1); end.","E_TYPE");
    compile_error("begin MatrRandom(1,2,3,4,5); end.","E_TYPE");
    result("begin var m:=new integer[2,3]; m[0,1]:=7; m[1,2]:=9; Writeln(m.Length,' ',m.Rank,' ',m.GetLength(0),' ',m.GetLength(1)); Writeln(m.GetLowerBound(0),' ',m.GetUpperBound(0),' ',m.GetUpperBound(1),' ',High(m)); foreach var x in m do Write(x,'|'); end.","6 2 2 3\n0 1 2 1\n0|7|0|0|0|9|");
    result("type Mat=array[,] of integer; begin var m:Mat; Writeln(Length(m),' ',Length(m,99),' ',High(m),' ',m=nil); SetLength(m,2,3); m[1,2]:=8; var old:=m; SetLength(m,3,2); Writeln(m.GetLength(0),' ',m.GetLength(1),' ',m[1,1],' ',old[1,2]); end.","0 0 -1 True\n3 2 0 8\n");
    result("begin var m:=new integer[2,2]; m[0,0]:=1; m[0,1]:=2; m[1,0]:=3; m[1,1]:=4; SetLength(m,3,3); foreach var x in m do Write(x); Writeln; SetLength(m,1,2); foreach var x in m do Write(x); end.","120340000\n12");
    result("begin var m:=new integer[1,1]; m[0,0]:=7; var old:=m; SetLength(m,1,1); Writeln(m=old,' ',m[0,0]); m[0,0]:=9; Writeln(old[0,0]); end.","False 7\n7\n");
    result("begin var a:=new integer[0,5]; var b:=new integer[3,0]; Writeln(a.Length,' ',a.GetUpperBound(0),' ',a.GetUpperBound(1)); Writeln(b.Length,' ',b.GetUpperBound(0),' ',b.GetUpperBound(1)); foreach var x in b do Write('bad'); end.","0 -1 4\n0 2 -1\n");
    result("type Mat=array[,] of integer; function Make:Mat; begin Result:=new integer[1,2]; Result[0,1]:=8; end; procedure Grow(var m:Mat); begin SetLength(m,2,3); end; begin var m:=Make; Grow(m); Writeln(m[0,1],' ',m.GetLength(1)); end.","8 3\n");
    result("type R=record x:integer; a:array of integer; end; begin var m:=new R[1,2]; m[0,0].x:=4; m[0,0].a:=Arr(7); var c:=Copy(m); c[0,0].x:=9; c[0,0].a[0]:=8; Writeln(m[0,0].x,' ',m[0,0].a[0],' ',c.Rank,' ',c.GetLength(1)); end.","4 8 2 2\n");
    result("begin var m:=new integer[2,2]; FillByValue(m,7); foreach var x in m do Write(x); var c:=Copy(m); c[0,0]:=8; Writeln(' ',m[0,0],' ',c[0,0]); end.","7777 7 8\n");
    result("begin var a:=new real[1,2]; a[0,1]:=1.5; var b:=new boolean[2,1]; b[1,0]:=true; Writeln(a[0,0],' ',a[0,1],' ',b[1,0]); var c:=new char[1,2]; c[0,0]:='Я'; Writeln(c[0,0],' ',Ord(c[0,1])); end.","0 1.5 True\nЯ 0\n");
    result("begin var a:=new integer[2]; Writeln(a.Rank,' ',a.GetLength(0),' ',a.GetUpperBound(0),' ',a.GetLowerBound(0)); Writeln(PABCSystem.Length(a,0)); end.","1 2 1 0\n2\n");
    result("type Mat=array[,] of integer; var m:Mat; var trace:string; function I:integer; begin trace:=trace+'I'; Result:=1; end; function J:integer; begin trace:=trace+'J'; Result:=0; end; begin m:=new integer[2,2]; m[I(),J()]:=7; Writeln(trace,' ',m[1,0]); end.","IJ 7\n");
    result("function R:integer; begin Write('R'); Result:=2; end; function C:integer; begin Write('C'); Result:=3; end; begin var m:=new integer[R(),C()]; Writeln(m.Length); end.","RC6\n");
    result("type Mat=array[,] of integer; var m:Mat; function Rows:integer; begin m:=new integer[1,1]; m[0,0]:=99; Result:=2; end; begin m:=new integer[1,1]; m[0,0]:=7; SetLength(m,Rows(),2); Writeln(m[0,0],' ',m.GetLength(0)); end.","7 2\n");
    result("type Mat=array[,] of integer; var a:array of Mat; var trace:string; function Index:integer; begin trace:=trace+'I'; Result:=0; end; begin a:=new Mat[1]; a[0]:=new integer[1,1]; a[0][0,0]:=7; SetLength(a[Index()],2,2); Writeln(trace,' ',a[0][0,0]); end.","II 7\n");
    result("begin var m:=new integer[2,2]; var old:=m; foreach var x in m do begin SetLength(m,1,1); Write(x); end; Writeln(' ',old.Length,' ',m.Length); end.","0000 4 1\n");
    for (const auto source:{"begin var a:array[,] of integer; Writeln(a.Length); end.","begin var a:array of integer; Writeln(a.Length); end.","begin var a:array[,] of integer; Writeln(a.Rank); end.","begin var a:array[,] of integer; Writeln(a.GetLength(0)); end.","begin var a:array[,] of integer; a[0,0]:=1; end."}) runtime_error(source,"E_NIL");
    for (const auto source:{"begin var a:=new integer[1,2]; Writeln(a[0,2]); end.","begin var a:=new integer[1,2]; a[-1,0]:=1; end.","begin var a:=new integer[3,0]; a[0,0]:=1; end.","begin var a:=new integer[1,2]; Writeln(a.GetLength(2)); end.","begin var a:=new integer[2]; Writeln(a.GetUpperBound(-1)); end."}) runtime_error(source,"E_BOUNDS");
    for (const auto source:{"begin var n:integer:=-1; var a:=new integer[n,2]; end.","begin var a:=new integer[1,1]; var n:integer:=-1; SetLength(a,n,2); end."}) runtime_error(source,"E_RANGE");
    runtime_error("begin var a:=new integer[2147483647,2147483647]; end.","E_LIMIT");
    for (const auto source:{"begin var a:=new integer[2,2]; Writeln(a[0]); end.","begin var a:=new integer[2]; Writeln(a[0,0]); end.","begin var a:=new integer[2,2]; SetLength(a,2); end.","begin var a:=new integer[2]; SetLength(a,2,3); end.","begin var a:=new integer[1,2]; Sort(a); end.","begin var a:=new integer[1,2]; Reverse(a); end.","begin var a:=new integer[1,2]; var b:array of integer:=a; end.","begin var a:=new integer[1,2]; Writeln(a.GetLength(1.5)); end."}) compile_error(source,"E_TYPE");
    compile_error("begin var a:array[,,] of integer; end.","E_UNSUPPORTED");
    compile_error("begin var a:=new integer[1,2,3]; end.","E_UNSUPPORTED");
    auto program=abc::compile("begin var a:=new integer[1,2]; Writeln(a[0,1]); end.");
    program.types[0].rank=3; error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin var a:=new integer[1,2]; Writeln(a[0,1]); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::get_index) instruction.operand=1;
    error([&]{abc::verify(program);},"E_BYTECODE");
    program=abc::compile("begin var a:=new integer[1,2]; Writeln(a.Rank); end.");
    for (auto& instruction:program.code) if (instruction.op==abc::Op::array_query) instruction.operand=999;
    error([&]{abc::verify(program);},"E_BYTECODE");
}

void console_unicode_output() {
    result("begin foreach var c in '🌍' do Write(c); end.","🌍");
    result("begin Write(#55356); Write(#57101); end.","🌍");
    result("begin Writeln(#55356,#57101); end.","🌍\n");
    result("begin Write(#55356); Write(''); Write(#57101); end.","🌍");
    result("begin Write(#57101); end.","�");
    result("begin Write(#55356); end.","�");
    result("begin Write(#55356); Exit; Write(#57101); end.","�");
    result("begin Write(#55356); Write(#55356); Write(#57101); end.","�🌍");
    result("begin Write(#55356); Write(1); Write(#57101); end.","�1�");
    result("begin var s:='🌍'; Write(Copy(s,1,1)); Write(Copy(s,2,1)); end.","🌍");
    result("uses Arrays; begin WritelnArray(Arr(#55356,#57101),''); WritelnArray(Arr(#55356,#57101)); end.","🌍\n� �\n");
    result("uses Collections; begin var q:=new Queue<char>; q.Enqueue(#57101); Write(#55356); q.Print; end.","🌍 ");
    result("uses Collections; begin var l:=new LinkedList<char>; l.AddLast(#57101); Write(#55356); l.Print; end.","🌍 ");
    result("begin Write(#55356); var n:=ReadInteger(#57101); Writeln(n); end.","🌍 7\n","7\n");
    result("begin Write(#0); end.",std::string(1,'\0'));
    abc::ExecutionOptions options; options.output_limit=4;
    require(run("begin Write(#55356); Write(#57101); end.","",options)=="🌍","UTF-8 output limit counts paired surrogate bytes once");
    options.output_limit=3;
    error([&]{(void)run("begin Write(#55356); Write(#57101); end.","",options);},"E_LIMIT");
    options.output_limit=2;
    error([&]{(void)run("begin Write(#55356); end.","",options);},"E_LIMIT");
    std::istringstream input; std::ostringstream output;
    error([&]{abc::execute(abc::compile("begin Write(#55356); Writeln(1 div 0); end."),input,output);},"E_DIV_ZERO");
    require(output.str()=="�","Pending console surrogate is flushed without replacing the execution error");
    struct BrokenBuffer:std::streambuf {
        std::streamsize xsputn(const char*,std::streamsize) override {return 0;}
        int sync() override {return -1;}
    } buffer;
    std::ostream broken(&buffer); broken.exceptions(std::ios::badbit|std::ios::failbit);
    error([&]{abc::execute(abc::compile("begin Writeln('x'); end."),input,broken);},"E_OUTPUT");
    std::istringstream final_token("7"); final_token.exceptions(std::ios::badbit|std::ios::failbit|std::ios::eofbit);
    std::ostringstream token_output;
    abc::execute(abc::compile("begin Writeln(ReadInteger); end."),final_token,token_output);
    require(token_output.str()=="7\n","A final token survives EOF exceptions enabled by the C++ host");
    std::istringstream ended; ended.exceptions(std::ios::badbit|std::ios::failbit|std::ios::eofbit);
    std::ostringstream eof_output;
    abc::execute(abc::compile("begin Writeln(Ord(ReadChar)); end."),ended,eof_output);
    require(eof_output.str()=="65535\n","EOF character survives host stream exception flags");
    struct BrokenInput:std::streambuf {
        int_type underflow() override {throw std::ios_base::failure("broken input");}
    } input_buffer;
    std::istream failed(&input_buffer); failed.exceptions(std::ios::badbit);
    error([&]{abc::execute(abc::compile("begin ReadInteger; end."),failed,eof_output);},"E_INPUT");
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
    result("uses Arrays; type R=record x:integer; end; begin var a:=new R[1]; WritelnArray(a); end.","(0)\n");
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
    for (const auto source:{"begin GCD(1.0,2.0); end.","begin var a:int64:=GCD(MaxUInt64,1); end.","begin LCM(1.0,2); end.","begin var i:integer:=LCM(1,2); end.","begin var a:int64:=2; var i:integer:=GCD(a,2); end.","begin Atan2(1); end.","begin Hypot('abc',2); end.","begin Lerp(0,1,true); end.","begin Remap(1,0,2,10); end."}) compile_error(source,"E_TYPE");
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
    result("begin writeln(nil); end.", "nil\n");
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
    runtime_error("begin var f:Text; f.Writeln(Arr(1)); end.","E_IO");
    for (const auto* source:{"begin var f:Text; f.Print(f); end.",
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
    console_unicode_output();
    rectangular_arrays();
    structural_output();
    try_console_read();
    string_conversions();
    string_cursor_input();
    string_arrays();
    school_library();
    school_truth_table_print();
    school_truth_tables();
    string_value_methods();
    school_ip_classes();
    lazy_ip_sequences();
    collection_sequence_views();
    school_big_integer_library();
    school_fold_library();
    unicode_identifiers_and_school_aliases();
    unicode_character_library();
    tuple_library();
    school_minmax_library();
    big_integer_library();
    school_text_library();
    school_swap_substrings();
    regex_search_library();
    sequence_to_list_library();
    sequence_queries_library();
    sequence_single_library();
    sequence_factories_library();
    sequence_ranges_library();
    stepped_ranges_library();
    big_ranges_library();
    partition_points_library();
    real_ranges_library();
    school_factor_library();
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

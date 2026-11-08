#include "abc/core.hpp"
#include "../src/array_operations.hpp"
#include "../src/list_operations.hpp"
#include "../src/sequences.hpp"
#include "../src/runtime_values.hpp"
#include <array>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
int checks=0,failures=0;void check(bool ok,std::string_view message){++checks;if(!ok){++failures;if(failures<=16)std::cerr<<"FAIL "<<message<<'\n';}}
std::vector<std::string> split(std::string_view s,char delimiter='\t'){std::vector<std::string> r;std::size_t first=0;for(;;){auto last=s.find(delimiter,first);r.emplace_back(s.substr(first,last==s.npos ? s.size()-first : last-first));if(last==s.npos)break;first=last+1;}return r;}
std::string run(std::string_view source,abc::ExecutionOptions options={}){std::istringstream in;std::ostringstream out;abc::execute(abc::compile(source),in,out,options);return out.str();}
void result(std::string_view source,std::string_view expected){try{check(run(source)==expected,source);}catch(const std::exception& e){check(false,std::string(source)+" -> "+e.what());}}
template<class F>void error(F action,std::string_view code){try{action();check(false,std::string("Expected ")+std::string(code));}catch(const abc::Error& e){check(e.code==code && !e.russian.empty() && *e.what(),std::string(code)+" -> "+e.code+" "+e.what());}catch(const std::exception& e){check(false,e.what());}}
void reference(){
 const auto dir=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::ifstream source(dir/"sequence-equality-program.pas");check(bool(source),"Sequence equality source present");std::ostringstream text;text<<source.rdbuf();const auto p=abc::compile(text.str());std::ifstream input(dir/"sequence-equality-reference.tsv");check(bool(input),"Independent SequenceEqual corpus present");std::string line;int rows=0;
 while(std::getline(input,line)){if(!line.empty() && line.back()=='\r')line.pop_back();if(line.empty() || line.front()=='#')continue;const auto f=split(line);if(f.size()!=14){check(false,"Invalid SequenceEqual reference row");continue;}std::string parameters;for(int n=0;n<12;++n)parameters+=f[n]+" ";std::istringstream in(parameters+"\n");std::ostringstream out;std::string code="ok";try{abc::ExecutionOptions options;options.cleanup_instruction_limit=100000;options.cleanup_allocation_limit=65536;abc::execute(p,in,out,options);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty(),"SequenceEqual fault is bilingual");}check(out.str()==f[12] && code==f[13],parameters+"expected "+f[12]+" "+f[13]+" got "+out.str()+" "+code);++rows;
 }check(rows==22616,"All declared equality, policy, source protocol, mutation, alias and disposal traces consumed");
}
void semantics(){
 result("type C=class(IEquatable<C>) public function Equals(x:C):boolean;begin Write('E;');Result:=False;end;end;begin var c:=new C;Writeln(SequenceEqual(Arr(c),Arr(c)),' ',SequenceEqual(Arr(c),Arr(c),nil));var a:=new IEquatable<C>[1];a[0]:=c;Writeln(a.SequenceEqual(a));end.","E;E;False False\nTrue\n");
 result("type R=record n:integer;end;begin var a:=new R[2];var b:=new R[2];a[1].n:=7;b[1].n:=7;Writeln(SequenceEqual(a,b));b[1].n:=8;Writeln(a.SequenceEqual(b,nil));end.","True\nFalse\n");
 result("type R=record n:integer;a:array of integer;end;C=class(IEqualityComparer<R>) public function Equals(a,b:R):boolean;begin a.n:=9;a.a[0]+=1;Result:=a.n=b.n;end;function GetHashCode(a:R):integer:=0;end;begin var a:R;a.n:=1;a.a:=Arr(0);var b:=a;b.n:=9;var x:=Arr(a);Writeln(SequenceEqual(x,Arr(b),new C),' ',x[0].n,' ',x[0].a[0]);end.","True 1 1\n");
 result("type I=interface(IEqualityComparer<integer>) end;C=class(I) public function Equals(a,b:integer):boolean:=a mod 10=b mod 10;function GetHashCode(a:integer):integer:=a mod 10;end;begin var c:I:=new C;Writeln(PABCSystem.SequenceEqual(Arr(1,2),Arr(11,12),c));Writeln(Range(3,1).SequenceEqual(new integer[0],nil));Writeln('ab'.SequenceEqual(Arr('a','b'),nil));end.","True\nTrue\nTrue\n");
 for(const auto source:{"begin SequenceEqual(Arr(1),Arr(1),EqualityComparer<string>.Default);end.","begin SequenceEqual(Arr(1),Arr('a'));end.","begin SequenceEqual(new integer[1,1],Arr(1));end.","begin SequenceEqual(Arr(1));end.","begin SequenceEqual(Arr(1),Arr(1),nil,nil);end."})error([&]{abc::compile(source);},"E_TYPE");
}
void metadata(){
 const auto p=abc::compile("begin var x:=EqualityComparer<string>.Default;SequenceEqual(Arr(1),Arr(1));SequenceEqual(Arr(1),Arr(1),nil);end.");for(const auto& op:p.code)if(op.op==abc::Op::sequence_equal)check(op.operand==0 || abc::type_info(p,static_cast<abc::Type>(op.operand)).kind==abc::CompoundKind::equality_comparer_interface,"Exact canonical equality policy or historical zero operand");
 for(int mode=0;mode<7;++mode){auto bad=p;abc::Type policy=abc::Type::invalid,factory=abc::Type::invalid;for(std::size_t n=0;n<p.types.size();++n){const auto& t=p.types[n];if(t.kind==abc::CompoundKind::equality_comparer_interface && t.element==abc::Type::string)policy=static_cast<abc::Type>(64+n);if(t.kind==abc::CompoundKind::equality_comparer)factory=static_cast<abc::Type>(64+n);if(mode==6 && t.kind==abc::CompoundKind::enumerator)bad.types[n].element=abc::Type::string;}
 for(auto& op:bad.code)if(op.op==abc::Op::sequence_equal){if(mode==0)op.type=abc::Type::integer;if(mode==1)op.operand=static_cast<std::size_t>(abc::Type::invalid);if(mode==2)op.operand=std::size_t{1}<<40U;if(mode==3)op.operand=static_cast<std::size_t>(policy);if(mode==4)op.operand=static_cast<std::size_t>(factory);if(mode==5)op.operand=0;}error([&]{abc::verify(bad);},"E_BYTECODE");}
}
void limits_and_order(){
 const auto source="function L:sequence of integer;begin Write('L');Result:=nil;end;function R:sequence of integer;begin Write('R');Result:=nil;end;function P:IEqualityComparer<integer>;begin Write('P');Result:=nil;end;begin SequenceEqual(L,R,P);end.";const auto p=abc::compile(source);std::istringstream in;std::ostringstream out;error([&]{abc::execute(p,in,out);},"E_NIL");check(out.str()=="LRP","All three arguments evaluated once before nil validation");
 const auto infinite="type C=class(IEqualityComparer<integer>) public function Equals(a,b:integer):boolean;begin while True do begin end;Result:=False;end;function GetHashCode(a:integer):integer:=0;end;begin SequenceEqual(Arr(1),Arr(1),new C);end.";for(int mode=0;mode<2;++mode){abc::ExecutionOptions options;int calls=0;if(mode)options.cancelled=[&]{return ++calls>100;};else options.instruction_limit=500;error([&]{run(infinite,options);},mode ? "E_CANCELLED" : "E_LIMIT");}
 const auto dispose_loop="type C=class(IEnumerator<integer>) public function MoveNext:boolean:=True;property Current:integer read One;function One:integer:=1;procedure Reset;begin end;procedure Dispose;begin while True do begin end;end;end;S=class(IEnumerable<integer>) public function GetEnumerator:IEnumerator<integer>:=new C;end;P=class(IEqualityComparer<integer>) public function Equals(a,b:integer):boolean;begin var zero:=0;Result:=1 div zero=0;end;function GetHashCode(a:integer):integer:=0;end;begin SequenceEqual(new S,new S,new P);end.";abc::ExecutionOptions limited;limited.instruction_limit=500;limited.cleanup_instruction_limit=200;error([&]{run(dispose_loop,limited);},"E_LIMIT");
 const auto halt="type C=class(IEnumerator<integer>) public function MoveNext:boolean:=True;property Current:integer read One;function One:integer:=1;procedure Reset;begin end;procedure Dispose;begin Write('D');end;end;S=class(IEnumerable<integer>) public function GetEnumerator:IEnumerator<integer>:=new C;end;P=class(IEqualityComparer<integer>) public function Equals(a,b:integer):boolean;begin Write('E');Halt(7);Result:=False;end;function GetHashCode(a:integer):integer:=0;end;begin SequenceEqual(new S,new S,new P);end.";std::istringstream input;std::ostringstream output;check(abc::execute(abc::compile(halt),input,output)==7 && output.str()=="E","Halt retains its exit code and skips user Dispose callbacks");
}
}
int main(){try{reference();semantics();metadata();limits_and_order();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" sequence equality checks, "<<failures<<" failures\n";return failures?1:0;}

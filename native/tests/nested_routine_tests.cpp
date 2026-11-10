#include "abc/core.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
unsigned checks=0,failures=0;
void check(bool ok,std::string_view label){++checks;if(!ok){++failures;if(failures<=20)std::cerr<<"FAIL "<<label<<'\n';}}
std::string run(std::string_view source,abc::ExecutionOptions options={}){std::istringstream in;std::ostringstream out;abc::execute(abc::compile(source),in,out,options);return out.str();}
void result(std::string_view source,std::string_view expected){try{const auto actual=run(source);check(actual==expected,std::string(source)+" actual="+actual);}catch(const std::exception& e){check(false,std::string(source)+" "+e.what());}}
void error(std::string_view source,std::string_view expected,abc::ExecutionOptions options={}){std::istringstream in;std::ostringstream out;try{abc::execute(abc::compile(source),in,out,options);check(false,"Expected "+std::string(expected));}catch(const abc::Error& e){check(e.code==expected&&out.str().empty()&&!e.russian.empty()&&*e.what()&&e.location.line>0,std::string(e.what())+" "+e.code+" output="+out.str());}}
std::string decoded(std::string_view s){std::u16string r;for(std::size_t n=0;n<s.size();n+=4)r+=static_cast<char16_t>(std::stoul(std::string(s.substr(n,4)),nullptr,16));return abc::to_utf8(r);}
void reference(){const auto folder=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::ifstream source(folder/"nested-routines-reference.pas");check(bool(source),"Typed local routine reference exists");std::ostringstream text;text<<source.rdbuf();const auto program=abc::compile(text.str());std::ifstream data(folder/"nested-routines-reference.tsv");check(bool(data),"Independent closure execution reference exists");std::string line;unsigned rows=0;while(std::getline(data,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line[0]=='#')continue;const auto a=line.find('\t'),b=line.find('\t',a+1),c=line.find('\t',b+1);if(c==line.npos)throw std::runtime_error("Bad closure reference row");std::istringstream in(line.substr(0,c));std::ostringstream out;try{abc::execute(program,in,out);check(out.str()==decoded(line.substr(c+1)),"Escaped aliases, independent activations and nested recursion row "+std::to_string(rows));}catch(const std::exception& e){check(false,e.what());}++rows;}check(rows==1071,"All independent closure executions checked");}
void semantics(){
 result("procedure P;var x:=1;procedure IncX;begin Inc(x);end;begin IncX;IncX;Writeln(x);end;begin P;P;end.","3\n3\n");
 result("procedure P;var x:=1;procedure A;begin Inc(x);end;procedure B;begin A;Inc(x);end;begin B;Writeln(x);end;begin P;end.","3\n");
 result("function F(n:integer):integer;function G(x:integer):integer;begin if x=0 then G:=n else G:=G(x-1)+1;end;begin Result:=G(3);end;begin Writeln(F(10));end.","13\n");
 result("function F:integer;function G:integer;begin G:=2;G+=3;end;begin F:=G();end;begin Writeln(F);end.","5\n");
 result("procedure P;var n:=2;procedure Add(var x:integer;const y:integer);begin x+=y;Inc(n);end;begin Add(n,3);Writeln(n);end;begin P;end.","6\n");
 result("procedure P;procedure Show(params a:array of integer);begin Writeln(a.Length);end;begin Show(1,2,3);end;begin P;end.","3\n");
 result("procedure P;const n=7;function G(x:integer):integer:=n+x;begin Writeln(G(2));end;begin P;end.","9\n");
 result("procedure P;var n:=4;procedure Q;const step=2;procedure R;begin n+=step;end;begin R;end;begin Q;Writeln(n);end;begin P;end.","6\n");
 result("function Make(n:integer):Func0<integer>;function Next:integer;begin Inc(n);Result:=n;end;begin Result:=Next;end;begin var a:=Make(0);var b:=Make(10);var alias:=a;Writeln(a(),' ',alias(),' ',b(),' ',a());end.","1 2 11 3\n");
 result("procedure P;function R(n:integer):integer;begin if n=0 then Exit(9);Result:=R(n-1);end;begin Writeln(R(4));end;begin P;end.","9\n");
 result("procedure P;var n:=3;function G:integer;begin Result:=n;end;begin var a:=new integer[2];a[0]:=G();a[1]:=G();Writeln(a[0]+a[1]);end;begin P;end.","6\n");
 result("function Value:integer:=99;procedure P;function Value:integer:=7;begin Writeln(Value());end;begin P;Writeln(Value);end.","7\n99\n");
 result("procedure P;var x:=1;procedure G;begin Inc(x);end;begin var a:Action0:=G;a();Writeln(x);end;begin P;end.","2\n");
 result("procedure P(var n:integer);procedure Q;procedure R;begin Inc(n);end;begin R;end;begin Q;end;begin var n:=8;P(n);Writeln(n);end.","9\n");
 result("function F:integer;procedure SetValue;begin Result:=42;end;begin SetValue;end;begin Writeln(F);end.","42\n");
 result("procedure P;procedure Q(Result:integer);begin Writeln(Result);end;begin Q(17);end;begin P;end.","17\n");
 result("procedure P(out n:integer);procedure Q;begin n:=19;end;begin Q;end;begin var n:=1;P(n);Writeln(n);end.","19\n");
 result("type R=record n:integer;procedure P;end;procedure R.P;procedure Q;begin Inc(n);end;begin Q;end;begin var r:R;r.n:=20;r.P;Writeln(r.n);end.","21\n");
 result("function Make(var n:integer):Func0<integer>;function Next:integer;begin Inc(n);Result:=n;end;begin Result:=Next;end;begin var n:=10;var f:=Make(n);Writeln(f(),' ',n);end.","11 11\n");
 error("procedure P(var n:integer);procedure Q;begin var f:=()->n;end;begin Write('bad');Q;end;begin var n:=1;P(n);end.","E_TYPE");
 error("procedure P;function F(n:integer):integer:=n;begin Write('bad');F('text');end;begin P;end.","E_TYPE");
 error("procedure P;procedure Q(var n:integer);begin end;begin Write('bad');Q(1);end;begin P;end.","E_TYPE");
 error("procedure P;procedure Q(const n:integer);begin n:=2;end;begin Write('bad');end;begin P;end.","E_TYPE");
 error("procedure P;procedure Q;begin end;procedure Q;begin end;begin Write('bad');end;begin P;end.","E_DUPLICATE");
 error("procedure P;var Q:=1;procedure Q;begin end;begin Write('bad');end;begin P;end.","E_DUPLICATE");
 error("procedure P;procedure Q;begin end;begin Write('bad');Q:=nil;end;begin P;end.","E_TYPE");
 error("procedure P;procedure Q;begin end;begin end;begin Write('bad');Q;end.","E_NAME");
 error("procedure P;function F(F:integer):integer:=F;begin Write('bad');end;begin P;end.","E_DUPLICATE");
 error("procedure P;procedure Q(n:integer:=1);begin end;begin Write('bad');Q;end;begin P;end.","E_UNSUPPORTED");
 abc::ExecutionOptions limited;limited.call_limit=12;error("procedure P;function F(n:integer):integer;begin Result:=F(n+1);end;begin F(0);end;begin P;end.","E_LIMIT",limited);
 for(const unsigned depth:{200U,300U}){std::string deep;for(unsigned n=0;n<depth;++n)deep+="procedure P"+std::to_string(n)+";";for(unsigned n=0;n<depth;++n)deep+="begin end;";deep+="begin end.";if(depth==300)error(deep,"E_LIMIT");else try{abc::compile(deep);check(true,"Deep declarations fit the native compilation stack");}catch(const abc::Error& e){check(e.code=="E_LIMIT"&&!e.russian.empty()&&e.location.line>0,"Deep declarations fail safely within the compilation stack budget");}}
}
}
int main(){try{reference();semantics();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" nested routine checks, "<<failures<<" failures\n";return failures?1:0;}

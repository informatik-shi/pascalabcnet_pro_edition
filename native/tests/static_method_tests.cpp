#include "abc/core.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
unsigned checks=0,failures=0;
void check(bool ok,std::string_view label){++checks;if(!ok){++failures;if(failures<=25)std::cerr<<"FAIL "<<label<<'\n';}}
std::string contents(const std::filesystem::path& p){std::ifstream in(p,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
std::string run(std::string_view source,std::string_view input={}){std::istringstream in{std::string(input)};std::ostringstream out;abc::execute(abc::compile(source),in,out);return out.str();}
void result(std::string_view source,std::string_view expected){try{const auto actual=run(source);check(actual==expected,std::string(source)+" actual="+actual);}catch(const std::exception& e){check(false,std::string(source)+" "+e.what());}}
void error(std::string_view source,std::string_view code,std::string_view prefix={}){std::istringstream in;std::ostringstream out;try{abc::execute(abc::compile(source),in,out);check(false,"Expected "+std::string(code));}catch(const abc::Error& e){check(e.code==code&&out.str()==prefix&&!e.russian.empty()&&e.location.line>0,std::string(source)+" actual="+e.code+" "+out.str());}}
void reference(){const auto path=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";const auto source=contents(path/"static-methods-template.pas");check(!source.empty(),"Typed static-method recipe exists");std::ifstream fixture(path/"static-methods-reference.tsv");check(bool(fixture),"Actual CLR static-method executions exist");std::string line;unsigned rows=0;while(std::getline(fixture,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line[0]=='#')continue;std::istringstream fields(line);std::string mode,seed,delta,units;std::getline(fields,mode,'\t');std::getline(fields,seed,'\t');std::getline(fields,delta,'\t');std::getline(fields,units);std::u16string expected;for(std::size_t p=0;p<units.size();p+=4)expected+=static_cast<char16_t>(std::stoul(units.substr(p,4),nullptr,16));try{const auto actual=run(source,mode+" "+seed+" "+delta);check(actual==abc::to_utf8(expected),"CLR static-method row "+std::to_string(rows)+" actual="+actual);}catch(const std::exception& e){check(false,e.what());}++rows;}check(rows==64,"All CLR static-method traces checked");}
void semantics(){
 result("type C=class public static function F(n:integer):integer:=n+1;end;begin Writeln(C.F(6));end.","7\n");
 result("type C=class static function F:integer:=7;end;begin Writeln(C.F);end.","7\n");
 result("type C=class static function F(n:integer):integer;end;static function C.F(n:integer):integer;begin Result:=n+1;end;begin Writeln(C.F(6));end.","7\n");
 result("type C=class static function F:integer;end;static function C.F;begin Result:=7;end;begin Writeln(C.F);end.","7\n");
 result("type C=class static procedure P(var n:integer);begin Inc(n);end;end;begin var n:=6;C.P(n);Writeln(n);end.","7\n");
 result("type C=class static function F(n:integer):integer;begin if n=0 then Result:=1 else Result:=n*F(n-1);end;end;begin Writeln(C.F(5));end.","120\n");
 result("type C=class static function F(n:integer):integer:=n+1;function G:integer:=F(6);end;begin var o:=new C;Writeln(o.G);end.","7\n");
 result("type C=class n:integer;static function F(n:integer):integer;begin var g:=()->n+1;Result:=g();end;end;begin Writeln(C.F(6));end.","7\n");
 result("type C=class static function F(n:integer):integer:=n+1;end;begin var f:=C.F;Writeln(f(6));end.","7\n");
 result("type C=class static function F:Func0<integer>;begin Result:=()->7;end;end;begin var f:Func0<integer>:=C.F;Writeln(f());end.","7\n");
 result("type C=class static function F:Func0<integer>;begin Result:=()->7;end;static function G:integer;begin var f:Func0<integer>:=F;Result:=f();end;end;begin Writeln(C.G);end.","7\n");
 result("type C=class static function F(n:integer):integer:=n+1;end;begin Writeln(Arr(1,2,3).Select(C.F).Sum);end.","9\n");
 result("type C=class static n:integer:=F;static function F:integer:=7;end;begin Writeln(C.n);end.","7\n");
 result("type C=class static n:=F;static function F:integer:=7;end;begin Writeln(C.n);end.","7\n");
 result("type C=class static function F(n:integer):integer:=n+1;static function G:integer;begin var F:=()->9;Result:=F();end;end;begin Writeln(C.G);end.","9\n");
 result("type C=class static procedure P(n:integer);begin Writeln(n);end;static procedure Q;begin var p:Action<integer>:=P;p(7);end;end;begin C.Q;end.","7\n");
 result("type C=class static function F(n:integer:=7):integer:=n;static function F(s:string):integer:=s.Length;end;begin Writeln(C.F,' ',C.F('xy'));end.","7 2\n");
 result("type C=class static function Abs(n:integer):integer:=n+10;static function G:integer:=Arr(1,2).Select(Abs).Sum;end;begin Writeln(C.G);end.","23\n");
 result("type C=class static function F(n:integer):integer:=n+10;static function F(s:string):integer:=s.Length;end;begin Writeln(Arr(1,2).Select(C.F).Sum);end.","23\n");
 result("type C=class static function F(n:integer):integer:=n;end;const values:array of integer=(C.F(7),C.F(8));begin Writeln(values[0],' ',values[1]);end.","7 8\n");
 error("type C=class static function F(n:integer):integer:=n+1;end;begin Write('bad');C.F('xy');end.","E_TYPE");
 error("type C=class static procedure P(var n:integer);begin Inc(n);end;end;begin Write('bad');C.P(1);end.","E_TYPE");
 error("type C=class n:integer;static function F:integer:=n;end;begin Write('bad');end.","E_TYPE");
 error("type C=class static function F:integer:=1;static function F:integer:=2;end;begin Write('bad');end.","E_DUPLICATE");
 error("type C=class static function F:integer:=1;property F:integer read 2;end;begin Write('bad');end.","E_DUPLICATE");
 error("type C=class F:integer;static function F:integer:=1;end;begin Write('bad');end.","E_DUPLICATE");
 error("type C=class function F:integer;end;static function C.F:integer;begin Result:=7;end;begin Write('bad');end.","E_TYPE");
 error("type C=class static procedure P;begin Writeln(Self);end;end;begin Write('bad');end.","E_NAME");
 error("type C=class static n:integer:=1 div 0;static function F:integer:=7;end;begin Write('before');Writeln(C.F);end.","E_DIV_ZERO","before");
 error("type C=class static n:integer:=1 div 0;static function F:integer:=7;end;begin var f:Func0<integer>:=C.F;Write('before');Writeln(f());end.","E_DIV_ZERO","before");
 auto original=abc::compile("type C=class static function F:integer:=7;end;begin Writeln(C.F);end.");
 for(unsigned mutation=0;mutation<7;++mutation){auto p=original;auto& m=p.static_methods[0];if(mutation==0)m.owner=abc::Type::integer;if(mutation==1)m.routine=p.routines.size();if(mutation==2)m.name="wrong";if(mutation==3)p.static_methods.push_back(m);if(mutation==4)p.static_methods.clear();if(mutation==5)p.routines[m.routine].owner=m.owner;if(mutation==6)m.access=abc::MemberAccess::explicit_interface_member;try{abc::verify(p);check(false,"Forged static method rejected");}catch(const abc::Error& e){check(e.code=="E_BYTECODE","Static method metadata checked");}}
}
}
int main(){try{reference();semantics();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" static method checks, "<<failures<<" failures\n";return failures?1:0;}

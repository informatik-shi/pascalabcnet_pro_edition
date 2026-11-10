#include "abc/core.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
unsigned checks=0,failures=0;
void check(bool ok,std::string_view label){++checks;if(!ok){++failures;if(failures<=25)std::cerr<<"FAIL "<<label<<'\n';}}
std::string run(std::string_view source,std::string_view input={}){std::istringstream in{std::string(input)};std::ostringstream out;abc::execute(abc::compile(source),in,out);return out.str();}
void result(std::string_view source,std::string_view expected){try{const auto actual=run(source);check(actual==expected,std::string(source)+" actual="+actual);}catch(const std::exception& e){check(false,std::string(source)+" "+e.what());}}
void error(std::string_view source,std::string_view code,std::string_view prefix={}){std::istringstream in;std::ostringstream out;try{abc::execute(abc::compile(source),in,out);check(false,"Expected "+std::string(code));}catch(const abc::Error& e){check(e.code==code&&out.str()==prefix&&!e.russian.empty()&&e.location.line>0,std::string(source)+" actual="+e.code+" "+out.str());}}
void reference(){const auto path=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::ifstream source_file(path/"static-properties-template.pas",std::ios::binary);const std::string source{std::istreambuf_iterator<char>(source_file),{}};check(!source.empty(),"Static-property source recipe exists");std::ifstream fixture(path/"static-properties-reference.tsv");check(bool(fixture),"Actual CLR property executions exist");std::string line;unsigned rows=0;while(std::getline(fixture,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line[0]=='#')continue;std::istringstream fields(line);std::string mode,seed,delta,units;std::getline(fields,mode,'\t');std::getline(fields,seed,'\t');std::getline(fields,delta,'\t');std::getline(fields,units);std::u16string expected;for(std::size_t p=0;p<units.size();p+=4)expected+=static_cast<char16_t>(std::stoul(units.substr(p,4),nullptr,16));try{const auto actual=run(source,mode+" "+seed+" "+delta);check(actual==abc::to_utf8(expected),"CLR static-property row "+std::to_string(rows)+" actual="+actual);}catch(const std::exception& e){check(false,e.what());}++rows;}check(rows==64,"All actual CLR property traces checked");}
void semantics(){
 result("type C=class static n:integer=7;static property P:integer read n write n;end;begin Writeln(C.P);C.P:=9;Writeln(C.P);end.","7\n9\n");
 result("type C=class static property P:integer read 7;end;begin Writeln(C.P);end.","7\n");
 result("type C=class static n:integer:=7;static property P:integer read n+1 write n:=value-1;end;begin Writeln(C.P);C.P:=10;Writeln(C.n);end.","8\n9\n");
 result("type C=class static n:integer:=7;static property P:integer read n write n;static function F:integer:=P;function G:integer:=P;end;begin Writeln(C.F);var o:=new C;Writeln(o.G);end.","7\n7\n");
 result("type C=class static n:integer:=7;static function Get(i:integer):integer:=n+i;static procedure Put(i,v:integer);begin n:=v-i;end;static property P[i:integer]:integer read Get write Put;default;end;begin Writeln(C.P[2]);C[2]:=12;Writeln(C[2],' ',C.n);end.","9\n12 10\n");
 result("type C=class static property P[i:integer]:integer read i+7;end;begin Writeln(C.P[2]);end.","9\n");
 result("type C=class static property P[i:integer]:integer read i;end;begin Writeln(C.P[7]);end.","7\n");
 result("type C=class static property F[i:integer]:Func0<integer> read ()->i;end;begin var a:=C.F[1];var b:=C.F[2];Writeln(a(),' ',b());end.","1 2\n");
 result("type C=class static property P:Func0<integer> read ()->7;end;begin var f:=C.P;Writeln(f());Writeln(C.P());end.","7\n7\n");
 result("type R=record n:integer;static property Zero:R read default(R);static function F(n:integer):integer:=n+1;end;begin Writeln(R.Zero.n,' ',R.F(6));end.","0 7\n");
 result("type R=record n:integer:=7;static property Zero:R read default(R);end;begin var value:R;Writeln(value.n,' ',R.Zero.n);end.","7 0\n");
 result("var n:integer:=7;type R=record static property P:integer read n write n;end;begin R.P+=2;Writeln(R.P);end.","9\n");
 result("type C=class static n:integer=2;static property P:integer read n;end;function F(n:integer:=C.P):integer:=n;begin Writeln(F);end.","2\n");
 result("begin var n:integer=7;var f:Func0<integer> =()->n;Writeln(f());end.","7\n");
 result("type C=class static property P:integer read 7;static function F:integer;begin var P:=9;Result:=P;end;end;begin Writeln(C.F);end.","9\n");
 error("type C=class static property P:integer read 7;end;begin Write('bad');C.P:=8;end.","E_TYPE");
 error("var n:integer;type C=class static property P:integer write n;end;begin Write('bad');Writeln(C.P);end.","E_TYPE");
 error("type C=class n:integer;static property P:integer read n;end;begin Write('bad');end.","E_TYPE");
 error("type C=class static property P:integer read 7;static property P:integer read 8;end;begin Write('bad');end.","E_DUPLICATE");
 error("type C=class static n:integer;static property P:integer read n;end;procedure F(var n:integer);begin end;begin Write('bad');F(C.P);end.","E_TYPE");
 error("type C=class static property P:integer read 1 div 0;end;begin Write('before');Writeln(C.P);end.","E_DIV_ZERO","before");
 error("type C=class static property P[i:integer]:integer read i;end;begin Write('bad');Writeln(C.P);end.","E_TYPE");
 error("type C=class static procedure Put(i,v:integer);begin end;static property P[i:integer]:integer write Put;end;begin Write('bad');C.P:=7;end.","E_TYPE");
 auto original=abc::compile("type C=class static n:integer:=7;static property P:integer read n write n;end;begin Writeln(C.P);end.");
 for(unsigned mutation=0;mutation<10;++mutation){auto p=original;auto& m=p.static_properties[0];if(mutation==0)m.owner=abc::Type::integer;if(mutation==1)m.type=abc::Type::string;if(mutation==2)m.getter=p.routines.size();if(mutation==3)m.setter=p.routines.size();if(mutation==4)p.static_properties.push_back(m);if(mutation==5)p.static_properties.clear();if(mutation==6)m.indices.push_back(abc::Type::integer);if(mutation==7){m.getter.reset();m.setter.reset();}if(mutation==8)m.access=abc::MemberAccess::explicit_interface_member;if(mutation==9)m.default_indexer=true;try{abc::verify(p);check(false,"Forged static property rejected");}catch(const abc::Error& e){check(e.code=="E_BYTECODE","Static property metadata checked");}}
}
}
int main(){try{reference();semantics();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" static property checks, "<<failures<<" failures\n";return failures?1:0;}

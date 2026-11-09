#include "abc/core.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
int checks=0,failures=0;
void check(bool ok,std::string_view label){++checks;if(!ok){++failures;if(failures<=20)std::cerr<<"FAIL "<<label<<'\n';}}
std::string run(std::string_view source,abc::ExecutionOptions options={}){std::istringstream in;std::ostringstream out;abc::execute(abc::compile(source),in,out,options);return out.str();}
void result(std::string_view source,std::string_view expected){try{const auto actual=run(source);check(actual==expected,std::string(source)+" -> "+actual);}catch(const std::exception& e){check(false,e.what());}}
template<class F>void error(F action,std::string_view code){try{action();check(false,"Expected "+std::string(code));}catch(const abc::Error& e){check(e.code==code&&!e.russian.empty()&&*e.what(),std::string(e.what())+" "+e.code);}}
std::vector<std::string> split(std::string_view text){std::vector<std::string> f;std::size_t n=0;for(;;){const auto end=text.find('\t',n);f.emplace_back(text.substr(n,end==text.npos?end:end-n));if(end==text.npos)return f;n=end+1;}}
std::string decode(std::string_view hex){if(hex=="-")return {};std::u16string value;for(std::size_t n=0;n<hex.size();n+=4)value.push_back(static_cast<char16_t>(std::stoul(std::string(hex.substr(n,4)),nullptr,16)));return abc::to_utf8(value);}
void reference(){
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";
 for(const auto variant:{"typed","inferred"}){
  std::ifstream source(fixtures/("record-initialization-"+std::string(variant)+".pas"));check(bool(source),"Record initialization fixture exists");std::ostringstream text;text<<source.rdbuf();const auto p=abc::compile(text.str());
  std::ifstream data(fixtures/"record-initialization-reference.tsv");check(bool(data),"Record initialization oracle exists");std::string row;int rows=0;
  while(std::getline(data,row)){if(!row.empty()&&row.back()=='\r')row.pop_back();if(row.empty()||row.front()=='#')continue;const auto f=split(row);if(f.size()!=6){check(false,"Malformed initialization row");continue;}
   std::istringstream in(f[0]+" "+f[1]+" "+f[2]+" "+f[3]);std::ostringstream out;std::string code="ok";
   try{abc::execute(p,in,out);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty()&&*e.what()&&e.location.line>0,"Bilingual initializer error with source position");}
   check(code==f[5]&&out.str()==decode(f[4]),std::string(variant)+" "+row+" actual "+code+" "+out.str());++rows;
  }
  check(rows==1215,"All source-derived initializer outcomes checked");
 }
}
void semantics(){
 result("var x:integer:=99;type R=record x:integer:=1;end;O=record r:R;end;begin var o:O;Writeln(o.r.x,' ',x);end.","1 99\n");
 result("type R=record x:=1/2;s:='a'+'b';a:=new integer[2,3];end;begin var r:R;Writeln(r.x,' ',r.s,' ',r.a.Length);end.","0.5 ab 6\n");
 result("type R=record w:word:=2;x:=w+w;y:=-w;end;C=class public x:=1/2;end;begin var r:R;var c:=new C;Writeln(r.x,' ',r.y,' ',c.x);end.","4 -2 0.5\n");
 result("var calls:integer;type R=record x:integer:=1;procedure Bump;begin Inc(x);end;end;begin var r:R;var copy:=r;r.Bump;Writeln(r.x,' ',copy.x);end.","2 1\n");
 result("type R=record x:integer:=3;end;begin var a:=new R[1,1];SetLength(a,2,2);Writeln(a[0,0].x,' ',a[0,1].x,' ',a[1,0].x,' ',a[1,1].x);end.","3 3 3 3\n");
 for(const auto source:{"type R=record x:=nil;end;begin Write('bad');end.","type R=record x:=y;y:=x;end;begin Write('bad');end.","type R=record x:integer:='bad';end;begin Write('bad');end.","type R=record x:=Unknown;end;begin Write('bad');end."}){
  std::istringstream in;std::ostringstream out;try{abc::execute(abc::compile(source),in,out);check(false,"Invalid initializer rejected");}catch(const abc::Error& e){check(out.str().empty()&&!e.russian.empty(),"Initializer types checked before execution");}
 }
 abc::ExecutionOptions limited;limited.call_limit=8;
 error([&]{run("type R=record x:integer:=Again;end;function Again:integer;begin var r:R;Result:=r.x;end;begin var r:R;end.",limited);},"E_LIMIT");
 limited={};limited.cancelled=[] {return true;};error([&]{run("type R=record x:integer:=1;end;begin var a:=new R[100];end.",limited);},"E_CANCELLED");
 auto p=abc::compile("type R=record x:integer:=1;end;begin var a:=new R[2];var b:=default(R);end.");
 auto bad=p;for(auto& i:bad.code)if(i.op==abc::Op::array_new)i.operand=0;error([&]{abc::verify(bad);},"E_BYTECODE");
 bad=p;for(auto& i:bad.code)if(i.op==abc::Op::make_default&&i.operand==2)i.operand=6;error([&]{abc::verify(bad);},"E_BYTECODE");
 bad=p;for(auto& r:bad.routines)if(r.name.find("$initialize_fields")!=std::string::npos)r.parameters[0].by_reference=false;error([&]{abc::verify(bad);},"E_BYTECODE");
 error([&]{abc::compile("begin var r:record x:integer:=1;end;end.");},"E_UNSUPPORTED");
 error([&]{abc::compile("type R=record x:integer:=1;end;O=record nested:record r:R;end;end;begin var o:O;end.");},"E_UNSUPPORTED");
 error([&]{abc::compile("var a:=b;var b:=a;type R=record x:=a;end;begin Write('bad');end.");},"E_TYPE");
 std::string chain;for(int n=0;n<280;++n)chain+="var g"+std::to_string(n)+":=g"+std::to_string(n+1)+";";chain+="var g280:=1;type R=record x:=g0;end;begin Write('bad');end.";
 error([&]{abc::compile(chain);},"E_LIMIT");
 p=abc::compile("type R=record x:integer:=1;constructor();begin x:=2;end;end;begin var r:R;end.");
 for(auto& i:p.code)if(i.op==abc::Op::construct_user){i.op=abc::Op::make_default;i.operand=1;}
 error([&]{abc::verify(p);},"E_BYTECODE");
}
}
int main(){try{reference();semantics();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" record initialization checks, "<<failures<<" failures\n";return failures?1:0;}

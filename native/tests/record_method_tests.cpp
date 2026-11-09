#include "abc/core.hpp"
#include <array>
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
 for(const auto stem:{"record-method","record-collection"}){
 std::ifstream source(fixtures/(std::string(stem)+"-program.pas"));check(bool(source),"Typed record method fixture exists");std::ostringstream text;text<<source.rdbuf();const auto p=abc::compile(text.str());
 std::ifstream data(fixtures/(std::string(stem)+"-reference.tsv"));check(bool(data),"Independent record method corpus exists");std::string row;int rows=0;
 while(std::getline(data,row)){if(!row.empty()&&row.back()=='\r')row.pop_back();if(row.empty()||row.front()=='#')continue;const auto f=split(row);if(f.size()!=6){check(false,"Malformed record method row");continue;}
  std::istringstream in(f[0]+" "+f[1]+" "+f[2]+" "+f[3]);std::ostringstream out;std::string code="ok";try{abc::execute(p,in,out);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty()&&*e.what(),"Bilingual record method error");}
  check(code==f[5]&&out.str()==decode(f[4]),row+" actual "+code+" "+out.str());++rows;
 }check(rows==(std::string_view(stem)=="record-method"?1320:252),"All independent record method outcomes checked");
 }
}
void semantics(){
 result("type R=record x:integer;constructor(n:integer);begin x:=n;end;procedure Add(n:integer);begin x+=n;end;end;begin var r:=new R;Writeln(r.x);r.Add(3);Writeln(r.x);end.","0\n3\n");
 result("type R=record x:integer;function F(n:integer):integer;end;function R.F(n:integer):integer;begin x+=n;Result:=x;end;begin var r:R;Writeln(r.F(4),' ',r.x);end.","4 4\n");
 result("type R=record x:integer;function F:integer:=x+1;function F(n:integer):integer:=x+n;end;begin var r:R;Writeln(r.F,' ',r.F(5));end.","1 5\n");
 result("type R=record x:integer;constructor(n:integer);begin x:=n;end;procedure Replace(var n:integer);begin Self:=new R(10);n+=1;end;end;begin var r:=new R(2);r.Replace(r.x);Writeln(r.x);end.","11\n");
 result("begin var a:=Arr(1,2,3,4);System.Array.Reverse(a,1,2);Writeln(a);System.Array.Reverse(a);Writeln(a);end.","[1,3,2,4]\n[4,2,3,1]\n");
 result("type R=record x:integer;procedure Add(n:integer:=2);begin x+=n;end;procedure Many(params a:array of integer);begin foreach var n in a do x+=n;end;end;begin var r:R;r.Add;r.Many(1,3);Writeln(r.x);end.","6\n");
 for(const auto source:{"type R=record x:integer;procedure F(var n:integer);begin Inc(n);end;end;begin Write('bad');var r:R;r.F(1);end.","type R=record procedure F;begin Missing:=1;end;end;begin Write('bad');end.","type I=interface function F:integer;end;R=record(I)private function F:integer:=1;end;begin Write('bad');end.","type R=record x:integer;function F:integer->integer:=n->n+x;end;begin Write('bad');end."}){
  std::istringstream in;std::ostringstream out;try{abc::execute(abc::compile(source),in,out);check(false,"Invalid record method source rejected");}catch(const abc::Error& e){check(out.str().empty()&&!e.russian.empty(),"Static record method error before output");}
 }
 abc::ExecutionOptions limited;limited.call_limit=8;error([&]{run("type R=record procedure Again;begin Self.Again;end;end;begin var r:R;r.Again;end.",limited);},"E_LIMIT");
 auto p=abc::compile("type I=interface procedure F;end;R=record(I)public procedure F;begin end;end;begin var i:I:=new R;i.F;end.");
 auto bad=p;for(auto& routine:bad.routines)if(routine.owner!=abc::Type::invalid)routine.parameters[0].by_reference=false;error([&]{abc::verify(bad);},"E_BYTECODE");
 bad=p;for(auto& type:bad.types)if(type.kind==abc::CompoundKind::record)type.native_record=abc::NativeRecord::ordinal_range;error([&]{abc::verify(bad);},"E_BYTECODE");
}
void retained_arrays(){
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::array<abc::Program,4> programs;
 for(int owner=0;owner<2;++owner)for(int kind=0;kind<2;++kind){std::ifstream source(fixtures/("record-list-alias-"+std::to_string(owner)+"-"+std::to_string(kind)+".pas"));check(bool(source),"Closed retained array fixture exists");std::ostringstream text;text<<source.rdbuf();programs[owner*2+kind]=abc::compile(text.str());}
 std::ifstream data(fixtures/"record-list-alias-reference.tsv");check(bool(data),"Independent retained array corpus exists");std::string row;int rows=0;
 while(std::getline(data,row)){if(!row.empty()&&row.back()=='\r')row.pop_back();if(row.empty()||row.front()=='#')continue;const auto f=split(row);if(f.size()!=8){check(false,"Malformed retained array row");continue;}
  std::istringstream in(f[2]+" "+f[3]+" "+f[4]+" "+f[5]);std::ostringstream out;std::string code="ok";try{abc::execute(programs.at(std::stoi(f[0])*2+std::stoi(f[1])),in,out);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty()&&*e.what(),"Bilingual retained array error");}
  check(code==f[7]&&out.str()==decode(f[6]),row+" actual "+code+" "+out.str());++rows;
 }check(rows==1560,"All independent retained array outcomes checked");
}
}
int main(){try{reference();semantics();retained_arrays();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" record method checks, "<<failures<<" failures\n";return failures?1:0;}

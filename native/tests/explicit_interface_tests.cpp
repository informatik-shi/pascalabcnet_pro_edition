#include "abc/core.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
int checks=0,failures=0;
void check(bool ok,std::string_view message){++checks;if(!ok){++failures;if(failures<=24)std::cerr<<"FAIL "<<message<<'\n';}}
std::string run(std::string_view source){std::istringstream in;std::ostringstream out;abc::execute(abc::compile(source),in,out);return out.str();}
void result(std::string_view source,std::string_view expected){try{check(run(source)==expected,source);}catch(const std::exception& e){check(false,e.what());}}
template<class F>void error(F action,std::string_view code){try{action();check(false,"Expected "+std::string(code));}catch(const abc::Error& e){check(e.code==code&&!e.russian.empty(),std::string(e.what())+" "+e.code);}}
std::vector<std::string> split(std::string_view text){std::vector<std::string> f;std::size_t n=0;for(;;){const auto end=text.find('\t',n);f.emplace_back(text.substr(n,end==text.npos?end:end-n));if(end==text.npos)return f;n=end+1;}}
std::string decode(std::string_view hex){if(hex=="-")return {};std::u16string value;for(std::size_t n=0;n<hex.size();n+=4)value.push_back(static_cast<char16_t>(std::stoul(std::string(hex.substr(n,4)),nullptr,16)));return abc::to_utf8(value);}
void reference(){
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";
 std::ifstream source(fixtures/"explicit-interface-program.pas");check(bool(source),"Typed explicit implementation fixture exists");std::ostringstream text;text<<source.rdbuf();const auto program=abc::compile(text.str());
 std::ifstream data(fixtures/"explicit-interface-reference.tsv");check(bool(data),"Independent explicit implementation corpus exists");std::string row;int rows=0;
 while(std::getline(data,row)){
  if(!row.empty()&&row.back()=='\r')row.pop_back();if(row.empty()||row.front()=='#')continue;const auto f=split(row);if(f.size()!=5){check(false,"Malformed explicit implementation row");continue;}
  std::istringstream in(f[0]+" "+f[1]+" "+f[2]);std::ostringstream out;std::string code="ok";
  try{abc::execute(program,in,out);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty()&&*e.what(),"Bilingual explicit implementation error");}
  check(code==f[4]&&out.str()==decode(f[3]),row+" actual "+code+" "+out.str());++rows;
 }check(rows==24,"All independent explicit implementation outcomes checked");
}
void semantics(){
 result("type C=class(System.Collections.Generic.IEnumerable<integer>)private function System.Collections.Generic.IEnumerable<integer>.GetEnumerator:IEnumerator<integer>:=Seq(4,5).GetEnumerator;end;begin var c:=new C;var e:IEnumerable<integer>:=c;Writeln(e.ToArray);end.","[4,5]\n");
 result("type I=interface function F:integer;end;C=class(I)public F:integer:=7;function I.F:integer:=F+1;end;begin var c:=new C;var i:I:=c;Writeln(c.F,' ',i.F);end.","7 8\n");
 result("type I=interface function F:integer;end;C=class(I)private function I.F:integer;public function F:integer;end;function C.F:integer:=1;function C.I.F:integer:=2;begin var c:=new C;var i:I:=c;Writeln(c.F,' ',i.F);end.","1 2\n");
 for(const auto source:{
  "type I=interface function F:integer;end;C=class(I)function I.F:integer:=1;end;begin Write('bad');var c:=new C;Writeln(c.F);end.",
  "type I=interface function F:integer;end;C=class function I.F:integer:=1;end;begin Write('bad');end.",
  "type I=interface function F:integer;end;C=class(I)function I.F:string:='x';end;begin Write('bad');end.",
  "type I=interface function F:integer;end;C=class(I)function I.G:integer:=1;end;begin Write('bad');end.",
  "type I=interface function F:integer;end;Alias=I;C=class(I)function I.F:integer:=1;function Alias.F:integer:=2;end;begin Write('bad');end.",
  "type N=integer;C=class function N.F:integer:=1;end;begin Write('bad');end.",
  "type I=interface function F:integer;end;J=interface function I.F:integer;end;begin Write('bad');end."
 }){std::istringstream in;std::ostringstream out;try{abc::execute(abc::compile(source),in,out);check(false,"Invalid explicit implementation rejected");}catch(const abc::Error& e){check(out.str().empty()&&!e.russian.empty(),"Static explicit implementation error before output");}}
 auto p=abc::compile("type I=interface function F:integer;end;C=class(I)function I.F:integer:=7;end;begin var i:I:=new C;Writeln(i.F);end.");
 for(std::size_t n=0;n<p.types.size();++n)if(p.types[n].kind==abc::CompoundKind::object_class){auto bad=p;bad.types[n].interfaces.edit().implementations.clear();error([&]{abc::verify(bad);},"E_BYTECODE");}
 for(std::size_t n=0;n<p.routines.size();++n)if(p.routines[n].access==abc::MemberAccess::explicit_interface_member){auto bad=p;bad.routines[n].owner=abc::Type::invalid;error([&]{abc::verify(bad);},"E_BYTECODE");}
}
}
int main(){try{reference();semantics();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" explicit interface checks, "<<failures<<" failures\n";return failures?1:0;}

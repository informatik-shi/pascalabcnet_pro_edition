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
 std::ifstream source(fixtures/"set-interface-program.pas");check(bool(source),"Typed ISet fixture exists");std::ostringstream text;text<<source.rdbuf();const auto direct=text.str();auto inherited=direct;
 const auto declaration=inherited.find("TraceSet=class(ISet<integer>)");check(declaration!=std::string::npos,"Named inherited ISet fixture anchor exists");
 inherited.replace(declaration,std::string("TraceSet=class(ISet<integer>)").size(),"TaggedSet=interface(ISet<integer>)end;TraceSet=class(TaggedSet)");
 for(const auto& code:{direct,inherited}){
  const auto program=abc::compile(code);std::ifstream data(fixtures/"set-interface-reference.tsv");check(bool(data),"Independent ISet corpus exists");std::string row;int rows=0;
  while(std::getline(data,row)){
   if(!row.empty()&&row.back()=='\r')row.pop_back();if(row.empty()||row.front()=='#')continue;const auto f=split(row);if(f.size()!=10){check(false,"Malformed ISet row");continue;}
   std::string input;for(int n=0;n<8;++n)input+=f[n]+" ";std::istringstream in(input);std::ostringstream out;std::string error="ok";
   try{abc::execute(program,in,out);}catch(const abc::Error& e){error=e.code;check(!e.russian.empty()&&*e.what(),"Bilingual ISet error");}
   check(error==f[9]&&out.str()==decode(f[8]),input+"expected "+f[9]+" "+decode(f[8])+" got "+error+" "+out.str());++rows;
  }check(rows==3168,"All independent ISet outcomes checked");
 }
}
void semantics(){
 result("begin var s:ISet<integer>:=new HashSet<integer>;Writeln(s.Add(1),' ',s.Add(1));var c:ICollection<integer>:=s;c.Add(2);Writeln(c.Count);end.","True False\n2\n");
 result("type S=System.Collections.Generic.ISet<integer>;begin var s:S:=new HashSet<integer>;s.Add(3);var e:IEnumerable<integer>:=s;var same:=ISet&<integer>(e);Writeln(same=s,' ',same.Contains(3),' ',TypeToTypeName(typeof(S)));end.","True True ISet<integer>\n");
 result("begin var s:ISet<integer>:=new HashSet<integer>(Arr(1,2));s.UnionWith(s);Writeln(s.Count);s.IntersectWith(s);Writeln(s.Count);s.SymmetricExceptWith(s);Writeln(s.Count);end.","2\n2\n0\n");
 error([]{run("begin var c:ICollection<integer>:=new List<integer>;var s:=ISet&<integer>(c);end.");},"E_CAST");
 error([]{run("begin var s:ISet<integer>:=new HashSet<integer>;s.Add('wrong');end.");},"E_TYPE");
 auto p=abc::compile("begin var s:ISet<integer>:=new HashSet<integer>;s.Add(1);end.");
 for(std::size_t n=0;n<p.types.size();++n)if(p.types[n].kind==abc::CompoundKind::set_interface){auto bad=p;bad.types[n].interfaces.edit().parents.clear();error([&]{abc::verify(bad);},"E_BYTECODE");bad=p;bad.types[n].key=abc::Type::integer;error([&]{abc::verify(bad);},"E_BYTECODE");}
}
}
int main(){try{reference();semantics();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" set interface checks, "<<failures<<" failures\n";return failures?1:0;}

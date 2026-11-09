#include "abc/core.hpp"
#include "../src/interfaces.hpp"
#include "../src/record_interface.hpp"
#include "../src/runtime_values.hpp"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
int checks=0,failures=0;
void check(bool ok,std::string_view message){++checks;if(!ok){++failures;if(failures<=24)std::cerr<<"FAIL "<<message<<'\n';}}
std::string run(std::string_view source,abc::ExecutionOptions options={}){std::istringstream in;std::ostringstream out;abc::execute(abc::compile(source),in,out,options);return out.str();}
void result(std::string_view source,std::string_view expected){try{const auto actual=run(source);check(actual==expected,std::string(source)+" -> "+actual);}catch(const std::exception& e){check(false,std::string(source)+" -> "+e.what());}}
template<class F>void error(F action,std::string_view code){try{action();check(false,"Expected "+std::string(code));}catch(const abc::Error& e){check(e.code==code&&!e.russian.empty()&&*e.what(),std::string(e.what())+" "+e.code);}}
std::vector<std::string> split(std::string_view text){std::vector<std::string> f;std::size_t n=0;for(;;){const auto end=text.find('\t',n);f.emplace_back(text.substr(n,end==text.npos?end:end-n));if(end==text.npos)return f;n=end+1;}}
std::string decode(std::string_view hex){if(hex=="-")return {};std::u16string value;for(std::size_t n=0;n<hex.size();n+=4)value.push_back(static_cast<char16_t>(std::stoul(std::string(hex.substr(n,4)),nullptr,16)));return abc::to_utf8(value);}
void reference(){
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";
 for(int kind=0;kind<2;++kind){
  std::ifstream source(fixtures/("range-interface-"+std::to_string(kind)+".pas"));check(bool(source),"Typed range interface fixture exists");std::ostringstream text;text<<source.rdbuf();const auto program=abc::compile(text.str());
  std::ifstream data(fixtures/"range-interface-reference.tsv");check(bool(data),"Independent range interface corpus exists");std::string row;int rows=0;
  while(std::getline(data,row)){
   if(!row.empty()&&row.back()=='\r')row.pop_back();if(row.empty()||row.front()=='#')continue;const auto f=split(row);if(f.size()!=11){check(false,"Malformed range interface row");continue;}if(std::stoi(f[0])!=kind)continue;
   std::string input;for(int n=1;n<9;++n)input+=f[n]+" ";std::istringstream in(input);std::ostringstream out;std::string code="ok";
   try{abc::execute(program,in,out);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty()&&*e.what(),"Bilingual range interface error");}
   check(code==f[10]&&out.str()==decode(f[9]),row+" actual "+code+" "+out.str());++rows;
  }check(rows==1344,"All independent range interface outcomes checked");
 }
}
void semantics(){
 result("begin var r:=new IntRange(1,3);var a:ICollection<integer>:=r;var b:=a;var e:IEnumerable<integer>:=a;var copy:=IntRange(e);r:=new IntRange(4,5);Writeln(a=b,' ',a=e,' ',copy.Low,' ',copy.High);Writeln(e.ToArray,' ',a.Count);end.","True True 1 3\n[1,2,3] 3\n");
 result("begin var r:=new IntRange(1,3);var a:IEnumerable<integer>:=r;var b:IEnumerable<integer>:=r;Writeln(a=b,' ',TypeToTypeName(a.GetType));var c:=ICollection&<integer>(a);Writeln(a=c,' ',c.Count);end.","False IntRange\nTrue 3\n");
 result("type C=ICollection<integer>;begin var a:C:=new IntRange(1,3);var e:IEnumerable<integer>:=a;var comp:=EqualityComparer<IEnumerable<integer>>.Default;Writeln(comp.Equals(e,a),' ',comp.GetHashCode(e)=comp.GetHashCode(a));end.","True True\n");
 result("begin var r:=new CharRange('a','c');var e:IEquatable<CharRange>:=r;Writeln(e.Equals(r),' ',e.Equals(new CharRange('a','b')));var c:=ICollection&<char>(e);Writeln(c.Count);end.","True False\n3\n");
 error([]{run("begin var a:ICollection<integer>:=new List<integer>;Write('before');var r:=IntRange(a);end.");},"E_CAST");
 error([]{run("begin var a:ICollection<integer>:=nil;var r:=IntRange(a);end.");},"E_NIL");
 for(const auto source:{"begin Write('bad');var c:ICollection<char>:=new IntRange(1,3);end.","type I=interface property P:integer read;end;begin Write('bad');var c:I:=new IntRange(1,3);end.","begin Write('bad');var a:IList<integer>:=new IntRange(1,3);end.","begin Write('bad');var a:ICollection<char>:=new CharRange('a','c');var r:=IntRange(a);end."}){
  std::istringstream in;std::ostringstream out;try{abc::execute(abc::compile(source),in,out);check(false,"Invalid record interface conversion rejected");}catch(const abc::Error& e){check(out.str().empty()&&!e.russian.empty(),"Static range interface error before output");}
 }
 abc::ExecutionOptions cancelled;cancelled.cancelled=[] {return true;};error([&]{run("begin var c:ICollection<integer>:=new IntRange(1,3);Writeln(c.Count);end.",cancelled);},"E_CANCELLED");
 auto program=abc::compile("begin var c:ICollection<integer>:=new IntRange(1,3);Writeln(c.Count);end.");
 abc::Type range=abc::Type::invalid,collection=abc::Type::invalid;for(std::size_t n=0;n<program.types.size();++n){if(program.types[n].native_record==abc::NativeRecord::ordinal_range)range=static_cast<abc::Type>(64+n);if(program.types[n].kind==abc::CompoundKind::collection_interface)collection=static_cast<abc::Type>(64+n);}
 abc::Values values{program,10000};auto value=std::get<std::shared_ptr<abc::Aggregate>>(values.zero(range));auto box=abc::record_interface_copy(values,value,true);auto copy=abc::record_interface_copy(values,box,false);
 check(box!=value&&copy!=box&&box->boxed&&!copy->boxed&&std::get<std::shared_ptr<abc::Aggregate>>(values.copy(box))==box,"Box copies value once; unbox copies again; interface copies retain identity");
 error([&]{abc::interface_cast(values,collection,range,false,box);},"E_BYTECODE");
 error([&]{abc::native_record_interface_invoke(values,collection,99,box,{},{});},"E_BYTECODE");
 error([&]{const std::array<abc::Value,1> extra{std::int64_t{1}};abc::native_record_interface_invoke(values,collection,0,box,extra,{});},"E_BYTECODE");
 auto bad=program;bad.types[static_cast<unsigned>(range)-64U].element=abc::Type::string;error([&]{abc::verify(bad);},"E_BYTECODE");
}
}
int main(){try{reference();semantics();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" range interface checks, "<<failures<<" failures\n";return failures?1:0;}

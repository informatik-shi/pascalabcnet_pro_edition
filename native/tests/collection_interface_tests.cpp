#include "abc/core.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
int checks=0,failures=0;
void check(bool ok,std::string_view message){++checks;if(!ok){++failures;if(failures<=22)std::cerr<<"FAIL "<<message<<'\n';}}
std::string run(std::string_view source,abc::ExecutionOptions options={}){std::istringstream input;std::ostringstream output;abc::execute(abc::compile(source),input,output,options);return output.str();}
void result(std::string_view source,std::string_view expected){try{check(run(source)==expected,source);}catch(const std::exception& e){check(false,std::string(source)+": "+e.what());}}
template<class F>void error(F action,std::string_view code){try{action();check(false,"Expected "+std::string(code));}catch(const abc::Error& e){check(e.code==code&&!e.russian.empty()&&*e.what(),std::string(code)+": "+e.code+" "+e.what());}}
std::vector<std::string> split(std::string_view text){std::vector<std::string> fields;std::size_t at=0;for(;;){auto end=text.find('\t',at);fields.emplace_back(text.substr(at,end==text.npos?end:end-at));if(end==text.npos)return fields;at=end+1;}}
std::string decode(std::string_view hex){if(hex=="-")return {};std::u16string text;for(std::size_t n=0;n<hex.size();n+=4)text.push_back(static_cast<char16_t>(std::stoul(std::string(hex.substr(n,4)),nullptr,16)));return abc::to_utf8(text);}
void reference(){
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::ifstream source(fixtures/"collection-interface-program.pas");check(bool(source),"Static collection interface program exists");std::ostringstream text;text<<source.rdbuf();const auto program=abc::compile(text.str());
 std::ifstream input(fixtures/"collection-interface-reference.tsv");check(bool(input),"Independent ICollection corpus exists");std::string row;int rows=0;
 while(std::getline(input,row)){
  if(!row.empty()&&row.back()=='\r')row.pop_back();if(row.empty()||row.front()=='#')continue;const auto f=split(row);if(f.size()!=8){check(false,"Malformed collection interface row");continue;}
  std::istringstream in(f[0]+" "+f[1]+" "+f[2]+" "+f[3]+" "+f[4]+" "+f[5]);std::ostringstream out;std::string code="ok";
  try{abc::execute(program,in,out);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty()&&*e.what(),"Collection interface error is bilingual");}
  check(code==f[7]&&out.str()==decode(f[6]),"kind "+f[0]+" operation "+f[1]+" size "+f[2]+" nil "+f[3]+" index "+f[4]+" expected "+f[7]+" got "+code+" output "+out.str());++rows;
 }
 check(rows==768,"All independent ICollection outcomes checked");
}
void semantics(){
 result("begin var c:=ICollection&<integer>(Arr(1,2));var e:IEnumerable<integer>:=c;var same:=IEnumerable&<integer>(e);Writeln(same.Count,' ',c.IsReadOnly);end.","2 True\n");
 result("begin var e:=Range(1,3).Select(x->x*2);var same:=IEnumerable&<integer>(e);Writeln(same.ToArray);end.","[2,4,6]\n");
 result("begin var l:=new List<integer>;var c:ICollection<integer>:=l;c.Add(7);c.Add(8);var e:=c.GetEnumerator;c.Clear;Writeln(c.Count);end.","0\n");
 result("type P=KeyValuePair<integer,integer>;begin var d:=new Dictionary<integer,integer>;var c:ICollection<P>:=d;c.Add(KV(1,10));c.Add(KV(2,20));Writeln(c.Count,' ',c.IsReadOnly,' ',c.Contains(KV(2,20)));var a:=new P[3];c.CopyTo(a,1);Writeln(a[2].Key,' ',a[2].Value);Writeln(c.Remove(KV(2,99)),' ',c.Remove(KV(2,20)),' ',c.Count);c.Clear;Writeln(c.Count);end.","2 False True\n2 20\nFalse True 1\n0\n");
 result("type C=ICollection<integer>;begin var c:C:=Arr(4);var e:IEnumerable<integer>:=c;var b:=C(e);Writeln(b=c,' ',b.Count);var d:=ICollection&<integer>(e);Writeln(d=c);end.","True 1\nTrue\n");
 for(const auto source:{"begin Writeln('side effect');var c:ICollection<integer>:=Arr('x');end.","begin Writeln('side effect');var c:=new ICollection<integer>;end.","begin var c:ICollection<integer>:=Arr(1);c.Add('x');end.","begin var c:ICollection<integer>:=Arr(1);c.Count:=2;end."}){
  std::ostringstream out;try{std::istringstream in;abc::execute(abc::compile(source),in,out);check(false,"Invalid interface program rejected");}catch(const abc::Error& e){check(out.str().empty()&&!e.russian.empty(),"Static interface rejection precedes output");}
 }
 error([]{run("begin var c:ICollection<integer>:=Arr(1);c.Add(2);end.");},"E_READONLY");
 error([]{run("begin var d:=new Dictionary<integer,integer>;var c:ICollection<integer>:=d.Keys;c.Clear;end.");},"E_READONLY");
 error([]{run("begin var e:=Range(1,3).Select(x->x);var c:=ICollection&<integer>(e);end.");},"E_CAST");
 result("type R=record n:integer;end;begin var r:R;r.n:=7;var l:=new List<R>;var c:ICollection<R>:=l;c.Add(r);r.n:=9;var a:=new R[1];c.CopyTo(a,0);a[0].n:=8;Writeln(l[0].n,' ',a[0].n,' ',r.n);end.","7 8 9\n");
 error([]{run("begin var l:=new List<integer>;var c:ICollection<integer>:=l;var e:=c.GetEnumerator;c.Clear;e.MoveNext;end.");},"E_MODIFIED");
 auto p=abc::compile("begin var c:ICollection<integer>:=Arr(1);Writeln(c.Count);end.");
 for(std::size_t n=0;n<p.types.size();++n)if(p.types[n].kind==abc::CompoundKind::collection_interface){
  auto bad=p;bad.types[n].key=abc::Type::integer;error([&]{abc::verify(bad);},"E_BYTECODE");
  bad=p;bad.types[n].interfaces.edit().parents.clear();error([&]{abc::verify(bad);},"E_BYTECODE");
  bad=p;bad.types[n].element=abc::Type::string;error([&]{abc::verify(bad);},"E_BYTECODE");
 }
}
}
int main(){try{reference();semantics();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" collection interface checks, "<<failures<<" failures\n";return failures?1:0;}

#include "abc/core.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
int checks=0,failures=0;
void check(bool ok,std::string_view message){++checks;if(!ok){++failures;if(failures<=24)std::cerr<<"FAIL "<<message<<'\n';}}
std::string run(std::string_view source,abc::ExecutionOptions options={}){std::istringstream input;std::ostringstream output;abc::execute(abc::compile(source),input,output,options);return output.str();}
void result(std::string_view source,std::string_view expected){try{check(run(source)==expected,source);}catch(const std::exception& e){check(false,std::string(source)+": "+e.what());}}
template<class F>void error(F action,std::string_view code){try{action();check(false,"Expected "+std::string(code));}catch(const abc::Error& e){check(e.code==code&&!e.russian.empty()&&*e.what(),std::string(code)+": "+e.code+" "+e.what());}}
std::vector<std::string> split(std::string_view text){std::vector<std::string> fields;std::size_t at=0;for(;;){auto end=text.find('\t',at);fields.emplace_back(text.substr(at,end==text.npos?end:end-at));if(end==text.npos)return fields;at=end+1;}}
std::string decode(std::string_view hex){if(hex=="-")return {};std::u16string text;for(std::size_t n=0;n<hex.size();n+=4)text.push_back(static_cast<char16_t>(std::stoul(std::string(hex.substr(n,4)),nullptr,16)));return abc::to_utf8(text);}
void reference(){
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::ifstream source(fixtures/"dictionary-interface-program.pas");check(bool(source),"Static IDictionary program exists");std::ostringstream text;text<<source.rdbuf();const auto program=abc::compile(text.str());std::ifstream input(fixtures/"dictionary-interface-reference.tsv");check(bool(input),"Independent IDictionary corpus exists");std::string row;int rows=0;
 while(std::getline(input,row)){
  if(!row.empty()&&row.back()=='\r')row.pop_back();if(row.empty()||row.front()=='#')continue;const auto f=split(row);if(f.size()!=9){check(false,"Malformed IDictionary row");continue;}std::string data;for(int i=0;i<7;++i)data+=f[i]+" ";std::istringstream in(data);std::ostringstream out;std::string code="ok";
  try{abc::execute(program,in,out);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty()&&*e.what(),"IDictionary error is bilingual");}
  check(code==f[8]&&out.str()==decode(f[7]),data+"expected "+f[8]+" got "+code+" output "+out.str());++rows;
 }check(rows==1200,"All independent IDictionary outcomes checked");
}
void overloads(){
 result("type I=interface function F(x:integer):integer;overload;function F(x:string):string;overload;end;C=class(I)public function F(x:integer):integer;overload;begin Result:=x+1;end;function F(x:string):string;overload;begin Result:=x+'!';end;property Number[i:integer]:integer read F;property Word[i:string]:string read F;end;begin var c:=new C;Writeln(c.F(2),' ',c.F('abc'),' ',c.Number[4],' ',c.Word['def']);var i:I:=c;Writeln(i.F(6),' ',i.F('ghi'));end.","3 abc! 5 def!\n7 ghi!\n");
 result("type A=interface function F(x:integer):integer;end;B=interface function F(x:string):string;end;I=interface(A,B)end;C=class(I)public function F(x:integer):integer:=x+1;function F(x:string):string:=x+'!';end;begin var i:I:=new C;Writeln(i.F(2),' ',i.F('abc'));end.","3 abc!\n");
 result("type C=class public function F(x:integer):integer;overload;function F(x:string):string;overload;end;function C.F(x:string):string;begin Result:=x+'!';end;function C.F(x:integer):integer;begin Result:=x+1;end;begin var c:=new C;Writeln(c.F(2),' ',c.F('abc'));end.","3 abc!\n");
 result("type C=class function F(x:int64):integer:=1;function F(x:real):integer:=2;function G(x:integer):integer:=3;function G(x:integer;y:integer:=4):integer:=y;function H(params x:array of integer):integer:=x.Length;function H(x:string):integer:=9;end;begin var c:=new C;Writeln(c.F(1),' ',c.F(1.5),' ',c.G(2),' ',c.G(2,7),' ',c.H(),' ',c.H(1,2),' ',c.H('abc'));end.","1 2 3 7 0 2 9\n");
 result("type C=class procedure F(var x:integer);begin Inc(x);end;procedure F(var x:string);begin x+='!';end;function G(f:integer->integer):integer:=f(3);function G(x:string):integer:=8;end;begin var c:=new C;var i:=2;var s:='abc';c.F(i);c.F(s);Writeln(i,' ',s,' ',c.G(x->x+1));end.","3 abc! 4\n");
 for(const auto source:{"type C=class function F(x:integer):integer:=1;function F(x:integer):string:='x';end;begin Write('bad');end.","type C=class function F(x:string):integer:=1;function F(x:array of integer):integer:=2;end;begin Write('bad');var c:=new C;Writeln(c.F(nil));end.","type C=class function F(x:integer):integer:=1;function F(x:string):integer:=2;end;begin Write('bad');var c:=new C;Writeln(c.F(True));end.","type I=interface procedure F(x:integer);procedure F(x:integer);end;begin Write('bad');end."}){
  std::ostringstream out;try{std::istringstream in;abc::execute(abc::compile(source),in,out);check(false,"Invalid overload program rejected");}catch(const abc::Error& e){check(out.str().empty()&&!e.russian.empty(),"Overload errors precede output");}
 }
}
void semantics(){
 result("type R=record n:integer;end;begin var r:R;r.n:=7;var d:IDictionary<string,R>:=new Dictionary<string,R>;d.Add('a',r);r.n:=9;var v:R;Writeln(d.TryGetValue('a',v),' ',v.n);v.n:=8;Writeln(d['a'].n,' ',r.n);end.","True 7\n7 9\n");
 result("type D=System.Collections.Generic.IDictionary<string,integer>;P=KeyValuePair<string,integer>;begin var original:=new Dictionary<string,integer>;var d:D:=original;d.Add('a',1);var c:ICollection<P>:=d;var e:IEnumerable<P>:=c;var same:=IDictionary&<string,integer>(e);Writeln(same=d,' ',same['a'],' ',TypeToTypeName(typeof(D)));end.","True 1 IDictionary<string, integer>\n");
 error([]{run("begin var d:IDictionary<string,integer>:=new Dictionary<string,integer>;d.TryGetValue('a',1);end.");},"E_TYPE");
 error([]{run("type D=IDictionary<string,integer>;P=KeyValuePair<string,integer>;begin var c:ICollection<P>:=new List<P>;D(c).Clear;end.");},"E_CAST");
 auto p=abc::compile("begin var d:IDictionary<string,integer>:=new Dictionary<string,integer>;d.Add('a',1);end.");for(std::size_t n=0;n<p.types.size();++n)if(p.types[n].kind==abc::CompoundKind::dictionary_interface){auto bad=p;bad.types[n].interfaces.edit().parents.clear();error([&]{abc::verify(bad);},"E_BYTECODE");bad=p;bad.types[n].key=abc::Type::integer;error([&]{abc::verify(bad);},"E_BYTECODE");}
 p=abc::compile("type I=interface procedure F(x:integer);procedure F(x:string);end;begin end.");for(std::size_t n=0;n<p.types.size();++n)if(p.types[n].kind==abc::CompoundKind::user_interface){auto bad=p;bad.types[n].interfaces.edit().members[1].parameters=bad.types[n].interfaces.read().members[0].parameters;error([&]{abc::verify(bad);},"E_BYTECODE");}
}
}
int main(){try{reference();overloads();semantics();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" dictionary interface checks, "<<failures<<" failures\n";return failures?1:0;}

#include "abc/core.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
int checks=0,failures=0;
void check(bool ok,std::string_view message){++checks;if(!ok){++failures;if(failures<=24)std::cerr<<"FAIL "<<message<<'\n';}}
std::string run(std::string_view source,abc::ExecutionOptions options={}){std::istringstream in;std::ostringstream out;abc::execute(abc::compile(source),in,out,options);return out.str();}
void result(std::string_view source,std::string_view expected){try{check(run(source)==expected,source);}catch(const std::exception& e){check(false,e.what());}}
template<class F>void error(F action,std::string_view code){try{action();check(false,"Expected "+std::string(code));}catch(const abc::Error& e){check(e.code==code&&!e.russian.empty(),std::string(e.what())+" "+e.code);}}
std::vector<std::string> split(std::string_view text){std::vector<std::string> f;std::size_t n=0;for(;;){const auto end=text.find('\t',n);f.emplace_back(text.substr(n,end==text.npos?end:end-n));if(end==text.npos)return f;n=end+1;}}
std::string decode(std::string_view hex){if(hex=="-")return {};std::u16string value;for(std::size_t n=0;n<hex.size();n+=4)value.push_back(static_cast<char16_t>(std::stoul(std::string(hex.substr(n,4)),nullptr,16)));return abc::to_utf8(value);}
void reference(){
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";
 std::ifstream source(fixtures/"explicit-property-program.pas");check(bool(source),"Typed explicit property program exists");std::ostringstream text;text<<source.rdbuf();const auto program=abc::compile(text.str());
 std::ifstream data(fixtures/"explicit-property-reference.tsv");check(bool(data),"Independent explicit property corpus exists");std::string row;int rows=0;
 while(std::getline(data,row)){
  if(!row.empty()&&row.back()=='\r')row.pop_back();if(row.empty()||row.front()=='#')continue;const auto f=split(row);if(f.size()!=5){check(false,"Malformed explicit property row");continue;}
  std::istringstream in(f[0]+" "+f[1]+" "+f[2]);std::ostringstream out;std::string code="ok";
  try{abc::execute(program,in,out);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty()&&*e.what(),"Bilingual explicit property error");}
  check(code==f[4]&&out.str()==decode(f[3]),row+" actual "+code+" "+out.str());++rows;
 }check(rows==24,"All independent explicit property outcomes checked");
}
void semantics(){
 result("function F:boolean;begin Write('B');Result:=True;end;begin Writeln(boolean(F));end.","BTrue\n");
 result("type C=class property Number:integer read (Global);property Next:integer read Self.Number+1;end;var Global:integer:=8;begin var c:=new C;Writeln(c.Number,' ',c.Next);Global:=2;Writeln(c.Next);end.","8 9\n3\n");
 result("type I=interface function Enumerator:integer;end;C=class(I)function I.Enumerator:integer:=7;end;begin var i:I:=new C;Writeln(i.Enumerator);end.","7\n");
 result("type C=class x:integer;property P:integer write begin var k:=value+1;x:=k;end read x;end;begin var c:=new C;c.P:=3;Writeln(c.P);end.","4\n");
 result("type C=class x:integer;procedure Tick;begin Inc(x);end;procedure Tick(n:integer);begin x+=n;end;property A:integer read x write Tick();property B:integer read x write Tick;end;begin var c:=new C;c.A:=7;Writeln(c.A);c.B:=9;Writeln(c.B);end.","1\n10\n");
 result("type C=class x:integer:=5;property F[i:integer]:integer->integer read (n->i+n+x);end;begin var c:=new C;var f:=c.F[2];c.x:=9;var g:=c.F[7];Writeln(f(3),' ',g(3));c:=nil;Writeln(f(3));end.","14 19\n14\n");
 result("type C=class i:integer:=99;property P[i:integer]:integer read (i);end;begin var c:=new C;Writeln(c.P[3]);end.","3\n");
 result("type C=class(ICollection<integer>)private data:List<integer>:=new List<integer>(Arr(4,5));property System.Collections.Generic.ICollection<integer>.Count:integer read data.Count;property ICollection<integer>.IsReadOnly:boolean read boolean(False);public procedure Add(n:integer);begin data.Add(n);end;procedure Clear;begin data.Clear;end;function Contains(n:integer):boolean:=data.Contains(n);procedure CopyTo(a:array of integer;i:integer);begin data.CopyTo(a,i);end;function Remove(n:integer):boolean:=data.Remove(n);function GetEnumerator:IEnumerator<integer>:=data.AsEnumerable.GetEnumerator;end;begin var c:ICollection<integer>:=new C;Writeln(c.Count,' ',c.IsReadOnly);c.Add(6);var e:IEnumerable<integer>:=c;Writeln(e.ToArray);end.","2 False\n[4,5,6]\n");
 for(const auto source:{
  "type I=interface property P:integer read;end;C=class(I)property I.P:integer read 1;end;begin Write('bad');var c:=new C;Writeln(c.P);end.",
  "type I=interface property P:integer read;end;C=class property I.P:integer read 1;end;begin Write('bad');end.",
  "type I=interface property P:integer read;end;C=class(I)property I.P:string read 'bad';end;begin Write('bad');end.",
  "type I=interface property P[i:integer]:integer read;end;C=class(I)property I.P[i:string]:integer read i.Length;end;begin Write('bad');end.",
  "type I=interface property P:integer read;end;Alias=I;C=class(I)property I.P:integer read 1;property Alias.P:integer read 2;end;begin Write('bad');end.",
  "type C=class property P:integer read 'bad';end;begin Write('bad');end.",
  "type C=class property P:integer read Unknown;end;begin Write('bad');end.",
  "type C=class property P:integer read Global;end;var Global:integer:=8;begin Write('bad');end.",
  "type C=class x:integer;property P:integer write x:='wrong';end;begin Write('bad');end.",
  "type I=interface property Item[i:integer]:integer read;default;end;C=class(I)property I.Item[i:integer]:integer read 1;end;begin Write('bad');var c:=new C;Writeln(c[0]);end.",
  "begin Write('bad');Writeln(boolean(1));end."
 }){std::istringstream in;std::ostringstream out;try{abc::execute(abc::compile(source),in,out);check(false,"Invalid property program rejected");}catch(const abc::Error& e){check(out.str().empty()&&!e.russian.empty(),"Static property error before output");}}
 abc::ExecutionOptions limited;limited.instruction_limit=200;
 error([&]{run("type C=class property A:integer read (B);property B:integer read (A);end;begin var c:=new C;Writeln(c.A);end.",limited);},"E_LIMIT");
 auto p=abc::compile("type I=interface property P:integer read;end;C=class(I)property I.P:integer read 7;end;begin var i:I:=new C;Writeln(i.P);end.");
 for(std::size_t n=0;n<p.types.size();++n)if(p.types[n].kind==abc::CompoundKind::object_class){auto bad=p;const auto id=bad.types[n].interfaces.read().implementations[0].routines[0];bad.routines[id].result=abc::Type::string;error([&]{abc::verify(bad);},"E_BYTECODE");}
}
}
int main(){try{reference();semantics();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" explicit property checks, "<<failures<<" failures\n";return failures?1:0;}

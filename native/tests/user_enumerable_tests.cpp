#include "abc/core.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
int checks=0,failures=0;
void check(bool ok,std::string_view label){++checks;if(!ok){++failures;std::cerr<<"FAIL "<<label<<'\n';}}
std::vector<std::string> split(std::string_view s){std::vector<std::string> out;std::size_t at=0;for(;;){auto end=s.find('\t',at);out.emplace_back(s.substr(at,end==s.npos ? s.size()-at : end-at));if(end==s.npos)break;at=end+1;}return out;}
std::string run(std::string_view source,abc::ExecutionOptions options={}){std::istringstream input;std::ostringstream output;abc::execute(abc::compile(source),input,output,options);return output.str();}
void result(std::string_view source,std::string_view expected){try{auto actual=run(source);check(actual==expected,std::string(source)+" -> "+actual);}catch(const std::exception& e){check(false,std::string(source)+" -> "+e.what());}}
template<class F>void error(F f,std::string_view code){try{f();check(false,"Expected "+std::string(code));}catch(const abc::Error& e){check(e.code==code && !e.russian.empty(),std::string(code)+" -> "+e.code+" "+e.what());}catch(const std::exception& e){check(false,e.what());}}
void reference(){
 const auto dir=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";
 std::ifstream source(dir/"user-enumerables-program.pas",std::ios::binary);check(bool(source),"Native IEnumerable reference program present");
 std::ostringstream text;text<<source.rdbuf();const auto program=abc::compile(text.str());
 std::ifstream in(dir/"user-enumerables-reference.tsv");check(bool(in),"Independent CLR IEnumerable traces present");std::string line;std::size_t rows=0;
 while(std::getline(in,line)){
  if(!line.empty() && line.back()=='\r')line.pop_back();if(line.empty() || line.front()=='#')continue;auto f=split(line);if(f.size()!=5){check(false,"Malformed IEnumerable reference row");continue;}
  std::istringstream input(f[0]+" "+f[1]+" "+f[2]+"\n");std::ostringstream output;std::string code="ok";
  try{abc::execute(program,input,output);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty() && *e.what(),"User enumerable error is bilingual");}catch(const std::exception& e){code=e.what();}
  check(code==f[4] && output.str()==f[3],"scenario "+f[0]+" count "+f[1]+" flags "+f[2]+" expected "+f[3]+" "+f[4]+" got "+output.str()+" "+code);++rows;
 }
 check(rows==1161,"All independent foreach/query/method/fault traces checked");
}
void semantics(){
 result("type I=interface(IEnumerable<integer>) end;S=class(I) public function GetEnumerator:IEnumerator<integer>:=Range(1,3).GetEnumerator;end;begin var c:=new S;var i:I:=c;var a:IEnumerable<integer>:=c;var b:IEnumerable<integer>:=i;assert(a=b);assert(a=c);assert(i=c);assert(I(a)=i);assert(TypeName(a)='S');assert(a.GetType=typeof(S));assert(Count(c)=3);assert(c.Count=3);assert(i.ToArray.SequenceEqual(Arr(1,2,3)));var total:=0;foreach var x in c do total+=x;assert(total=6);var conditional:=if True then c else a;assert(conditional=a);assert(2 in c);end.","");
 result("type I=interface function F:integer;end;C=class(I) public function F:integer:=7;end;begin var c:=new C;foreach var x:I in Arr(c) do Writeln(x.F,' ',x=c);end.","7 True\n");
 result("type R=record n:integer;end;S=class(IEnumerable<R>) public function GetEnumerator:IEnumerator<R>;begin var r:R;r.n:=7;var items:IEnumerable<R>:=Arr(r);Result:=items.GetEnumerator;end;end;begin var s:=new S;var a:=s.ToArray;var r:=a[0];r.n:=9;Writeln(a[0].n,' ',s.First.n);end.","7 7\n");
 result("type S=class(IEnumerable<integer>) public function GetEnumerator:IEnumerator<integer>:=Range(1,3).GetEnumerator;end;begin var s:=new S;var list:=new List<integer>;list.AddRange(s);Writeln(list);Writeln(HSet(s.AsEnumerable));Writeln(MatrByRow(1,3,s));end.","[1,2,3]\n{1,2,3}\n[[1,2,3]]\n");
 for(const auto s:{"type S=class(IEnumerable<integer>) end;begin Writeln('never');end.","type S=class(IEnumerable<integer>) public function GetEnumerator:IEnumerator<real>:=Range(1.0,2.0,1.0).GetEnumerator;end;begin Writeln('never');end.","type S=class(IEnumerable<integer>) private function GetEnumerator:IEnumerator<integer>:=Range(1,2).GetEnumerator;end;begin Writeln('never');end."})error([&]{abc::compile(s);},"E_TYPE");
 error([]{run("type S=class(IEnumerable<integer>) public function GetEnumerator:IEnumerator<integer>:=nil;end;begin var s:IEnumerable<integer>:=new S;s.Count;end.");},"E_NIL");
}
void metadata(){
 auto p=abc::compile("type S=class(IEnumerable<integer>) public function GetEnumerator:IEnumerator<integer>:=Range(1,2).GetEnumerator;end;begin var s:IEnumerable<integer>:=new S;var other:sequence of real;s.Count;end.");abc::verify(p);
 std::size_t owner=0;abc::Type alternate=abc::Type::invalid;
 for(std::size_t i=0;i<p.types.size();++i){const auto& type=p.types[i];if(type.kind==abc::CompoundKind::object_class && type.name=="s")owner=i;if(type.kind==abc::CompoundKind::sequence && type.element==abc::Type::real)alternate=static_cast<abc::Type>(64+i);}
 check(alternate!=abc::Type::invalid,"Independent closed element type known before execution");
 auto bad=p;bad.types[owner].interfaces.edit().implementations.front().target=alternate;error([&]{abc::verify(bad);},"E_BYTECODE");
 bad=p;bad.types[owner].interfaces.edit().implementations.front().routines.clear();error([&]{abc::verify(bad);},"E_BYTECODE");
 abc::verify(p);
 result("type S=class(IEnumerable<integer>) public function GetEnumerator:IEnumerator<integer>:=Lst(1,2).GetEnumerator;end;begin var s:=new S;assert(s.GetEnumerator.GetType=typeof(List<integer>.Enumerator));Writeln(s.ToArray);end.","[1,2]\n");
}
void limits(){
 const auto recursion="type C=class(IEnumerator<integer>) public function MoveNext:boolean:=(new S).Count>0;property Current:integer read GetCurrent;function GetCurrent:integer:=1;procedure Reset;begin end;procedure Dispose;begin end;end;S=class(IEnumerable<integer>) public function GetEnumerator:IEnumerator<integer>:=new C;end;begin (new S).Count;end.";
 error([&]{run(recursion);},"E_LIMIT");
 const std::string types="type C=class(IEnumerator<integer>) public function MoveNext:boolean:=True;property Current:integer read GetCurrent;function GetCurrent:integer:=1;procedure Reset;begin end;procedure Dispose;begin Write('closed');end;end;S=class(IEnumerable<integer>) public function GetEnumerator:IEnumerator<integer>:=new C;end;";
 for(const auto tail:{"s.Count;","s.Sum;","foreach var x in s do begin end;"})for(int mode=0;mode<3;++mode){
  const auto program=abc::compile(types+"begin var s:IEnumerable<integer>:=new S;"+tail+"end.");std::istringstream input;std::ostringstream output;abc::ExecutionOptions options;int polls=0;
  if(mode==0)options.instruction_limit=200;else if(mode==1)options.cancelled=[&]{return ++polls>150;};else options.allocation_limit=256;
  try{abc::execute(program,input,output,options);check(false,"Expected bounded infinite enumerable");}catch(const abc::Error& e){check(e.code==(mode==1 ? "E_CANCELLED" : "E_LIMIT") && !e.russian.empty() && output.str()=="closed","Dispose executes once under instruction/allocation/cancellation exhaustion: "+output.str()+" "+e.code);}
 }
 const auto halted=types+"begin var s:IEnumerable<integer>:=new S;foreach var x in s do Halt(7);end.";
 {std::istringstream input;std::ostringstream output;check(abc::execute(abc::compile(halted),input,output)==7 && output.str().empty(),"Halt preserves immediate termination without executing user finally code");}
 const auto failed=types+"begin var s:IEnumerable<integer>:=new S;foreach var x in s do begin var zero:=0;Writeln(1 div zero);end;end.";
 {std::istringstream input;std::ostringstream output;try{abc::execute(abc::compile(failed),input,output);check(false,"Expected loop body error");}catch(const abc::Error& e){check(e.code=="E_DIV_ZERO" && output.str()=="closed","Loop-body failure disposes the actual cursor before propagation");}}
}
void files(){
 error([]{run("type C=class(IEnumerator<integer>) public function MoveNext:boolean:=True;property Current:integer read One;function One:integer:=1;procedure Reset;begin end;procedure Dispose;begin Sleep(-1);end;end;S=class(IEnumerable<integer>) public function GetEnumerator:IEnumerator<integer>:=new C;end;begin foreach var x in new S do begin var zero:=0;Writeln(1 div zero);end;end.");},"E_CANCELLED");
 const std::string name="abc-user-enumerable-cleanup.tmp";
 const auto source="type C=class(IEnumerator<integer>) private f:Text;public constructor Create;begin f:=OpenWrite('"+name+"');end;function MoveNext:boolean:=True;property Current:integer read One;function One:integer:=1;procedure Reset;begin end;procedure Dispose;begin f.Write('closed');f.Close;end;end;S=class(IEnumerable<integer>) public function GetEnumerator:IEnumerator<integer>:=new C;end;begin var s:=new S;foreach var x in s do begin end;end.";
 for(int mode=0;mode<3;++mode){abc::ExecutionOptions options;int polls=0;if(mode==0)options.instruction_limit=300;else if(mode==1)options.cancelled=[&]{return ++polls>300;};else options.allocation_limit=400;
  error([&]{run(source,options);},mode==1 ? "E_CANCELLED" : "E_LIMIT");std::ifstream input(name,std::ios::binary);std::ostringstream text;text<<input.rdbuf();check(text.str()=="closed","User Dispose writes and closes a real file under resource exhaustion");}
 std::filesystem::remove(name);
}
}
int main(){try{reference();semantics();metadata();limits();files();}catch(const std::exception& e){check(false,e.what());}std::cout<<(checks-failures)<<'/'<<checks<<" user enumerable checks passed\n";return failures ? 1 : 0;}

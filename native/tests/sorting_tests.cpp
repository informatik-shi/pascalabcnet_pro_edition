#include "abc/core.hpp"
#include "../src/sorting.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
int checks=0,failures=0;
void check(bool ok,std::string_view text){++checks;if(!ok){++failures;if(failures<=14)std::cerr<<"FAIL "<<text<<'\n';}}
std::string run(std::string_view source,abc::ExecutionOptions options={}){std::istringstream input;std::ostringstream output;abc::execute(abc::compile(source),input,output,options);return output.str();}
void result(std::string_view source,std::string_view expected){try{const auto actual=run(source);check(actual==expected,std::string(source)+" -> "+actual);}catch(const std::exception& e){check(false,std::string(source)+" -> "+e.what());}}
template<class F>void error(F f,std::string_view code){try{f();check(false,"Expected "+std::string(code));}catch(const abc::Error& e){check(e.code==code && !e.russian.empty() && *e.what(),std::string(code)+" -> "+e.code+" "+e.what());}catch(const std::exception& e){check(false,e.what());}}
std::vector<std::string> split(std::string_view s){std::vector<std::string> out;std::size_t at=0;for(;;){auto end=s.find('\t',at);out.emplace_back(s.substr(at,end==s.npos?s.size()-at:end-at));if(end==s.npos)break;at=end+1;}return out;}
void reference(){
 const auto dir=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";
 std::ifstream source(dir/"sorting-program.pas");check(bool(source),"Pascal sort reference program present");std::ostringstream text;text<<source.rdbuf();const auto program=abc::compile(text.str());
 std::ifstream file(dir/"sorting-reference.tsv");check(bool(file),"Independent Framework 4.7.2 sort corpus present");std::string line;int rows=0;
 while(std::getline(file,line)){if(!line.empty() && line.back()=='\r')line.pop_back();if(line.empty() || line.front()=='#')continue;auto f=split(line);if(f.size()!=7){check(false,"Invalid sort reference row");continue;}
  std::istringstream input(f[0]+" "+f[1]+" "+f[2]+" "+f[3]+" "+f[4]+"\n");std::ostringstream output;std::string code="ok";
  try{abc::execute(program,input,output);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty(),"Sort fault is bilingual");}catch(const std::exception& e){code=e.what();}
  check(output.str()==f[5] && code==f[6],"scenario "+f[0]+" size "+f[1]+" seed "+f[2]+" mode "+f[3]+" nil "+f[4]+" expected "+f[5]+" "+f[6]+" got "+output.str()+" "+code);++rows;
 }check(rows==3842,"All exact default/delegate/predicate/key/reentrant/fault sort traces checked");
}
void semantics(){
 result("begin var a:=Arr(3,1,2);var alias:=a;Sort(a,(x,y)->x-y);Writeln(a,' ',a=alias);Sort(a,(x,y)->x>y);Writeln(a);SortDescending(a,x->x mod 2);Writeln(a);end.","[1,2,3] True\n[3,2,1]\n[1,3,2]\n");
 result("begin var a:=Arr('aa','Aa','zz');Sort(a);Writeln(a);Sort&<string>(a);Writeln(a);a.Sort;Writeln(a);end.","[Aa,aa,zz]\n[aa,Aa,zz]\n[Aa,aa,zz]\n");
 result("uses Arrays;begin var a:=Arr('aa','Aa','zz');Arrays.Sort(a);Writeln(a);Arrays.SortDescending(a);Writeln(a);end.","[aa,Aa,zz]\n[zz,Aa,aa]\n");
 result("type R=record id,n:integer;end;function Make(i,n:integer):R;begin Result.id:=i;Result.n:=n;end;function Compare(a,b:R):integer;begin Result:=a.n-b.n;a.n:=100;b.n:=200;end;begin var a:=Arr(Make(0,3),Make(1,1),Make(2,2));Sort(a,Compare);foreach var r in a do Write(r.id,':',r.n,';');end.","1:1;2:2;0:3;");
 result("type R=record id,n:integer;end;function Make(i,n:integer):R;begin Result.id:=i;Result.n:=n;end;begin var a:=Arr(Make(0,3),Make(1,1),Make(2,2));Sort(a,x->x.n);foreach var r in a do Write(r.id,':',r.n,';');end.","1:1;2:2;0:3;");
 result("type C=class end;begin var a:=new C[0];Sort(a);var b:=new C[1];Sort(b);Writeln(a.Length,' ',b.Length);end.","0 1\n");
 error([]{run("type C=class end;begin var a:=Arr(new C,new C);Sort(a);end.");},"E_SORT");
 result("type E=(A,B,C);begin var a:=Arr(C,A,B);Sort(a);Writeln(a);SortDescending(a);Writeln(a);end.","[A,B,C]\n[C,B,A]\n");
 result("begin var a:=Arr(3bi,1bi,2bi);Sort(a);Writeln(a);end.","[1,2,3]\n");
 result("var count:integer;function Key(x:integer):integer;begin count+=1;Result:=0;end;begin var a:=Arr(3,2,1);Sort(a,Key);Writeln(a,' ',count);end.","[3,2,1] 3\n");
 for(const auto tail:{"Sort(a,f);","a.Sort(f);","SortDescending(a,k);","a.SortDescending(k);"})error([&]{run(std::string("type F=function(a,b:integer):integer;K=function(a:integer):integer;begin var a:array of integer;var f:F;var k:K;")+tail+"end.");},"E_NIL");
 for(const auto source:{"begin Sort(Arr(1,2),(x,y,z)->x);end.","begin Sort(Arr(1,2),(x,y)->'bad');end.","begin SortDescending(Arr(1,2),(x,y)->x-y);end.","begin Arr(1,2).Sort((x,y)->x<y);end.","begin Sort&<real>(Arr(1,2));end.","begin Sort&<integer,string>(Arr(1,2),x->x);end.","begin Sort(new integer[2,2]);end."})error([&]{abc::compile(source);},"E_TYPE");
 result("procedure Sort(a:array of integer);begin Writeln('own');end;begin var a:=Arr(2,1);Sort(a);PABCSystem.Sort(a);Writeln(a);end.","own\n[1,2]\n");
}
void extra_syntax(){
 error([]{run("begin Sort(Arr(new Text,new Text));end.");},"E_SORT");
 error([]{run("type C=class(IComparable<C>) public function CompareTo(x:C):integer;begin Sort(Arr(Self,x));Result:=0;end;end;begin Sort(Arr(new C,new C));end.");},"E_LIMIT");
 result("uses Arrays;begin var a:=Arr('aa','Aa','zz');Arrays.Sort&<string>(a);Writeln(a);a.Sort&<string>;Writeln(a);PABCSystem.Sort&<string>(a);Writeln(a);end.","[aa,Aa,zz]\n[aa,Aa,zz]\n[aa,Aa,zz]\n");
}
void metadata(){
 auto p=abc::compile("begin var a:=Arr(2,1);Sort(a,(x,y)->x-y);end.");abc::verify(p);
 for(unsigned mode=0;mode<7;++mode){auto bad=p;for(auto& op:bad.code)if(op.op==abc::Op::sort_call){if(mode==0)op.operand|=128;if(mode==1)op.operand|=std::size_t{1}<<40U;if(mode==2)op.type=abc::Type::integer;if(mode==3)op.operand=1;if(mode==4)op.operand|=abc::sort_generic_key;if(mode==5)op.operand=(op.operand&~std::size_t{15})|3;if(mode==6)op.operand=0;break;}error([&]{abc::verify(bad);},"E_BYTECODE");}
}
void limits(){
 const auto infinite="function C(a,b:integer):integer;begin while True do begin end;Result:=0;end;begin Sort(Arr(2,1),C);end.";
 for(int mode=0;mode<2;++mode){abc::ExecutionOptions options;int calls=0;if(mode==0)options.instruction_limit=300;else options.cancelled=[&]{return ++calls>100;};error([&]{run(infinite,options);},mode?"E_CANCELLED":"E_LIMIT");}
 error([]{run("begin var a:=ArrGen(1000,x->x);Sort(a,(x,y)->-1);end.");},"E_COMPARER");
}
}
int main(){try{reference();semantics();extra_syntax();metadata();limits();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" sorting checks, "<<failures<<" failures\n";return failures?1:0;}

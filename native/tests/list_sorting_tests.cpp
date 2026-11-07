#include "abc/core.hpp"
#include "../src/sorting.hpp"
#include "../src/runtime_values.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
int checks=0,failures=0;
void check(bool ok,std::string_view text){++checks;if(!ok){++failures;if(failures<=14)std::cerr<<"FAIL "<<text<<'\n';}}
std::string run(std::string_view source,abc::ExecutionOptions options={}){std::istringstream input;std::ostringstream output;abc::execute(abc::compile(source),input,output,options);return output.str();}
void result(std::string_view source,std::string_view expected){try{auto actual=run(source);check(actual==expected,std::string(source)+" -> "+actual);}catch(const std::exception& e){check(false,std::string(source)+" -> "+e.what());}}
template<class F>void error(F f,std::string_view code){try{f();check(false,"Expected "+std::string(code));}catch(const abc::Error& e){check(e.code==code && !e.russian.empty() && *e.what(),std::string(code)+" -> "+e.code+" "+e.what());}catch(const std::exception& e){check(false,e.what());}}
std::vector<std::string> split(std::string_view s){std::vector<std::string> out;std::size_t at=0;for(;;){auto end=s.find('\t',at);out.emplace_back(s.substr(at,end==s.npos?s.size()-at:end-at));if(end==s.npos)break;at=end+1;}return out;}
void reference(){
 const auto dir=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";
 std::ifstream source(dir/"list-sorting-program.pas");check(bool(source),"Pascal List sorting program present");std::ostringstream text;text<<source.rdbuf();const auto program=abc::compile(text.str());
 std::ifstream file(dir/"list-sorting-reference.tsv");check(bool(file),"Independent Framework List corpus present");std::string line;int rows=0;
 while(std::getline(file,line)){if(!line.empty() && line.back()=='\r')line.pop_back();if(line.empty() || line.front()=='#')continue;auto f=split(line);if(f.size()!=7){check(false,"Invalid List sorting row");continue;}
  std::istringstream input(f[0]+" "+f[1]+" "+f[2]+" "+f[3]+" "+f[4]+"\n");std::ostringstream output;std::string code="ok";
  try{abc::execute(program,input,output);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty(),"List sort fault is bilingual");}catch(const std::exception& e){code=e.what();}
  check(output.str()==f[5] && code==f[6],"scenario "+f[0]+" size "+f[1]+" pad "+f[2]+" mode "+f[3]+" nil "+f[4]+" expected "+f[5]+" "+f[6]+" got "+output.str()+" "+code);++rows;
 }check(rows==14231,"All independent List storage/sort/comparer/range/key/version/finally traces checked");
}
void semantics(){
 result("begin var l:=Lst(3,1,2);var alias:=l;var e:=l.GetEnumerator;l.Sort((a,b)->a-b);Writeln(l,' ',e.MoveNext);Sort(l,x->-x);Writeln(l,' ',alias,' ',l=alias);l.Sort(Comparer<integer>.Default);Writeln(l);var order:IComparer<integer>;l.Sort(1,2,order);Writeln(l);end.","[1,2,3] True\n[3,2,1] [1,2,3] False\n[1,2,3]\n[1,2,3]\n");
 result("begin var l:=Lst('aa','Aa','zz');Sort(l);Writeln(l);l.Sort;Writeln(l);Sort&<string>(l);Writeln(l);var alias:=l;Sort(l,x->x);Writeln(l,' ',l=alias);Sort&<string,string>(l,x->x);Writeln(l);end.","[Aa,aa,zz]\n[aa,Aa,zz]\n[aa,Aa,zz]\n[Aa,aa,zz] False\n[aa,Aa,zz]\n");
 result("type R=record id,n:integer;end;function Make(i,n:integer):R;begin Result.id:=i;Result.n:=n;end;function Compare(a,b:R):integer;begin Result:=a.n-b.n;a.n:=100;end;begin var l:=Lst(Make(0,3),Make(1,1),Make(2,2));l.Sort(Compare);foreach var x in l do Write(x.id,':',x.n,';');var alias:=l;SortDescending(l,x->x.n);Writeln(' ',l=alias,' ',l.Capacity,' ',alias[0].n);end.","1:1;2:2;0:3; False 3 1\n");
 result("type R=record n:integer;end;var l:List<R>;procedure P(var n:integer);begin l.Capacity:=20;l.TrimExcess;n:=7;end;begin var r:R;l:=new List<R>(Arr(r));var e:=l.GetEnumerator;P(l[0].n);Writeln(l[0].n,' ',l.Capacity,' ',e.MoveNext,' ',e.Current.n);end.","0 1 True 0\n");
 result("procedure Reorder(var l:List<integer>);begin Sort(l,x->-x);end;begin var l:=Lst(1,2,3);var alias:=l;Reorder(l);Writeln(l,' ',alias,' ',l=alias);end.","[3,2,1] [1,2,3] False\n");
 for(const auto body:{"l.Sort;","Sort(l);","l.Sort(Comparer<integer>.Default);","l.Sort(0,0,nil);"})error([&]{run(std::string("begin var l:=LstInt();var e:=l.GetEnumerator;")+body+"e.MoveNext;end.");},"E_MODIFIED");
 result("begin var l:=LstInt();var e:=l.GetEnumerator;l.Sort((a,b)->a-b);Writeln(e.MoveNext);end.","False\n");
 error([]{run("begin var l:=Lst(1,2);var e:=l.GetEnumerator;l.AddRange(new integer[0]);e.MoveNext;end.");},"E_MODIFIED");
 for(const auto args:{"-1,0,nil","0,-1,nil","4,0,nil","0,4,nil","2147483647,2147483647,nil"})error([&]{run(std::string("begin var l:=Lst(3,2,1);l.Sort(")+args+");end.");},"E_RANGE");
 error([]{run("type F=function(a,b:integer):integer;begin var l:=LstInt();var f:F;l.Sort(f);end.");},"E_NIL");
 error([]{run("type F=function(a:integer):integer;begin var l:=LstInt();var f:F;Sort(l,f);end.");},"E_NIL");
 error([]{run("begin var l:List<integer>;l.Sort(Comparer<integer>.Default);end.");},"E_NIL");
 for(const auto source:{"begin Sort(Lst(1,2),x->x);end.","begin var l:=Lst(1,2);l.Sort(x->x);end.","begin var l:=Lst(1,2);l.Sort((a,b)->a<b);end.","begin var l:=Lst(1,2);l.Sort&<integer>;end.","begin var l:=Lst(1,2);SortDescending(l,(a,b)->a-b);end.","begin var l:=Lst(1,2);l.Sort(0,1,Comparer<string>.Default);end.","begin var l:=Lst(1,2);l.Sort(nil);end.","uses Arrays;begin Arrays.Sort(Lst(1,2));end."})error([&]{abc::compile(source);},"E_TYPE");
}
void explicit_key_context(){
 result("begin var l:=Lst(3,1,2);Sort&<integer,real>(l,x->x);Writeln(l);var a:=Arr(3,1,2);Sort&<integer,real>(a,x->x);Writeln(a);end.","[1,2,3]\n[1,2,3]\n");
 auto p=abc::compile("begin var l:=Lst(2,1);Sort&<integer,real>(l,x->x);end.");int typed=0;for(const auto& op:p.code)if(op.op==abc::Op::sort_call){auto callback=abc::sort_callback(op.operand);check(abc::type_info(p,callback).element==abc::Type::real,"Explicit key result conversion is known before execution");++typed;}check(typed==1,"Closed contextual key callback emitted");
}
void metadata(){
 auto p=abc::compile("begin var l:=Lst(2,1);Sort(l,x->x);l.Sort(0,1,Comparer<integer>.Default);end.");abc::verify(p);
 for(int mode=0;mode<6;++mode){auto bad=p;for(auto& op:bad.code)if(op.op==abc::Op::sort_call){if(mode==0)op.operand|=abc::sort_extension;if(mode==1)op.operand|=abc::sort_range;if(mode==2)op.operand=(op.operand&~std::size_t{15})|5;if(mode==3)op.type=abc::Type::integer;if(mode==4)op.operand|=std::size_t{1}<<40U;if(mode==5)op.operand=0;break;}error([&]{abc::verify(bad);},"E_BYTECODE");}
}
void limits(){
 const auto infinite="function C(a,b:integer):integer;begin while True do begin end;Result:=0;end;begin var l:=Lst(2,1);l.Sort(C);end.";
 for(int mode=0;mode<2;++mode){abc::ExecutionOptions options;int calls=0;if(mode==0)options.instruction_limit=300;else options.cancelled=[&]{return ++calls>100;};error([&]{run(infinite,options);},mode?"E_CANCELLED":"E_LIMIT");}
 error([]{run("type C=class(IComparer<integer>) public function Compare(a,b:integer):integer;begin var l:=Lst(a,b);l.Sort(Self);Result:=0;end;end;begin var l:=Lst(2,1);l.Sort(new C);end.");},"E_LIMIT");
}
}
int main(){try{reference();semantics();explicit_key_context();metadata();limits();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" List sorting checks, "<<failures<<" failures\n";return failures?1:0;}

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
 std::ifstream source(dir/"array-sorting-program.pas");check(bool(source),"Pascal sort reference program present");std::ostringstream text;text<<source.rdbuf();const auto program=abc::compile(text.str());
 std::ifstream file(dir/"array-sorting-reference.tsv");check(bool(file),"Independent Framework 4.7.2 sort corpus present");std::string line;int rows=0;
 while(std::getline(file,line)){if(!line.empty() && line.back()=='\r')line.pop_back();if(line.empty() || line.front()=='#')continue;auto f=split(line);if(f.size()!=9){check(false,"Invalid sort reference row");continue;}
  std::istringstream input(f[0]+" "+f[1]+" "+f[2]+" "+f[3]+" "+f[4]+" "+f[5]+" "+f[6]+"\n");std::ostringstream output;std::string code="ok";
  try{abc::execute(program,input,output);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty(),"Sort fault is bilingual");}catch(const std::exception& e){code=e.what();}
  check(output.str()==f[7] && code==f[8],"scenario "+f[0]+" size "+f[1]+" seed "+f[2]+" mode "+f[3]+" nil "+f[4]+" expected "+f[7]+" "+f[8]+" got "+output.str()+" "+code);++rows;
 }check(rows==3791,"All exact System.Array default/comparer/range/mutation/fault traces checked");
}
void semantics(){
 result("begin var a:=Arr(9,3,1,2,8);var alias:=a;System.Array.Sort(a,1,3);Writeln(a,' ',a=alias);Array.Sort(a,(x,y)->y-x);Writeln(a);System.Array.Sort&<integer>(a,Comparer<integer>.Default);Writeln(a);end.","[9,1,2,3,8] True\n[9,8,3,2,1]\n[1,2,3,8,9]\n");
 result("begin var a:=Arr('aa','Aa','zz');System.Array.Sort(a);Writeln(a);Array.Sort&<string>(a,0,2,nil);Writeln(a);end.","[aa,Aa,zz]\n[aa,Aa,zz]\n");
 result("type R=record n:integer;end;function Make(n:integer):R;begin Result.n:=n;end;function C(a,b:R):integer;begin Result:=a.n-b.n;a.n:=99;end;begin var a:=Arr(Make(3),Make(1),Make(2));Array.Sort(a,C);foreach var r in a do Write(r.n);end.","123");
 for(const auto args:{"-1,0","0,-1","4,0","0,4","2147483647,2147483647","-1,0,nil","0,-1,nil","4,0,nil","0,4,nil"})error([&]{run(std::string("begin var a:=Arr(3,2,1);Array.Sort(a,")+args+");end.");},"E_RANGE");
 for(const auto n:{"0","1","2"})error([&]{run(std::string("type F=function(a,b:integer):integer;begin var a:=new integer[")+n+"];var f:F;System.Array.Sort(a,f);end.");},"E_NIL");
 result("begin var a:=Arr(2,1);var order:IComparer<integer>;Array.Sort(a,order);Writeln(a);Array.Sort&<integer>(a);Writeln(a);end.","[1,2]\n[1,2]\n");
 error([]{run("begin var a:array of integer;Array.Sort(a,-1,-1);end.");},"E_NIL");
 error([]{run("type C=class end;begin System.Array.Sort(Arr(new C,new C));end.");},"E_SORT");
 for(const auto source:{"begin Array.Sort(Lst(1,2));end.","begin Array.Sort(new integer[2,2]);end.","begin Array.Sort&<real>(Arr(2,1));end.","begin Array.Sort&<integer,string>(Arr(2,1));end.","begin Array.Sort(Arr(2,1),nil);end.","begin Array.Sort(Arr(2,1),(a,b)->a<b);end.","begin Array.Sort(Arr(2,1),x->x);end.","begin Array.Sort(Arr(2,1),0,2,(a,b)->a-b);end.","begin Array.Sort(Arr(2,1),Comparer<string>.Default);end."})error([&]{abc::compile(source);},"E_TYPE");
 result("type A=class public procedure Sort(a:array of integer);begin Write('own');end;end;B=class public &Array:A;end;begin var System:=new B;System.Array:=new A;System.Array.Sort(Arr(2,1));var &Array:=new A;Array.Sort(Arr(2,1));end.","ownown");
}
void argument_order(){
 result("var a:array of integer;function Start:integer;begin Write('I');a:=Arr(7,8);Result:=0;end;function Count:integer;begin Write('C');Result:=3;end;function Order:IComparer<integer>;begin Write('K');Result:=nil;end;begin a:=Arr(3,2,1);var old:=a;Array.Sort(a,Start,Count,Order);Writeln(' ',old,' ',a);end.","ICK [1,2,3] [7,8]\n");
}
void metadata(){
 auto p=abc::compile("begin var a:=Arr(2,1);Array.Sort(a,(a,b)->a-b);Array.Sort(a,Comparer<integer>.Default);Array.Sort(a,0,1);end.");abc::verify(p);
 for(int mode=0;mode<7;++mode){auto bad=p;for(auto& op:bad.code)if(op.op==abc::Op::sort_call){if(mode==0)op.operand|=abc::sort_range;if(mode==1)op.operand|=abc::sort_extension;if(mode==2)op.operand|=abc::sort_descending;if(mode==3)op.operand|=abc::sort_generic_key;if(mode==4)op.type=abc::Type::integer;if(mode==5)op.operand=(op.operand&~std::size_t{15})|9;if(mode==6)op.operand=static_cast<unsigned>(abc::SortMethod::system_comparison);break;}error([&]{abc::verify(bad);},"E_BYTECODE");}
}
void limits(){
 const auto infinite="function C(a,b:integer):integer;begin while True do begin end;Result:=0;end;begin Array.Sort(Arr(2,1),C);end.";
 for(int mode=0;mode<2;++mode){abc::ExecutionOptions options;int calls=0;if(mode==0)options.instruction_limit=300;else options.cancelled=[&]{return ++calls>100;};error([&]{run(infinite,options);},mode?"E_CANCELLED":"E_LIMIT");}
 error([]{run("type C=class(IComparer<integer>) public function Compare(a,b:integer):integer;begin Array.Sort(Arr(a,b),Self);Result:=0;end;end;begin Array.Sort(Arr(2,1),new C);end.");},"E_LIMIT");
}
}
int main(){try{reference();semantics();argument_order();metadata();limits();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" System.Array sorting checks, "<<failures<<" failures\n";return failures?1:0;}

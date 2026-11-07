#include "abc/core.hpp"
#include "../src/comparison.hpp"
#include "../src/runtime_values.hpp"
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
namespace {
int checks=0,failures=0;
void check(bool ok,std::string_view label){++checks;if(!ok){++failures;std::cerr<<"FAIL "<<label<<'\n';}}
std::string run(std::string_view source,abc::ExecutionOptions options={}){std::istringstream in;std::ostringstream out;abc::execute(abc::compile(source),in,out,options);return out.str();}
void result(std::string_view source,std::string_view expected){try{auto actual=run(source);check(actual==expected,std::string(source)+" -> "+actual);}catch(const std::exception& e){check(false,std::string(source)+" -> "+e.what());}}
template<class F>void error(F f,std::string_view code){try{f();check(false,"Expected "+std::string(code));}catch(const abc::Error& e){check(e.code==code && !e.russian.empty() && *e.what(),std::string(code)+" -> "+e.code+" "+e.what());}catch(const std::exception& e){check(false,e.what());}}
std::vector<std::string> split(std::string_view s){std::vector<std::string> out;std::size_t at=0;for(;;){auto end=s.find('\t',at);out.emplace_back(s.substr(at,end==s.npos?s.size()-at:end-at));if(end==s.npos)break;at=end+1;}return out;}
void reference(){
 const auto dir=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";
 std::ifstream source(dir/"comparers-program.pas");check(bool(source),"Pascal comparer reference program present");std::ostringstream text;text<<source.rdbuf();auto program=abc::compile(text.str());
 std::ifstream file(dir/"comparers-reference.tsv");check(bool(file),"Independent comparer corpus present");std::string line;int rows=0;
 while(std::getline(file,line)){if(!line.empty() && line.back()=='\r')line.pop_back();if(line.empty() || line.front()=='#')continue;auto f=split(line);if(f.size()!=7){check(false,"Invalid comparer reference row");continue;}
  std::istringstream input(f[0]+" "+f[1]+" "+f[2]+" "+f[3]+" "+f[4]+"\n");std::ostringstream output;std::string code="ok";
  try{abc::execute(program,input,output);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty(),"Comparer failure is bilingual");}catch(const std::exception& e){code=e.what();}
  check(output.str()==f[5] && code==f[6],"scenario "+f[0]+" "+f[1]+","+f[2]+","+f[3]+" mode "+f[4]+" expected "+f[5]+" "+f[6]+" got "+output.str()+" "+code);++rows;
 }check(rows==2304,"All independent comparer and range traces verified");
}
void scalar_reference(){
 std::ifstream file(std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures"/"comparers-scalars-reference.tsv");check(bool(file),"Independent primitive comparison corpus present");std::string line;int rows=0;
 while(std::getline(file,line)){if(!line.empty() && line.back()=='\r')line.pop_back();if(line.empty() || line.front()=='#')continue;auto f=split(line);if(f.size()!=4){check(false,"Invalid scalar reference row");continue;}const auto expr=[&](const std::string& x){return f[0]=="boolean" ? x : f[0]+"("+x+")";};const auto a=expr(f[1]),b=expr(f[2]);
 result("begin var a:="+a+";var b:="+b+";Writeln(a.CompareTo(b),' ',Comparer<"+f[0]+">.Default.Compare(a,b));end.",f[3]+" "+f[3]+"\n");++rows;
 }check(rows==148,"All primitive comparison magnitudes verified");
}
void semantics(){
 result("begin var b:byte:=2;Writeln(10.CompareTo(b),' ',('abc').CompareTo(nil));end.","1 1\n");
 error([]{run("begin var s:string:=nil;s.CompareTo('abc');end.");},"E_NIL");
 result("begin var q:=Comparer<integer>.Default;Writeln(q.Compare(1,2),' ',q.Compare(2,1),' ',q.Compare(2,2),' ',q=Comparer<integer>.Default);var i:IComparer<integer>:=q;Writeln(i.Compare(4,3),' ',i=q);end.","-1 1 0 True\n1 True\n");
 result("type IntOrder=System.Collections.Generic.Comparer<integer>;begin var q:=IntOrder.Default;Writeln(q.Compare(3,4),' ',System.Collections.Generic.Comparer<integer>.Default=q);end.","-1 True\n");
 result("begin var offset:=2;var q:=Comparer<integer>.Create((a,b)->offset*(a-b));offset:=3;Writeln(q.Compare(5,2));end.","9\n");
 result("begin Writeln(1.CompareTo(2),' ',True.CompareTo(False),' ',('aa').CompareTo('aa'),' ',(3bi).CompareTo(2bi));end.","-1 1 0 1\n");
 result("type E=(A,B,C);begin Writeln(E.B.CompareTo(E.C),' ',Comparer<E>.Default.Compare(E.C,E.A),' ',TypeName(Comparer<E>.Default));end.","-1 1 ObjectComparer<E>\n");
 result("type I=interface(System.IComparable<I>) property N:integer read;end;C=class(I) public value:integer;property N:integer read value;function CompareTo(other:I):integer;begin Result:=value-other.N;end;end;begin var a:=new C;var b:=new C;b.value:=7;var x:I:=a;var y:I:=b;Writeln(Comparer<I>.Default.Compare(x,y),' ',InRange(x,x,y));end.","-7 True\n");
 result("type C=class end;begin var q:=Comparer<C>.Default;var a:=new C;Writeln(q.Compare(a,a),' ',q.Compare(nil,a),' ',q.Compare(a,nil),' ',q.Compare(nil,nil));end.","0 -1 1 0\n");
 result("begin var a:=Arr(1);var q:=Comparer<array of integer>.Default;Writeln(q.Compare(a,a),' ',q.Compare(nil,a));end.","0 -1\n");
 result("begin var q:=Comparer<string>.Default;Writeln(q.Compare(nil,nil),' ',q.Compare(nil,'a'),' ',q.Compare('a',nil),' ',q.Compare('abc','abc'));end.","0 -1 1 0\n");
 result("begin var q:=Comparer<integer>.Default;var c:=Comparer<integer>.Create((a,b)->a-b);Writeln(TypeName(q),' ',TypeName(c),' ',TypeToTypeName(typeof(Comparer<integer>)),' ',TypeToTypeName(typeof(IComparer<integer>)));Writeln(q.GetType=c.GetType,' ',q.GetType=typeof(Comparer<integer>));end.","GenericComparer<integer> ComparisonComparer<integer> Comparer<integer> IComparer<integer>\nFalse False\n");
 error([]{run("begin Comparer<integer>.Create(nil);end.");},"E_NIL");
 error([]{run("begin var i:IComparer<integer>;i.Compare(1,2);end.");},"E_NIL");
 error([]{run("type C=class end;begin Comparer<C>.Default.Compare(new C,new C);end.");},"E_COMPARE");
 error([]{run("type R=record n:integer;end;begin var r:R;Comparer<R>.Default.Compare(r,r);end.");},"E_COMPARE");
 error([]{run("begin var a:=Arr(1);Comparer<array of integer>.Default.Compare(a,Arr(1));end.");},"E_COMPARE");
 for(auto s:{"begin new Comparer<integer>;end.","begin new IComparer<integer>;end.","begin new IComparable<integer>;end.","begin Comparer<integer>.Create((a,b)->True);end.","begin Comparer<integer>.Default.Compare('a',2);end.","type C=class(IComparable<C>) public function CompareTo(x:integer):integer:=0;end;begin Writeln('never');end.","type C=class(IComparer<C>) public function Compare(a,b:C):boolean:=False;end;begin end.","type C=class end;begin var c:=new C;InRange(c,c,c);end."})error([&]{abc::compile(s);},s==std::string_view("type C=class end;begin var c:=new C;InRange(c,c,c);end.")?"E_UNSUPPORTED":"E_TYPE");
}
void reference_identity(){
 result("begin var s:string:=nil;Writeln(s.InRange(nil,'abc'),' ',s.Between(nil,'abc'),' ',s.Between(nil,nil,False));end.","True True False\n");
 result("begin var t:=(1,2);Writeln(Comparer<(integer,integer)>.Default.Compare(t,t));end.","0\n");
 result("function F(n:integer):integer:=n;begin var f1:Func<integer,integer>:=F;var f2:Func<integer,integer>:=F;Writeln(Comparer<Func<integer,integer>>.Default.Compare(f1,f1));end.","0\n");
 error([]{run("function F(n:integer):integer:=n;begin var f1:Func<integer,integer>:=F;var f2:Func<integer,integer>:=F;Comparer<Func<integer,integer>>.Default.Compare(f1,f2);end.");},"E_COMPARE");
}
void floating(){
 abc::Program p;abc::Values v{p,100000};abc::ExecutionOptions options;
 const auto nan=std::numeric_limits<double>::quiet_NaN(),inf=std::numeric_limits<double>::infinity();
 const std::array<double,7> numbers{nan,-inf,-2.,-0.,0.,2.,inf};
 for(auto a:numbers)for(auto b:numbers){const int expected=std::isnan(a)?std::isnan(b)?0:-1:std::isnan(b)?1:a<b?-1:a>b?1:0;check(abc::comparison_value(v,abc::Type::real,a,b,true,false,options)==expected,"Double NaN/signed-zero/infinity order");check(abc::comparison_value(v,abc::Type::single,float(a),float(b),false,false,options)==expected,"Single NaN/signed-zero/infinity order");}
}
void metadata(){
 auto p=abc::compile("begin var q:=Comparer<integer>.Create((a,b)->a-b);Writeln(q.Compare(2,1));end.");abc::verify(p);
 std::size_t owner=0,iface=0;for(std::size_t i=0;i<p.types.size();++i){if(p.types[i].kind==abc::CompoundKind::comparer)owner=i;if(p.types[i].kind==abc::CompoundKind::comparer_interface)iface=i;}
 for(int mode=0;mode<7;++mode){auto bad=p;if(mode==0)bad.types[owner].key=abc::Type::integer;if(mode==1)bad.types[owner].element=abc::Type::unit;if(mode==2)bad.types[owner].name="invalid";if(mode==3)bad.types[iface].key=abc::Type::integer;if(mode==4)bad.types[iface].interfaces.edit().members.push_back({"fake",abc::Type::integer,{}});if(mode==5)bad.types[owner].fields.push_back({"x",abc::Type::integer});if(mode==6){auto key=static_cast<unsigned>(bad.types[owner].key)-64;bad.types[key].parameters[0].by_reference=true;}error([&]{abc::verify(bad);},"E_BYTECODE");}
 for(int mode=0;mode<3;++mode){auto bad=p;for(auto& op:bad.code)if(op.op==abc::Op::comparer_create){if(mode==0)op.operand=2;if(mode==1)op.type=abc::Type::integer;if(mode==2)op.operand=0;break;}error([&]{abc::verify(bad);},"E_BYTECODE");}
 abc::verify(p);
}
void limits(){
 const auto source="type C=class(IComparable<C>) public function CompareTo(x:C):integer:=Comparer<C>.Default.Compare(Self,x);end;begin var c:=new C;Comparer<C>.Default.Compare(c,c);end.";
 error([&]{run(source);},"E_LIMIT");
 const auto infinite="type C=class(IComparable<C>) public function CompareTo(x:C):integer;begin while True do begin end;Result:=0;end;end;begin var c:=new C;Comparer<C>.Default.Compare(c,c);end.";
 for(int mode=0;mode<2;++mode){abc::ExecutionOptions options;int calls=0;if(mode==0)options.instruction_limit=300;else options.cancelled=[&]{return ++calls>100;};error([&]{run(infinite,options);},mode?"E_CANCELLED":"E_LIMIT");}
 abc::ExecutionOptions options;options.allocation_limit=1;error([&]{run("begin Comparer<integer>.Default;end.",options);},"E_LIMIT");
}
}
int main(){try{reference();scalar_reference();semantics();reference_identity();floating();metadata();limits();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" comparer checks, "<<failures<<" failures\n";return failures?1:0;}

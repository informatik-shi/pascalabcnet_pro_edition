#include "abc/core.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
int checks=0,failures=0;
void check(bool ok,std::string_view message){++checks;if(!ok){++failures;if(failures<=18)std::cerr<<"FAIL "<<message<<'\n';}}
std::string run(std::string_view source,abc::ExecutionOptions options={}){std::istringstream input;std::ostringstream output;abc::execute(abc::compile(source),input,output,options);return output.str();}
void result(std::string_view source,std::string_view expected){try{check(run(source)==expected,source);}catch(const std::exception& e){check(false,std::string(source)+": "+e.what());}}
template<class F>void error(F action,std::string_view code){try{action();check(false,"Expected "+std::string(code));}catch(const abc::Error& e){check(e.code==code&&!e.russian.empty()&&*e.what(),std::string(code)+": "+e.code+" "+e.what());}}
std::vector<std::string> split(std::string_view text){std::vector<std::string> fields;std::size_t at=0;for(;;){auto end=text.find('\t',at);fields.emplace_back(text.substr(at,end==text.npos?end:end-at));if(end==text.npos)return fields;at=end+1;}}
std::string decode(std::string_view hex){if(hex=="-")return {};std::u16string text;for(std::size_t n=0;n<hex.size();n+=4)text.push_back(static_cast<char16_t>(std::stoul(std::string(hex.substr(n,4)),nullptr,16)));return abc::to_utf8(text);}
void reference(){
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::ifstream source(fixtures/"array-generics-program.pas");check(bool(source),"Static generic array program exists");std::ostringstream text;text<<source.rdbuf();const auto program=abc::compile(text.str());
 std::ifstream input(fixtures/"array-generics-reference.tsv");check(bool(input),"Independent .NET generic array corpus exists");std::string row;int rows=0;
 while(std::getline(input,row)){
  if(!row.empty()&&row.back()=='\r')row.pop_back();if(row.empty()||row.front()=='#')continue;const auto f=split(row);if(f.size()!=7){check(false,"Malformed generic array row");continue;}
  std::istringstream in(f[0]+" "+f[1]+" "+f[2]+" "+f[3]+" "+f[4]);std::ostringstream out;std::string code="ok";
  try{abc::execute(program,in,out);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty()&&*e.what(),"Generic array error is bilingual");}
  check(code==f[6]&&out.str()==decode(f[5]),"Operation "+f[0]+" size "+f[1]+" nil "+f[2]+" form "+f[3]+" width "+f[4]+" expected "+f[6]+" got "+code+" output "+out.str());++rows;
 }
 check(rows==720,"All independent generic array outcomes checked");
}
void semantics(){
 result("uses Arrays;begin var m:=new real[1,2];m[0,0]:=1.5;m[0,1]:=2.25;Arrays.WriteMatrix&<real>(m);Arrays.WriteMatrix(m);var p:=Arrays.WriteMatrix&<real>;p(m,5);end.","1.5 2.25 \n1.50 2.25 \n  1.5  2.25 \n");
 result("uses Arrays;type R=record n:integer;a:array of integer;end;F=procedure(a:array of R;v:R);begin var r:R;r.n:=1;r.a:=Arr(0);var a:=new R[2];var f:F:=FillByValue&<R>;f(a,r);a[0].n:=9;a[0].a[0]:=7;Writeln(a[1].n,' ',r.n,' ',r.a[0]);end.","1 1 7\n");
 result("uses Arrays;type E=(One,Two);V=array of integer;begin var a:=new E[2];FillByValue&<E>(a,Two);WritelnArray&<E>(a,'/');var s:=new string[2];FillByValue&<string>(s,nil);WritelnArray&<string>(s,'|');var outer:=new V[2];var inner:=Arr(1);FillByValue&<array of integer>(outer,inner);outer[0][0]:=8;Writeln(outer[1][0]);end.","Two/Two\nnil|nil\n8\n");
 result("uses Arrays;type P=procedure(a:array of integer;d:string);R=record p:P;end;procedure Apply(p:P;a:array of integer);begin p(a,'/');end;begin var r:R;r.p:=Arrays.WritelnArray&<integer>;Apply(r.p,Arr(1,2));var a:=new P[1];a[0]:=WritelnArray&<integer>;a[0](Arr(3,4),'|');end.","1/2\n3|4\n");
 result("uses Arrays;type P=procedure(a:array of integer;d:string);function Make:P:=Arrays.WritelnArray&<integer>;begin var p:=Make();p(Arr(1,2),'/');var make:Func0<P>:=()->Arrays.WritelnArray&<integer>;var q:=make();q(Arr(3,4),'|');end.","1/2\n3|4\n");
 result("uses Arrays;var a:array of integer;function Target:array of integer;begin Write('A');Result:=a;end;function Value:integer;begin Write('V');a:=Arr(8);Result:=7;end;begin a:=Arr(1,2);var old:=a;FillByValue&<integer>(Target(),Value());Writeln(old,' ',a);end.","AV[7,7] [8]\n");
 result("uses Arrays;function Delimiter:string;begin Write('D');Result:='|';end;begin WriteArray&<integer>(nil,Delimiter());WritelnArray&<integer>(nil);end.","D\n");
 result("uses Arrays;function WriteArray(a:array of integer;d:string):integer:=9;begin var p:=Arrays.WriteArray&<integer>;p(Arr(1,2),'/');Writeln(' ',WriteArray(Arr(1),'x'));end.","1/2 9\n");
 result("uses Arrays;type F=procedure(a:array[,] of integer;v:integer);begin var f:F:=Arrays.FillByValue&<integer>;var m:=new integer[2,2];f(m,8);WriteMatrix&<integer>(m);end.","8 8 \n8 8 \n");
 result("uses Arrays;begin var m:=new real[3,0];WriteMatrix&<real>(m,-1);end.","\n\n\n");
 auto cached=abc::compile("uses Arrays;begin var a:=Arrays.WritelnArray&<integer>;var b:=WritelnArray&<integer>;end.");int wrappers=0;for(const auto& r:cached.routines)if(r.name.starts_with("$arrays_writelnarray_"))++wrappers;check(wrappers==1,"Repeated qualified/unqualified procedure values share one typed wrapper");
 for(const auto* source:{
  "uses Arrays;begin Write('never');WriteArray&<integer,string>(Arr(1));end.",
  "uses Arrays;begin Write('never');WriteArray&<integer>();end.",
  "uses Arrays;begin Write('never');WriteArray&<real>(Arr(1));end.",
  "uses Arrays;begin Write('never');WriteArray&<integer>(new integer[1,1]);end.",
  "uses Arrays;begin WriteMatrix&<real>(new real[1,1],0,2);end.",
  "uses Arrays;begin WriteMatrix&<integer>(new integer[1]);end.",
  "uses Arrays;begin FillByValue&<integer>(nil,1);end.",
  "uses Arrays;begin var f:=FillByValue&<integer>;end.",
  "uses Arrays;type P=procedure(a:array of integer);begin var p:P:=Arrays.WriteArray&<integer>;end.",
  "uses Arrays;type P=procedure(var a:array of integer;v:integer);begin var p:P:=FillByValue&<integer>;end.",
  "uses Arrays;begin Arrays.Sort&<integer,string>(Arr(1));end.",
  "uses Arrays;begin Arrays.Sort&<integer>(Lst(1));end."
 })error([&]{run(source);},"E_TYPE");
 error([]{run("begin Write('never');FillByValue&<integer>(Arr(1),2);end.");},"E_UNSUPPORTED");
 error([]{run("uses Arrays;begin Arrays.Sort&<integer>(nil);end.");},"E_NIL");
 error([]{run("uses Arrays;begin WriteMatrix&<real>(nil);end.");},"E_NIL");
 error([]{run("uses Arrays;type F=procedure(a:array of integer;v:integer);begin var a:array of integer;var f:F:=FillByValue&<integer>;f(a,1);end.");},"E_NIL");
}
void metadata(){
 const auto original=abc::compile("uses Arrays;begin var unused:=new integer[1,1];WriteMatrix&<real>(new real[1,1]);end.");
 for(int mode=0;mode<3;++mode){auto p=original;for(auto& op:p.code)if(op.op==abc::Op::write_matrix){if(mode==0)op.operand=1;if(mode==1)op.operand=2;if(mode==2)op.type=abc::Type::integer;}error([&]{abc::verify(p);},"E_BYTECODE");}
}
}
int main(){try{reference();semantics();metadata();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" generic array checks, "<<failures<<" failures\n";return failures?1:0;}

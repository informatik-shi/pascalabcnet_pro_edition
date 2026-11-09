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
void reference(std::string_view base,int inputs,int expected_rows){
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::ifstream source(fixtures/(std::string(base)+"-program.pas"));check(bool(source),"Static indexed interface program exists");std::ostringstream text;text<<source.rdbuf();const auto program=abc::compile(text.str());
 std::ifstream input(fixtures/(std::string(base)+"-reference.tsv"));check(bool(input),"Independent indexed interface corpus exists");std::string row;int rows=0;
 while(std::getline(input,row)){
  if(!row.empty()&&row.back()=='\r')row.pop_back();if(row.empty()||row.front()=='#')continue;const auto f=split(row);if(f.size()!=static_cast<std::size_t>(inputs+2)){check(false,"Malformed indexed interface row");continue;}
  std::string data;for(int i=0;i<inputs;++i)data+=f[i]+" ";std::istringstream in(data);std::ostringstream out;std::string code="ok";
  try{abc::execute(program,in,out);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty()&&*e.what(),"Indexed interface error is bilingual");}
  check(code==f[inputs+1]&&out.str()==decode(f[inputs]),std::string(base)+" "+data+"expected "+f[inputs+1]+" got "+code+" output "+out.str());++rows;
 }
 check(rows==expected_rows,"All independent indexed interface outcomes checked");
}
void properties(){
 result("var calls:integer;function Index:integer;begin Result:=calls;Inc(calls);end;begin var i:IList<integer>:=Arr(1,2);i[Index()]+=9;Writeln(i[0],' ',i[1],' ',calls);end.","11 2 2\n");
 result("type I=interface property Cell[x,y:integer]:integer read write;default;end;G=class(I) private data:array[,] of integer:=new integer[3,3];function Get(x,y:integer):integer;begin Write('G');Result:=data[x,y];end;procedure Put(x,y,v:integer);begin Write('S');data[x,y]:=v;end;public property Cell[x,y:integer]:integer read Get write Put;default;end;var g:G:=new G;function Owner:G;begin Write('O');Result:=g;end;function Index:integer;begin Write('I');Result:=1;end;function Value:integer;begin Write('V');Result:=9;end;begin Owner()[Index(),2]:=Value();Writeln;Owner()[Index(),2]+=Value();Writeln;Writeln(g.Cell[1,2]);var i:I:=g;Writeln(i[1,2]);i[1,2]:=7;Writeln(g[1,2]);end.","OIVS\nOIOIGVS\nG18\nG18\nSG7\n");
 result("type I=interface property Cells[i:integer]:integer read write;default;end;J=interface(I) end;C=class(J) function Get(i:integer):integer:=i+1;procedure Put(i,v:integer);begin Writeln(i,' ',v);end;public property Cells[i:integer]:integer read Get write Put;end;begin var j:J:=new C;Writeln(j[3]);j[2]:=9;end.","4\n2 9\n");
 result("type C=class public n:integer;procedure Put(i,v:integer);begin n:=i+v;end;property Item[i:integer]:integer write Put;default;end;begin var c:=new C;c[2]:=7;Writeln(c.n);end.","9\n");
 result("type C=class function Get(i:integer):array of integer:=Arr(i,9);property Item[i:integer]:array of integer read Get;default;end;begin var c:=new C;Writeln(c[3][1]);end.","9\n");
 result("type C=class function Get(i:integer):integer:=i+4;property Item[i:integer]:integer read Get;default;end;D=class function Get:C:=new C;property Child:C read Get;end;begin var d:=new D;Writeln(d.Child[3]);end.","7\n");
 result("type C=class function Get(a,b,c:integer):integer:=a+b+c;property Item[a,b,c:integer]:integer read Get;default;end;begin var c:=new C;Writeln(c[1,2,3]);end.","6\n");
 result("type C=class function Get(key:string;i:integer):string:=key+IntToStr(i);property Named[key:string;i:integer]:string read Get;end;begin var c:=new C;Writeln(c.Named['x',7]);end.","x7\n");
 result("begin var a:=Arr(1,2);var i:IList<integer>:=a;i.Item[0]:=7;Writeln(a,' ',i.IsReadOnly);var c:ICollection<integer>:=i;var e:IEnumerable<integer>:=c;Writeln(e.ToArray);Writeln(TypeToTypeName(typeof(IList<integer>)));end.","[7,2] True\n[7,2]\nIList<integer>\n");
 for(const auto source:{"type C=class function Get(i:integer):integer:=i;property P[i:integer]:integer read Get;default;end;begin Write('bad');var c:=new C;c[0]:=2;end.","type C=class procedure Put(i,v:integer);begin end;property P[i:integer]:integer write Put;default;end;begin Write('bad');var c:=new C;Writeln(c[0]);end.","type C=class function Get(i:string):integer:=1;property P[i:integer]:integer read Get;end;begin Write('bad');end.","type C=class function Get(i:integer):integer:=1;property P[i:integer]:integer read Get;default;property Q[i:integer]:integer read Get;default;end;begin Write('bad');end.","type C=class function Get(i:integer):integer:=1;property P[i:integer]:integer read Get;end;begin Write('bad');var c:=new C;Writeln(c.P);end.","begin Write('bad');var c:IList<integer>:=new HashSet<integer>;end.","begin Write('bad');var i:IList<integer>:=Arr(1);Writeln(i['x']);end."}){
  std::ostringstream out;try{std::istringstream in;abc::execute(abc::compile(source),in,out);check(false,"Invalid indexer program rejected");}catch(const abc::Error& e){check(out.str().empty()&&!e.russian.empty(),"Static indexer rejection precedes output");}
 }
 error([]{run("begin var c:IList<integer>:=Arr(1);c.Insert(-1,2);end.");},"E_READONLY");
 error([]{run("begin var c:ICollection<integer>:=new HashSet<integer>;var i:=IList&<integer>(c);end.");},"E_CAST");
 error([]{run("begin var l:=new List<integer>(Arr(1));var i:IList<integer>:=l;var e:=i.GetEnumerator;i[0]:=2;e.MoveNext;end.");},"E_MODIFIED");
 abc::ExecutionOptions options;options.instruction_limit=40;error([&]{run("type C=class function Get(i:integer):integer:=Item[i];property Item[i:integer]:integer read Get;default;end;begin var c:=new C;Writeln(c[0]);end.",options);},"E_LIMIT");
 auto p=abc::compile("begin var i:IList<integer>:=Arr(1);Writeln(i[0]);end.");for(std::size_t n=0;n<p.types.size();++n)if(p.types[n].kind==abc::CompoundKind::list_interface){auto bad=p;bad.types[n].interfaces.edit().parents.clear();error([&]{abc::verify(bad);},"E_BYTECODE");bad=p;bad.types[n].key=abc::Type::string;error([&]{abc::verify(bad);},"E_BYTECODE");}
 p=abc::compile("type I=interface property Item[i:integer]:integer read write;default;end;begin end.");for(std::size_t n=0;n<p.types.size();++n)if(p.types[n].kind==abc::CompoundKind::user_interface){auto bad=p;bad.types[n].interfaces.edit().members[1].parameters[0].type=abc::Type::string;error([&]{abc::verify(bad);},"E_BYTECODE");bad=p;bad.types[n].interfaces.edit().members[0].parameters.clear();error([&]{abc::verify(bad);},"E_BYTECODE");}
}
}
int main(){try{reference("list-interface",5,456);reference("school-ilist",7,1152);properties();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" list interface checks, "<<failures<<" failures\n";return failures?1:0;}

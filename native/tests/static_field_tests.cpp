#include "abc/core.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
unsigned checks=0,failures=0;
void check(bool ok,std::string_view label){++checks;if(!ok){++failures;if(failures<=30)std::cerr<<"FAIL "<<label<<'\n';}}
std::string contents(const std::filesystem::path& p){std::ifstream in(p,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
void replace(std::string& text,std::string_view key,std::string_view value){for(auto p=text.find(key);p!=text.npos;p=text.find(key,p+value.size()))text.replace(p,key.size(),value);}
std::vector<std::string> split(std::string_view text){std::vector<std::string> out;std::size_t p=0;for(;;){const auto end=text.find('\t',p);out.emplace_back(text.substr(p,end==text.npos?end:end-p));if(end==text.npos)return out;p=end+1;}}
std::string decoded(std::string_view value){std::u16string out;for(std::size_t p=0;p<value.size();p+=4)out+=static_cast<char16_t>(std::stoul(std::string(value.substr(p,4)),nullptr,16));return abc::to_utf8(out);}
std::string run(std::string_view source,std::string_view input={}){std::istringstream in{std::string(input)};std::ostringstream out;abc::execute(abc::compile(source),in,out);return out.str();}
void result(std::string_view source,std::string_view expected){try{const auto actual=run(source);check(actual==expected,std::string(source)+" actual="+actual);}catch(const std::exception& e){check(false,std::string(source)+" "+e.what());}}
void error(std::string_view source,std::string_view code,std::string_view prefix={}){std::istringstream in;std::ostringstream out;try{abc::execute(abc::compile(source),in,out);check(false,"Expected "+std::string(code));}catch(const abc::Error& e){check(e.code==code&&out.str()==prefix&&*e.what()&&!e.russian.empty()&&e.location.line>0,std::string(source)+" actual="+e.code+" "+out.str());}}
void reference(){
 const auto path=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";const auto source=contents(path/"static-fields-template.pas");check(!source.empty(),"Static-field Pascal recipe exists");std::ifstream fixture(path/"static-fields-reference.tsv");check(bool(fixture),"Actual strict CLR class reference exists");std::string line;unsigned rows=0;
 while(std::getline(fixture,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line[0]=='#')continue;const auto f=split(line);if(f.size()!=6)throw std::runtime_error("Invalid static reference width");auto text=source;replace(text,"@LOW@",f[0]);replace(text,"@HIGH@",std::to_string(std::stoi(f[0])+std::stoi(f[1])-1));std::string values;for(int n=0;n<std::stoi(f[1]);++n){if(n)values+=',';values+="Element("+std::to_string(n)+")";}replace(text,"@VALUES@",values);try{const auto actual=run(text,f[2]+" "+f[3]+" "+f[4]);check(actual==decoded(f[5]),"Strict CLR read/store/reference/new/Low/copy/reentry row "+std::to_string(rows)+" actual="+actual);}catch(const std::exception& e){check(false,e.what());}++rows;
 }check(rows==160,"Every actual strict initializer execution checked");
}
void semantics(){
 result("type C=class static n:integer:=7;end;begin Writeln(C.n);C.n:=9;Writeln(C.n);end.","7\n9\n");
 result("type C=class static n:=7;end;begin Writeln(C.n);end.","7\n");
 result("type C=class static a:=b+1;static b:=2;end;begin Writeln(C.a,' ',C.b);end.","1 2\n");
 result("type C=class static a:array of integer:=(1,2);b:array of integer:=(3,4);end;begin var x:=new C;var y:=new C;x.b[0]:=99;Writeln(C.a[0],' ',x.b[0],' ',y.b[0]);end.","1 99 3\n");
 result("type R=record n:integer:=7;end;C=class static r:R;end;begin Writeln(C.r.n);C.r.n:=9;Writeln(C.r.n);end.","7\n9\n");
 result("type C=class static a:Func0<integer>:=()->7;end;begin Writeln(C.a());end.","7\n");
 result("function I:integer;begin Write('I');Result:=7;end;type C=class static n:integer:=I;end;procedure P;begin Inc(C.n);Writeln(C.n);end;begin P;P;end.","I8\n9\n");
 error("type C=class static n:integer:=1 div 0;end;begin Write('before');Writeln(C.n);end.","E_DIV_ZERO","before");
 error("type C=class static n:integer:=1;static n:integer:=2;end;begin Write('bad');end.","E_DUPLICATE");
 error("type C=class n:integer;static n:integer;end;begin Write('bad');end.","E_DUPLICATE");
 error("type C=class static n:=missing;end;begin Write('bad');end.","E_UNSUPPORTED");
 error("type C=class static a:=b;static b:=a;end;begin Write('bad');end.","E_TYPE");
 error("type C=class n:integer;static a:integer:=n;end;begin Write('bad');end.","E_TYPE");
 result("function E(n:integer):integer;begin Write('E');Write(a[0]);Result:=n;end;var a:array of integer:=(E(1),E(2));begin Writeln(a[0]);end.","E0E11\n");
 result("function E(n:integer):integer;begin Write('E');Write(a[0]);Result:=n;end;const a:array of integer=(E(1),E(2));begin Writeln(a[0]);end.","E0E11\n");
 result("type C=class data:array of integer:=(E(1),E(2));function E(n:integer):integer;begin Write('E');Write(data[0]);Result:=n;end;end;begin var o:=new C;Writeln(o.data[0]);end.","E0E11\n");
 result("type R=record data:array of integer:=(E(1),E(2));function E(n:integer):integer;begin Write('E');Write(data[0]);Result:=n;end;end;begin var o:R;Writeln(o.data[0]);end.","E0E11\n");
 result("var f:Func0<integer>;begin for var i:=0 to 1 do begin var a:array of integer:=(if i=0 then 41 else f(),i);if i=1 then Writeln(f());f:=()->a[0];end;Writeln(f());end.","41\n41\n");
 result("var a:array of integer:=(10,20);function E(n:integer):integer;begin Write('E');Write(a[0]);Result:=n;end;begin a:=(E(1),E(2));Writeln(a[0]);end.","E10E101\n");
 error("function E(n:integer):integer;begin Write('E');Write(a[0]);Result:=n;end;var a:=new integer[2](E(1),E(2));begin Writeln(a[0]);end.","E_NIL","E");
 auto original=abc::compile("type C=class static n:integer:=7;end;begin Writeln(C.n);end.");
 for(unsigned mutation=0;mutation<10;++mutation){auto p=original;auto& s=p.static_classes[0];if(mutation==0)s.owner=abc::Type::integer;if(mutation==1)s.state=p.locals.size();if(mutation==2)s.initializer=p.routines.size();if(mutation==3)s.fields[0].local=s.state;if(mutation==4)s.fields.push_back(s.fields[0]);if(mutation==5)p.locals[s.fields[0].local].global=false;if(mutation==6)p.routines[s.initializer].name="not-an-initializer";if(mutation==7)p.static_classes.push_back(s);if(mutation==8)p.static_classes.clear();if(mutation==9)s.fields.clear();try{abc::verify(p);check(false,"Invalid static metadata rejected");}catch(const abc::Error& e){check(e.code=="E_BYTECODE","Static storage metadata checked before execution");}}
}

void static_constructors() {
    result(R"(function Seed: integer;
begin Write('F'); Result := 4; end;
type C = class
  static x: integer := Seed;
  static constructor;
  begin Write('S', x); x += 2; end;
  constructor(n: integer); begin Write('C', n); end;
  class function ReadValue: integer := x;
end;
function Argument: integer; begin Write('A'); Result := 9; end;
begin
  Write('B'); var o := new C(Argument); Writeln(C.ReadValue);
  o := new C(3); Writeln(C.ReadValue);
end.)", "BAFS4C96\nC36\n");

    result(R"(type C = class
  static constructor := Write('S');
  static function F(n: integer): integer := n + 1;
  static property P: integer read 7;
end;
function Arg: integer; begin Write('A'); Result := 3; end;
begin
  var f: Func<integer, integer> := C.F;
  Write('B'); Writeln(f(Arg)); Writeln(C.P);
end.)", "BAS4\n7\n");

    result(R"(type C = class
  class constructor Create;
  class property P: integer read 7;
end;
class constructor C.Create;
begin
  var f: Func0<integer> := () -> C.P;
  Write('S', f());
  var inner := new C;
end;
begin
  Writeln(C.P); var outer := new C; Writeln(C.P);
end.)", "S77\n7\n");

    result(R"(type C = class static constructor := Write('unused'); end;
begin var o: C; assert(o = nil); var t := typeof(C); Write('B'); end.)", "B");
    result(R"(type A = class
  static constructor := Write('A', B.Value);
  static property Value: integer read 1;
end;
B = class
  static constructor := Write('B', A.Value);
  static property Value: integer read 2;
end;
begin Writeln(A.Value, B.Value); end.)", "B1A212\n");
    result(R"(type C = class
  static v: integer := 7;
  static constructor := exit;
end;
begin var o := new C; Writeln(C.v); end.)", "7\n");
    error("type C=class static constructor;end;begin Write('bad');end.", "E_NAME");
    error("type C=class end;static constructor C.Create;begin end;begin Write('bad');end.", "E_NAME");
    error("type C=class static constructor := exit;static constructor := exit;end;begin Write('bad');end.", "E_DUPLICATE");
    error("type C=class static constructor := exit;end;static constructor C.Create;begin end;begin Write('bad');end.", "E_DUPLICATE");
    error("type C=class static constructor(n:integer) := Write(n);end;begin Write('bad');end.", "E_TYPE");
    error("type C=class n:integer;static constructor := Inc(n);end;begin Write('bad');end.", "E_TYPE");
    error("type C=class static constructor := Write(1 div 0);end;begin Write('B');var o:=new C;end.", "E_DIV_ZERO", "B");
}
}
int main(){try{reference();semantics();static_constructors();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" static initialization checks, "<<failures<<" failures\n";return failures?1:0;}

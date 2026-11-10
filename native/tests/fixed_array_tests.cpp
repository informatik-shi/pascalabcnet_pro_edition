#include "abc/core.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
unsigned checks=0,failures=0;
std::filesystem::path folder;
void check(bool ok,std::string_view label){++checks;if(!ok){++failures;if(failures<=30)std::cerr<<"FAIL "<<label<<'\n';}}
std::string contents(const std::filesystem::path& p){std::ifstream in(p,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
void replace(std::string& text,std::string_view key,std::string_view value){for(auto pos=text.find(key);pos!=text.npos;pos=text.find(key,pos+value.size()))text.replace(pos,key.size(),value);}
std::vector<std::string> split(std::string_view text){std::vector<std::string> parts;std::size_t pos=0;for(;;){auto end=text.find('\t',pos);parts.emplace_back(text.substr(pos,end==text.npos?end:end-pos));if(end==text.npos)return parts;pos=end+1;}}
std::string unhex(std::string_view text){std::string out;for(std::size_t n=0;n<text.size();n+=2)out+=static_cast<char>(std::stoul(std::string(text.substr(n,2)),nullptr,16));return out;}
std::string decoded(std::string_view text){std::u16string out;for(std::size_t n=0;n<text.size();n+=4)out+=static_cast<char16_t>(std::stoul(std::string(text.substr(n,4)),nullptr,16));return abc::to_utf8(out);}
std::string run(std::string_view source){std::istringstream in;std::ostringstream out;abc::execute(abc::compile(source),in,out);return out.str();}
void result(std::string_view source,std::string_view expected){try{auto actual=run(source);check(actual==expected,std::string(source)+" actual="+actual);}catch(const std::exception& e){check(false,std::string(source)+" "+e.what());}}
void error(std::string_view source,std::string_view code,std::string_view prefix={},abc::ExecutionOptions options={}){std::istringstream in;std::ostringstream out;try{abc::execute(abc::compile(source),in,out,options);check(false,"Expected "+std::string(code));}catch(const abc::Error& e){check(e.code==code&&out.str()==prefix&&*e.what()&&!e.russian.empty()&&e.location.line>0,std::string(source)+" actual="+e.code+" "+out.str());}}
void reference(){
 const auto path=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";
 auto source=contents(path/"fixed-array-copy-template.pas");check(!source.empty(),"Pascal fixed copy reference exists");
 std::ifstream fixture(path/"fixed-array-copy-reference.tsv");check(bool(fixture),"Independent fixed copy traces exist");std::string line;unsigned rows=0;
 while(std::getline(fixture,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line[0]=='#')continue;auto fields=split(line);if(fields.size()!=6)throw std::runtime_error("Bad fixed reference row");auto program_text=source;replace(program_text,"@LOW@",fields[0]);replace(program_text,"@HIGH@",std::to_string(std::stoi(fields[0])+std::stoi(fields[1])-1));const auto program=abc::compile(program_text);std::istringstream in(fields[2]);std::ostringstream out;std::string code="ok";try{abc::execute(program,in,out);}catch(const abc::Error& e){code=e.code;check(*e.what()&&!e.russian.empty()&&e.location.line==26,"EOF reference diagnostic has both languages and original call line");}
  check(out.str()==decoded(fields[3]),"Initializer, Clone, const and typed-file trace row "+std::to_string(rows)+" actual="+out.str());check(contents(folder/"native-fixed-reference.tmp")==unhex(fields[4]),"Packed fixed record bytes row "+std::to_string(rows));check(code==fields[5],"Reference outcome row "+std::to_string(rows));++rows;
 }check(rows==18,"Every Framework and modern reference case checked");
}
void constant_reference(){
 const auto path=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";const auto source=contents(path/"array-constants-template.pas");check(!source.empty(),"Array constant Pascal recipe exists");std::ifstream fixture(path/"array-constants-reference.tsv");check(bool(fixture),"Independent constant-pool reference exists");std::string line;unsigned rows=0;
 while(std::getline(fixture,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line[0]=='#')continue;const auto f=split(line);if(f.size()!=5)throw std::runtime_error("Bad array constant reference row");auto program_text=source;replace(program_text,"@LOW@",f[0]);replace(program_text,"@HIGH@",std::to_string(std::stoi(f[0])+std::stoi(f[1])-1));std::string values,pairs;for(int n=0;n<std::stoi(f[1]);++n){if(n){values+=',';pairs+=',';}const auto value=n*11-17;values+=std::to_string(value);pairs+='('+std::to_string(value)+','+std::to_string(value+1)+')';}replace(program_text,"@VALUES@",values);replace(program_text,"@PAIRS@",pairs);std::istringstream in(f[2]+" "+f[3]);std::ostringstream out;try{abc::execute(abc::compile(program_text),in,out);check(out.str()==decoded(f[4]),"Static pools, owned copies and shared inner arrays row "+std::to_string(rows)+" actual="+out.str());}catch(const std::exception& e){check(false,e.what());}++rows;
 }check(rows==81,"Every independent static-pool execution checked");
}
void semantics(){
 result("type A=array[-2..1] of integer;begin var a:A:=(4,5,6,7);var b:=a;b[-2]:=99;Writeln(a[-2],' ',b[-2],' ',Low(a),' ',High(a),' ',a.Length);end.","4 99 -2 1 4\n");
 result("type A=array[7..7] of integer;begin var a:A:=(42);var b:A:=(a);Writeln(a[7],' ',b[7]);end.","42 42\n");
 result("type A=array[0..1] of integer;begin var a:A:=(2,3);var b:A:=(a);Writeln(b[0],' ',b[1]);end.","2 3\n");
 result("type A=array[1..2,3..4] of integer;begin var a:A:=((1,2),(3,4));var b:=a;b[1,3]:=77;Writeln(a[1,3],' ',b[1,3],' ',a[2][4]);end.","1 77 4\n");
 result("type A=array[1..1,3..3] of integer;begin var a:A:=((42));Writeln(a[1,3]);end.","42\n");
 result("type A=array['a'..'c'] of integer;begin var a:A;a['b']:=18;Writeln(a['b'],' ',Low(a),' ',High(a));end.","18 97 99\n");
 result("type A=array[false..true] of integer;begin var a:A;a[true]:=15;Writeln(a[true],' ',Low(a),' ',High(a));end.","15 0 1\n");
 result("type E=(red,green,blue);A=array[E] of integer;begin var a:A;a[green]:=17;Writeln(a[green],' ',Low(a),' ',High(a));end.","17 0 2\n");
 result("type E=(red,green,blue);A=array[green..blue] of integer;begin var a:A;a[green]:=17;Writeln(a[green],' ',Low(a),' ',High(a));end.","17 1 2\n");
 result("type A=array[byte] of integer;begin var a:A;a[255]:=19;Writeln(a[255],' ',a.Length);end.","19 256\n");
 result("type A=array[boolean] of integer;begin var a:A;a[true]:=19;Writeln(a[true],' ',a.Length);end.","19 2\n");
 result("type A=array[3..4] of integer;function F:A;begin Write('bad');end;begin Writeln(Low(F),' ',High(F),' ',Low(A),' ',High(A));end.","3 4 3 4\n");
 result("type A=array[3..4] of integer;begin var a:A:=(3,4);foreach var n in a do Write(n);Writeln;var b:=a.Clone;b[3]:=99;Writeln(a[3],' ',b[3]);end.","34\n3 99\n");
 result("type A=array[1..2] of integer;procedure P;procedure C(const a:A);begin Writeln(a[1]);end;begin var a:A:=(7,8);C(a);end;begin P;end.","7\n");
 result("type A=array[1..2] of integer;begin var a:A:=(3,4);var b:=a;Writeln(a=b,' ',a<>b);b[2]:=5;Writeln(a=b,' ',a<>b);end.","True False\nFalse True\n");
 result("type A=array[1..2] of array of integer;begin var a:A;a[1]:=Arr(1,2);var b:=a;b[1][0]:=99;Writeln(a[1][0]);end.","99\n");
 result("type A=array[1..2] of integer;R=record a:A;end;begin var r:R;r.a[1]:=17;var s:=r;s.a[1]:=99;Writeln(r.a[1],' ',s.a[1]);end.","17 99\n");
 error("type A=array[2..1] of integer;begin Write('bad');end.","E_RANGE");
 error("type A=array[0..1048576] of integer;begin Write('bad');end.","E_LIMIT");
 error("var n:=2;type A=array[0..n] of integer;begin Write('bad');end.","E_NAME");
 error("type A=array[1..2] of integer;begin var a:A;Writeln(a[0]);end.","E_BOUNDS");
 error("type A=array[1..2] of integer;begin var a:A:=nil;end.","E_TYPE");
 error("type A=array[1..2] of integer;begin var a:A:=default(A);end.","E_NIL");
 error("type A=array[1..2] of integer;function Index:integer;begin Write('i');Result:=0;end;begin Writeln(default(A)[Index]);end.","E_NIL","i");
 result("var calls:integer;function Bump:integer;begin Inc(calls);Result:=calls;end;type R=record n:integer:=Bump;end;A=array[1..2] of R;procedure P;procedure C(const a:A);begin Writeln(calls);end;procedure V(a:A);begin Writeln(calls);end;begin var a:A;C(a);V(a);end;begin P;end.","2\n4\n");
 result("type A=array[1..2] of integer;procedure P(var a:A);begin a[1]:=7;end;begin var a:A;P(a);Writeln(a[1]);end.","7\n");
 result("type A=array[-1..1] of integer;function F:A;begin Result[-1]:=42;end;begin var a:=F;Writeln(a[-1]);end.","42\n");
 result("type A=array[1..2] of integer;function Index:integer;begin Write('i');Result:=1;end;begin var a:A;a[Index]+=3;Writeln(a[1]);end.","i3\n");
 result("begin var a:array of integer:=(1,2,3,4,5,6,7,8,9,10);Writeln(a.Length,' ',a[9]);end.","10 10\n");
 result("begin var a:array of integer:=(42);Writeln(a.Length,' ',a[0]);end.","1 42\n");
 result("type A=array[-4..5] of integer;begin var a:A:=(1,2,3,4,5,6,7,8,9,10);Writeln(a[-4],' ',a[5]);end.","1 10\n");
 result("begin var a:array of array of integer:=((1,2),(3,4));Writeln(a[0][1],' ',a[1][0]);end.","2 3\n");
 result("type A=array[byte(1)..byte(2)] of integer;begin var a:A;var i:=1;a[i]:=42;Writeln(a[i]);end.","42\n");
 result("type A=array[byte(1)..int64(2)] of integer;begin var a:A;var i:int64:=2;a[i]:=42;Writeln(a[i]);end.","42\n");
 result("type A=array[longword(1)..2] of integer;begin var a:A;var i:int64:=2;a[i]:=42;Writeln(a[i]);end.","42\n");
 result("var calls:integer;function Bump:integer;begin Inc(calls);Result:=calls;end;type A=array[1..2] of integer;R=record n:integer:=Bump;a:A;end;begin var r,s:R;r.a[1]:=7;var a:array of R:=(r,s);a[0].a[1]:=99;Writeln(calls,' ',r.a[1],' ',a[0].a[1]);end.","2 7 99\n");
 result("function F(n:integer):integer;begin Write(n);Result:=n;end;begin var a:array of integer:=(F(1),F(2),F(3));Writeln(' ',a.Length);end.","123 3\n");
 error("begin var t:=(1,2,3,4,5,6,7,8);end.","E_UNSUPPORTED");
 abc::ExecutionOptions limited;limited.allocation_limit=8;error("function F:integer;begin Write('bad');Result:=1;end;begin Write('before');var a:array of integer:=(F,F,F,F,F,F,F,F,F,F);end.","E_LIMIT","before",limited);
 for(auto source:{"begin var a:=new integer[1];SetLength(a,2);end.","begin var a:=new integer[1,1];end."}){auto p=abc::compile(source);for(auto& instruction:p.code)if(instruction.op==abc::Op::resize_array||(instruction.op==abc::Op::array_new&&p.types[static_cast<unsigned>(instruction.type)-64U].rank==2)){instruction.operand=~std::size_t{0};break;}try{abc::verify(p);check(false,"Raw storage marker rejected outside rank-one creation");}catch(const abc::Error& e){check(e.code=="E_BYTECODE","Raw storage operand is verified");}}
 result("type A=array[1..3] of integer;const a:A=(2,3,4);begin Writeln(a[2]);var b:=a;b[2]:=99;Writeln(a[2],' ',b[2]);end.","3\n3 99\n");
 result("const a:array of integer=(2,3,4);begin Writeln(a[1]);end.","3\n");
 result("type A=array[1..2,3..4] of integer;const a:A=((1,2),(3,4));begin Writeln(a[2,4]);end.","4\n");
 result("const a:array of array of integer=((1,2),(3,4));begin Writeln(a[1,0]);end.","3\n");
 result("procedure P;const a:array of integer=(7,8);begin Writeln(a[0]);end;begin P;P;end.","7\n7\n");
 error("type A=array[1..2] of integer;const a:A=(1,2);begin Write('bad');a[1]:=3;end.","E_TYPE");
 error("const a:array of integer=(1,2);begin Write('bad');a[0]:=3;end.","E_TYPE");
 result("function F:integer;begin Write('F');Result:=1;end;const a:array of integer=(F,2);begin Writeln(a[0]);end.","F1\n");
 result("const a:array of integer=(Random(1),2);begin Writeln(a[0]);end.","0\n");
 result("type A=array[1..2] of array of integer;begin var a:A;a[1]:=Arr(3,4);a[1,0]:=7;Writeln(a[1,0]);end.","7\n");
 result("type A=array[1..2] of integer;begin var a:array of A:=((3,4),(5,6));a[0,1]:=7;Writeln(a[0,1]);end.","7\n");
 result("begin var a:array of array[,] of integer;SetLength(a,1);a[0]:=new integer[2,2];a[0,1,1]:=7;Writeln(a[0,1,1]);end.","7\n");
 result("const a:array of integer=(1,3);n=a[1]+2;type A=array[a[0]..n] of integer;begin var b:A;b[5]:=7;Writeln(n,' ',Low(b),' ',High(b),' ',b[5]);end.","5 1 5 7\n");
 result("const a:array of integer=(1,2);begin var b:=a;b[0]:=99;Writeln(a[0]);end.","99\n");
 result("procedure P;const a:array of integer=(1,2);begin var b:=a;Inc(b[0]);Writeln(a[0]);end;begin P;P;end.","2\n3\n");
 result("type A=array[-1..0] of integer;const a:A=(3,4);n=a[-1]+1;begin Writeln(n);end.","4\n");
 result("const a:array of array of integer=((2,3),(4,5));n=a[1,1];type A=array[1..n] of integer;begin var b:A;Writeln(b.Length);end.","5\n");
 result("const a:array of integer=nil;begin Writeln(a=nil);end.","True\n");
 error("const a:array of integer=(1,2);begin Write('before');Writeln(a[9]);end.","E_BOUNDS","before");
 result("const a:array[1..2] of integer=(7,8);begin Writeln(a[1]);end.","7\n");
 result("type E=(red,green,blue);const a:array of E=(red,blue);n=Ord(a[1]);begin Writeln(n);end.","2\n");
 result("procedure P;const a:array of integer=(1,2);begin var b:=a;Inc(b[0]);Writeln(a[0]);end;procedure Q;const a:array of integer=(1,2);begin var b:=a;Inc(b[0]);Writeln(a[0]);end;begin P;Q;P;Q;end.","2\n2\n3\n3\n");
 error("var n:=1;const a:array of integer=(n,2);begin Write('bad');end.","E_TYPE");
 {std::istringstream in("17");std::ostringstream out;abc::execute(abc::compile("const a:array of integer=(ReadInteger,2);begin Writeln(a[0]);end."),in,out);check(out.str()=="17\n","Standard input initializes array constants at execution, never compilation");}
 result("const a=Arr(1,2);n=a[0]+a[1];begin Writeln(n);end.","3\n");
 error("function F:array of integer:=Arr(1,2);const a=F;begin Write('bad');end.","E_TYPE");
 result("function F:integer;begin Write('F');Result:=3;end;const a:array of integer=(F+1,2);begin Writeln(a[0]);end.","F4\n");
 result("const a=new integer[3];begin Writeln(a.Length);end.","3\n");
 error("const a=ReadAllLines('constant-missing-file-xyz');begin Write('bad');end.","E_IO");
 result("var calls:integer;function F:integer;begin Inc(calls);Result:=calls;end;const a:array of integer=(F,2);begin Writeln(calls,' ',a[0]);end.","1 1\n");
 result("var calls:integer:=5;function F:integer;begin Inc(calls);Result:=calls;end;const a:array of integer=(F,2);begin Writeln(calls,' ',a[0]);end.","5 1\n");
 auto original=abc::compile("type A=array[1..2] of integer;begin var a:A;a[1]:=3;end.");
 for(unsigned change=0;change<5;++change){auto p=original;if(change==0)p.fixed_arrays.clear();if(change==1)p.fixed_arrays[0].index=static_cast<abc::Type>(999999);if(change==2)p.fixed_arrays[0].upper=0;if(change==3)p.fixed_arrays.push_back(p.fixed_arrays[0]);if(change==4)p.types[static_cast<unsigned>(p.fixed_arrays[0].type)-64U].key=abc::Type::integer;try{abc::verify(p);check(false,"Forged fixed metadata rejected");}catch(const abc::Error& e){check(e.code=="E_BYTECODE","Invalid fixed shape fails verification");}}
 if(sizeof(void*)==8)check(sizeof(abc::Parameter)==16,"const marker fits existing parameter padding");
}
}
int main(){const auto previous=std::filesystem::current_path();folder=std::filesystem::temp_directory_path()/("ABC native fixed "+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directory(folder);std::filesystem::current_path(folder);try{reference();constant_reference();semantics();}catch(const std::exception& e){check(false,e.what());}std::filesystem::current_path(previous);std::error_code ignored;std::filesystem::remove_all(folder,ignored);std::cout<<checks<<" fixed array checks, "<<failures<<" failures\n";return failures?1:0;}

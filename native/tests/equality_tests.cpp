#include "abc/core.hpp"
#include "../src/equality.hpp"
#include "../src/runtime_values.hpp"
#include <bit>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
namespace {
int checks=0,failures=0;void check(bool ok,std::string_view message){++checks;if(!ok){++failures;if(failures<=20)std::cerr<<"FAIL "<<message<<'\n';}}
std::vector<std::string> split(std::string_view s,char sep='\t'){std::vector<std::string> result;std::size_t start=0;for(;;){auto end=s.find(sep,start);result.emplace_back(s.substr(start,end==s.npos ? s.size()-start : end-start));if(end==s.npos)break;start=end+1;}return result;}
std::string run(std::string_view source,abc::ExecutionOptions options={}){std::istringstream input;std::ostringstream output;abc::execute(abc::compile(source),input,output,options);return output.str();}
void result(std::string_view source,std::string_view expected){try{check(run(source)==expected,std::string(source));}catch(const std::exception& e){check(false,std::string(source)+" -> "+e.what());}}
template<class F>void error(F action,std::string_view code){try{action();check(false,std::string("Expected ")+std::string(code));}catch(const abc::Error& e){check(e.code==code && !e.russian.empty() && *e.what(),std::string(code)+" -> "+e.code+" "+e.what());}catch(const std::exception& e){check(false,e.what());}}
std::uint64_t hex(std::string_view s){std::uint64_t n=0;auto r=std::from_chars(s.data(),s.data()+s.size(),n,16);if(r.ec!=std::errc{})throw std::runtime_error("Bad hex input");return n;}
abc::Value scalar(abc::Type type,std::string_view s){
 if(type==abc::Type::single)return std::bit_cast<float>(static_cast<std::uint32_t>(hex(s)));if(type==abc::Type::real)return std::bit_cast<double>(hex(s));
 if(type==abc::Type::string){if(s=="nil")return abc::NilString{};std::u16string value;if(s!="empty")for(const auto& c:split(s,','))value.push_back(static_cast<char16_t>(hex(c)));return value;}
 if(type==abc::Type::big_integer)return abc::BigInteger::parse(s);
 const auto text=std::string(s);if(type==abc::Type::uint64)return static_cast<std::uint64_t>(std::stoull(text));if(type==abc::Type::longword)return static_cast<std::uint32_t>(std::stoul(text));
 const auto n=std::stoll(text);switch(type){case abc::Type::integer:return static_cast<std::int32_t>(n);case abc::Type::int64:return static_cast<std::int64_t>(n);case abc::Type::word:return static_cast<std::uint16_t>(n);case abc::Type::smallint:return static_cast<std::int16_t>(n);case abc::Type::byte:return static_cast<std::uint8_t>(n);case abc::Type::shortint:return static_cast<std::int8_t>(n);case abc::Type::character:return static_cast<char16_t>(n);case abc::Type::boolean:return n!=0;default:return abc::EnumValue{type,static_cast<std::int32_t>(n)};}
}
void scalar_reference(){
 std::ifstream input(std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures"/"equality-reference.tsv");check(bool(input),"Independent scalar equality corpus present");std::map<std::string,abc::Program> programs;std::string line;int rows=0;
 while(std::getline(input,line)){if(!line.empty() && line.back()=='\r')line.pop_back();if(line.empty() || line.front()=='#')continue;auto f=split(line);if(f.size()!=7){check(false,"Invalid scalar equality row");continue;}auto at=programs.find(f[0]);if(at==programs.end()){auto type=f[0]=="enum" ? "Number" : f[0];auto source=std::string(f[0]=="enum" ? "type Number=(Zero,One,Two);" : "")+"begin var c:=EqualityComparer<"+type+">.Default;end.";at=programs.emplace(f[0],abc::compile(source)).first;}
  const auto& p=at->second;abc::Type owner=abc::Type::invalid;for(const auto& op:p.code)if(op.op==abc::Op::comparer_create)owner=op.type;check(abc::compound(owner),"Closed equality factory metadata available");const auto type=abc::type_info(p,owner).element;abc::Values values{p,1'048'576};const auto a=scalar(type,f[1]),b=scalar(type,f[2]);
  const auto equal=abc::equality_value(values,type,a,b,{});const auto hasha=abc::equality_hash(values,type,a,{}),hashb=abc::equality_hash(values,type,b,{});const auto implementation=abc::equality_implementation(p,type);const auto label=implementation==18 ? "G" : implementation==19 ? "O" : implementation==20 ? "B" : "E";
  check(equal==(f[3]=="True"),f[0]+" "+f[1]+" "+f[2]+" Equals");check(hasha==std::stoi(f[4]),f[0]+" "+f[1]+" hash expected "+f[4]+" got "+std::to_string(hasha));check(hashb==std::stoi(f[5]),f[0]+" "+f[2]+" second hash");check(label==f[6],"Concrete default equality implementation");++rows;
 }check(rows==942,"Complete scalar equality/hash corpus consumed");
}
void class_reference(){
 const auto dir=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::ifstream source(dir/"equality-classes-program.pas");check(bool(source),"User equality source present");std::ostringstream text;text<<source.rdbuf();const auto p=abc::compile(text.str());std::ifstream input(dir/"equality-classes-reference.tsv");check(bool(input),"Independent user equality corpus present");std::string line;int rows=0;
 while(std::getline(input,line)){if(!line.empty() && line.back()=='\r')line.pop_back();if(line.empty() || line.front()=='#')continue;auto f=split(line);if(f.size()!=6){check(false,"Bad class equality row");continue;}std::istringstream in(f[0]+" "+f[1]+" "+f[2]+" "+f[3]+"\n");std::ostringstream out;std::string code="ok";try{abc::execute(p,in,out);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty(),"Equality callback fault bilingual");}check(out.str()==f[4] && code==f[5],"class scenario "+f[0]+" a "+f[1]+" b "+f[2]+" mode "+f[3]+" expected "+f[4]+" "+f[5]+" got "+out.str()+" "+code);++rows;
 }check(rows==1125,"All exact typed/nil/user/mutation/fault equality traces checked");
}
void semantics(){
 result("begin var e:=EqualityComparer<integer>.Default;var a:=e;var q:IEqualityComparer<integer>:=e;Writeln(e=EqualityComparer<integer>.Default,' ',q.Equals(3,3),' ',q.GetHashCode(-3),' ',TypeName(a));end.","True True -3 GenericEqualityComparer<integer>\n");
 result("begin var a:=EqualityComparer<byte>.Default;var b:=System.Collections.Generic.EqualityComparer<string>.Default;Writeln(TypeName(a),' ',TypeName(b),' ',b.Equals(nil,nil),' ',b.Equals(nil,''),' ',b.GetHashCode(nil));end.","ByteEqualityComparer GenericEqualityComparer<string> True False 0\n");
 result("type C=class(IEquatable<C>) public n:integer;constructor Create(n:integer);begin Self.n:=n;end;function Equals(x:C):boolean;begin Write('E;');Result:=n=x.n;end;end;begin var a:=new C(7);var b:=new C(7);var e:=EqualityComparer<C>.Default;Writeln(e.Equals(a,b),' ',e.Equals(a,a),' ',e.Equals(a,nil),' ',e.Equals(nil,nil),' ',TypeName(e));end.","E;E;True True False True GenericEqualityComparer<C>\n");
 result("type R=record n:integer;end;Eq=class(IEqualityComparer<R>) public function Equals(a,b:R):boolean;begin Result:=a.n=b.n;a.n:=99;end;function GetHashCode(a:R):integer;begin Result:=a.n;a.n:=99;end;end;begin var a,b:R;a.n:=7;b.n:=7;var e:IEqualityComparer<R>:=new Eq;Writeln(e.Equals(a,b),' ',e.GetHashCode(a),' ',a.n,' ',b.n);end.","True 7 7 7\n");
 result("type I=interface(IEqualityComparer<integer>) end;C=class(I) public function Equals(a,b:integer):boolean:=a mod 2=b mod 2;function GetHashCode(a:integer):integer:=a mod 2;end;begin var e:I:=new C;var q:IEqualityComparer<integer>:=e;Writeln(q.Equals(1,3),' ',q.GetHashCode(5));end.","True 1\n");
 for(const auto source:{"begin new EqualityComparer<integer>;end.","begin EqualityComparer<integer>.Default.Equals(1,'bad');end.","begin var q:IEqualityComparer<integer>:=EqualityComparer<real>.Default;end.","type C=class(IEquatable<integer>) public function Equals(a:integer):integer:=a;end;begin end."})error([&]{abc::compile(source);},"E_TYPE");
 error([]{abc::compile("begin EqualityComparer<integer>.Create((a,b)->a=b);end.");},"E_NAME");
 error([]{run("begin var e:IEqualityComparer<integer>;e.Equals(1,1);end.");},"E_NIL");
 error([]{run("type C=class(IEquatable<C>) public function Equals(x:C):boolean;begin var zero:=0;Result:=1 div zero=0;end;end;begin var a:=new C;EqualityComparer<C>.Default.Equals(a,a);end.");},"E_DIV_ZERO");
}
void downcasts(){
 error([]{abc::compile("type I=interface end;C=class end;function F(x:I):C:=C(x);begin end.");},"E_TYPE");
 result("type C=class(IEquatable<C>) public n:integer;function Equals(x:C):boolean:=Self=x;end;begin var c:=new C;c.n:=7;var e:IEquatable<C>:=c;var d:=C(e);Writeln(c=d,' ',d.n);var nilvalue:IEquatable<C>;Writeln(C(nilvalue)=nil);end.","True 7\nTrue\n");
 error([]{run("type C=class(IEquatable<C>) public function Equals(x:C):boolean:=False;end;D=class(IEquatable<C>) public function Equals(x:C):boolean:=False;end;begin var value:IEquatable<C>:=new D;C(value);end.");},"E_CAST");
}
void metadata(){
 auto p=abc::compile("begin var e:=EqualityComparer<integer>.Default;Writeln(e.Equals(1,2));end.");abc::verify(p);for(int mode=0;mode<4;++mode){auto bad=p;if(mode==0){for(auto& op:bad.code)if(op.op==abc::Op::comparer_create){op.operand=1;break;}}if(mode==1){for(auto& t:bad.types)if(t.kind==abc::CompoundKind::equality_comparer)t.name="wrong";}if(mode==2){for(auto& t:bad.types)if(t.kind==abc::CompoundKind::equality_comparer_interface)t.element=abc::Type::invalid;}if(mode==3){for(auto& op:bad.code)if(op.op==abc::Op::interface_call){op.operand=2;break;}}error([&]{abc::verify(bad);},"E_BYTECODE");}
}
void invalid_class_cast_metadata(){
 auto p=abc::compile("type C=class(IEquatable<C>) public function Equals(x:C):boolean:=False;end;D=class end;begin var c:=new C;var i:IEquatable<C>:=c;var a:=C(i);end.");abc::Type invalid=abc::Type::invalid;for(std::size_t n=0;n<p.types.size();++n)if(p.types[n].name=="d")invalid=static_cast<abc::Type>(64+n);check(abc::compound(invalid),"Unrelated class metadata exists");for(auto& op:p.code)if(op.op==abc::Op::interface_cast && (op.operand&1U)){op.operand=(static_cast<std::size_t>(invalid)<<1U)|1U;break;}error([&]{abc::verify(p);},"E_BYTECODE");
}
void limits(){
 error([]{run("type C=class(IEquatable<C>) public function Equals(x:C):boolean:=EqualityComparer<C>.Default.Equals(Self,x);end;begin var a:=new C;EqualityComparer<C>.Default.Equals(a,a);end.");},"E_LIMIT");
 const auto source="type C=class(IEquatable<C>) public function Equals(x:C):boolean;begin while True do begin end;Result:=False;end;end;begin var a:=new C;EqualityComparer<C>.Default.Equals(a,a);end.";
 for(int mode=0;mode<2;++mode){abc::ExecutionOptions o;int count=0;if(mode)o.cancelled=[&]{return ++count>100;};else o.instruction_limit=300;error([&]{run(source,o);},mode?"E_CANCELLED":"E_LIMIT");}
}
}
int main(){try{scalar_reference();class_reference();semantics();downcasts();metadata();invalid_class_cast_metadata();limits();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" equality checks, "<<failures<<" failures\n";return failures?1:0;}

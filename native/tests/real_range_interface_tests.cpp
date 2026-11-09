#include "abc/core.hpp"
#include "../src/interfaces.hpp"
#include "../src/record_interface.hpp"
#include "../src/runtime_values.hpp"
#include "../src/real_interval.hpp"
#include "../src/equality.hpp"
#include "../src/list_operations.hpp"
#include <array>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
using abc::Type;using abc::Value;using abc::CompoundKind;
int checks=0,failures=0;
void check(bool ok,std::string_view label){++checks;if(!ok){++failures;if(failures<=20)std::cerr<<"FAIL "<<label<<'\n';}}
std::string boolean(bool value){return value?"True":"False";}
template<class F>void error(F action,std::string_view code){try{action();check(false,"Expected "+std::string(code));}catch(const abc::Error& e){check(e.code==code&&!e.russian.empty()&&*e.what(),std::string(e.what())+" "+e.code);}}
std::vector<std::string> split(std::string_view text){std::vector<std::string> f;std::size_t n=0;for(;;){const auto end=text.find('\t',n);f.emplace_back(text.substr(n,end==text.npos?end:end-n));if(end==text.npos)return f;n=end+1;}}
std::string decode(std::string_view hex){if(hex=="-")return {};std::u16string value;for(std::size_t n=0;n<hex.size();n+=4)value.push_back(static_cast<char16_t>(std::stoul(std::string(hex.substr(n,4)),nullptr,16)));return abc::to_utf8(value);}
struct Types{Type range=Type::invalid,eq=Type::invalid,listR=Type::invalid,listE=Type::invalid,setR=Type::invalid,setE=Type::invalid,tuple=Type::invalid;};
Types types(const abc::Program& p){Types t;for(std::size_t n=0;n<p.types.size();++n)if(p.types[n].native_record==abc::NativeRecord::real_interval)t.range=static_cast<Type>(64+n);for(std::size_t n=0;n<p.types.size();++n)if(p.types[n].kind==CompoundKind::equatable_interface&&p.types[n].element==t.range)t.eq=static_cast<Type>(64+n);for(std::size_t n=0;n<p.types.size();++n){const auto id=static_cast<Type>(64+n);const auto& type=p.types[n];if(type.kind==CompoundKind::tuple&&type.fields.size()==2&&type.fields[0].type==t.range)t.tuple=id;if(type.kind==CompoundKind::list){if(type.element==t.range)t.listR=id;if(type.element==t.eq)t.listE=id;}if(type.kind==CompoundKind::hash_set){if(type.element==t.range)t.setR=id;if(type.element==t.eq)t.setE=id;}}return t;}
bool contains(abc::Values& values,Type list,const Value& a,const Value& b){auto object=std::get<std::shared_ptr<abc::Aggregate>>(values.zero(list,true));values.list_append_cell(object,values.cell(abc::type_info(values.program,list).element,values.copy(a)));return abc::list_contains(values,object,b,{});}
std::int32_t count(abc::Values& values,Type set,const Value& a,const Value& b){const auto object=abc::hash_set_invoke(values,set,Type::invalid,abc::HashSetMethod::construct,{},{});for(const auto* item:{&a,&b}){const std::array<Value,2> args{object,*item};(void)abc::hash_set_invoke(values,set,Type::invalid,abc::HashSetMethod::add,args,{});}const std::array<Value,1> args{object};return std::get<std::int32_t>(abc::hash_set_invoke(values,set,Type::invalid,abc::HashSetMethod::count,args,{}));}
void native(const abc::Program& p,const Types& t,const std::vector<std::string>& f,std::ostream& out){
 abc::Values values{p,100000};const auto make=[&](int offset)->Value{auto r=std::get<std::shared_ptr<abc::Aggregate>>(values.zero(t.range));for(int n=0;n<2;++n)values.write(r->items[n],std::bit_cast<double>(std::stoull(f[offset+n],nullptr,16)));return r;};
 const auto ra=make(3),rb=make(5);Value a=abc::interface_cast(values,t.range,t.eq,false,ra),b=abc::interface_cast(values,t.range,t.eq,false,rb);const auto nil=std::stoi(f[2]);if(nil&1)a=values.zero(t.eq);if(nil&2)b=values.zero(t.eq);const auto same=a;
 const std::array<Value,2> pair{ra,rb};out<<boolean(std::get<bool>(abc::real_interval_invoke(values,t.range,abc::RealIntervalMethod::equal,pair)))<<'\n';
 out<<boolean(abc::equality_value(values,t.range,ra,rb,{}))<<'\n';out<<boolean(values.equal(a,same))<<' '<<boolean(values.equal(a,b))<<'\n';
 out<<boolean(abc::equality_value(values,t.eq,a,b,{}))<<' '<<boolean(abc::equality_hash(values,t.eq,a,{})==abc::equality_hash(values,t.eq,b,{}))<<'\n';
 out<<boolean(abc::equality_value(values,t.eq,a,a,{}))<<'\n';
 const auto pairValue=[&](const Value& r)->Value{auto pair=std::get<std::shared_ptr<abc::Aggregate>>(values.zero(t.tuple,true));pair->items.push_back(values.cell(t.range,values.copy(r)));pair->items.push_back(values.cell(Type::integer,std::int32_t{1}));return pair;};out<<boolean(abc::equality_value(values,t.tuple,pairValue(ra),pairValue(rb),{}))<<'\n';
 out<<boolean(contains(values,t.listR,ra,rb))<<' '<<count(values,t.setR,ra,rb)<<'\n';out<<boolean(contains(values,t.listE,a,b))<<' '<<count(values,t.setE,a,b)<<'\n';
 const std::array<Value,1> arg{rb};out<<boolean(std::get<bool>(abc::interface_invoke(values,t.eq,0,a,arg,{})))<<'\n';
 const std::array<Value,2> copied{abc::interface_cast(values,t.eq,t.range,true,a),ra};out<<boolean(std::get<bool>(abc::real_interval_invoke(values,t.range,abc::RealIntervalMethod::equal,copied)))<<'\n';
}
void reference(){
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::ifstream source(fixtures/"real-range-interface-program.pas");check(bool(source),"Typed finite RealRange fixture exists");std::ostringstream text;text<<source.rdbuf();const auto p=abc::compile(text.str());const auto t=types(p);
 check(t.eq!=Type::invalid&&t.listR!=Type::invalid&&t.listE!=Type::invalid&&t.setR!=Type::invalid&&t.setE!=Type::invalid&&t.tuple!=Type::invalid,"Closed native RealRange contracts and consumer types exist");
 std::ifstream data(fixtures/"real-range-interface-reference.tsv");check(bool(data),"Independent RealRange corpus exists");std::string row;int rows=0,sourceRows=0;
 while(std::getline(data,row)){if(!row.empty()&&row.back()=='\r')row.pop_back();if(row.empty()||row.front()=='#')continue;const auto f=split(row);if(f.size()!=9){check(false,"Malformed RealRange row");continue;}
  std::ostringstream out;std::string code="ok";try{native(p,t,f,out);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty(),"Bilingual runtime RealRange error");}check(code==f[8]&&out.str()==decode(f[7]),row+" runtime "+code+" "+out.str());++rows;
  if(std::stoi(f[0])>=8||std::stoi(f[1])>=8)continue;std::istringstream in(f[0]+" "+f[1]+" "+f[2]);std::ostringstream printed;code="ok";try{abc::execute(p,in,printed);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty(),"Bilingual source RealRange error");}check(code==f[8]&&printed.str()==decode(f[7]),row+" source "+code+" "+printed.str());++sourceRows;
 }check(rows==900&&sourceRows==256,"All 900 runtime and 256 source outcomes checked");
}
std::string run(std::string_view source){std::istringstream in;std::ostringstream out;abc::execute(abc::compile(source),in,out);return out.str();}
void semantics(){
 check(run("type E=IEquatable<RealRange>;begin var r:=new RealRange(1,2);var a:E:=r;var b:=a;r:=new RealRange(3,4);var old:=RealRange(a);Writeln(a=b,' ',old.Low,' ',old.High);Writeln(TypeToTypeName(a.GetType));end.")=="True 1 2\nRealRange\n","RealRange copied box lifetime, unboxing and actual type");
 error([]{run("type R=RealRange;E=IEquatable<R>;C=class(E) public function Equals(r:R):boolean:=true;end;begin var e:E:=new C;var r:=R(e);end.");},"E_CAST");
 error([]{run("type E=IEquatable<RealRange>;begin var a:E:=nil;var r:=RealRange(a);end.");},"E_NIL");
 for(const auto code:{"begin Write('bad');var c:ICollection<real>:=new RealRange(1,2);end.","begin Write('bad');var c:IEquatable<IntRange>:=new RealRange(1,2);end.","begin Write('bad');var c:ICollection<integer>:=new IntRange(1,2);var r:=RealRange(c);end."}){std::istringstream in;std::ostringstream out;try{abc::execute(abc::compile(code),in,out);check(false,"Invalid RealRange contract rejected");}catch(const abc::Error& e){check(out.str().empty()&&!e.russian.empty(),"Static RealRange error before output");}}
 auto p=abc::compile("type E=IEquatable<RealRange>;begin var a:E:=new RealRange(1,2);Writeln(a.Equals(new RealRange(1,2)));end.");auto t=types(p);abc::Values values{p,1000};auto value=abc::interface_cast(values,t.range,t.eq,false,values.zero(t.range));abc::ExecutionOptions limited;limited.instruction_limit=0;error([&]{abc::interface_invoke(values,t.eq,0,value,std::array<Value,1>{values.zero(t.range)},limited);},"E_LIMIT");
 auto bad=p;bad.types[static_cast<unsigned>(t.range)-64U].element=Type::integer;error([&]{abc::verify(bad);},"E_BYTECODE");
}
}
int main(){try{reference();semantics();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" RealRange interface checks, "<<failures<<" failures\n";return failures?1:0;}

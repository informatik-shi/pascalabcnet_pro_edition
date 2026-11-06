#include "abc/core.hpp"
#include "../src/dictionary.hpp"
#include "../src/runtime_values.hpp"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
using abc::Type;using abc::Value;using abc::DictionaryMethod;using Object=std::shared_ptr<abc::Aggregate>;
int checks=0,failures=0;
void check(bool ok,std::string_view label){++checks;if(!ok){++failures;std::cerr<<"FAIL "<<label<<'\n';}}
template<class F>void error(F action,std::string_view code){try{action();check(false,"Expected enumerator error");}catch(const abc::Error& e){check(e.code==code && !e.russian.empty() && *e.what(),e.what());}catch(const std::exception& e){check(false,e.what());}}
std::string run(std::string_view source){std::istringstream in;std::ostringstream out;abc::execute(abc::compile(source),in,out);return out.str();}
void result(std::string_view source,std::string_view expected){try{const auto actual=run(source);check(actual==expected,std::string(source)+" -> "+actual);}catch(const std::exception& e){check(false,std::string(source)+" -> "+e.what());}}
Value call(abc::Values& v,Type owner,DictionaryMethod m,std::initializer_list<Value> args,Type payload=Type::invalid,const abc::ExecutionOptions& options={}){return abc::dictionary_invoke(v,owner,payload,m,{args.begin(),args.size()},options);}
std::vector<std::string> split(std::string_view s,char delimiter){std::vector<std::string> result;std::size_t at=0;for(;;){auto end=s.find(delimiter,at);result.emplace_back(s.substr(at,end==s.npos ? s.size()-at : end-at));if(end==s.npos)break;at=end+1;}return result;}
struct Types {Type dictionary=Type::invalid;std::array<Type,3> source{},iterator{};};
Types types(const abc::Program& p){Types result;for(std::size_t i=0;i<p.types.size();++i){const auto& t=p.types[i];auto id=static_cast<Type>(64+i);if(t.kind==abc::CompoundKind::dictionary)result.dictionary=id;if(abc::dictionary_enumerator_type(t)){const auto kind=p.types[static_cast<unsigned>(t.key)-64U].kind;const auto at=kind==abc::CompoundKind::dictionary ? 0U : kind==abc::CompoundKind::dictionary_keys ? 1U : 2U;result.source[at]=t.key;result.iterator[at]=id;}}return result;}
const char* setup="begin var d:=new Dictionary<integer,string>;var e:=d.GetEnumerator;var k:=d.Keys.GetEnumerator;var v:=d.Values.GetEnumerator;end.";
std::string text(const Value& value){if(std::holds_alternative<abc::NilString>(value))return "~";return abc::to_utf8(std::get<std::u16string>(value));}
void reference(){
 auto program=abc::compile(setup);const auto t=types(program);abc::Values values{program,4'194'304};
 auto dictionary=call(values,t.dictionary,DictionaryMethod::construct,{});
 const auto keys=call(values,t.dictionary,DictionaryMethod::keys,{dictionary},t.source[1]);
 const auto items=call(values,t.dictionary,DictionaryMethod::values,{dictionary},t.source[2]);
 const std::array<Value,3> sources{dictionary,keys,items};std::array<std::array<std::shared_ptr<abc::Cell>,4>,3> slots;
 for(std::size_t kind=0;kind<3;++kind)for(auto& slot:slots[kind])slot=values.cell(t.iterator[kind],values.zero(t.iterator[kind]));
 std::ifstream in(std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures"/"dictionary-enumerators-reference.tsv");check(bool(in),"Independent Framework enumerator corpus present");
 std::string line;std::size_t rows=0;
 while(std::getline(in,line)){
  if(!line.empty() && line.back()=='\r')line.pop_back();if(line.empty() || line.front()=='#')continue;const auto f=split(line,'\t');
  if(f.size()!=5){check(false,"Malformed enumerator corpus");continue;}const auto slot=static_cast<std::size_t>(std::stoi(f[1]));const auto arg=std::int32_t(std::stoi(f[2]));const auto value=abc::from_utf8(f[3]);std::string actual;
  auto outcome=[&](auto action){try{return action();}catch(const abc::Error& e){return e.code;}};
  if(f[0]=="new" || f[0]=="copy" || f[0]=="default"){
   for(std::size_t kind=0;kind<3;++kind){const auto v=f[0]=="new" ? call(values,t.source[kind],DictionaryMethod::get_enumerator,{sources[kind]},t.iterator[kind]) : f[0]=="copy" ? values.copy(slots[kind][static_cast<std::size_t>(arg)]->value) : values.zero(t.iterator[kind]);values.write(slots[kind][slot],v);}actual="ok";
  }else if(f[0]=="next" || f[0]=="current" || f[0]=="dispose"){
   for(std::size_t kind=0;kind<3;++kind){if(kind)actual+='|';actual+=outcome([&]()->std::string{
    if(f[0]=="next")return std::get<bool>(call(values,t.iterator[kind],DictionaryMethod::enumerator_move,{slots[kind][slot]})) ? "True" : "False";
    if(f[0]=="dispose"){call(values,t.iterator[kind],DictionaryMethod::enumerator_dispose,{slots[kind][slot]->value});return "ok";}
    const auto current=call(values,t.iterator[kind],DictionaryMethod::enumerator_current,{slots[kind][slot]->value});
    if(kind==2)return text(current);if(kind==1)return std::to_string(std::get<std::int32_t>(current));const auto pair=std::get<Object>(current);return std::to_string(std::get<std::int32_t>(pair->items[0]->value))+":"+text(pair->items[1]->value);
   });}
  }else actual=outcome([&]()->std::string{
   if(f[0]=="clear"){call(values,t.dictionary,DictionaryMethod::clear,{dictionary});return "ok";}
   if(f[0]=="remove")return std::get<bool>(call(values,t.dictionary,DictionaryMethod::remove,{dictionary,arg})) ? "True" : "False";
   call(values,t.dictionary,f[0]=="add" ? DictionaryMethod::add : DictionaryMethod::set_item,{dictionary,arg,value});return "ok";
  });
  check(actual==f[4],"Reference row "+std::to_string(rows)+" "+f[0]+" expected "+f[4]+" got "+actual);++rows;
 }
 check(rows>=2000,"Independent enumerator corpus covers at least 2000 operations");
}
void language(){
 result("begin var d:=DictInt((1,10),(2,20));var e:=d.GetEnumerator();Writeln(e.Current.Key,' ',e.Current.Value);var c:=e;Writeln(e.MoveNext,' ',e.Current.Key,' ',c.Current.Key);Writeln(c.MoveNext(),' ',c.Current.Key);e.Dispose;Writeln(e.MoveNext,' ',e.Current.Value,' ',c.Current.Value);Writeln(e.MoveNext,' ',e.Current.Key,' ',e.Current.Value,' ',e.MoveNext);end.","0 0\nTrue 1 0\nTrue 1\nTrue 20 10\nFalse 0 0 False\n");
 result("type E=Dictionary<integer,string>.Enumerator;function F(e:E):E;begin assert(e.MoveNext);Result:=e;end;procedure P(var e:E);begin assert(e.MoveNext);end;begin var e:=Dict((1,'abc'),(2,'xyz')).GetEnumerator;var f:=F(e);Writeln(e.Current.Key,' ',f.Current.Value);P(e);Writeln(e.Current.Key,' ',f.Current.Key);end.","0 abc\n1 1\n");
 result("type K=Dictionary<integer,string>.KeyCollection.Enumerator;V=System.Collections.Generic.Dictionary<integer,string>.ValueCollection.Enumerator;begin var d:=Dict((1,'abc'),(2,'xyz'));var k:K:=d.Keys.GetEnumerator;var v:V:=d.Values.GetEnumerator;Writeln(k.Current,' ',v.Current=nil);while k.MoveNext do begin assert(v.MoveNext);Writeln(k.Current,'=',v.Current);end;assert(not v.MoveNext);Writeln(k.Current,' ',v.Current=nil);end.","0 True\n1=abc\n2=xyz\n0 True\n");
 result("type E=Dictionary<integer,integer>.Enumerator;R=record e:E;end;begin var a:=DictInt((1,2),(3,4)).GetEnumerator;var r:R;r.e:=a;var s:=r;assert(r.e.MoveNext);Writeln(r.e.Current.Key,' ',s.e.Current.Key);var ar:=Arr(a);assert(ar[0].MoveNext);Writeln(ar[0].Current.Key,' ',a.Current.Key);var list:=Lst(a);assert(list[0].MoveNext);Writeln(list[0].Current.Key);end.","1 0\n1 0\n0\n");
 result("begin var d:=DictInt((1,2));var e:=d.GetEnumerator;assert(e.MoveNext);d[1]:=9;Writeln(e.Current.Value);e.Dispose;Writeln(e.Current.Value);end.","2\n2\n");
 result("type R=record n:integer;end;begin var r:R;r.n:=5;var d:=new Dictionary<integer,R>;d.Add(1,r);var e:=d.GetEnumerator;assert(e.MoveNext);var r2:=e.Current.Value;r2.n:=9;Writeln(e.Current.Value.n);end.","5\n");
 result("begin var d:=DictInt((1,2));Writeln(d.GetEnumerator.MoveNext,' ',d.GetEnumerator.Current.Key);var e:=d.Keys.GetEnumerator;Writeln(TypeName(e),' ',TypeName(d.Values.GetEnumerator),' ',TypeName(d.GetEnumerator));assert(e.GetType=typeof(Dictionary<integer,integer>.KeyCollection.Enumerator));end.","True 0\nEnumerator<integer, integer> Enumerator<integer, integer> Enumerator<integer, integer>\n");
 result("type E=Dictionary<integer,string>.ValueCollection.Enumerator;begin var e:E;e.Dispose;Writeln(e.Current=nil);e:=new E;Writeln(e.Current=nil);end.","True\nTrue\n");
 for(const auto source:{"begin var d:=DictInt((1,2));var e:=d.GetEnumerator;d[1]:=2;e.MoveNext;end.","begin var d:=DictInt();var e:=d.Values.GetEnumerator;d.Add(1,2);e.MoveNext;end.","begin var d:=DictInt((1,2));var e:=d.Keys.GetEnumerator;while e.MoveNext do begin end;d.Clear;e.MoveNext;end."})error([&]{run(source);},"E_MODIFIED");
 result("type C=class e:Dictionary<integer,C>.Enumerator;end;begin for var i:=1 to 30 do begin var c:=new C;var d:=new Dictionary<integer,C>;d.Add(1,c);c.e:=d.GetEnumerator;assert(c.e.MoveNext);assert(c.e.Current.Value=c);end;end.","");
 result("type E=Dictionary<integer,integer>.Enumerator;procedure P(const e:E);begin assert(e.MoveNext);assert(e.Current.Key=0);end;begin var e:=DictInt((1,2)).GetEnumerator;P(e);assert(e.Current.Key=0);end.","");
 error([]{abc::compile("begin var e:=DictInt().GetEnumerator;e.Reset;end.");},"E_NAME");
 error([]{run("type E=Dictionary<integer,integer>.Enumerator;begin var e:E;e.MoveNext;end.");},"E_NIL");
 error([]{run("begin var d:Dictionary<integer,integer>;d.GetEnumerator;end.");},"E_NIL");
 for(const auto source:{"begin var e:=DictInt().GetEnumerator;e.Current:=KV(1,2);end.","begin var e:=DictInt().GetEnumerator;e.Current();end.","begin var e:=DictInt().GetEnumerator;e.MoveNext(1);end.","begin var e:=DictInt().GetEnumerator;var k:=DictInt().Keys.GetEnumerator;k:=e;end."})error([&]{abc::compile(source);},"E_TYPE");
}
void metadata(){
 const auto p=abc::compile(setup);const auto t=types(p);abc::verify(p);
 for(std::size_t kind=0;kind<3;++kind){
  const auto at=static_cast<unsigned>(t.iterator[kind])-64U;
  for(std::size_t field=0;field<4;++field){auto bad=p;bad.types[at].fields[field].type=Type::boolean;error([&]{abc::verify(bad);},"E_BYTECODE");}
  auto bad=p;bad.types[at].key=t.iterator[kind];error([&]{abc::verify(bad);},"E_BYTECODE");
  bad=p;bad.types[at].element=Type::real;error([&]{abc::verify(bad);},"E_BYTECODE");
 }
 auto bad=abc::compile("begin var e:=DictInt().GetEnumerator;e.MoveNext;end.");for(auto& op:bad.code)if(op.op==abc::Op::dictionary_call && (op.operand&255U)==static_cast<std::size_t>(DictionaryMethod::enumerator_move))op.operand=(static_cast<std::size_t>(Type::invalid)<<8U)|static_cast<std::size_t>(DictionaryMethod::enumerator_move_copy);error([&]{abc::verify(bad);},"E_BYTECODE");
 abc::Values values{p,1000};auto d=call(values,t.dictionary,DictionaryMethod::construct,{});call(values,t.dictionary,DictionaryMethod::add,{d,std::int32_t{1},std::u16string(u"abc")});auto e=call(values,t.dictionary,DictionaryMethod::get_enumerator,{d},t.iterator[0]);auto cell=values.cell(t.iterator[0],e);
 abc::ExecutionOptions cancelled;cancelled.cancelled=[]{return true;};error([&]{call(values,t.iterator[0],DictionaryMethod::enumerator_move,{cell},Type::invalid,cancelled);},"E_CANCELLED");
 abc::ExecutionOptions limited;limited.instruction_limit=1;error([&]{call(values,t.iterator[0],DictionaryMethod::enumerator_move,{cell},Type::invalid,limited);},"E_LIMIT");
 check(std::get<bool>(call(values,t.iterator[0],DictionaryMethod::enumerator_move,{cell})),"Cancelled and limited MoveNext leave first item available");
 abc::Values exhausted{p,0};error([&]{call(exhausted,t.dictionary,DictionaryMethod::get_enumerator,{d},t.iterator[0]);},"E_LIMIT");
}
}
int main(){reference();language();metadata();std::cout<<(checks-failures)<<'/'<<checks<<" dictionary enumerator checks passed\n";return failures ? 1 : 0;}

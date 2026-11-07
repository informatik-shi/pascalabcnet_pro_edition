#include "abc/core.hpp"
#include "../src/enumerator.hpp"
#include "../src/runtime_values.hpp"
#include <filesystem>
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
using abc::Type;using abc::EnumeratorMethod;using Object=std::shared_ptr<abc::Aggregate>;
int checks=0,failures=0;
void check(bool ok,std::string_view label){++checks;if(!ok){++failures;std::cerr<<"FAIL "<<label<<'\n';}}
std::string run(std::string_view source){std::istringstream in;std::ostringstream out;abc::execute(abc::compile(source),in,out);return out.str();}
void result(std::string_view source,std::string_view expected){try{const auto actual=run(source);check(actual==expected,std::string(source)+" -> "+actual);}catch(const std::exception& e){check(false,std::string(source)+" -> "+e.what());}}
template<class F>void error(F f,std::string_view code){try{f();check(false,"Expected collection enumerator error");}catch(const abc::Error& e){check(e.code==code && !e.russian.empty() && *e.what(),std::string(code)+" -> "+e.code+" "+e.what());}catch(const std::exception& e){check(false,e.what());}}
std::vector<std::string> split(std::string_view s){std::vector<std::string> out;std::size_t at=0;for(;;){auto end=s.find('\t',at);out.emplace_back(s.substr(at,end==s.npos ? s.size()-at : end-at));if(end==s.npos)break;at=end+1;}return out;}
void reference(){
 std::ifstream in(std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures"/"collection-enumerators-reference.tsv");check(bool(in),"Independent collection struct/box corpus present");std::string line,key,initial,prefix;std::size_t rows=0;
 while(std::getline(in,line)){
  if(!line.empty() && line.back()=='\r')line.pop_back();if(line.empty() || line.front()=='#')continue;auto f=split(line);if(f.size()!=6){check(false,"Malformed collection reference row");continue;}
  const bool list=f[0]=="list";const auto group=f[0]+f[1];if(group!=key){key=group;prefix.clear();const auto root=list ? "List" : "HashSet";initial="type E="+std::string(root)+"<integer>.Enumerator;begin var c:=new "+root+"<integer>;var seq:IEnumerable<integer>:=c;var es:=new E[4];var boxes:=new IEnumerator<integer>[4];";}
  const auto& op=f[2];const auto e="es["+f[3]+"]",arg="es["+f[4]+"]",box="boxes["+f[3]+"]";std::string statement,target;bool read=false;
  if(op=="new")statement=e+":=c.GetEnumerator;";else if(op=="default")statement=e+":=new E;";else if(op=="copy")statement=e+":="+arg+";";else if(op=="box")statement=box+":="+arg+";";else if(op=="inew")statement=box+":=seq.GetEnumerator;";else if(op=="alias")statement=box+":=boxes["+f[4]+"];";
  else if(op=="type"){target="if "+box+"=nil then Writeln('nil') else Writeln("+box+".GetType=typeof(E));";read=true;}
  else if(op=="equal" || op=="identity"){target="Writeln("+(op=="equal" ? e+"="+arg : box+"=boxes["+f[4]+"]")+");";read=true;}
  else if(op=="next" || op=="current" || op=="dispose")statement=e+"."+(op=="next" ? "MoveNext" : op=="current" ? "Current" : "Dispose")+";";
  else if(op=="inext" || op=="icurrent" || op=="ireset" || op=="idispose")statement=box+"."+(op=="inext" ? "MoveNext" : op=="icurrent" ? "Current" : op=="ireset" ? "Reset" : "Dispose")+";";
  else if(op=="add")statement="c.Add("+f[4]+");";else if(op=="remove")statement="c.Remove("+f[4]+");";else if(op=="clear")statement="c.Clear;";else if(op=="trim")statement="c.TrimExcess;";else if(op=="set")statement="if c.Count>0 then c[0]:="+f[4]+";";else if(op=="reverse")statement="c.Reverse;";else{check(false,"Unknown collection operation");continue;}
  if(target.empty()){const bool value=op=="next" || op=="current" || op=="inext" || op=="icurrent" || op=="remove" || (op=="add" && !list);target=value ? "Writeln("+statement.substr(0,statement.size()-1)+");" : statement+"Writeln('ok');";}
  const auto program=initial+prefix+target+"end.";if(f[5].starts_with("E_"))error([&]{run(program);},f[5]);else{result(program,f[5]+"\n");if(!read && op!="current" && op!="icurrent")prefix+=statement;}
  ++rows;
 }
 check(rows==7584,"All independent struct and box operations checked through native Pascal");
}
void semantics(){
 result("type L=List<integer>;E=L.Enumerator;H=System.Collections.Generic.HashSet<integer>.Enumerator;begin var l:=Lst(1,2,3);var e:E:=l.GetEnumerator;var a:=e;assert(e.MoveNext);Writeln(e.Current,' ',a.Current);assert(a.MoveNext);e.Dispose;assert(e.MoveNext);Writeln(e.Current,' ',a.Current);var h:H:=HSet(4,5).GetEnumerator;var k:=h;assert(h.MoveNext);Writeln(h.Current,' ',k.Current);Writeln(TypeName(e),' ',TypeName(h));assert(e.GetType<>h.GetType);end.","1 0\n2 1\n4 0\nEnumerator<integer> Enumerator<integer>\n");
 result("type E=List<integer>.Enumerator;function Advance(e:E):E;begin assert(e.MoveNext);Result:=e;end;procedure P(var e:E);begin assert(e.MoveNext);end;procedure ReadOnly(const e:E);begin assert(e.MoveNext);assert(e.Current=0);end;begin var e:=Lst(1,2).GetEnumerator;ReadOnly(e);var a:=Advance(e);Writeln(e.Current,' ',a.Current);P(e);Writeln(e.Current);var r:=Arr(e);assert(r[0].MoveNext);Writeln(r[0].Current,' ',e.Current);var l:=Lst(e);assert(l[0].MoveNext);Writeln(l[0].Current);end.","0 1\n1\n2 1\n1\n");
 result("type R=record n:integer;end;begin var r:R;r.n:=5;var l:=Lst(r);var e:=l.GetEnumerator;assert(e.MoveNext);r.n:=9;l[0]:=r;Writeln(e.Current.n);var v:=e.Current;v.n:=20;Writeln(e.Current.n);var s:=HSet(r);var k:=s.GetEnumerator;assert(k.MoveNext);v:=k.Current;v.n:=30;Writeln(k.Current.n);end.","5\n5\n9\n");
 result("begin var l:=Lst('abc','xyz');var e:=l.GetEnumerator;assert(e.Current=nil);while e.MoveNext do begin end;Writeln(e.Current=nil);var h:=HSet('abc','xyz');var k:=h.GetEnumerator;assert(k.Current=nil);while k.MoveNext do begin end;Writeln(k.Current=nil);end.","True\nTrue\n");
 result("type E=List<integer>.Enumerator;H=HashSet<integer>.Enumerator;begin var l:=Lst(1,2);var s:IEnumerable<integer>:=l;var e:=s.GetEnumerator;assert(e.GetType=typeof(E));var a:IEnumerator<integer>:=l.GetEnumerator;assert(a<>e);assert(a.MoveNext);a.Dispose;assert(a.MoveNext);Writeln(a.Current);a.Reset;Writeln(a.Current);var n:=SetOf(7,8);var k:=n.GetEnumerator;assert(k.GetType=typeof(H));assert(k.MoveNext);n:=SetOf(99);Writeln(k.Current);assert(k.MoveNext);Writeln(k.Current);end.","2\n0\n7\n8\n");
 result("type D=Dictionary<integer,string>;K=D.KeyCollection;E=K.Enumerator;V=D.ValueCollection.Enumerator;begin var d:=Dict((1,'abc'));var k:E:=d.Keys.GetEnumerator;var v:V:=d.Values.GetEnumerator;assert(k.MoveNext);assert(v.MoveNext);Writeln(k.Current,' ',v.Current);end.","1 abc\n");
 result("type C=class e:List<C>.Enumerator;h:HashSet<C>.Enumerator;i:IEnumerator<C>;end;begin for var n:=1 to 30 do begin var c:=new C;var l:=Lst(c);c.e:=l.GetEnumerator;assert(c.e.MoveNext);var h:=HSet(c);c.h:=h.GetEnumerator;c.i:=c.h;assert(c.i.MoveNext);assert(c.i.Current=c);end;end.","");
 for(const auto type:{"List<integer>.Enumerator","HashSet<integer>.Enumerator"}){
  const auto prefix="type E="+std::string(type)+";begin var e:E;";result(prefix+"e.Dispose;Writeln(e.Current);end.","0\n");error([&]{run(prefix+"e.MoveNext;end.");},"E_NIL");error([&]{run(prefix+"var i:IEnumerator<integer>:=e;i.Reset;end.");},"E_NIL");error([&]{abc::compile(prefix+"e.Reset;end.");},"E_NAME");error([&]{abc::compile(prefix+"e.Current:=1;end.");},"E_TYPE");
 }
 error([]{abc::compile("begin var l:List<integer>.Enumerator;var h:HashSet<integer>.Enumerator;l:=h;end.");},"E_TYPE");
 error([]{abc::compile("begin var e:IEnumerator<real>:=Lst(1).GetEnumerator;end.");},"E_TYPE");
 std::string deep="type R0=record n:integer;end;";for(int i=1;i<31;++i)deep+="R"+std::to_string(i)+"=record r:R"+std::to_string(i-1)+";end;";deep+="begin var l:=new List<R30>;var e:=l.GetEnumerator;Writeln(TypeName(e));end.";result(deep,"Enumerator<R30>\n");
}
void metadata(){
 auto p=abc::compile("begin var l:=Lst(1);var e:=l.GetEnumerator;e.MoveNext;var h:=HSet(1);var k:=h.GetEnumerator;end.");abc::verify(p);
 for(std::size_t i=0;i<p.types.size();++i)if(abc::collection_enumerator_type(p.types[i])){
  for(std::size_t field=0;field<4;++field){auto bad=p;bad.types[i].fields[field].type=Type::boolean;error([&]{abc::verify(bad);},"E_BYTECODE");}
  auto bad=p;bad.types[i].key=static_cast<Type>(64+i);error([&]{abc::verify(bad);},"E_BYTECODE");
 }
 auto bad=p;for(auto& op:bad.code)if(op.op==abc::Op::enumerator_call && (op.operand&255U)==static_cast<std::size_t>(EnumeratorMethod::collection_move))op.operand=(static_cast<std::size_t>(Type::invalid)<<8U)|static_cast<std::size_t>(EnumeratorMethod::collection_move_copy);error([&]{abc::verify(bad);},"E_BYTECODE");
 for(std::size_t i=0;i<p.types.size();++i)if(abc::collection_enumerator_type(p.types[i])){
  const auto iterator=static_cast<Type>(64+i),owner=p.types[i].key;abc::Values values{p,1000};
  auto source=abc::type_info(p,owner).kind==abc::CompoundKind::list ? values.array(owner,1) : std::get<Object>(values.zero(owner,true));
  if(abc::type_info(p,owner).kind==abc::CompoundKind::hash_set){const std::array<abc::Value,2> args{source,std::int32_t{1}};abc::hash_set_invoke(values,owner,Type::invalid,abc::HashSetMethod::add,args,{});}
  auto e=abc::enumerator_invoke(values,owner,iterator,EnumeratorMethod::get_collection,source,{});auto cell=values.cell(iterator,e);
  abc::ExecutionOptions cancelled;cancelled.cancelled=[]{return true;};error([&]{abc::enumerator_invoke(values,iterator,Type::invalid,EnumeratorMethod::collection_move,cell,cancelled);},"E_CANCELLED");
  abc::ExecutionOptions limited;limited.instruction_limit=1;error([&]{abc::enumerator_invoke(values,iterator,Type::invalid,EnumeratorMethod::collection_move,cell,limited);},"E_LIMIT");
  check(std::get<bool>(abc::enumerator_invoke(values,iterator,Type::invalid,EnumeratorMethod::collection_move,cell,{})),"Failed cancelled/limited moves preserve first item");
  abc::Values exhausted{p,0};error([&]{abc::enumerator_invoke(exhausted,owner,iterator,EnumeratorMethod::get_collection,source,{});},"E_LIMIT");
 }
}
}
int main(){reference();semantics();metadata();std::cout<<(checks-failures)<<'/'<<checks<<" collection enumerator checks passed\n";return failures ? 1 : 0;}

#include "abc/core.hpp"
#include "../src/enumerator.hpp"
#include "../src/runtime_values.hpp"
#include "../src/sequences.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
namespace {
using abc::Type;using abc::Value;using abc::EnumeratorMethod;using Object=std::shared_ptr<abc::Aggregate>;
int checks=0,failures=0;
void check(bool ok,std::string_view label){++checks;if(!ok){++failures;std::cerr<<"FAIL "<<label<<'\n';}}
template<class F>void error(F action,std::string_view code){try{action();check(false,"Expected interface error");}catch(const abc::Error& e){check(e.code==code && !e.russian.empty() && *e.what(),std::string(code)+" -> "+e.code+" "+e.what());}catch(const std::exception& e){check(false,e.what());}}
std::string run(std::string_view source,const abc::ExecutionOptions& options={}){std::istringstream in;std::ostringstream out;abc::execute(abc::compile(source),in,out,options);return out.str();}
void result(std::string_view source,std::string_view expected){try{const auto actual=run(source);check(actual==expected,std::string(source)+" -> "+actual);}catch(const std::exception& e){check(false,std::string(source)+" -> "+e.what());}}
std::vector<std::string> split(std::string_view s){std::vector<std::string> out;std::size_t at=0;for(;;){auto end=s.find('\t',at);out.emplace_back(s.substr(at,end==s.npos ? s.size()-at : end-at));if(end==s.npos)break;at=end+1;}return out;}
void reference(){
 const std::map<std::string,std::string> sources{{"array","Arr(1,2,3)"},{"empty_array","new integer[0]"},{"list","Lst(1,2,3)"},{"hashset","new HashSet<integer>(Arr(1,2,3))"},{"keys","DictInt((11,1),(22,2),(33,3)).Keys"},{"values","DictInt((11,1),(22,2),(33,3)).Values"},{"range","Range(1,3)"},{"repeat","SeqFill(3,7)"},{"empty_range","Range(2,1)"},{"empty_repeat","SeqFill(0,7)"},{"range_char","Range('a','c')"},{"empty_range_char","Range('b','a')"},{"where","Arr(1,2,3).Where(x->x<>2)"},{"where_indexed","Arr(1,2,3).Where((x,i)->i<>1)"},{"select","Arr(1,2,3).Select(x->x*2)"},{"select_indexed","Arr(1,2,3).Select((x,i)->x+i)"},{"iterate","Iterate(1,x->x+1)"},{"seqgen","SeqGen(3,1,x->x+1)"},{"seqgen_index","SeqGen(3,i->i+1)"},{"seqwhile","SeqWhile(1,x->x+1,x->x<4)"},{"string","'abc'"},{"empty_string","''"}};
 std::ifstream in(std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures"/"generic-enumerators-reference.tsv");check(bool(in),"Independent generic-interface corpus present");std::string line,key,prefix,initial;std::size_t rows=0;
 while(std::getline(in,line)){
  if(!line.empty() && line.back()=='\r')line.pop_back();if(line.empty() || line.front()=='#')continue;const auto f=split(line);if(f.size()!=5){check(false,"Malformed interface corpus");continue;}
  const auto group=f[0]+f[1];if(group!=key){key=group;prefix.clear();initial="begin var s:IEnumerable<"+std::string((f[0].find("string")!=std::string::npos || f[0].find("char")!=std::string::npos) ? "char" : "integer")+">:="+sources.at(f[0])+";var e0:=s.GetEnumerator;var e1:=e0;";}
  const auto object="e"+f[2];const auto statement=f[3]=="alias" ? "e1:=e0;" : object+"."+(f[3]=="next" ? "MoveNext" : f[3]=="current" ? "Current" : f[3]=="dispose" ? "Dispose" : "Reset")+";";
  const auto target=f[3]=="alias" ? "e1:=e0;Writeln(e0=e1);" : f[3]=="next" || f[3]=="current" ? "Writeln("+((f[3]=="current" && (f[0].find("string")!=std::string::npos || f[0].find("char")!=std::string::npos)) ? "Ord("+statement.substr(0,statement.size()-1)+")" : statement.substr(0,statement.size()-1))+");" : statement+"Writeln('ok');";
  const auto program=initial+prefix+target+"end.";
  if(f[4].starts_with("E_"))error([&]{run(program);},f[4]);else{result(program,f[4]+"\n");if(f[3]!="current")prefix+=statement;}
  ++rows;
 }
 check(rows==4488,"All independent interface operations checked through Pascal bindings");
}
void language(){
 result("type I=IEnumerator<integer>;function Take(e:I):integer;begin assert(e.MoveNext);Result:=e.Current;end;begin var s:System.Collections.Generic.IEnumerable<integer>:=Arr(1,2,3);var e:System.Collections.Generic.IEnumerator<integer>:=s.GetEnumerator;var a:=e;Writeln(Take(a),' ',e.Current,' ',Take(e),' ',a.Current);e.Reset;Writeln(Take(a),' ',TypeName(e));end.","1 1 2 2\n1 IEnumerator<integer>\n");
 result("type E=Dictionary<integer,integer>.Enumerator;I=IEnumerator<KeyValuePair<integer,integer>>;begin var d:=DictInt((1,10),(2,20),(3,30));var r:E:=d.GetEnumerator;var e:I:=r;assert(e.MoveNext);Writeln(r.Current.Key,' ',e.Current.Key);var a:=e;assert(a.MoveNext);Writeln(e.Current.Key,' ',a=e);var b:I:=r;Writeln(b<>e,' ',b.Current.Key);e.Dispose;assert(a.MoveNext);Writeln(e.Current.Value);e.Reset;Writeln(a.Current.Key);assert(e.GetType=typeof(E));end.","0 1\n2 True\nTrue 0\n30\n0\n");
 result("type I=IEnumerator<integer>;procedure Replace(var e:I);begin e:=Range(8,9).GetEnumerator;end;begin var s:IEnumerable<integer>:=Arr(1,2);var e:=s.GetEnumerator;var a:=e;Replace(e);assert(e.MoveNext);assert(a.MoveNext);Writeln(e.Current,' ',a.Current);var empty:IEnumerable<integer>:=new integer[0];var other:IEnumerable<integer>:=[];Writeln(empty.GetEnumerator=other.GetEnumerator);end.","8 1\nTrue\n");
 result("type I=IEnumerator<integer>;S=IEnumerable<integer>;begin var s:=S(Arr(3,4));var e:=I(s.GetEnumerator);var a:=IEnumerator&<integer>(e);Writeln(a=e,' ',IEnumerable&<integer>(Arr(8)).First);end.","True 8\n");
 result("begin var d:=DictInt((1,2));var s:IEnumerable<KeyValuePair<integer,integer>>:=d;var e:=s.GetEnumerator;assert(e.GetType=typeof(Dictionary<integer,integer>.Enumerator));assert(e.MoveNext);d[1]:=9;Writeln(e.Current.Value);e.Dispose;Writeln(e.Current.Value);end.","2\n2\n");
 result("type R=record n:integer;end;begin var r:R;r.n:=1;var a:=Arr(r);var s:IEnumerable<R>:=a;var e:=s.GetEnumerator;assert(e.MoveNext);r.n:=5;a[0]:=r;Writeln(e.Current.n);var l:=Lst(r);var t:IEnumerable<R>:=l;var k:=t.GetEnumerator;assert(k.MoveNext);r.n:=9;l[0]:=r;Writeln(k.Current.n);end.","5\n5\n");
 result("type C=class e:IEnumerator<C>;end;begin for var i:=1 to 30 do begin var c:=new C;var d:=new Dictionary<integer,C>;d.Add(1,c);c.e:=d.Values.GetEnumerator;assert(c.e.MoveNext);assert(c.e.Current=c);end;end.","");
 result("type C=class n:integer;e:IEnumerator<integer>;end;begin for var i:=1 to 30 do begin var c:=new C;c.n:=i;c.e:=Seq(1).Select(x->x+c.n).GetEnumerator;assert(c.e.MoveNext);assert(c.e.Current=i+1);end;end.","");
 result("begin var n:=0;var e:=Seq(1,2).Select(x->begin n+=1;Result:=x;end).GetEnumerator;assert(n=0);assert(e.MoveNext);Writeln(e.Current,' ',e.Current,' ',n);e.Dispose;Writeln(e.MoveNext,' ',e.Current,' ',n);end.","1 1 1\nFalse 0 1\n");
 for(const auto source:{"begin var e:IEnumerator<integer>;e.MoveNext;end.","begin var e:IEnumerator<integer>;e.Dispose;end.","begin var e:IEnumerator<integer>;Writeln(e.Current);end.","begin var e:IEnumerator<integer>;e.Reset;end."})error([&]{run(source);},"E_NIL");
 error([]{run("begin var d:=DictInt((1,2));var e:IEnumerator<KeyValuePair<integer,integer>>:=d.GetEnumerator;d.Add(2,3);e.Reset;end.");},"E_MODIFIED");
 error([]{run("begin var l:=Lst(1,2);var s:IEnumerable<integer>:=l;var e:=s.GetEnumerator;assert(e.MoveNext);l[0]:=1;e.MoveNext;end.");},"E_MODIFIED");
 error([]{run("begin var e:=Range(1,3).GetEnumerator;e.Reset;end.");},"E_NOT_SUPPORTED");
 for(const auto source:{"begin Writeln('never');var e:IEnumerator<real>:=Range(1,3).GetEnumerator;end.","begin Writeln('never');var e:IEnumerator<integer>:=DictInt().GetEnumerator;end.","begin var e:=new IEnumerator<integer>;end.","begin var e:=Range(1,3).GetEnumerator;e.Current:=2;end.","begin var e:=Range(1,3).GetEnumerator;e.Current();end.","begin var e:=Range(1,3).GetEnumerator;e.MoveNext(1);end."})error([&]{abc::compile(source);},"E_TYPE");
}
void locations(){
 for(const auto source:{"begin\n var e:IEnumerator<integer>;\n e\n  .MoveNext;\nend.","begin\n var s:IEnumerable<integer>;\n s\n  .GetEnumerator;\nend."})try{run(source);check(false,"Expected multiline interface nil error");}catch(const abc::Error& e){check(e.code=="E_NIL" && e.location.line==4 && e.russian=="Нельзя обратиться к пустой ссылке nil","Interface member source location preserved");}
 result("begin var r:IntRange:=1..2;var e:=r.GetEnumerator;assert(e.MoveNext);e.Dispose;assert(e.MoveNext);Writeln(e.Current);var c:CharRange:='a'..'b';var ce:=c.GetEnumerator;assert(ce.MoveNext);ce.Dispose;Writeln(ce.MoveNext,' ',Ord(ce.Current));var s:=SetOf(7,8);var k:=s.GetEnumerator;assert(k.MoveNext);k.Dispose;assert(k.MoveNext);Writeln(k.Current);k.Reset;assert(k.MoveNext);Writeln(k.Current);end.","2\nFalse 0\n8\n7\n");
 error([]{run("begin var r:IntRange:=2..1;var e:=r.GetEnumerator;Writeln(e.Current);end.");},"E_ITERATOR_STATE");
}
void resources(){
 const auto file=std::filesystem::path("generic-enumerator-lines.tmp");{std::ofstream out(file);out<<"one\ntwo\n";}
 abc::ExecutionOptions options;
 try{check(run("begin var e:=ReadLines('generic-enumerator-lines.tmp').GetEnumerator;assert(e.Current=nil);assert(e.MoveNext);Writeln(e.Current);e.Dispose;Writeln(e.Current=nil,' ',e.MoveNext);var f:=ReadLines('generic-enumerator-lines.tmp').GetEnumerator;assert(f.MoveNext);Writeln(f.Current);f.Dispose;for var i:=1 to 300 do begin var g:=ReadLines('generic-enumerator-lines.tmp').GetEnumerator;assert(g.MoveNext);g.Dispose;end;end.",options)=="one\nTrue False\none\n","Dispose closes file cursors and clears Current");}
 catch(const std::exception& e){check(false,e.what());}
 std::filesystem::remove(file);
}
void metadata(){
 auto p=abc::compile("begin var s:IEnumerable<integer>:=Arr(1);var e:=s.GetEnumerator;e.MoveNext;end.");Type iterator=Type::invalid,sequence=Type::invalid,array=Type::invalid;std::size_t iterator_index=0;
 for(std::size_t i=0;i<p.types.size();++i){const auto& t=p.types[i];if(t.kind==abc::CompoundKind::enumerator){iterator=static_cast<Type>(64+i);iterator_index=i;}if(t.kind==abc::CompoundKind::sequence)sequence=static_cast<Type>(64+i);if(t.kind==abc::CompoundKind::array)array=static_cast<Type>(64+i);}
 abc::verify(p);auto bad=p;bad.types[iterator_index].fields.push_back({"fake",Type::integer,false});error([&]{abc::verify(bad);},"E_BYTECODE");bad=p;bad.types[iterator_index].key=Type::integer;error([&]{abc::verify(bad);},"E_BYTECODE");bad=p;for(auto& op:bad.code)if(op.op==abc::Op::enumerator_call){op.operand=255;break;}error([&]{abc::verify(bad);},"E_BYTECODE");
 abc::Values values{p,1000};auto items=values.array(array,1);auto source=abc::sequence_create(values,sequence,items,abc::SequenceProvider::array);auto e=abc::enumerator_invoke(values,sequence,iterator,EnumeratorMethod::get,source,{});
 abc::ExecutionOptions cancelled;cancelled.cancelled=[]{return true;};error([&]{abc::enumerator_invoke(values,iterator,Type::invalid,EnumeratorMethod::move,e,cancelled);},"E_CANCELLED");abc::ExecutionOptions limited;limited.instruction_limit=0;error([&]{abc::enumerator_invoke(values,iterator,Type::invalid,EnumeratorMethod::move,e,limited);},"E_LIMIT");
 check(std::get<bool>(abc::enumerator_invoke(values,iterator,Type::invalid,EnumeratorMethod::move,e,{})),"Cancelled MoveNext leaves cursor unchanged");abc::Values exhausted{p,0};error([&]{abc::enumerator_invoke(exhausted,sequence,iterator,EnumeratorMethod::get,source,{});},"E_LIMIT");
}
}
int main(){reference();language();locations();resources();metadata();std::cout<<(checks-failures)<<'/'<<checks<<" generic enumerator checks passed\n";return failures ? 1 : 0;}

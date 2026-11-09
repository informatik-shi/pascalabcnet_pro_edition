#include "abc/core.hpp"
#include "../src/dictionary.hpp"
#include "../src/runtime_values.hpp"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
using abc::DictionaryMethod;
namespace {
int checks=0,failures=0;
void check(bool ok,std::string_view label){++checks;if(!ok){++failures;std::cerr<<"FAIL "<<label<<'\n';}}
template<class F>void error(F action,std::string_view code){
 try{action();check(false,"Expected dictionary error");}
 catch(const abc::Error& e){check(e.code==code && !e.russian.empty() && std::string_view(e.what()).size()>0,std::string(code)+" -> "+e.code+": "+e.what());}
 catch(const std::exception& e){check(false,e.what());}
}
abc::Type append(abc::Program& p,abc::TypeInfo info){p.types.push_back(std::move(info));return static_cast<abc::Type>(63U+p.types.size());}
struct Shape{abc::Type pair,dictionary,keys,values;};
Shape shape(abc::Program& p,abc::Type key,abc::Type value){
 abc::TypeInfo pair{"keyvaluepair",abc::CompoundKind::record,abc::Type::invalid,{{"key",key,false},{"value",value,false}}};pair.native_record=abc::NativeRecord::key_value_pair;
 auto item=append(p,std::move(pair));auto d=append(p,{"dictionary",abc::CompoundKind::dictionary,item,{},key});
 auto keys=append(p,{"dictionarykeys",abc::CompoundKind::dictionary_keys,key,{},d});
 auto values=append(p,{"dictionaryvalues",abc::CompoundKind::dictionary_values,value,{},d});
 return {item,d,keys,values};
}
abc::Value call(abc::Values& values,abc::Type owner,DictionaryMethod method,std::initializer_list<abc::Value> args,abc::Type payload=abc::Type::invalid,const abc::ExecutionOptions& options={}){
 return abc::dictionary_invoke(values,owner,payload,method,std::span<const abc::Value>(args.begin(),args.size()),options);
}
std::vector<std::string> split(std::string_view line,char separator){
 std::vector<std::string> result;std::size_t at=0;for(;;){auto next=line.find(separator,at);result.emplace_back(line.substr(at,next==line.npos ? line.size()-at : next-at));if(next==line.npos)break;at=next+1;}return result;
}
std::string snapshot(const std::shared_ptr<abc::Aggregate>& d){
 std::string result;for(const auto& cell:d->items)if(cell){if(!result.empty())result+=',';const auto& p=std::get<std::shared_ptr<abc::Aggregate>>(cell->value);result+=std::to_string(std::get<std::int32_t>(p->items[0]->value))+":"+std::to_string(std::get<std::int32_t>(p->items[1]->value));}return result;
}
std::string run(std::string_view source){
 auto p=abc::compile(source);std::istringstream in;std::ostringstream out;abc::execute(p,in,out);return out.str();
}
void result(std::string_view source,std::string_view expected){
 try{const auto actual=run(source);check(actual==expected,std::string(source)+" -> "+actual);}
 catch(const std::exception& e){check(false,std::string(source)+" -> "+e.what());}
}
void error_output(std::string_view source,std::string_view code,std::string_view expected){
 try{auto p=abc::compile(source);std::istringstream in;std::ostringstream out;
 error([&]{abc::execute(p,in,out);},code);check(out.str()==expected,"Dictionary evaluation order before failure");}catch(const std::exception& e){check(false,e.what());}
}
void language(){
 result("begin var d:=new Dictionary<IntRange,integer>;d.Add(1..1,10);d.Add(2..2,20);Writeln(d.Count,' ',d[1..1],' ',d[2..2]);d.Remove(1..1);Writeln(d.Count,' ',d[2..2]);var f:=new Dictionary<single,string>;f.Add(single(0),'zero');Writeln(f.ContainsKey(single.Parse('-0')));end.","2 10 20\n1 20\nTrue\n");
 result("begin var d:=DictInt((1,2));var f:integer->integer;Writeln(d.GetOrAdd(1,f),' ',d.GetOrAdd(1,nil));end.","2 2\n");
 result("function F(k:integer):integer;begin Result:=k*2;end;function V:integer;begin Write('V');Result:=99;end;begin var d:=DictInt((1,2));Writeln(d.GetOrAdd(3,F),' ',d.Get(1,V()));end.","V6 2\n");
 result("type C=class data:Dictionary<integer,C>;end;begin for var i:=1 to 30 do begin var c:=new C;c.data:=new Dictionary<integer,C>;c.data.Add(1,c);var v:=c.data.Values;assert(v.First=c);end;end.","");
 error([]{run("begin var d:=DictInt();var f:integer->integer;d.GetOrAdd(1,f);end.");},"E_NIL");
 error([]{run("begin var f:integer->integer;(new integer[0]).ToDictionary(f);end.");},"E_NIL");
 error([]{abc::compile("begin var d:=new Dictionary<integer,string>;d.GetOrAdd(1,nil);end.");},"E_TYPE");
 error_output("function S(i:integer):string;begin Write(i);if i=2 then Writeln(1 div 0);Result:=nil;end;begin Seq(1,2).Select(S).EachCount;end.","E_DIV_ZERO","12");
 result("begin var d:=Dict((1,'abc'),(2,'xyz'));Writeln(d,' ',Copy(d));var e:=Dict(KV('abc',1),Pair('xyz',2));Writeln(e);var f:=Dict(Seq('abc','xyz'),Seq(10,20));Writeln(f);var g:=DictStrInt(('a',1),('b',2));Writeln(g);Writeln(DictInt(),' ',DictInt([]));var h:Dictionary<string,integer>:=[];Writeln(h.Count);Writeln(Dict&<string,integer>(('a',3)),' ',PABCSystem.Copy&<string,integer>(g));Writeln(Get(g,'missing',9),' ',PABCSystem.GetOrAdd(g,'zz',k->k.Length));end.","{(1,abc),(2,xyz)} {(1,abc),(2,xyz)}\n{(abc,1),(xyz,2)}\n{(abc,10),(xyz,20)}\n{(a,1),(b,2)}\n{} {}\n0\n{(a,3)} {(a,1),(b,2)}\n9 2\n");
 result("begin var d:=new System.Collections.Generic.Dictionary<string,integer>;d.Add('abc',3);d.Add('xyz',4);var keys:=d.Keys;var vals:=d.Values;Writeln(d,' ',keys,' ',vals);d['new']:=5;Writeln(keys.Count,' ',vals.Contains(5),' ',keys.Contains('new'));var a:=new string[4];keys.CopyTo(a,1);Writeln(a);foreach var pair in d do Writeln(pair.Key,'=',pair.Value);Writeln(d.Select(kv->kv.Value).ToArray,' ',keys.ToArray,' ',TypeName(d),' ',TypeName(keys),' ',TypeName(KV(1,2)));end.","{(abc,3),(xyz,4)} [abc,xyz] [3,4]\n3 True True\n[,abc,xyz,new]\nabc=3\nxyz=4\nnew=5\n[3,4,5] [abc,xyz,new] Dictionary<string, integer> KeyCollection<string, integer> KeyValuePair<integer, integer>\n");
 result("var calls:string;d:Dictionary<integer,integer>;function Dst:Dictionary<integer,integer>;begin calls:=calls+'D';Result:=d;end;function Key:integer;begin calls:=calls+'K';Result:=1;end;function Rhs:integer;begin calls:=calls+'R';Result:=5;end;begin d:=DictInt((1,2),(2,3));Dst()[Key()]+=Rhs();Writeln(d,' ',calls);var c:=d+DictInt((1,99),(3,4));Writeln(c,' ',d);c-=1;Writeln(c);c-=Seq(2,9);Writeln(c);Writeln((d-1),' ',(d-Seq(1,2)));d+=DictInt((1,7),(4,8));Writeln(d,' ',4 in d,' ',7 in d.Values);end.","{(1,7),(2,3)} DKDKR\n{(1,99),(2,3),(3,4)} {(1,7),(2,3)}\n{(2,3),(3,4)}\n{(3,4)}\n{(2,3)} {}\n{(1,7),(2,3),(4,8)} True True\n");
 result("begin var p:=new KeyValuePair<string,string>;Writeln(p.Key=nil,' ',p.Value=nil,' ',p.ToString);var d:=new Dictionary<string,string>;var s:='previous';Writeln(d.TryGetValue('missing',s),' ',s=nil);end.","True True [, ]\nFalse True\n");
 result("type R=record n:integer;end;begin var d:=new Dictionary<integer,R>;var r:R;r.n:=3;d.Add(1,r);r.n:=7;var a:=d[1];a.n:=9;var c:=Copy(d);d[1]:=r;Writeln(a.n,' ',d[1].n,' ',c[1].n);var m:=new Dictionary<integer,array of integer>;var b:=Arr(1);m.Add(1,b);var n:=Copy(m);b[0]:=8;Writeln(n[1]);end.","9 7 3\n[8]\n");
 result("begin Writeln(Seq(3,1,3,2,1).EachCount,' ',Seq('abc','xyz').Each(s->s.Length),' ',Seq(1,2).ToDictionary(x->x+10),' ',Seq(1,2).ToDictionary(x->x+10,x->x*2));end.","{(3,2),(1,2),(2,1)} {(abc,3),(xyz,3)} {(11,1),(12,2)} {(11,2),(12,4)}\n");
 result("begin Writeln(PABCSystem.EachCount(Seq(2,2,3)),' ',Each(Seq(1,2),x->x+1),' ',ToDictionary(Seq(1,2),x->x+1));Writeln(Seq(1,2).ToDictionary&<integer,string,integer>(x->IntToStr(x),x->x),' ',EachCount&<integer>(Seq(1,1)));end.","{(2,2),(3,1)} {(1,2),(2,3)} {(2,1),(3,2)}\n{(1,1),(2,2)} {(1,2)}\n");
 result("begin var d:=DictInt((1,2));var keys:=d.Keys;d.Clear;d.Add(3,4);Writeln(keys,' ',keys.Count,' ',(keys= d.Keys));var n:Dictionary<integer,integer>;n.Update(DictInt());n-=new integer[0];end.","[3] 1 True\n");
 for(const auto source:{"begin Writeln('NEVER');var d:=new Dictionary<string,integer>;d.Add(1,2);end.","begin var d:=new Dictionary<string,integer>;var r:real;d.TryGetValue('abc',r);end.","begin var p:=KV(1,2);p.Key:=3;end.","procedure P(var n:integer);begin end;begin var p:=KV(1,2);P(p.Value);end.","begin var d:=DictInt();d.Count();end.","begin var d:=DictInt();d.Keys.Count();end.","begin var d:=DictInt();d.Keys.CopyTo(new real[0],0);end.","begin Dict();end.","begin Dict(KV(1,2),KV('abc',2));end.","begin Seq(1).ToDictionary((x,i)->x);end."})
  error([&]{abc::compile(source);},"E_TYPE");
 error([]{run("begin var d:=DictInt((1,2));d.Add(1,3);end.");},"E_KEY_DUPLICATE");
 error([]{run("begin var d:=DictInt();Writeln(d[1]);end.");},"E_KEY_NOT_FOUND");
 error([]{run("begin var d:=new Dictionary<string,integer>;d.Add(nil,1);end.");},"E_NIL");
 error([]{run("begin var d:=DictInt((1,2));d.Keys.CopyTo(nil,-1);end.");},"E_NIL");
 error([]{run("begin var d:=DictInt((1,2));d.Values.CopyTo(new integer[1],-1);end.");},"E_RANGE");
 error([]{run("begin var d:=DictInt((1,2));d.Values.CopyTo(new integer[0],0);end.");},"E_RANGE");
 error([]{run("begin Dict(Seq(1),Seq(1,2));end.");},"E_RANGE");
 result("begin var d:Dictionary<integer,integer>;Writeln(Copy(d)=nil,' ',PABCSystem.Copy&<integer,integer>(d)=nil);end.","True True\n");
 error([]{run("begin DictStr(nil);end.");},"E_NIL");
 error_output("begin var d:=DictInt((1,2),(2,3));foreach var p in d do begin Write(p.Value);d[p.Key]:=p.Value;end;end.","E_MODIFIED","2");
 error_output("begin var d:=DictInt((1,2),(2,3));foreach var k in d.Keys do begin Write(k);d.Remove(k);end;end.","E_MODIFIED","1");
 error_output("function S(i:integer):string;begin Write(i);Result:=nil;end;begin Seq(1,2).Select(S).EachCount;end.","E_NIL","12");
 error_output("function S(i:integer):string;begin Write(i);Result:=nil;end;function V(s:string):integer;begin Write('V');Result:=0;end;begin Seq(1,2).Select(S).Each(V);end.","E_NIL","1V");
 error_output("begin Seq(1,1,2).ToDictionary(x->begin Write('K');Result:=x;end,x->begin Write('V');Result:=x;end);end.","E_KEY_DUPLICATE","KVKV");
 auto bad=abc::compile("begin var d:=DictInt();d.Add(1,2);end.");for(auto& op:bad.code)if(op.op==abc::Op::dictionary_call){op.operand=999;break;}error([&]{abc::verify(bad);},"E_BYTECODE");
 bad=abc::compile("begin var p:=KV(1,2);Writeln(p.Key);end.");for(auto& op:bad.code)if(op.op==abc::Op::get_field){op.op=abc::Op::ref_field;break;}error([&]{abc::verify(bad);},"E_BYTECODE");
}
void reference(){
 auto p=abc::compile("begin end.");const auto s=shape(p,abc::Type::integer,abc::Type::integer);abc::verify(p);abc::Values values{p,1'048'576};
 auto d=call(values,s.dictionary,DictionaryMethod::construct,{});const auto object=std::get<std::shared_ptr<abc::Aggregate>>(d);
 std::ifstream in(std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures"/"dictionary-reference.tsv");
 check(bool(in),"Independent Framework Dictionary corpus present");std::string line;std::size_t rows=0;
 std::string source="begin var d:=new Dictionary<integer,integer>;var n:integer;";std::string expected;
 while(std::getline(in,line)){
  if(!line.empty() && line.back()=='\r')line.pop_back();if(line.empty() || line.front()=='#')continue;auto fields=split(line,'\t');
  if(fields.size()!=6){check(false,"Malformed dictionary reference");continue;}
  if(fields[5]=="-")fields[5].clear();
  const auto key=std::int32_t(std::stoi(fields[1])),value=std::int32_t(std::stoi(fields[2]));std::string outcome;
  try{
   if(fields[0]=="add"){call(values,s.dictionary,DictionaryMethod::add,{d,key,value});outcome="ok";}
   else if(fields[0]=="set"){call(values,s.dictionary,DictionaryMethod::set_item,{d,key,value});outcome="ok";}
   else if(fields[0]=="clear"){call(values,s.dictionary,DictionaryMethod::clear,{d});outcome="ok";}
   else if(fields[0]=="get")outcome=std::to_string(std::get<std::int32_t>(call(values,s.dictionary,DictionaryMethod::get_item,{d,key})));
   else if(fields[0]=="try"){auto output=values.cell(abc::Type::integer,std::int32_t{999});const auto found=std::get<bool>(call(values,s.dictionary,DictionaryMethod::try_get_value,{d,key,output}));outcome=std::string(found ? "True:" : "False:")+std::to_string(std::get<std::int32_t>(output->value));}
   else{const auto method=fields[0]=="remove" ? DictionaryMethod::remove : fields[0]=="key" ? DictionaryMethod::contains_key : DictionaryMethod::contains_value;outcome=std::get<bool>(call(values,s.dictionary,method,{d,fields[0]=="value" ? value : key})) ? "True" : "False";}
  }catch(const abc::Error& e){outcome=e.code;}
  const auto& k=fields[1];const auto& v=fields[2];const auto& op=fields[0];
  if(op=="add")source+="if d.ContainsKey("+k+") then Write('E_KEY_DUPLICATE') else begin d.Add("+k+","+v+");Write('ok');end;";
  else if(op=="set")source+="d["+k+"]:="+v+";Write('ok');";
  else if(op=="clear")source+="d.Clear;Write('ok');";
  else if(op=="get")source+="if d.ContainsKey("+k+") then Write(d["+k+"]) else Write('E_KEY_NOT_FOUND');";
  else if(op=="try")source+="Write(d.TryGetValue("+k+",n),':',n);";
  else source+="Write(d."+std::string(op=="remove" ? "Remove" : op=="key" ? "ContainsKey" : "ContainsValue")+"("+(op=="value" ? v : k)+"));";
  source+="Writeln('|',d.Count,'|',d);";std::string display="{";if(!fields[5].empty())for(const auto& pair:split(fields[5],',')){if(display.size()>1)display+=',';const auto kv=split(pair,':');display+="("+kv[0]+","+kv[1]+")";}display+="}";expected+=fields[3]+"|"+fields[4]+"|"+display+"\n";
  check(outcome==fields[3],fields[0]+" "+fields[1]+" result -> "+outcome);
  check(object->count==static_cast<std::size_t>(std::stoul(fields[4])),"Dictionary count matches independent reference");
  check(snapshot(object)==fields[5],"Dictionary slot order matches independent reference");
  ++rows;
 }
 check(rows==2048,"All Dictionary reference operations consumed");source+="end.";try{check(run(source)==expected,"Full Dictionary corpus also matches through Pascal bindings");}catch(const std::exception& e){check(false,e.what());}
}
void semantics(){
 auto p=abc::compile("begin end.");const auto s=shape(p,abc::Type::string,abc::Type::string);abc::verify(p);abc::Values values{p,1'048'576};
 auto d=call(values,s.dictionary,DictionaryMethod::construct,{});auto object=std::get<std::shared_ptr<abc::Aggregate>>(d);
 for(const auto method:{DictionaryMethod::contains_key,DictionaryMethod::get_item,DictionaryMethod::remove})error([&]{call(values,s.dictionary,method,{d,abc::NilString{}});},"E_NIL");
 error([&]{call(values,s.dictionary,DictionaryMethod::add,{d,abc::NilString{},std::u16string(u"value")});},"E_NIL");
 call(values,s.dictionary,DictionaryMethod::add,{d,std::u16string(u"key"),abc::NilString{}});
 check(std::holds_alternative<abc::NilString>(call(values,s.dictionary,DictionaryMethod::get_item,{d,std::u16string(u"key")})),"Dictionary permits nil values");
 auto version=object->version;call(values,s.dictionary,DictionaryMethod::set_item,{d,std::u16string(u"key"),abc::NilString{}});
 check(object->version==version+1,"Framework setter invalidates enumeration even for equal values");
 version=object->version;call(values,s.dictionary,DictionaryMethod::remove,{d,std::u16string(u"absent")});check(object->version==version,"Failed removal preserves version");
 auto copy=call(values,s.dictionary,DictionaryMethod::copy_value,{d});call(values,s.dictionary,DictionaryMethod::set_item,{d,std::u16string(u"key"),std::u16string(u"new")});
 check(std::holds_alternative<abc::NilString>(call(values,s.dictionary,DictionaryMethod::get_item,{copy,std::u16string(u"key")})),"Copy owns its entries");
 const auto v1=call(values,s.dictionary,DictionaryMethod::keys,{d},s.keys),v2=call(values,s.dictionary,DictionaryMethod::keys,{d},s.keys);
 check(std::get<std::shared_ptr<abc::Aggregate>>(v1)==std::get<std::shared_ptr<abc::Aggregate>>(v2),"Keys view identity is cached");
 auto output=values.cell(abc::Type::string,std::u16string(u"previous"));call(values,s.dictionary,DictionaryMethod::try_get_value,{d,std::u16string(u"absent"),output});check(std::holds_alternative<abc::NilString>(output->value),"Missing TryGetValue writes generic nil string");
 version=object->version;call(values,s.dictionary,DictionaryMethod::remove,{d,std::u16string(u"key")});call(values,s.dictionary,DictionaryMethod::clear,{d});check(object->version==version+2,"Clear after all removals still invalidates historical entries");
 version=object->version;call(values,s.dictionary,DictionaryMethod::clear,{d});check(object->version==version,"Clear of never-used slots preserves version");
 error([&]{call(values,s.dictionary,DictionaryMethod::construct_capacity,{std::int32_t{-1}});},"E_RANGE");
 error([&]{call(values,s.dictionary,DictionaryMethod::construct_capacity,{std::int32_t{INT32_MAX}});},"E_LIMIT");
 auto nil=values.zero(s.dictionary);call(values,s.dictionary,DictionaryMethod::update,{nil,d});
 call(values,s.dictionary,DictionaryMethod::add,{d,std::u16string(u"x"),std::u16string(u"y")});
 error([&]{call(values,s.dictionary,DictionaryMethod::update,{d,d});},"E_MODIFIED");
 error([&]{call(values,s.dictionary,DictionaryMethod::update,{nil,d});},"E_NIL");
 abc::ExecutionOptions cancelled;cancelled.cancelled=[]{return true;};error([&]{call(values,s.dictionary,DictionaryMethod::count,{d},abc::Type::invalid,cancelled);},"E_CANCELLED");
 auto bad=p;bad.types[static_cast<unsigned>(s.pair)-64U].fields[0].name="mutable";error([&]{abc::verify(bad);},"E_BYTECODE");
 bad=p;bad.types[static_cast<unsigned>(s.dictionary)-64U].key=abc::Type::integer;error([&]{abc::verify(bad);},"E_BYTECODE");
 bad=p;bad.types[static_cast<unsigned>(s.keys)-64U].element=abc::Type::integer;error([&]{abc::verify(bad);},"E_BYTECODE");
}
}
int main(){reference();semantics();language();std::cout<<(checks-failures)<<'/'<<checks<<" Dictionary checks passed\n";return failures ? 1 : 0;}

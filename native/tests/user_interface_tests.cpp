#include "abc/core.hpp"
#include "../src/interfaces.hpp"
#include "../src/runtime_values.hpp"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
using abc::Type;using abc::CompoundKind;
int checks=0,failures=0;
void check(bool ok,std::string_view label){++checks;if(!ok){++failures;std::cerr<<"FAIL "<<label<<'\n';}}
std::string run(std::string_view source,abc::ExecutionOptions options={}){std::istringstream in;std::ostringstream out;abc::execute(abc::compile(source),in,out,options);return out.str();}
void result(std::string_view source,std::string_view expected){try{const auto actual=run(source);check(actual==expected,std::string(source)+" -> "+actual);}catch(const std::exception& e){check(false,std::string(source)+" -> "+e.what());}}
template<class F>void error(F f,std::string_view code){try{f();check(false,"Expected "+std::string(code));}catch(const abc::Error& e){check(e.code==code && !e.russian.empty() && *e.what(),std::string(code)+" -> "+e.code+" "+e.what());}catch(const std::exception& e){check(false,e.what());}}
std::vector<std::string> split(std::string_view s){std::vector<std::string> out;std::size_t at=0;for(;;){auto end=s.find('\t',at);out.emplace_back(s.substr(at,end==s.npos ? s.size()-at : end-at));if(end==s.npos)break;at=end+1;}return out;}
const std::string declarations=R"pas(type IValue=interface
 property Value:integer read write;
 function Add(a:integer):integer;
 procedure Change(var x:integer);
 function Sum(params xs:array of integer):integer;
end;
IDouble=interface(IValue) function Twice:integer;end;
A=class(IDouble)
 private n:integer;
 public property Value:integer read n write n;
 function Add(a:integer):integer;begin n+=a;Result:=n;end;
 procedure Change(var x:integer);begin x+=n;end;
 function Sum(params xs:array of integer):integer;begin Result:=n;foreach var x in xs do Result+=x;end;
 function Twice:integer;begin Result:=n*2;end;
end;
B=class(IValue)
 private n:integer;
 public property Value:integer read n write n;
 function Add(a:integer):integer;begin n-=a;Result:=n;end;
 procedure Change(var x:integer);begin x-=n;end;
 function Sum(params xs:array of integer):integer;begin Result:=n;foreach var x in xs do Result-=x;end;
end;
)pas";
void reference(){
 std::ifstream in(std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures"/"user-interfaces-reference.tsv");check(bool(in),"Independent named interface corpus present");std::string line,key,prefix;std::size_t rows=0;
 const auto initial=declarations+"begin var slots:=new IValue[4];slots[0]:=new A;slots[1]:=new B;var x:integer;";
 while(std::getline(in,line)){
  if(!line.empty() && line.back()=='\r')line.pop_back();if(line.empty() || line.front()=='#')continue;auto f=split(line);if(f.size()!=5){check(false,"Malformed interface reference row");continue;}
  if(f[0]!=key){key=f[0];prefix.clear();}const auto& op=f[1];const auto slot="slots["+f[2]+"]";std::string statement,target;bool mutation=false;
  if(op=="get")target="Writeln("+slot+".Value);";
  else if(op=="set"){statement=slot+".Value:="+f[3]+";";mutation=true;}
  else if(op=="add"){statement=slot+".Add("+f[3]+");";target="Writeln("+slot+".Add("+f[3]+"));";mutation=true;}
  else if(op=="change")target="x:="+f[3]+";"+slot+".Change(x);Writeln(x);";
  else if(op=="sum")target="Writeln("+slot+".Sum(1,"+f[3]+",2));";
  else if(op=="alias"){statement=slot+":=slots["+f[3]+"];";mutation=true;}
  else if(op=="identity")target="Writeln("+slot+"=slots["+f[3]+"]);";
  else if(op=="cast")target="Writeln(IDouble("+slot+").Twice);";
  else if(op=="newa" || op=="newb" || op=="nil"){statement=slot+":="+(op=="nil" ? "nil" : op=="newa" ? "new A" : "new B")+";";mutation=true;}
  else{check(false,"Unknown interface reference operation");continue;}
  if(target.empty())target=statement+"Writeln('ok');";const auto program=initial+prefix+target+"end.";
  if(f[4].starts_with("E_"))error([&]{run(program);},f[4]);else{result(program,f[4]+"\n");if(mutation)prefix+=statement;}++rows;
 }
 check(rows==1152,"All independent class/interface dispatch outcomes checked through native Pascal");
}
void semantics(){
 result("type I=interface property P:integer read write;end;C=class(I) private n:integer;function GetP:integer:=n;procedure SetP(v:integer);begin n:=v;end;public property P:integer read GetP write SetP;end;begin var i:I:=new C;i.P:=9;Writeln(i.P);i.P+=1;Writeln(i.P);end.","9\n10\n");
 result(declarations+"begin var i:=IValue(nil);var d:=IDouble(nil);Writeln(i=nil,' ',d=nil);end.","True True\n");
 result(declarations+"function Up(x:IDouble):IValue;begin Result:=x;end;procedure Replace(var x:IValue);begin x:=new B;end;begin var c:=new A;var d:IDouble:=c;var i:=Up(d);i.Value:=7;Writeln(d.Twice,' ',i.Sum,' ',i.Sum(Arr(1,2)));var a:=Arr(i);a[0].Value+=2;Writeln(c.Value,' ',TypeName(i),' ',i.GetType=typeof(A));Replace(i);Writeln(i<>a[0],' ',i.Add(3));end.","14 7 10\n9 A True\nTrue -3\n");
 result("type Root=interface function F:integer;end;L=interface(Root) end;R=interface(Root) end;D=interface(L,R) end;C=class(D) public function F:integer:=42;end;begin var d:D:=new C;var l:L:=d;var r:R:=d;var base:Root:=l;Writeln(base.F,' ',Root(r)=base);end.","42 True\n");
 result("type Empty=interface end;C=class(Empty) end;begin var c:=new C;var i:Empty:=c;var a:=i;Writeln(a=i,' ',a<>nil,' ',TypeName(i));var n:Empty;Writeln(n=nil);end.","True True C\nTrue\n");
 result("type Node=interface property Next:Node read write;end;C=class(Node) public link:Node;property Next:Node read link write link;end;begin for var n:=1 to 30 do begin var c:=new C;var i:Node:=c;i.Next:=i;assert(i.Next=i);end;end.","");
 result(declarations+"var count:integer;holder:IValue;function Receiver:IValue;begin count+=1;Result:=holder;end;begin holder:=new A;Receiver.Value:=5;Receiver.Value+=2;Writeln(count,' ',holder.Value);end.","3 7\n");
 result(declarations+"begin var n:IValue;var d:=IDouble(n);Writeln(d=nil);var a:IValue:=new A;var b:IValue:=new B;Writeln(IDouble(a).Twice);end.","True\n0\n");
 error([&]{run(declarations+"begin var i:IValue:=new B;IDouble(i).Twice;end.");},"E_CAST");
 for(const auto tail:{"i.Add(1);","i.Value:=1;","Writeln(i.Value);","i.Change(x);","i.Sum;"})error([&]{run(declarations+"begin var i:IValue;var x:integer;"+tail+"end.");},"E_NIL");
 result("type I=interface procedure P(var x:integer);end;C=class(I) public procedure P(var x:integer);begin x+=2;end;end;begin var i:I:=new C;var a:=Arr(1,2);i.P(a[0]);Writeln(a);end.","[3,2]\n");
}
const std::string cursor=R"pas(type I=interface(IEnumerator<integer>) property Calls:integer read;end;
Cursor=class(I)
 private n:integer;counter:integer;disposed:integer;
 public function MoveNext:boolean;begin counter+=1;n+=1;Result:=n<=3;end;
 property Current:integer read n;
 property Calls:integer read counter;
 procedure Dispose;begin disposed+=1;end;
 procedure Reset;begin n:=0;end;
 function Closed:integer:=disposed;
end;
)pas";
void enumerators(){
 result(cursor+"begin var c:=new Cursor;var i:I:=c;var e:IEnumerator<integer>:=i;var a:=e;Writeln(e.MoveNext,' ',i.Current,' ',a.MoveNext,' ',e.Current);a.Dispose;e.Reset;Writeln(e.MoveNext,' ',e.Current,' ',i.Calls,' ',c.Closed);Writeln(TypeName(e),' ',e.GetType=typeof(Cursor),' ',a=e);end.","True 1 True 2\nTrue 1 3 1\nCursor True True\n");
 result(cursor+"begin var c:=new Cursor;var e:IEnumerator<integer>:=c;var i:=I(e);assert(i.MoveNext);Writeln(i.Current,' ',i.Calls);end.","1 1\n");
 error([&]{run(cursor+"begin var e:IEnumerator<integer>;e.MoveNext;end.");},"E_NIL");
 error([&]{abc::compile(cursor+"begin var e:IEnumerator<real>:=new Cursor;end.");},"E_TYPE");
}
void rejected(){
 error([]{abc::compile("type I=interface procedure P(x:integer;x:integer);end;begin end.");},"E_DUPLICATE");
 for(const auto source:{
 "type I=interface function F:integer;end;C=class(I) end;begin Writeln('never');end.",
 "type I=interface function F:integer;end;C=class(I) private function F:integer:=1;end;begin Writeln('never');end.",
 "type I=interface function F:integer;end;C=class(I) public function F:real:=1;end;begin Writeln('never');end.",
 "type I=interface procedure P(var x:integer);end;C=class(I) public procedure P(x:integer);begin end;end;begin end.",
 "type I=interface property P:integer read write;end;C=class(I) public n:integer;property P:integer read n;end;begin end.",
 "type I=interface property P:integer read;end;C=class(I) private n:integer;property P:integer read n;end;begin end.",
 "type I=interface(I) end;begin end.",
 "type I=interface(J) end;J=interface(I) end;begin end.",
 "type I=interface(integer) end;begin end.",
 "type I=interface function F:integer;end;J=interface function F:real;end;K=interface(I,J) end;begin end.",
 "type I=interface procedure P(params x:integer);end;begin end.",
 "type I=interface end;J=interface end;C=class(I) end;begin var i:I:=new C;var j:J:=i;end.",
 "type I=interface end;begin var i:=new I;end.",
 "type I=interface function F:integer;end;C=class(I) public function F:integer:=1;end;begin var i:I:=new C;i.F(1);end.",
 "type I=interface procedure P(var n:integer);end;C=class(I) public procedure P(var n:integer);begin end;end;begin var i:I:=new C;i.P(1);end."
 })error([&]{abc::compile(source);},"E_TYPE");
 error([]{abc::compile("type I=interface procedure P;procedure P;end;begin end.");},"E_DUPLICATE");
 error([]{abc::compile("type I=interface property P:integer read;property P:integer read;end;begin end.");},"E_DUPLICATE");
 error([]{abc::compile("type I=interface function F:integer;begin Result:=1;end;end;begin end.");},"E_TYPE");
 error([]{abc::compile("type C=class(IEnumerable<integer>) end;begin end.");},"E_UNSUPPORTED");
}
void metadata(){
 const auto p=abc::compile(declarations+"begin var i:IValue:=new A;i.Value:=1;Writeln(i.Add(2));end.");abc::verify(p);std::size_t iface=0,cls=0;
 for(std::size_t i=0;i<p.types.size();++i){if(p.types[i].name=="ivalue")iface=i;if(p.types[i].name=="a")cls=i;}
 check(p.types[iface].kind==CompoundKind::user_interface && p.types[cls].kind==CompoundKind::object_class,"Known interface and implementation types before execution");
 for(int mode=0;mode<15;++mode){auto bad=p;auto& contract=bad.types[iface].interfaces.edit();auto& impl=bad.types[cls].interfaces.edit().implementations;
  if(mode==0)contract.members.push_back(contract.members.front());
  if(mode==1)contract.parents.push_back(static_cast<Type>(64+iface));
  if(mode==2)contract.members[0].result=Type::invalid;
  if(mode==3)impl[0].routines.clear();
  if(mode==4)impl[0].routines[0]=bad.routines.size();
  if(mode==5)bad.routines[impl[0].routines[0]].access=abc::MemberAccess::private_member;
  if(mode==6)bad.types[iface].fields.push_back({"fake",Type::integer,false});
  if(mode==7)bad.types[cls].interfaces.edit().members.push_back(contract.members.front());
  if(mode==8)impl.push_back(impl.front());
  if(mode==9)impl.erase(impl.begin()+1);
  if(mode==10)bad.types[iface].parameters.push_back({Type::integer,false,false});
  if(mode==11)bad.types[iface].rank=2;
  if(mode==12)for(auto& m:contract.members)if(m.kind==abc::InterfaceMemberKind::set)m.parameters[0].type=Type::real;
  if(mode==13)contract.members[0].kind=static_cast<abc::InterfaceMemberKind>(-1);
  if(mode==14)contract.members[0].kind=static_cast<abc::InterfaceMemberKind>(99);
  error([&]{abc::verify(bad);},"E_BYTECODE");
 }
 abc::verify(p);check(p.types[iface].interfaces.read().parents.empty(),"Copied metadata mutations do not affect original program");
 for(int mode=0;mode<3;++mode){auto bad=p;for(auto& op:bad.code)if(op.op==abc::Op::interface_call){if(mode==0)op.operand=999;if(mode==1)op.type=Type::integer;if(mode==2)op.type=static_cast<Type>(64+cls);break;}error([&]{abc::verify(bad);},"E_BYTECODE");}
}
void limits(){
 error([]{run("type C=class(IEnumerator<integer>) public function MoveNext:boolean;begin var e:IEnumerator<integer>:=self;Result:=e.MoveNext;end;property Current:integer read GetCurrent;function GetCurrent:integer:=0;procedure Dispose;begin end;procedure Reset;begin end;end;begin var e:IEnumerator<integer>:=new C;e.MoveNext;end.");},"E_LIMIT");
 try{run(declarations+"begin var i:IValue:=new B;IDouble(i).Twice;end.");check(false,"Expected checked cast error");}catch(const abc::Error& e){check(e.code=="E_CAST" && std::string_view(e.what())=="Value does not implement the requested interface" && e.russian=="Значение не реализует требуемый интерфейс","Checked interface cast has exact English and Russian diagnostics");}
 const auto s=declarations+"begin var i:IValue:=new A;while True do i.Add(1);end.";
 abc::ExecutionOptions options;options.instruction_limit=1000;error([&]{run(s,options);},"E_LIMIT");options={};int polls=0;options.cancelled=[&]{return ++polls>10;};error([&]{run(s,options);},"E_CANCELLED");
 error([]{run("type I=interface procedure P;end;C=class(I) public procedure P;begin var i:I:=self;i.P;end;end;begin var i:I:=new C;i.P;end.");},"E_LIMIT");
 try{run(declarations+"begin\n var i:IValue;\n i\n .Add(1);\nend.");check(false,"Expected member location error");}catch(const abc::Error& e){check(e.code=="E_NIL" && e.russian=="Нельзя обратиться к пустой ссылке nil" && e.location.line>1,"Interface runtime diagnostics contain Russian and member location");}
}
}
int main(){reference();semantics();enumerators();rejected();metadata();limits();std::cout<<(checks-failures)<<'/'<<checks<<" user interface checks passed\n";return failures ? 1 : 0;}

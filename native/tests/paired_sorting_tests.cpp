#include "abc/core.hpp"
#include "../src/sorting.hpp"
#include "../src/runtime_values.hpp"
#include <bit>
#include <iomanip>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
int checks=0,failures=0;
void check(bool ok,std::string_view text){++checks;if(!ok){++failures;if(failures<=14)std::cerr<<"FAIL "<<text<<'\n';}}
std::string run(std::string_view source,abc::ExecutionOptions options={}){std::istringstream input;std::ostringstream output;abc::execute(abc::compile(source),input,output,options);return output.str();}
void result(std::string_view source,std::string_view expected){try{const auto actual=run(source);check(actual==expected,std::string(source)+" -> "+actual);}catch(const std::exception& e){check(false,std::string(source)+" -> "+e.what());}}
template<class F>void error(F f,std::string_view code){try{f();check(false,"Expected "+std::string(code));}catch(const abc::Error& e){check(e.code==code && !e.russian.empty() && *e.what(),std::string(code)+" -> "+e.code+" "+e.what());}catch(const std::exception& e){check(false,e.what());}}
std::vector<std::string> split(std::string_view s){std::vector<std::string> out;std::size_t at=0;for(;;){auto end=s.find('\t',at);out.emplace_back(s.substr(at,end==s.npos?s.size()-at:end-at));if(end==s.npos)break;at=end+1;}return out;}
std::string trace_difference(std::string_view expected,std::string_view actual,std::string_view expected_code,std::string_view actual_code){std::size_t i=0;while(i<std::min(expected.size(),actual.size()) && expected[i]==actual[i])++i;return " byte "+std::to_string(i)+" expected "+std::string(expected.substr(i,100))+" "+std::string(expected_code)+" got "+std::string(actual.substr(i,100))+" "+std::string(actual_code);}
void reference(){
 const auto dir=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";
 std::ifstream source(dir/"paired-sorting-program.pas");check(bool(source),"Pascal sort reference program present");std::ostringstream text;text<<source.rdbuf();const auto program=abc::compile(text.str());
 std::ifstream file(dir/"paired-sorting-reference.tsv");check(bool(file),"Independent Framework 4.7.2 sort corpus present");std::string line;int rows=0;
 while(std::getline(file,line)){if(!line.empty() && line.back()=='\r')line.pop_back();if(line.empty() || line.front()=='#')continue;auto f=split(line);if(f.size()!=9){check(false,"Invalid sort reference row");continue;}
  std::istringstream input(f[0]+" "+f[1]+" "+f[2]+" "+f[3]+" "+f[4]+" "+f[5]+" "+f[6]+"\n");std::ostringstream output;std::string code="ok";
  try{abc::ExecutionOptions options;options.cleanup_instruction_limit=1'000'000;options.cleanup_allocation_limit=262144;abc::execute(program,input,output,options);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty(),"Sort fault is bilingual");}catch(const std::exception& e){code=e.what();}
  check(output.str()==f[7] && code==f[8],"scenario "+f[0]+" size "+f[1]+" seed "+f[2]+" mode "+f[3]+" nil "+f[4]+trace_difference(f[7],output.str(),f[8],code));++rows;
 }check(rows==13101,"All paired default/comparer/range/alias/mutation/finally traces checked");
}
void primitive_reference(){
 const auto dir=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::ifstream source(dir/"paired-primitive-program.pas");check(bool(source),"Primitive paired program present");std::ostringstream text;text<<source.rdbuf();const auto p=abc::compile(text.str());
 std::ifstream file(dir/"paired-primitive-reference.tsv");check(bool(file),"Primitive Framework paired corpus present");std::string line;int rows=0;
 while(std::getline(file,line)){if(!line.empty() && line.back()=='\r')line.pop_back();if(line.empty() || line.front()=='#')continue;auto f=split(line);if(f.size()!=6){check(false,"Invalid primitive paired row");continue;}
  std::istringstream input(f[0]+" "+f[1]+" "+f[2]+" "+f[3]+"\n");std::ostringstream output;std::string code="ok";try{abc::ExecutionOptions options;options.cleanup_instruction_limit=1'000'000;options.cleanup_allocation_limit=262144;abc::execute(p,input,output,options);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty(),"Primitive pair failure bilingual");}
  check(output.str()==f[4] && code==f[5],"primitive scenario "+f[0]+" size "+f[1]+" kind "+f[2]+" seed "+f[3]+trace_difference(f[4],output.str(),f[5],code));++rows;
 }check(rows==846,"All primitive pair traces checked");
}
void floating_reference(){
 const auto dir=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::ifstream input(dir/"paired-floating-reference.tsv");check(bool(input),"Independent IEEE paired corpus present");
 std::array<abc::Program,2> programs;std::array<std::vector<abc::Instruction>,2> operations;
 for(int precision=0;precision<2;++precision){const auto type=precision ? "single" : "real";programs[precision]=abc::compile(std::string("begin var k:=new ")+type+"[0];var v:=new "+type+"[0];var ids:=new integer[0];var l:=new List<"+type+">;l.Sort;Array.Sort(k,v);Array.Sort(k,ids);Array.Sort(k);end.");for(const auto& op:programs[precision].code)if(op.op==abc::Op::sort_call)operations[precision].push_back(op);check(operations[precision].size()==4,"Closed IEEE sort metadata available");}
 const std::array<std::uint64_t,11> bits{0x7ff8000000000001ULL,0xfff8000000000002ULL,0,0x8000000000000000ULL,0x7ff0000000000000ULL,0xfff0000000000000ULL,0x3ff0000000000000ULL,0xbff0000000000000ULL,0x401c000000000000ULL,0x7ff0000000000001ULL,0xfff0000000000002ULL};
 const std::array<std::uint32_t,11> fb{0x7fc00001U,0xffc00002U,0,0x80000000U,0x7f800000U,0xff800000U,0x3f800000U,0xbf800000U,0x40e00000U,0x7f800001U,0xff800002U};
 std::string line;int rows=0;while(std::getline(input,line)){if(!line.empty() && line.back()=='\r')line.pop_back();if(line.empty() || line.front()=='#')continue;auto f=split(line);if(f.size()!=7){check(false,"Invalid IEEE paired row");continue;}
  const auto precision=std::stoi(f[0]),scenario=std::stoi(f[1]),size=std::stoi(f[2]),seed=std::stoi(f[3]);const auto& program=programs[precision];auto op=operations[precision][scenario==7 ? 0 : scenario==6 ? 3 : scenario==1 || scenario==5 ? 2 : 1];if(scenario==4 || scenario==5)op.operand|=abc::sort_range;
  abc::Values values{program,1'048'576};auto keys=values.array(op.type,static_cast<std::size_t>(size),true);if(scenario==7)values.set_list_capacity(keys,static_cast<std::size_t>(size+7));
  const auto number=[&](std::size_t at)->abc::Value{return precision ? abc::Value{std::bit_cast<float>(fb[at%fb.size()])} : abc::Value{std::bit_cast<double>(bits[at%bits.size()])};};
  for(int i=0;i<size;++i)values.write(keys->items[static_cast<std::size_t>(i)],number(static_cast<std::size_t>((i*17+seed)%11)),false);
  std::shared_ptr<abc::Aggregate> items;
  if(abc::sort_paired(op.operand)){items=values.array(abc::sort_items(op.operand),static_cast<std::size_t>(size),true);for(int i=0;i<size;++i)values.write(items->items[static_cast<std::size_t>(i)],scenario==1 || scenario==5 ? abc::Value{std::int32_t{100+i}} : number(static_cast<std::size_t>(((i*17+seed)%11+3)%11)),false);if(scenario==2)items=std::get<std::shared_ptr<abc::Aggregate>>(values.zero(abc::sort_items(op.operand)));if(scenario==3)items=keys;}
  abc::Value source=keys,payload=items;const auto start=scenario==4 || scenario==5 ? (size>1 ? 1 : 0) : 0,count=scenario==4 || scenario==5 ? std::max(0,size-2) : -1;std::string code="ok";
  try{abc::sort_array(values,op.type,op.operand,source,nullptr,{},start,count,abc::sort_paired(op.operand) ? &payload : nullptr);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty(),"IEEE pair error bilingual");}
  const auto dump=[&](const std::shared_ptr<abc::Aggregate>& a){if(!a || a->nil)return std::string("nil");std::ostringstream out;for(std::size_t i=0;i<a->items.size();++i){if(i)out<<',';const auto& value=a->items[i]->value;if(abc::type_info(program,a->type).element==abc::Type::integer)out<<std::get<std::int32_t>(value);else if(precision)out<<std::hex<<std::uppercase<<std::setfill('0')<<std::setw(8)<<std::bit_cast<std::uint32_t>(std::get<float>(value));else out<<std::hex<<std::uppercase<<std::setfill('0')<<std::setw(16)<<std::bit_cast<std::uint64_t>(std::get<double>(value));}return out.str();};
  const auto actual=dump(keys)+" "+dump(items),expected=f[4]+" "+f[5];check(actual==expected && code==f[6],"IEEE precision "+f[0]+" scenario "+f[1]+" size "+f[2]+" seed "+f[3]+trace_difference(expected,actual,f[6],code));++rows;
 }check(rows==704,"All exact IEEE key/payload/zero/NaN/list/alias states checked");
}
void enum_reference(){
 const auto dir=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::ifstream source(dir/"paired-enum-program.pas");check(bool(source),"Nominal enum paired program present");std::ostringstream text;text<<source.rdbuf();const auto program=abc::compile(text.str());std::ifstream input(dir/"paired-enum-reference.tsv");check(bool(input),"Independent enum paired corpus present");std::string line;int rows=0;
 while(std::getline(input,line)){if(!line.empty() && line.back()=='\r')line.pop_back();if(line.empty() || line.front()=='#')continue;auto f=split(line);if(f.size()!=5){check(false,"Bad enum paired row");continue;}std::istringstream in(f[0]+" "+f[1]+" "+f[2]+"\n");std::ostringstream out;std::string code="ok";try{abc::execute(program,in,out);}catch(const abc::Error& e){code=e.code;}check(out.str()==f[3] && code==f[4],"enum scenario "+f[0]+" size "+f[1]+" seed "+f[2]+trace_difference(f[3],out.str(),f[4],code));++rows;}check(rows==154,"All enum fast/generic/list/alias sort states checked");
}
void semantics(){
 result("begin var k:=Arr(3,1,2);var v:=Arr('cc','aa','bb');System.Array.Sort(k,v);Writeln(k,' ',v);Array.Sort&<integer,string>(k,v,Comparer<integer>.Create((a,b)->b-a));Writeln(k,' ',v);Array.Sort&<integer,string>(k,nil);Writeln(k);end.","[1,2,3] [aa,bb,cc]\n[3,2,1] [cc,bb,aa]\n[1,2,3]\n");
 result("begin var k:=Arr('zz','Aa','aa');var v:=Arr(0,1,2);Array.Sort&<string,integer>(k,v);Writeln(k,' ',v);end.","[aa,Aa,zz] [2,1,0]\n");
 result("type R=record n:integer;end;function Make(n:integer):R;begin Result.n:=n;end;begin var k:=Arr(3,1,2);var r:=Make(7);var v:=Arr(Make(30),r,Make(20));Array.Sort(k,v);r.n:=99;Writeln(k,' ',v[0].n,' ',v[1].n,' ',v[2].n);end.","[1,2,3] 7 20 30\n");
 for(const auto source:{"begin Array.Sort&<integer>(nil);end.","begin Array.Sort&<integer,string>(nil,nil);end.","begin var k:array of integer;var v:=Arr('x');Array.Sort(k,v,-1,-1);end."})error([&]{run(source);},"E_NIL");
 for(const auto source:{"begin Array.Sort(Arr(2,1),new string[0]);end.","begin Array.Sort(Arr(2,1),new string[1],0,2);end.","begin Array.Sort(Arr(2,1),new string[4],-1,0);end.","begin Array.Sort(Arr(2,1),new string[4],0,-1);end."})error([&]{run(source);},"E_RANGE");
 for(const auto source:{"begin Array.Sort&<integer,string>(Arr(2,1));end.","begin Array.Sort&<integer>(Arr(2,1),Arr('a','b'));end.","begin Array.Sort&<integer,real>(Arr(2,1),Arr('a','b'));end.","begin Array.Sort(Arr(2,1),Lst('a','b'));end.","begin Array.Sort(Arr(2,1),new string[2,2]);end.","begin Array.Sort(Arr(2,1),Arr('a','b'),(a,b)->a-b);end.","begin Array.Sort(Arr(2,1),Arr('a','b'),Comparer<string>.Default);end."})error([&]{abc::compile(source);},"E_TYPE");
}
void argument_order(){
 result("var k:array of integer;v:array of string;function Items:array of string;begin Write('V');k:=Arr(9);Result:=v;end;function Index:integer;begin Write('I');Result:=0;end;function Count:integer;begin Write('C');v:=Arr('new');Result:=3;end;function Order:IComparer<integer>;begin Write('O');Result:=nil;end;begin k:=Arr(3,1,2);v:=Arr('cc','aa','bb');var a:=k;var b:=v;Array.Sort(k,Items,Index,Count,Order);Writeln(' ',a,' ',b,' ',k,' ',v);end.","VICO [1,2,3] [aa,bb,cc] [9] [new]\n");
}
void metadata(){
 auto p=abc::compile("begin var k:=Arr(2,1);var v:=Arr('b','a');Array.Sort(k,v,Comparer<integer>.Default);end.");abc::verify(p);
 for(int mode=0;mode<9;++mode){auto bad=p;for(auto& op:bad.code)if(op.op==abc::Op::sort_call){if(mode==0)op.operand&=(std::size_t{1}<<20U)-1;if(mode==1)op.operand|=abc::sort_extension;if(mode==2)op.operand|=abc::sort_descending;if(mode==3)op.operand|=abc::sort_generic_key;if(mode==4)op.type=abc::Type::integer;if(mode==5)op.operand=(op.operand&~std::size_t{15})|7;if(mode==6)op.operand|=std::size_t{1}<<40U;if(mode==7)op.operand=(op.operand&~(std::size_t{4095}<<8U))|(static_cast<std::size_t>(abc::Type::invalid)<<8U);if(mode==8)op.operand|=abc::sort_range;break;}error([&]{abc::verify(bad);},"E_BYTECODE");}
}
void cleanup_budget(){
 const auto source="type G=class(IEnumerator<integer>) private n:integer;public function MoveNext:boolean;begin n+=1;Result:=n=1;end;property Current:integer read Zero;function Zero:integer:=0;procedure Reset;begin end;procedure Dispose;begin for var i:=1 to 5000 do n+=i;Write('done');end;end;S=class(IEnumerable<integer>) public function GetEnumerator:IEnumerator<integer>:=new G;end;begin foreach var x in new S do begin var zero:=0;var q:=1 div zero;end;end.";
 for(int mode=0;mode<2;++mode){abc::ExecutionOptions options;if(mode)options.cleanup_instruction_limit=100000;std::istringstream input;std::ostringstream output;try{abc::execute(abc::compile(source),input,output,options);check(false,"Cleanup budget should preserve or replace fault");}catch(const abc::Error& e){check(e.code==(mode ? "E_DIV_ZERO" : "E_LIMIT") && !e.russian.empty(),"Configured cleanup reserve stays bounded and bilingual");}check(output.str()==(mode ? "done" : ""),"Only configured larger cleanup reserve completes observer");}
}
void limits(){
 error([]{run("type C=class(IComparer<integer>) public function Compare(a,b:integer):integer;begin var k:=Arr(a,b);var v:=Arr('a','b');Array.Sort(k,v,Self);Result:=0;end;end;begin Array.Sort(Arr(2,1),Arr('b','a'),new C);end.");},"E_LIMIT");
 const auto infinite="function C(a,b:integer):integer;begin while True do begin end;Result:=0;end;begin Array.Sort(Arr(2,1),Arr('b','a'),Comparer<integer>.Create(C));end.";
 for(int mode=0;mode<2;++mode){abc::ExecutionOptions o;int calls=0;if(mode==0)o.instruction_limit=300;else o.cancelled=[&]{return ++calls>100;};error([&]{run(infinite,o);},mode?"E_CANCELLED":"E_LIMIT");}
}
}
int main(){try{reference();primitive_reference();floating_reference();enum_reference();semantics();argument_order();metadata();cleanup_budget();limits();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" paired sorting checks, "<<failures<<" failures\n";return failures?1:0;}

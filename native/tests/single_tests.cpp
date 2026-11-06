#include "abc/core.hpp"
#include "../src/single.hpp"
#include "../src/stdlib.hpp"
#include <array>
#include <bit>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>
namespace {
int checks=0,failures=0;
void check(bool ok,std::string_view description){++checks;if(!ok){++failures;std::cerr<<"FAIL "<<description<<'\n';}}
std::string run(std::string_view source,std::string input="",abc::ExecutionOptions options={}){
 auto p=abc::compile(source);std::istringstream in(input);std::ostringstream out;abc::execute(p,in,out,options);return out.str();
}
void result(std::string_view source,std::string_view expected,std::string input="",abc::ExecutionOptions options={}){
 try{auto actual=run(source,input,options);check(actual==expected,std::string(source)+" -> "+actual);}
 catch(const std::exception& e){check(false,std::string(source)+" -> "+e.what());}
}
template<class F>void error(F action,std::string_view code){
 try{action();check(false,std::string("Expected ")+std::string(code));}
 catch(const abc::Error& e){check(e.code==code && std::string_view(e.what()).size()>0 && !e.russian.empty(),std::string("Expected ")+std::string(code)+", got "+e.code+": "+e.what());}
 catch(const std::exception& e){check(false,e.what());}
}
std::uint32_t bits(std::string_view s){std::uint32_t n=0;const auto r=std::from_chars(s.data(),s.data()+s.size(),n,16);if(r.ec!=std::errc{})throw std::runtime_error("Bad bits");return n;}
std::vector<std::string> split(std::string_view s,char separator){
 std::vector<std::string> out;std::size_t p=0;for(;;){auto end=s.find(separator,p);out.emplace_back(s.substr(p,end==s.npos ? s.size()-p : end-p));if(end==s.npos)break;p=end+1;}return out;
}
std::string text(float n){const std::array<abc::Value,1> args{n};return abc::to_utf8(std::get<std::u16string>(*abc::stdlib::invoke_text(abc::stdlib::Function::float_to_str,args,{})));}
void reference(){
 std::ifstream in(std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures"/"single-reference.tsv");
 check(bool(in),"Independent single corpus present");std::string row;std::size_t rows=0;
 while(std::getline(in,row)){
  if(!row.empty() && row.back()=='\r')row.pop_back();if(row.empty() || row.front()=='#')continue;
  auto c=split(row,'\t');if(c.size()!=4){check(false,"Malformed single reference");continue;}
  try{
   float value=0;const auto& kind=c[0];const auto& input=c[1];
   if(kind=="parse"){const auto parsed=abc::stdlib::try_parse_number(abc::Type::single,abc::Value{abc::from_utf8(input)},{});check(parsed.has_value(),input);if(!parsed)continue;value=std::get<float>(*parsed);}
   else if(kind=="bits")value=std::bit_cast<float>(bits(input));
   else if(kind=="double")value=abc::single_number(abc::Value{std::stod(input)},{});
   else if(kind=="int64")value=abc::single_number(abc::Value{static_cast<std::int64_t>(std::stoll(input))},{});
   else if(kind=="uint64")value=abc::single_number(abc::Value{static_cast<std::uint64_t>(std::stoull(input))},{});
   else if(kind=="big")value=abc::single_number(abc::Value{abc::BigInteger::parse(input)},{});
   else if(kind=="op"){
    const auto args=split(input,',');const auto op=args[1]=="+" ? abc::Op::add : args[1]=="-" ? abc::Op::subtract : args[1]=="*" ? abc::Op::multiply : abc::Op::divide_real;
    value=abc::single_binary(op,std::bit_cast<float>(bits(args[0])),std::bit_cast<float>(bits(args[2])),{});
   }else if(kind=="sum" || kind=="average"){
    std::string source="begin var a:=new single["+std::to_string(input.empty() ? 0 : split(input,',').size())+"];";std::size_t i=0;
    if(!input.empty())for(const auto& item:split(input,','))source+="a["+std::to_string(i++)+"]:=single.Parse('"+text(std::bit_cast<float>(bits(item)))+"');";
    source+="Writeln(a."+std::string(kind=="sum" ? "Sum" : "Average")+");end.";result(source,c[3]+"\n");++rows;continue;
   }else{check(false,"Unknown single reference kind");continue;}
   check(std::bit_cast<std::uint32_t>(value)==bits(c[2]),kind+" "+input+" exact binary32");
   check(text(value)==c[3],kind+" "+input+" canonical format -> "+text(value));
   if(kind=="parse")result("begin Writeln(single.Parse('"+input+"'));end.",c[3]+"\n");
   ++rows;
  }catch(const std::exception& e){check(false,row+" -> "+e.what());}
 }
 check(rows>1100,"Full independent binary32 corpus consumed");
}
void language(){
 result("begin var s:single;var e:integer;Val('1,234.5',s,e);Writeln(s,' ',e);Val(nil,s,e);Writeln(s,' ',e);Writeln(TryStrToSingle('1,234.5',s),' ',s);Writeln(single.TryParse('1,234.5',s),' ',s);end.","1234.5 0\n0 0\nFalse 0\nTrue 1234.5\n");
 result("begin var s:single:=2;s+=1;s*=2;s-=single(0.5);s/=2;Writeln(s,' ',-s,' ',Clamp(s,single(1),single(2)),' ',s.InRange(single(0),single(3)));end.","2.75 -2.75 2 True\n");
 result("begin var f:Text;Rewrite(f,'single-binary32-roundtrip.tmp');Write(f,MinSingle,' ',MaxSingle);Close(f);Reset(f);var a,b:single;Read(f,a,b);Close(f);Erase(f);Writeln(a=MinSingle,' ',b=MaxSingle);end.","True True\n");
 result("begin Writeln(single(#65));end.","65\n");
 result("var r:real;s:single;begin r:=3.14;s:=3.14;assert(r=3.14);s:=3.14;assert(s=single(3.14));end.","");
 result("begin var s:single;Writeln(s,' ',TypeName(s),' ',TypeToTypeName(typeof(System.Single)));s:=16777216;s:=s+1;Writeln(s,' ',TypeName(s+1),' ',TypeName(s+1.0),' ',TypeName(s/2),' ',TypeName(s/2.0));end.","0 single single\n16777216 single real single real\n");
 result("begin var s:single:=single.Parse('1.000000059604644775390626');Writeln(s,' ',single(1.000000059604644775390626),' ',single.Parse('1e-9999'),' ',single.Parse('-1e-9999'));end.","1.0000001 1 0 -0\n");
 result("begin Writeln(MaxSingle,' ',MinSingle,' ',single.MaxValue,' ',single.MinValue,' ',System.Single.Epsilon);end.","3.4028235E+38 1E-45 3.4028235E+38 -3.4028235E+38 1E-45\n");
 result("begin var s:single:=-3.25;Writeln(TypeName(Abs(s)),' ',Abs(s),' ',TypeName(Sqr(s)),' ',TypeName(Sign(s)),' ',Min(s,single(2)),' ',Max(s,single(2)));end.","single 3.25 real integer -3.25 2\n");
 result("procedure P(var s:single);begin s:=s+single(0.25);end;function F(x:single):single;begin Result:=x*2;end;begin var a:=new single[2];P(a[0]);var f:single->single:=F;Writeln(a,' ',f(a[0]));var absf:single->single:=Abs;Writeln(absf(single(-2)));end.","[0.25,0] 0.5\n2\n");
 result("type R=record x:single;end;C=class x:single;end;begin var r:R;var c:=new C;r.x:=1.25;c.x:=r.x;var b:=new single[1,2];b[0,1]:=c.x;Writeln(r.x,' ',c.x,' ',b[0,1],' ',TypeName(b));end.","1.25 1.25 1.25 array[,] of single\n");
 result("begin var a:=Arr(single(3),single(1),single(2));Sort(a);var l:=new List<single>;l.AddRange(a);var h:=new HashSet<single>;h.Add(single(0));h.Add(single.Parse('-0'));h.Add(single(2));Writeln(a,' ',l.Contains(single(1)),' ',h.Count,' ',h.Contains(single(0)));end.","[1,2,3] True 2 True\n");
 result("begin var a:=Arr(single(16777216),single(1),single(-16777216));Writeln(a.Sum,' ',a.Average,' ',TypeName(a.Sum),' ',TypeName(a.Average),' ',a.Sum(x->x));end.","1 0.33333334 single single 1\n");
 result("begin var a:=new single[0];Writeln(a.Sum,' ',TypeName(a.Sum));end.","0 single\n");
 result("begin var s:single:=9;Writeln(TryStrToSingle('3.25',s),' ',s);Writeln(TryStrToFloat('bad',s),' ',s);Writeln(System.Single.TryParse('1e-45',s),' ',s);var e:integer;Val('oops',s,e);Writeln(s,' ',e);Val('1.25',s,e);var t:string;Str(s,t);Writeln(t,' ',e);end.","True 3.25\nFalse 0\nTrue 1E-45\n0 1\n1.25 0\n");
 result("begin var s:single;Read(s);Writeln(s,' ',TypeName(s));Writeln(TryRead(s),' ',s);end.","1.0000001 single\nFalse 1.0000001\n","1.000000059604644775390626 bad");
 result("begin var s:single:=1.25;Writeln(s:6:2,' ',s.ToString,' ',FormatValue(s,5,1));end.","  1.25 1.25   1.2\n");
 result("begin var s:single:=-3.9;Writeln(integer(s),' ',int64(s),' ',BigInteger(s),' ',real(s),' ',TypeName(s.Print));end.","-3.9000000953674316 -3 -3 -3 -3.9000000953674316 real\n");
 result("const s=single(16777217);begin Writeln(s,' ',TypeName(s));end.","16777216 single\n");
 result("begin var a:=Arr(single(1),single(2));Writeln(a.Select(Abs).ToArray,' ',a.Select(x->x+single(0.5)).ToArray);end.","[1,2] [1.5,2.5]\n");
 for(const auto source:{"begin var s:single;var b:boolean:=s;end.","begin var s:single;Inc(s);end.","begin Writeln(single(2) div 1);end.","begin Writeln(single(2) mod 1);end.","begin var s:single;Read(single(1));end.","procedure P(var n:real);begin end;begin var s:single;P(s);end.","begin var a:=Arr(single(1));Writeln(a.Product);end.","begin var a:=Arr(single(1));Writeln(a.PartialSum.ToArray);end.","begin var r:real;TryStrToSingle('1',r);end.","begin single.MaxValue();end."})
  error([&]{abc::compile(source);},"E_TYPE");
 for(const auto source:{"begin var s:single:=MaxSingle;s:=s*2;end.","begin var s:single:=1e39;end.","begin Writeln(single.Parse('1e39'));end.","begin Writeln(integer(single(2147483648)));end.","begin Writeln(uint64(single(18446744073709551615)));end."})
  error([&]{run(source);},"E_OVERFLOW");
 error([]{run("begin var s:single:=1;Writeln(s/single(0));end.");},"E_DIV_ZERO");
 error([]{run("begin Writeln(single.Parse('bad'));end.");},"E_FORMAT");
 error([]{run("begin var a:=new single[0];Writeln(a.Average);end.");},"E_EMPTY");
 for(int mode=0;mode<3;++mode){auto damaged=abc::compile("begin var s:single;single.TryParse('1',s);end.");for(auto& op:damaged.code)if(op.op==abc::Op::try_parse){if(mode==0)op.operand=2;if(mode==1)op.type=abc::Type::real;if(mode==2)op.op=abc::Op::val;}error([&]{abc::verify(damaged);},"E_BYTECODE");}
 auto aggregate=abc::compile("begin Writeln(Arr(single(1)).Sum);end.");for(auto& op:aggregate.code)if(op.op==abc::Op::sequence_aggregate)op.operand=static_cast<std::size_t>(abc::SequenceAggregate::product);error([&]{abc::verify(aggregate);},"E_BYTECODE");
 auto p=abc::compile("begin var s:single:=1;var x:=s+s;var y:=s+1.0;end.");
 check(p.locals[0].type==abc::Type::single && p.locals[1].type==abc::Type::single && p.locals[2].type==abc::Type::real,"Binary32 types known before execution");
 auto bad=p;bool modified=false;for(auto& op:bad.code)if(op.op==abc::Op::add && op.type==abc::Type::single){op.type=abc::Type::real;modified=true;}check(modified,"Binary32 instruction found");error([&]{abc::verify(bad);},"E_BYTECODE");
 bad=abc::compile("begin Writeln(MaxSingle);end.");modified=false;for(auto& constant:bad.constants)if(std::holds_alternative<float>(constant)){constant=std::numeric_limits<float>::infinity();modified=true;}check(modified,"Binary32 constant found");error([&]{abc::verify(bad);},"E_BYTECODE");
}
}
int main(){reference();language();std::cout<<(checks-failures)<<'/'<<checks<<" single checks passed\n";return failures ? 1 : 0;}

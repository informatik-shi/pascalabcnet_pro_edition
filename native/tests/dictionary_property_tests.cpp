#include "abc/core.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
namespace {
int checks=0,failures=0;
void check(bool ok,std::string_view message){++checks;if(!ok){++failures;if(failures<=20)std::cerr<<"FAIL "<<message<<'\n';}}
std::string run(std::string_view source,abc::ExecutionOptions options={}){std::istringstream input;std::ostringstream output;abc::execute(abc::compile(source),input,output,options);return output.str();}
void result(std::string_view source,std::string_view expected){try{check(run(source)==expected,source);}catch(const std::exception& e){check(false,std::string(source)+": "+e.what());}}
template<class F>void error(F action,std::string_view code){try{action();check(false,"Expected "+std::string(code));}catch(const abc::Error& e){check(e.code==code&&!e.russian.empty()&&*e.what(),std::string(code)+": "+e.code+" "+e.what());}}
std::vector<std::string> split(std::string_view text){std::vector<std::string> fields;std::size_t at=0;for(;;){auto end=text.find('\t',at);fields.emplace_back(text.substr(at,end==text.npos?end:end-at));if(end==text.npos)return fields;at=end+1;}}
std::string decode(std::string_view hex){if(hex=="-")return {};std::u16string text;for(std::size_t n=0;n<hex.size();n+=4)text.push_back(static_cast<char16_t>(std::stoul(std::string(hex.substr(n,4)),nullptr,16)));return abc::to_utf8(text);}
void reference(){
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::ifstream input(fixtures/"dictionary-property-reference.tsv");check(bool(input),"Independent dictionary property corpus exists");std::map<std::pair<int,int>,abc::Program> programs;std::string row;int rows=0;
 while(std::getline(input,row)){
  if(!row.empty()&&row.back()=='\r')row.pop_back();if(row.empty()||row.front()=='#')continue;const auto f=split(row);if(f.size()!=9){check(false,"Malformed dictionary property row");continue;}
  const auto key=std::make_pair(std::stoi(f[0]),std::stoi(f[1]));if(!programs.contains(key)){std::ifstream source(fixtures/("dictionary-property-"+f[0]+"-"+f[1]+".pas"));check(bool(source),"Static typed dictionary program exists");std::ostringstream text;text<<source.rdbuf();programs.emplace(key,abc::compile(text.str()));}
  std::string data;for(int i=2;i<7;++i)data+=f[i]+" ";std::istringstream in(data);std::ostringstream out;std::string code="ok";
  try{abc::execute(programs.at(key),in,out);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty()&&*e.what(),"Dictionary property error is bilingual");}
  check(code==f[8]&&out.str()==decode(f[7]),"kind "+f[0]+" op "+f[1]+" "+data+"expected "+f[8]+" got "+code+" output "+out.str());++rows;
 }
 check(rows==9984&&programs.size()==13,"All independent dictionary outcomes and typed programs checked");
}
void semantics(){
 result("var d:Dictionary<integer,integer>;calls:integer;function Key:integer;begin Inc(calls);Result:=calls;end;begin d:=DictInt((1,10),(2,20));d[Key()]+=5;Writeln(d,' ',calls);end.","{(1,25),(2,20)} 2\n");
 result("var l:List<integer>;calls:integer;function Index:integer;begin Result:=calls;Inc(calls);end;begin l:=Lst(1,2);l[Index()]+=9;Writeln(l,' ',calls);end.","[11,2] 2\n");
 result("type R=record n:integer;end;var d:Dictionary<integer,R>;calls:integer;function Key:integer;begin Inc(calls);Result:=1;end;begin var r:R;r.n:=3;d:=new Dictionary<integer,R>;d.Add(1,r);Writeln(d[Key()].n,' ',calls);end.","3 1\n");
 for(const auto source:{"begin Write('bad');var d:=DictInt((1,2));d[1]+='x';end.","begin Write('bad');var d:=DictInt((1,2));d[1]/=2;end."}){std::ostringstream out;try{std::istringstream in;abc::execute(abc::compile(source),in,out);check(false,"Invalid dictionary operation rejected");}catch(const abc::Error& e){check(out.str().empty()&&!e.russian.empty(),"Dictionary static rejection precedes output");}}
 abc::ExecutionOptions options;options.instruction_limit=80;error([&]{run("var d:Dictionary<integer,integer>;function Key:integer;begin while True do begin end;Result:=1;end;begin d:=DictInt((1,1));d[Key()]+=2;end.",options);},"E_LIMIT");
}
void lambdas(){
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::ifstream input(fixtures/"dictionary-property-lambda-reference.tsv");check(bool(input),"Independent property lambda corpus exists");std::string row;int rows=0;
 while(std::getline(input,row)){if(!row.empty()&&row.back()=='\r')row.pop_back();if(row.empty())continue;const auto f=split(row);if(f.size()!=3){check(false,"Malformed property lambda row");continue;}
  const bool dictionary=f[0]=="0",list=f[0]=="1";const std::string type=dictionary?"Dictionary<integer,integer>":list?"List<integer>":"Holder";
  const std::string initializer=dictionary?"DictInt((1,2))":list?"Lst(2)":"new Holder";const std::string index=dictionary?"[1]":list?"[0]":".Value";
  const std::string source="type Holder=class public n:integer:=2;property Value:integer read n write n;end;var callbacks:List<integer->integer>;d:"+type+";function Receiver(f:integer->integer):"+type+";begin callbacks.Add(f);Result:=d;end;procedure Exercise;begin var bias:=7;callbacks:=new List<integer->integer>;d:="+initializer+";Receiver(x->x+"+(f[1]=="0"?"1":"bias")+")"+index+"+=5;Writeln(d"+index+",' ',callbacks.Count,' ',callbacks[0]=callbacks[1],' ',callbacks[0](1));end;begin Exercise;end.";
  result(source,decode(f[2]));++rows;
 }check(rows==6,"All captured and noncapturing property lambda cases checked");
}
}
int main(){try{reference();semantics();lambdas();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" dictionary property checks, "<<failures<<" failures\n";return failures?1:0;}

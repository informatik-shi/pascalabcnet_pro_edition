#include "abc/core.hpp"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
int checks=0,failures=0;
void check(bool ok,std::string_view label){++checks;if(!ok){++failures;if(failures<=20)std::cerr<<"FAIL "<<label<<'\n';}}
std::vector<std::string> split(std::string_view text){std::vector<std::string> f;std::size_t n=0;for(;;){const auto end=text.find('\t',n);f.emplace_back(text.substr(n,end==text.npos?end:end-n));if(end==text.npos)return f;n=end+1;}}
std::string decode(std::string_view hex){if(hex=="-")return {};std::u16string value;for(std::size_t n=0;n<hex.size();n+=4)value.push_back(static_cast<char16_t>(std::stoul(std::string(hex.substr(n,4)),nullptr,16)));return abc::to_utf8(value);}
void reference(){
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::array<abc::Program,2> programs;
 for(int owner=0;owner<2;++owner){std::ifstream source(fixtures/("list-user-bulk-"+std::to_string(owner)+".pas"));check(bool(source),"Closed bulk-operation fixture exists");std::ostringstream text;text<<source.rdbuf();programs[owner]=abc::compile(text.str());}
 std::ifstream data(fixtures/"list-user-bulk-reference.tsv");check(bool(data),"Independent Framework bulk-operation corpus exists");std::string row;int rows=0;
 while(std::getline(data,row)){if(!row.empty()&&row.back()=='\r')row.pop_back();if(row.empty()||row.front()=='#')continue;const auto f=split(row);if(f.size()!=9){check(false,"Malformed bulk-operation row");continue;}
  std::istringstream in(f[1]+" "+f[2]+" "+f[3]+" "+f[4]+" "+f[5]+" "+f[6]);std::ostringstream out;std::string code="ok";
  try{abc::execute(programs.at(std::stoi(f[0])),in,out);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty()&&*e.what()&&e.location.line>0,"Bilingual bulk-operation error with position");}
  check(code==f[8]&&out.str()==decode(f[7]),row+" actual "+code+" "+out.str());++rows;
 }
 check(rows==4200,"All independent bulk-operation outcomes checked");
}
void records(){
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::array<abc::Program,2> programs;
 for(int owner=0;owner<2;++owner){std::ifstream source(fixtures/("list-user-record-bulk-"+std::to_string(owner)+".pas"));check(bool(source),"Record bulk-operation fixture exists");std::ostringstream text;text<<source.rdbuf();programs[owner]=abc::compile(text.str());}
 std::ifstream data(fixtures/"list-user-record-bulk-reference.tsv");check(bool(data),"Independent record bulk-operation corpus exists");std::string row;int rows=0;
 while(std::getline(data,row)){if(!row.empty()&&row.back()=='\r')row.pop_back();if(row.empty()||row.front()=='#')continue;const auto f=split(row);if(f.size()!=8){check(false,"Malformed record bulk-operation row");continue;}
  std::istringstream in(f[1]+" "+f[2]+" "+f[3]+" "+f[4]+" "+f[5]);std::ostringstream out;std::string code="ok";
  try{abc::execute(programs.at(std::stoi(f[0])),in,out);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty()&&*e.what()&&e.location.line>0,"Bilingual record bulk error with position");}
  check(code==f[7]&&out.str()==decode(f[6]),row+" actual "+code+" "+out.str());++rows;
 }
 check(rows==336,"All independent record bulk-operation outcomes checked");
}
void limits_and_types(){
 for(const auto source:{"begin Write('bad');var a:=Lst(1);var c:ICollection<string>:=new List<string>;a.AddRange(c);end.","begin Write('bad');var a:=Lst(1);a.InsertRange('x',Arr(2));end."}){
  std::istringstream input;std::ostringstream output;try{abc::execute(abc::compile(source),input,output);check(false,"Wrong bulk-operation types rejected");}catch(const abc::Error& e){check(output.str().empty()&&!e.russian.empty(),"Bulk types known before execution");}
 }
 auto program=abc::compile("begin var a:=Lst(1);a.AddRange(Arr(2));end.");abc::ExecutionOptions options;options.cancelled=[] {return true;};std::istringstream input;std::ostringstream output;
 try{abc::execute(program,input,output,options);check(false,"Bulk operation cancellation");}catch(const abc::Error& e){check(e.code=="E_CANCELLED"&&!e.russian.empty(),"Bulk operation respects shared cancellation");}
}
}
int main(){try{reference();records();limits_and_types();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" user collection bulk checks, "<<failures<<" failures\n";return failures?1:0;}

#include "abc/core.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
namespace {
int checks=0,failures=0;
void check(bool ok,std::string_view label){++checks;if(!ok){++failures;if(failures<=20)std::cerr<<"FAIL "<<label<<'\n';}}
std::vector<std::string> split(std::string_view text){std::vector<std::string> fields;std::size_t begin=0;for(;;){const auto end=text.find('\t',begin);fields.emplace_back(text.substr(begin,end==text.npos?end:end-begin));if(end==text.npos)return fields;begin=end+1;}}
std::string decode(std::string_view hex){if(hex=="-")return {};std::u16string value;for(std::size_t n=0;n<hex.size();n+=4)value.push_back(static_cast<char16_t>(std::stoul(std::string(hex.substr(n,4)),nullptr,16)));return abc::to_utf8(value);}
void reference(){
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::map<std::pair<std::string,std::string>,abc::Program> programs;
 std::ifstream data(fixtures/"predicate-generics-reference.tsv");check(bool(data),"Generic predicate reference exists");std::string row;int rows=0;
 while(std::getline(data,row)){if(!row.empty()&&row.back()=='\r')row.pop_back();if(row.empty()||row.front()=='#')continue;const auto f=split(row);if(f.size()!=8){check(false,"Malformed generic predicate row");continue;}
  const auto key=std::make_pair(f[0],f[1]);
  if(!programs.contains(key)){
   std::ifstream source(fixtures/("predicate"+f[1]+"-program.pas"));check(bool(source),"Generic predicate source exists");std::ostringstream text;text<<source.rdbuf();programs.emplace(key,abc::compile(text.str(),f[0]));
  }
  std::istringstream input(f[2]+" "+f[3]+" "+f[4]+" "+f[5]);std::ostringstream output;std::string code="ok";
  try{abc::execute(programs.at(key),input,output);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty()&&*e.what()&&e.location.line>0,"Bilingual generic predicate error with source position");}
  check(code==f[7]&&output.str()==decode(f[6]),row+" actual "+code+" "+output.str());++rows;
 }
 check(rows==2304,"All generic predicate outcomes checked");check(programs.size()==4,"Both arities and source modules checked");
 for(const auto arity:{"2","3"})for(unsigned mode=0;mode<5;++mode){auto bad=programs.at({"Program",arity});for(auto& type:bad.types)if(type.name==std::string("predicate")+arity){
   if(mode==0)type.element=abc::Type::integer;if(mode==1)type.parameters.pop_back();if(mode==2)type.parameters[0].by_reference=true;if(mode==3)type.parameters.back().variadic=true;if(mode==4)type.parameters[0].type=abc::Type::unit;break;
  }
  try{abc::verify(bad);check(false,"Forged generic predicate accepted");}catch(const abc::Error& e){check(e.code=="E_BYTECODE"&&!e.russian.empty(),"Generic predicate metadata verified");}
 }
}
}
int main(){try{reference();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" generic predicate checks, "<<failures<<" failures\n";return failures?1:0;}

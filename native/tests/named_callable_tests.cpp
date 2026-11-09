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
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::ifstream source(fixtures/"named-callables-program.pas");check(bool(source),"Named delegate source exists");std::ostringstream text;text<<source.rdbuf();std::map<std::string,abc::Program> programs;
 std::ifstream data(fixtures/"named-callables-reference.tsv");check(bool(data),"Named delegate reference exists");std::string row;int rows=0;
 while(std::getline(data,row)){if(!row.empty()&&row.back()=='\r')row.pop_back();if(row.empty()||row.front()=='#')continue;const auto f=split(row);if(f.size()!=7){check(false,"Malformed named delegate row");continue;}
  if(!programs.contains(f[0]))programs.emplace(f[0],abc::compile(text.str(),f[0]));
  std::istringstream input(f[1]+" "+f[2]+" "+f[3]+" "+f[4]);std::ostringstream output;std::string code="ok";
  try{abc::execute(programs.at(f[0]),input,output);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty()&&*e.what()&&e.location.line>0,"Bilingual named delegate error with source position");}
  check(code==f[6]&&output.str()==decode(f[5]),row+" actual "+code+" "+output.str());++rows;
 }
 check(rows==1296,"All named delegate outcomes checked");check(programs.size()==2,"Both source module identities checked");
 for(unsigned mode=0;mode<4;++mode){auto bad=programs.at("Program");for(auto& type:bad.types)if(type.name.starts_with("$delegate:")){
   if(mode==0)type.display_name.clear();if(mode==1)type.name="$delegate:";if(mode==2)type.display_name=std::string(1,static_cast<char>(0xff));if(mode==3)type.display_name=std::string("a\0b",3);break;
  }
  try{abc::verify(bad);check(false,"Forged named delegate accepted");}catch(const abc::Error& e){check(e.code=="E_BYTECODE"&&!e.russian.empty(),"Named delegate metadata verified");}
 }
}
}
int main(){try{reference();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" named delegate checks, "<<failures<<" failures\n";return failures?1:0;}

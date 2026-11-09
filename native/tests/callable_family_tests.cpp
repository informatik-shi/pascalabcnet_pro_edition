#include "abc/core.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
int checks=0,failures=0;
void check(bool ok,std::string_view label){++checks;if(!ok){++failures;if(failures<=20)std::cerr<<"FAIL "<<label<<'\n';}}
std::vector<std::string> split(std::string_view text){std::vector<std::string> fields;std::size_t begin=0;for(;;){const auto end=text.find('\t',begin);fields.emplace_back(text.substr(begin,end==text.npos?end:end-begin));if(end==text.npos)return fields;begin=end+1;}}
std::string decode(std::string_view hex){if(hex=="-")return {};std::u16string value;for(std::size_t n=0;n<hex.size();n+=4)value.push_back(static_cast<char16_t>(std::stoul(std::string(hex.substr(n,4)),nullptr,16)));return abc::to_utf8(value);}
void reference(){
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::ifstream source(fixtures/"callable-family-program.pas");check(bool(source),"Delegate family source exists");std::ostringstream text;text<<source.rdbuf();const auto program=abc::compile(text.str());
 std::ifstream data(fixtures/"callable-family-reference.tsv");check(bool(data),"Delegate family reference exists");std::string row;int rows=0;
 while(std::getline(data,row)){if(!row.empty()&&row.back()=='\r')row.pop_back();if(row.empty()||row.front()=='#')continue;const auto f=split(row);if(f.size()!=6){check(false,"Malformed delegate family row");continue;}
  std::istringstream input(f[0]+" "+f[1]+" "+f[2]+" "+f[3]);std::ostringstream output;std::string code="ok";
  try{abc::execute(program,input,output);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty()&&*e.what()&&e.location.line>0,"Bilingual delegate conversion error with source position");}
  check(code==f[5]&&output.str()==decode(f[4]),row+" actual "+code+" "+output.str());++rows;
 }
 check(rows==504,"All delegate family outcomes checked");
 auto bad=program;for(auto& type:bad.types)if(type.name=="predicate")type.element=abc::Type::integer;
 try{abc::verify(bad);check(false,"Forged Predicate signature rejected");}catch(const abc::Error& e){check(e.code=="E_BYTECODE"&&!e.russian.empty(),"Predicate metadata verified");}
}
}
int main(){try{reference();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" callable family checks, "<<failures<<" failures\n";return failures?1:0;}

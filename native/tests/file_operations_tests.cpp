#include "abc/core.hpp"
#include "../src/file_operations.hpp"
#include "../src/file_encoding.hpp"
#include "../src/files.hpp"
#include "../src/text_file.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace {
int checks=0,failures=0;
void check(bool ok,std::string_view message){++checks;if(!ok){++failures;if(failures<=18)std::cerr<<"FAIL "<<message<<'\n';}}
std::vector<std::string> split(std::string_view text){std::vector<std::string> result;std::size_t start=0;for(;;){const auto end=text.find('\t',start);result.emplace_back(text.substr(start,end==text.npos?end:end-start));if(end==text.npos)return result;start=end+1;}}
std::string bytes(std::string_view hex){std::string result;if(hex=="-")return result;if(hex.size()%2)throw std::runtime_error("Invalid byte hex");for(std::size_t n=0;n<hex.size();n+=2)result.push_back(static_cast<char>(std::stoul(std::string(hex.substr(n,2)),nullptr,16)));return result;}
std::u16string text(std::string_view hex){std::u16string result;if(hex=="-")return result;if(hex.size()%4)throw std::runtime_error("Invalid UTF-16 hex");for(std::size_t n=0;n<hex.size();n+=4)result.push_back(static_cast<char16_t>(std::stoul(std::string(hex.substr(n,4)),nullptr,16)));return result;}
std::string hex(std::u16string_view value){static constexpr char digit[]="0123456789abcdef";std::string result;for(char16_t c:value)for(int shift=12;shift>=0;shift-=4)result+=digit[(c>>shift)&15];return result.empty()?"-":result;}
std::u16string canonical(std::u16string value){for(std::size_t n=0;n+1<value.size();++n)if(value[n]==u'\r'&&value[n+1]==u'\n')value.erase(n,1);return value;}
struct Temporary {
 std::filesystem::path root=std::filesystem::temp_directory_path();
 std::filesystem::path directory=root/("ABC-native-file-recipes-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 Temporary(){if(!std::filesystem::create_directory(directory))throw std::runtime_error("Cannot create recipe directory");}
 ~Temporary(){std::error_code error;const auto parent=std::filesystem::weakly_canonical(root,error);if(error)return;const auto target=std::filesystem::weakly_canonical(directory,error);if(!error&&target.parent_path()==parent&&target.filename().string().starts_with("ABC-native-file-recipes-"))std::filesystem::remove_all(target,error);}
};
std::string read_bytes(const std::filesystem::path& path){std::ifstream input(path,std::ios::binary);if(!input)throw std::runtime_error("Cannot read test bytes");return {std::istreambuf_iterator<char>{input},{}};}
void reference(){
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";
 std::ifstream input(fixtures/"file-operations-reference.tsv");check(bool(input),"Independent stream recipe fixture exists");
 Temporary temporary;const auto file=temporary.directory/"stream.bin";
 abc::FileContext files;abc::ExecutionOptions options;files.execution_options=&options;abc::TextEncoding encoding;
 files.default_encoding=[&](abc::SourceLocation){return &encoding;};
 const abc::Value name=file.u16string();std::string row;int rows=0;
 while(std::getline(input,row)){
  if(!row.empty()&&row.back()=='\r')row.pop_back();if(row.empty()||row.front()=='#')continue;
  const auto f=split(row);if(f.size()!=4){check(false,"Malformed file recipe row");continue;}
  const auto code=std::stoi(f[1]);encoding={static_cast<abc::TextEncodingKind>(code),code!=1251&&code!=20127,false};
  try{
   if(f[0]=="R"){
    const auto data=bytes(f[2]);{std::ofstream output(file,std::ios::binary|std::ios::trunc);output.write(data.data(),static_cast<std::streamsize>(data.size()));if(!output)throw std::runtime_error("Cannot prepare recipe input");}
    const auto actual=canonical(abc::file_operations_read(files,name,{}));
    check(actual==text(f[3]),"Read row "+std::to_string(rows)+" code "+f[1]+" bytes "+f[2].substr(0,42)+" expected "+f[3].substr(0,60)+" got "+hex(actual).substr(0,60));
   }else if(f[0]=="D"){
    const auto actual=abc::decode_text_bytes(bytes(f[2]),encoding,false,{});
    check(actual==text(f[3]),"Decode row "+std::to_string(rows)+" code "+f[1]+" bytes "+f[2].substr(0,42)+" expected "+f[3].substr(0,60)+" got "+hex(actual).substr(0,60));
   }else if(f[0]=="W"){
    const abc::Value value=f[2]=="nil"?abc::Value{abc::NilString{}}:abc::Value{text(f[2])};
    abc::file_operations_write(files,name,value,{});std::ifstream output(file,std::ios::binary);const std::string actual{std::istreambuf_iterator<char>{output},{}};
    check(actual==bytes(f[3]),"Write row "+std::to_string(rows)+" code "+f[1]+" text "+f[2].substr(0,52));
   }else check(false,"Unknown recipe kind");
  }catch(const std::exception& error){check(false,"Row "+std::to_string(rows)+": "+error.what());}
  ++rows;
 }
 check(rows==4459,"All independent complete stream recipes and direct decoder cases checked");
}
void sharing(){
#ifdef _WIN32
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";
 std::ifstream input(fixtures/"file-operations-sharing-windows.tsv");check(bool(input),"Independent Windows sharing fixture exists");
 Temporary temporary;const auto file=temporary.directory/"held.txt";const abc::Value name=file.u16string();
 abc::FileContext files;const abc::TextEncoding encoding{abc::TextEncodingKind::windows1251,false,false};files.default_encoding=[&](abc::SourceLocation){return &encoding;};
 std::string row;int rows=0;
 while(std::getline(input,row)){
  if(!row.empty()&&row.back()=='\r')row.pop_back();if(row.empty()||row.front()=='#')continue;
  const auto f=split(row);if(f.size()!=6){check(false,"Malformed sharing row");continue;}
  {std::ofstream output(file,std::ios::binary|std::ios::trunc);output<<"old";}
  const auto access=std::stoi(f[0]);const DWORD requested=(access&1?GENERIC_READ:0)|(access&2?GENERIC_WRITE:0);
  const auto held=::CreateFileW(file.c_str(),requested,static_cast<DWORD>(std::stoi(f[1])),nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(held==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot open external sharing holder");
  struct Close {HANDLE handle;~Close(){if(handle!=INVALID_HANDLE_VALUE)::CloseHandle(handle);}} close{held};
  std::string outcome="ok",result="-";
  try{if(f[2]=="R")result=hex(canonical(abc::file_operations_read(files,name,{0,9,3})));else abc::file_operations_write(files,name,std::u16string{u"changed"},{0,9,3});}
  catch(const abc::Error& error){outcome=error.code;check(error.location.line==9&&!error.russian.empty()&&*error.what(),"Sharing fault preserves caller location and both languages");}
  ::CloseHandle(held);close.handle=INVALID_HANDLE_VALUE;
  check(outcome==f[3]&&result==f[4]&&read_bytes(file)==bytes(f[5]),"Sharing access "+f[0]+" share "+f[1]+" operation "+f[2]);
  // An unsuccessful open cannot retain a hidden handle or consume the file quota.
  abc::file_operations_write(files,name,std::u16string{u"recovered"},{});
  check(canonical(abc::file_operations_read(files,name,{}))==u"recovered\n","File is usable after the sharing holder closes");
  ++rows;
 }
 check(rows==42,"All native Windows access/share combinations checked");
#endif
}
template<class F>void error(F action,std::string_view code){try{action();check(false,"Expected "+std::string(code));}catch(const abc::Error& caught){check(caught.code==code&&!caught.russian.empty()&&*caught.what(),"Bilingual "+std::string(code));}}
void recovery(){
 Temporary temporary;abc::FileContext files;abc::ExecutionOptions options;files.execution_options=&options;
 const abc::TextEncoding encoding{abc::TextEncodingKind::utf32_le,true,false};files.default_encoding=[&](abc::SourceLocation){return &encoding;};
 const auto path=temporary.directory/"repair.bin";const abc::Value name=path.u16string(),missing=(temporary.directory/"missing"/"file.txt").u16string();
 for(int n=0;n<100;++n){error([&]{abc::file_operations_read(files,missing,{});},"E_IO");error([&]{abc::file_operations_write(files,missing,std::u16string{u"x"},{});},"E_IO");}
 {std::ofstream output(path,std::ios::binary);output.write("\xff\xfe",2);}
 check(canonical(abc::file_operations_read(files,name,{}))==u"\ufffd\n","Incomplete configured UTF-32LE preamble remains decoder data at EOF");
 abc::file_operations_write(files,name,std::u16string{u"ready"},{});check(canonical(abc::file_operations_read(files,name,{}))==u"ready\n","Failed opens do not leak the shared Text quota");
 options.instruction_limit=1;
 error([&]{abc::file_operations_read(files,name,{});},"E_LIMIT");options.instruction_limit=10'000'000;
 options.cancelled=[](){return true;};error([&]{abc::file_operations_read(files,name,{});},"E_CANCELLED");options.cancelled={};
 abc::file_operations_write(files,name,std::u16string{u"after cancellation"},{});
 check(canonical(abc::file_operations_read(files,name,{}))==u"after cancellation\n","Cancellation releases native readers before reopening for write");
}
}
int main(){try{reference();sharing();recovery();}catch(const std::exception& error){check(false,error.what());}std::cout<<checks<<" file operation checks, "<<failures<<" failures\n";return failures?1:0;}

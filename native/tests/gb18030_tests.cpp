#include "abc/core.hpp"
#include "../src/file_encoding.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
namespace {
std::uint64_t checks=0,failures=0;
void check(bool ok,std::string_view label){++checks;if(!ok){++failures;if(failures<=12)std::cerr<<"FAIL "<<label<<'\n';}}
struct Reader{
 std::ifstream file;explicit Reader(const std::filesystem::path& path):file(path,std::ios::binary){if(!file)throw std::runtime_error("Missing GB18030 reference");}
 unsigned byte(){const auto b=file.get();if(b<0)throw std::runtime_error("Truncated GB18030 reference");return static_cast<unsigned char>(b);}
 unsigned word(){const auto a=byte();return a|(byte()<<8);}
 std::uint32_t number(){const auto a=word();return a|(word()<<16);}
 std::string bytes(unsigned n){std::string value;for(unsigned k=0;k<n;++k)value+=static_cast<char>(byte());return value;}
 std::string text(){unsigned n=0,s=0,b;do{b=byte();n|=(b&127)<<s;s+=7;if(s>28)throw std::runtime_error("Invalid reference string");}while(b&128);return bytes(n);}
};
bool bit(std::string_view bits,unsigned n){return (static_cast<unsigned char>(bits[n>>3])>>(n&7))&1;}
std::string packed(std::uint32_t value){std::string result;if(value>65535){result+=static_cast<char>(value>>24);result+=static_cast<char>(value>>16);result+=static_cast<char>(value>>8);}else if(value>255)result+=static_cast<char>(value>>8);result+=static_cast<char>(value);return result;}
std::u16string scalar(std::uint32_t value){if(value==0xffffffff)return u"?";if(value<65536)return std::u16string(1,static_cast<char16_t>(value));value-=65536;std::u16string s;s+=static_cast<char16_t>(0xd800+(value>>10));s+=static_cast<char16_t>(0xdc00+(value&1023));return s;}
std::string four(unsigned n){std::string result(4,'\0');result[3]=static_cast<char>(n%10+0x30);n/=10;result[2]=static_cast<char>(n%126+0x81);n/=126;result[1]=static_cast<char>(n%10+0x30);result[0]=static_cast<char>(n/10+0x81);return result;}
void reference(){
 Reader reader(std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures/gb18030-reference.bin");
 check(reader.bytes(8)=="ABCGB181"&&reader.number()==54936,"GB18030 reference header");const auto runtime=reader.text();
 abc::TextEncoding normal{abc::TextEncodingKind::gb18030,false,false},strict=normal;strict.strict=true;
 std::vector<std::uint32_t> encoded(65536);for(auto& value:encoded)value=reader.number();const auto exact=reader.bytes(8192);
 for(unsigned n=0;n<65536;++n){const auto input=std::u16string(1,static_cast<char16_t>(n));const auto expected=packed(encoded[n]);check(abc::encode_text_bytes(input,normal,{})==expected,"Every GB18030 BMP default encoding");
  try{const auto output=abc::encode_text_bytes(input,strict,{0,17,2});check(bit(exact,n)&&output==expected,"Strict BMP encoding");}catch(const abc::Error& e){check(!bit(exact,n)&&e.code=="E_ENCODING"&&!e.russian.empty(),"Strict BMP fallback");}
 }
 std::vector<char16_t> singles(256);for(auto& value:singles)value=static_cast<char16_t>(reader.word());const auto singleExact=reader.bytes(32);
 for(unsigned n=0;n<256;++n){const auto input=std::string(1,static_cast<char>(n));const auto expected=std::u16string(1,singles[n]);check(abc::decode_text_bytes(input,normal,false,{})==expected,"Every single GB18030 byte");
  try{const auto output=abc::decode_text_bytes(input,strict,false,{});check(bit(singleExact,n)&&output==expected,"Strict single GB18030 byte");}catch(const abc::Error& e){check(!bit(singleExact,n)&&e.code=="E_ENCODING"&&!e.russian.empty(),"Strict single GB18030 rejection");}
 }
 for(unsigned n=0;n<65536;++n){const auto value=reader.number();const auto flags=reader.byte();std::u16string expected(1,static_cast<char16_t>(value));if((flags&3)==2)expected+=static_cast<char16_t>(value>>16);std::string input;input+=static_cast<char>(n>>8);input+=static_cast<char>(n);
  check(abc::decode_text_bytes(input,normal,false,{})==expected,"Every GB18030 byte pair");
  try{const auto output=abc::decode_text_bytes(input,strict,false,{});check((flags&4)&&output==expected,"Strict GB18030 pair");}catch(const abc::Error& e){check(!(flags&4)&&e.code=="E_ENCODING"&&!e.russian.empty(),"Strict GB18030 pair rejection");}
 }
 const auto fourCount=reader.number();check(fourCount==1587600,"Complete GB18030 four-byte grammar");
 for(unsigned n=0;n<fourCount;++n){const auto value=reader.number();const auto input=four(n);const auto expected=scalar(value);check(abc::decode_text_bytes(input,normal,false,{})==expected,"Every four-byte GB18030 mapping/gap");
  try{const auto output=abc::decode_text_bytes(input,strict,false,{0,17,2});check(value!=0xffffffff&&output==expected,"Strict four-byte mapping");}catch(const abc::Error& e){check(value==0xffffffff&&e.code=="E_ENCODING"&&!e.russian.empty()&&e.location.line==17,"Strict four-byte gap");}
  for(unsigned cut=1;cut<=4;++cut){abc::TextDecoder decoder(normal,false);auto output=decoder.decode(std::string_view(input).substr(0,cut),false,{});output+=decoder.decode(std::string_view(input).substr(cut),true,{});check(output==expected,"Every four-byte decoder split");}
 }
 const auto scalarCount=reader.number();check(scalarCount==1048576,"All supplementary scalars referenced");
 for(unsigned n=0;n<scalarCount;++n){const auto expected=packed(reader.number());const auto input=scalar(n+65536);check(abc::encode_text_bytes(input,normal,{})==expected,"Every supplementary scalar encoding");check(abc::encode_text_bytes(input,strict,{})==expected,"Every strict supplementary encoding");}
 const auto cases=reader.number();check(cases==134400,"Full GB18030 streaming corpus");
 for(unsigned row=0;row<cases;++row){const auto input=reader.bytes(reader.byte());for(unsigned cut=0;cut<=input.size();++cut){const auto flags=reader.byte();std::u16string expected;for(unsigned n=0;n<(flags&127);++n)expected+=static_cast<char16_t>(reader.word());
  abc::TextDecoder decoder(normal,false);auto output=decoder.decode(std::string_view(input).substr(0,cut),false,{});output+=decoder.decode(std::string_view(input).substr(cut),true,{});check(output==expected,"Streaming invalid/incomplete prefix and final flush");
  try{abc::TextDecoder decoderStrict(strict,false);auto outputStrict=decoderStrict.decode(std::string_view(input).substr(0,cut),false,{0,17,2});outputStrict+=decoderStrict.decode(std::string_view(input).substr(cut),true,{0,17,2});check((flags&128)&&outputStrict==expected,"Strict streaming prefix");}catch(const abc::Error& e){check(!(flags&128)&&e.code=="E_ENCODING"&&!e.russian.empty()&&e.location.line==17,"Strict streaming rejection");}
 }}
 check(reader.file.peek()==std::char_traits<char>::eof(),"All reference data consumed");
 const auto program=abc::compile("begin var e:=Encoding.GetEncoding('GB18030');Writeln(e.CodePage);Writeln(e.GetType.FullName);Writeln(e=Encoding.GetEncoding(54936));Writeln(e.GetString(e.GetBytes('中文 🌍')));end.");std::istringstream input;std::ostringstream output;abc::execute(program,input,output);check(output.str()=="54936\n"+runtime+"\nTrue\n中文 🌍\n","Pascal GB18030 factory, metadata and full scalar round trip");
 try{abc::encode_text_bytes(std::u16string(262145,u'\u0080'),normal,{0,19,1});check(false,"GB18030 oversized output accepted");}catch(const abc::Error& e){check(e.code=="E_LIMIT"&&!e.russian.empty()&&e.location.line==19,"GB18030 expanded output bound");}
}
std::string unhex(std::string_view value,bool utf16){
 if(value=="-")return {};std::string bytes;std::u16string text;const auto width=utf16?4U:2U;
 for(std::size_t n=0;n<value.size();n+=width){const auto c=std::stoul(std::string(value.substr(n,width)),nullptr,16);if(utf16)text+=static_cast<char16_t>(c);else bytes+=static_cast<char>(c);}
 return utf16?abc::to_utf8(text):bytes;
}
void files(){
 struct Temporary{std::filesystem::path directory=std::filesystem::temp_directory_path()/("ABC GB18030 "+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));Temporary(){std::filesystem::create_directories(directory);}~Temporary(){std::error_code e;std::filesystem::remove_all(directory,e);}} temporary;
 const auto path=temporary.directory/std::filesystem::path(u8"данные.txt");const auto utf8=path.generic_u8string();std::string literal;for(auto c:utf8){literal+=static_cast<char>(c);if(c==u8'\'')literal+='\'';}
 const auto program=abc::compile("uses FilesOperations;var op:integer;begin Read(op);DefaultEncoding:=Encoding.GetEncoding(54936);var name:='"+literal+"';"
  "if op=0 then WriteStringToFile(name,'A'#13#10'Привет 日本語 中文 한국어 éＡ🌍'#0'Z');"
  "if op=1 then Write(ReadAllText(name));if op=2 then Write(ReadFileToEnd(name));"
  "if op=3 then WriteStringToFile(name,StringOfChar('A',1023)+#55357#56832+StringOfChar('B',1024)+#55357);"
  "if op=4 then begin var f:Text;Assign(f,name);Rewrite(f);Write(f,StringOfChar('A',1023)+#55357);Write(f,#56832+StringOfChar('B',1024)+#55357);Close(f);end;"
  "if op=5 then begin var f:Text;Assign(f,name);Rewrite(f);Write(f,#55357);Flush(f);Write(f,#56832);Close(f);end;end.");
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::ifstream data(fixtures/"gb18030-files-reference.tsv"),legacy(fixtures/"gb18030-files-framework-reference.tsv");check(bool(data)&&bool(legacy),"Both GB18030 file runtime references retained");
 std::string line,oldLine;unsigned rows=0,differences=0;
 while(std::getline(data,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line.front()=='#')continue;
  do{if(!std::getline(legacy,oldLine))throw std::runtime_error("Truncated legacy GB18030 file corpus");if(!oldLine.empty()&&oldLine.back()=='\r')oldLine.pop_back();}while(oldLine.empty()||oldLine.front()=='#');if(line!=oldLine)++differences;
  const auto first=line.find('\t'),second=line.find('\t',first+1);const auto op=std::stoi(line.substr(0,first));const bool write=op==0||op>=3;
  if(write){std::error_code e;std::filesystem::remove(path,e);}else{std::ofstream file(path,std::ios::binary);const auto bytes=unhex(std::string_view(line).substr(first+1,second-first-1),false);file.write(bytes.data(),bytes.size());}
  std::istringstream input(std::to_string(op));std::ostringstream output;abc::execute(program,input,output);const auto expected=unhex(std::string_view(line).substr(second+1),!write);
  if(write){std::ifstream file(path,std::ios::binary);std::ostringstream bytes;bytes<<file.rdbuf();check(output.str().empty()&&bytes.str()==expected,"GB18030 file write, surrogate buffer and Flush");}
  else{auto actual=output.str();if(op==2)for(std::size_t n=0;(n=actual.find("\r\n",n))!=actual.npos;)actual.erase(n,1);check(actual==expected,"GB18030 file read/BOM/EOF row "+std::to_string(rows));}
  ++rows;
 }
 check(rows==786,"All GB18030 file recipes checked");check(differences==490,"Recorded legacy GB18030 StreamReader EOF differences");
}
}
int main(){try{reference();files();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" GB18030 checks, "<<failures<<" failures\n";return failures?1:0;}

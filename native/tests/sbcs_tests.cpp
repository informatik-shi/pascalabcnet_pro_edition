#include "abc/core.hpp"
#include "../src/file_encoding.hpp"
#include "../src/sbcs.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
std::uint64_t checks=0,failures=0;
void check(bool ok,std::string_view label){++checks;if(!ok){++failures;if(failures<=20)std::cerr<<"FAIL "<<label<<'\n';}}
struct Reader{
 std::ifstream input;
 explicit Reader(const std::filesystem::path& path):input(path,std::ios::binary){if(!input)throw std::runtime_error("Missing SBCS reference");}
 std::string bytes(std::size_t n){std::string s(n,'\0');if(!input.read(s.data(),n))throw std::runtime_error("Truncated SBCS reference");return s;}
 std::uint32_t number(){const auto s=bytes(4);std::uint32_t n=0;for(unsigned i=0;i<4;++i)n|=static_cast<std::uint32_t>(static_cast<unsigned char>(s[i]))<<(8*i);return n;}
 std::string text(){unsigned n=0,shift=0,b;do{b=static_cast<unsigned char>(bytes(1)[0]);n|=(b&127U)<<shift;shift+=7;if(shift>28)throw std::runtime_error("Invalid reference string");}while(b&128);return bytes(n);}
};
bool bit(std::string_view data,unsigned n){return (static_cast<unsigned char>(data[n/8])>>(n%8))&1U;}
std::string run(const abc::Program& program,const std::string& data){std::istringstream input(data);std::ostringstream output;abc::execute(program,input,output);return output.str();}
void reference(){
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";Reader reader(fixtures/"sbcs-reference.bin");
 check(reader.bytes(8)=="ABCSBCS2","Reference header");const auto count=reader.number();check(count==95,"All Framework single-byte codecs");
 const auto metadata=abc::compile("begin var cp:integer;Readln(cp);var e:=Encoding.GetEncoding(cp);Writeln(e.CodePage);Writeln(e.GetType.FullName);Writeln(e=Encoding.GetEncoding(cp));end.");
 std::u16string bmp;for(unsigned n=0;n<65536;++n)bmp+=static_cast<char16_t>(n);
 std::string byte_values;for(unsigned n=0;n<256;++n)byte_values+=static_cast<char>(n);
 for(unsigned row=0;row<count;++row){
  const auto cp=reader.number();const auto name=reader.text(),runtime=reader.text();const auto fallback=static_cast<unsigned char>(reader.bytes(1)[0]);
  const bool hold_high=reader.bytes(1)[0]!=0;
  const auto decoded=reader.bytes(512),decode_strict=reader.bytes(32),encoded=reader.bytes(65536),encode_strict=reader.bytes(8192);
  const auto* codec=abc::single_byte_codec(static_cast<std::uint16_t>(cp));check(codec&&codec->replacement==fallback&&codec->hold_high_surrogate==hold_high,"Codec descriptor "+name);if(!codec)continue;
  abc::TextEncoding normal{static_cast<abc::TextEncodingKind>(cp),false,false},strict=normal;strict.strict=true;
  std::u16string expected;
  for(unsigned n=0;n<256;++n){expected+=static_cast<char16_t>(static_cast<unsigned char>(decoded[n*2])|(static_cast<unsigned char>(decoded[n*2+1])<<8));
   try{const auto got=abc::decode_text_bytes(std::string(1,static_cast<char>(n)),strict,false,{0,17,2});check(bit(decode_strict,n)&&got==std::u16string(1,expected.back()),"Strict decode "+name);}
   catch(const abc::Error& e){check(!bit(decode_strict,n)&&e.code=="E_ENCODING"&&!e.russian.empty()&&e.location.line==17,"Invalid byte rejected "+name);}
  }
  check(abc::decode_text_bytes(byte_values,normal,false,{})==expected,"All byte mappings "+name);
  check(abc::encode_text_bytes(bmp,normal,{})==encoded,"Every UTF-16 encode mapping "+name);
  std::u16string exact_chars;std::string exact_bytes;
  for(unsigned n=0;n<65536;++n){const auto value=abc::single_byte_encode(*codec,static_cast<char16_t>(n));const bool exact=bit(encode_strict,n);const auto byte=static_cast<unsigned char>(encoded[n]);
   check((value&255U)==byte&&bool(value&256U)==exact,"Default/exact encoding mapping");
   if(exact){exact_chars+=static_cast<char16_t>(n);exact_bytes+=static_cast<char>(byte);}
   else if(byte!=fallback||n%257==0||(n>=0xd800&&n<=0xdfff)){
    try{abc::encode_text_bytes(std::u16string(1,static_cast<char16_t>(n)),strict,{0,17,2});check(false,"Unmappable character accepted");}
    catch(const abc::Error& e){check(e.code=="E_ENCODING"&&!e.russian.empty()&&e.location.line==17,"Strict encoder rejects best-fit/fallback "+name);}
   }
  }
  check(abc::encode_text_bytes(exact_chars,strict,{})==exact_bytes,"Strict representable batch "+name);
  abc::TextDecoder stream(normal,false);std::u16string streamed;for(char b:byte_values)streamed+=stream.decode(std::string(1,b),false,{});streamed+=stream.decode({},true,{});
  check(streamed==expected,"Streaming all byte mappings "+name);
  check(abc::text_encoding_preamble({normal.kind,true,false}).empty(),"Single-byte codec has no preamble");
  check(run(metadata,std::to_string(cp)+"\n")==std::to_string(cp)+"\n"+runtime+"\nTrue\n","Pascal metadata and cached encoding "+name);
 }
 check(reader.input.peek()==std::char_traits<char>::eof(),"Complete reference consumed");
 const auto aliases_program=abc::compile("begin var name:=ReadlnString;Writeln(Encoding.GetEncoding(name).CodePage);end.");
 std::ifstream aliases(fixtures/"encoding-aliases-reference.tsv");std::string line;unsigned names=0,supported=0;
 while(std::getline(aliases,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line.front()=='#')continue;const auto tab=line.find('\t');const auto name=line.substr(0,tab);const auto cp=static_cast<unsigned>(std::stoul(line.substr(tab+1)));++names;
  const bool valid=abc::single_byte_codec(static_cast<std::uint16_t>(cp))||cp==65001||cp==1200||cp==1201||cp==12000||cp==12001;
  check(abc::encoding_alias_code(abc::from_utf8(name))==(valid?cp:0),"Alias descriptor "+name);
  if(valid){++supported;std::string upper=name;for(auto& c:upper)if(c>='a'&&c<='z')c=static_cast<char>(c-32);check(run(aliases_program,name+"\n")==std::to_string(cp)+"\n"&&run(aliases_program,upper+"\n")==std::to_string(cp)+"\n","Pascal lower/uppercase alias "+name);}
 }
 check(names==405&&supported==318,"All native-supported Framework aliases checked");
 std::ifstream invalid(fixtures/"encoding-name-reference.tsv");unsigned name_rows=0;
 while(std::getline(invalid,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line.front()=='#')continue;const auto a=line.find('\t'),b=line.find('\t',a+1);const auto hex=line.substr(0,a),code=line.substr(a+1,b-a-1),error=line.substr(b+1);std::u16string name;
  if(hex!="-"&&hex!="nil")for(std::size_t i=0;i<hex.size();i+=4)name+=static_cast<char16_t>(std::stoul(hex.substr(i,4),nullptr,16));
  try{const auto result=hex=="nil" ? run(abc::compile("begin Writeln(Encoding.GetEncoding(nil).CodePage);end."),"") : run(aliases_program,abc::to_utf8(name)+"\n");check(error=="ok"&&result==code+"\n","Framework name lookup "+hex);}
  catch(const abc::Error& e){check(e.code==error&&!e.russian.empty(),"Framework name error "+hex);}++name_rows;
 }
 check(name_rows==13,"All invalid/edge encoding names checked");
}
std::string unhex(std::string_view value,bool utf16){
 if(value=="-")return {};std::string bytes;std::u16string text;const auto width=utf16?4U:2U;
 for(std::size_t n=0;n<value.size();n+=width){const auto c=std::stoul(std::string(value.substr(n,width)),nullptr,16);if(utf16)text+=static_cast<char16_t>(c);else bytes+=static_cast<char>(c);}
 return utf16?abc::to_utf8(text):bytes;
}
void files(){
 struct Temporary{
  std::filesystem::path directory=std::filesystem::temp_directory_path()/("ABC SBCS "+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  Temporary(){std::filesystem::create_directories(directory);}~Temporary(){std::error_code e;std::filesystem::remove_all(directory,e);}
 } temporary;
 const auto path=temporary.directory/std::filesystem::path(u8"данные.txt");const auto utf8=path.generic_u8string();std::string literal;
 for(auto c:utf8){literal+=static_cast<char>(c);if(c==u8'\'')literal+='\'';}
 const auto program=abc::compile("uses FilesOperations;var cp,op:integer;begin Read(cp,op);DefaultEncoding:=Encoding.GetEncoding(cp);var name:='"+literal+"';"
  "if op=0 then WriteStringToFile(name,'A'#13#10'Привет éＡ界🌍'#0'Z');"
  "if (op=1) or (op=5) then Write(ReadAllText(name));if op=2 then Write(ReadFileToEnd(name));"
  "if op=3 then WriteStringToFile(name,StringOfChar('A',1023)+#55357#56832+StringOfChar('B',1024)+#55357);"
  "if op=4 then begin var f:Text;Assign(f,name);Rewrite(f);Write(f,StringOfChar('A',1023)+#55357);Write(f,#56832+StringOfChar('B',1024)+#55357);Close(f);end;end.");
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::ifstream data(fixtures/"sbcs-files-reference.tsv");check(bool(data),"SBCS file reference exists");
 std::string line;unsigned rows=0;
 while(std::getline(data,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line.front()=='#')continue;
  const auto first=line.find('\t'),second=line.find('\t',first+1);const auto cp=line.substr(0,first);const auto op=std::stoi(line.substr(first+1,second-first-1));
  const bool write=op==0||op==3||op==4;
  if(write){std::error_code e;std::filesystem::remove(path,e);}
  else {std::ofstream file(path,std::ios::binary);if(op==5)file<<"\xef\xbb\xbf"<<abc::to_utf8(u"Привет");else for(unsigned n=0;n<256;++n)file.put(static_cast<char>(n));}
  auto output=run(program,cp+" "+std::to_string(op));const auto expected=unhex(std::string_view(line).substr(second+1),!write);
  if(write){std::ifstream file(path,std::ios::binary);std::ostringstream bytes;bytes<<file.rdbuf();check(output.empty()&&bytes.str()==expected,"DefaultEncoding write, close and surrogate boundary "+cp);}
  else {if(op==2)for(std::size_t n=0;(n=output.find("\r\n",n))!=output.npos;)output.erase(n,1);check(output==expected,"ReadAllText/ReadFileToEnd/BOM with DefaultEncoding "+cp);}
  ++rows;
 }
 check(rows==570,"All 95 code-page file recipes checked");
}
}
int main(){try{reference();files();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" single-byte encoding checks, "<<failures<<" failures\n";return failures?1:0;}

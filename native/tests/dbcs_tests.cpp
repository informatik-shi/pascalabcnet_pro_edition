#include "abc/core.hpp"
#include "../src/dbcs.hpp"
#include "../src/file_encoding.hpp"
#include <filesystem>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
std::uint64_t checks=0,failures=0;
void check(bool ok,std::string_view text){++checks;if(!ok){++failures;if(failures<=20)std::cerr<<"FAIL "<<text<<'\n';}}
struct Reader{
 std::ifstream input;explicit Reader(const std::filesystem::path& p):input(p,std::ios::binary){if(!input)throw std::runtime_error("Missing DBCS reference");}
 std::string bytes(std::size_t n){std::string s(n,'\0');if(!input.read(s.data(),n))throw std::runtime_error("Truncated DBCS reference");return s;}
 unsigned number(){auto s=bytes(4);unsigned n=0;for(unsigned i=0;i<4;++i)n|=static_cast<unsigned>(static_cast<unsigned char>(s[i]))<<(i*8);return n;}
 std::string text(){unsigned n=0,shift=0,b;do{b=static_cast<unsigned char>(bytes(1)[0]);n|=(b&127U)<<shift;shift+=7;if(shift>28)throw std::runtime_error("Invalid length");}while(b&128);return bytes(n);}
};
unsigned word(std::string_view s,unsigned n){return static_cast<unsigned char>(s[n])|(static_cast<unsigned char>(s[n+1])<<8);}
bool bit(std::string_view s,unsigned n){return (static_cast<unsigned char>(s[n>>3])>>(n&7))&1U;}
std::string run(const abc::Program& p,const std::string& data){std::istringstream input(data);std::ostringstream output;abc::execute(p,input,output);return output.str();}
void reference(){
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";Reader reader(fixtures/"dbcs-reference.bin");
 check(reader.bytes(8)=="ABCDBCS1","DBCS reference header");const auto count=reader.number();check(count==23,"All Framework DBCS codecs");
 const auto metadata=abc::compile("begin var n:integer;Readln(n);var e:=Encoding.GetEncoding(n);Writeln(e.CodePage);Writeln(e.GetType.FullName);Writeln(e=Encoding.GetEncoding(n));end.");
 for(unsigned row=0;row<count;++row){
  const auto cp=reader.number();const auto name=reader.text(),runtime=reader.text();const auto fallback=static_cast<unsigned char>(reader.bytes(1)[0]);const auto replacement=word(reader.bytes(2),0);
  const auto leads=reader.bytes(32),singles=reader.bytes(512),single_exact=reader.bytes(32),pairs=reader.bytes(65536*4),pair_exact=reader.bytes(8192),encoded=reader.bytes(65536*2),encode_exact=reader.bytes(8192);
  const auto* codec=abc::double_byte_codec(static_cast<std::uint16_t>(cp));check(codec&&codec->replacement_byte==fallback&&codec->replacement_character==replacement,"DBCS descriptor "+name);if(!codec)continue;
  abc::TextEncoding normal{static_cast<abc::TextEncodingKind>(cp),false,false},strict=normal;strict.strict=true;
  for(unsigned n=0;n<256;++n){
   check(abc::double_byte_lead(*codec,static_cast<std::uint8_t>(n))==bit(leads,n),"Lead membership");
   const auto expected=std::u16string(1,static_cast<char16_t>(word(singles,n*2)));const auto input=std::string(1,static_cast<char>(n));
   check(abc::decode_text_bytes(input,normal,false,{})==expected,"Every single-byte final mapping");
   try{const auto result=abc::decode_text_bytes(input,strict,false,{0,9,2});check(bit(single_exact,n)&&result==expected,"Strict single-byte mapping");}catch(const abc::Error& e){check(!bit(single_exact,n)&&e.code=="E_ENCODING"&&!e.russian.empty(),"Strict single-byte error");}
  }
  std::u16string exact_chars;std::string exact_bytes;
  for(unsigned n=0;n<65536;++n){
   const auto actual=abc::double_byte_encode(*codec,static_cast<char16_t>(n)),value=word(encoded,n*2);check((actual&65535U)==value&&bool(actual&65536U)==bit(encode_exact,n),"Every BMP default/exact mapping");
   const auto input=std::u16string(1,static_cast<char16_t>(n));std::string expected;if(value>255)expected+=static_cast<char>(value>>8);expected+=static_cast<char>(value&255);
   check(abc::encode_text_bytes(input,normal,{})==expected,"Every BMP public encode mapping");
   if(bit(encode_exact,n)){exact_chars+=static_cast<char16_t>(n);exact_bytes+=expected;}
   else if(value!=fallback||n%257==0||(n>=0xd800&&n<=0xdfff)){
    try{abc::encode_text_bytes(input,strict,{0,9,2});check(false,"Strict best-fit/fallback accepted");}catch(const abc::Error& e){check(e.code=="E_ENCODING"&&!e.russian.empty()&&e.location.line==9,"Strict encoding error");}
   }
   std::string raw;raw+=static_cast<char>(n>>8);raw+=static_cast<char>(n&255);std::u16string decoded(1,static_cast<char16_t>(word(pairs,n*4)));if(!bit(leads,n>>8))decoded+=static_cast<char16_t>(word(pairs,n*4+2));
   check(abc::decode_text_bytes(raw,normal,false,{})==decoded,"Every byte-pair default mapping");
   abc::TextDecoder stream(normal,false);auto streamed=stream.decode(std::string_view(raw).substr(0,1),false,{});streamed+=stream.decode(std::string_view(raw).substr(1),true,{});check(streamed==decoded,"Every byte-pair split mapping");
   try{const auto result=abc::decode_text_bytes(raw,strict,false,{0,9,2});check(bit(pair_exact,n)&&result==decoded,"Strict byte-pair mapping");}catch(const abc::Error& e){check(!bit(pair_exact,n)&&e.code=="E_ENCODING"&&!e.russian.empty(),"Strict byte-pair error");}
  }
  if(cp==51932)for(unsigned n=0;n<65536;++n){
   std::string raw;raw+=static_cast<char>(0x8f);raw+=static_cast<char>(n>>8);raw+=static_cast<char>(n&255);std::u16string expected;
   for(unsigned p=0;p<3;){const auto b=static_cast<unsigned char>(raw[p++]);if(bit(leads,b)&&p<3){const auto code=(b<<8)|static_cast<unsigned char>(raw[p++]);expected+=static_cast<char16_t>(word(pairs,code*4));}else expected+=static_cast<char16_t>(word(singles,b*2));}
   check(abc::decode_text_bytes(raw,normal,false,{})==expected,"EUC-JP every JIS0212-prefix triple");
   for(unsigned cut=1;cut<=2;++cut){abc::TextDecoder stream(normal,false);auto value=stream.decode(std::string_view(raw).substr(0,cut),false,{});value+=stream.decode(std::string_view(raw).substr(cut),true,{});check(value==expected,"EUC-JP triple split at each boundary");}
  }
  check(abc::encode_text_bytes(exact_chars,strict,{})==exact_bytes,"Strict representable batch "+name);
  // All supplementary scalars, in bounded batches below the public 1 Mi limit.
  for(unsigned high=0xd800;high<=0xdbff;high+=32){std::u16string input;for(unsigned h=high;h<high+32;++h)for(unsigned low=0xdc00;low<=0xdfff;++low){input+=static_cast<char16_t>(h);input+=static_cast<char16_t>(low);}check(abc::encode_text_bytes(input,normal,{})==std::string(input.size(),static_cast<char>(fallback)),"Complete supplementary scalar grid "+name);}
  std::istringstream input(std::to_string(cp));std::ostringstream output;abc::execute(metadata,input,output);check(output.str()==std::to_string(cp)+"\n"+runtime+"\nTrue\n","Pascal DBCS metadata/cache "+name);
  unsigned lead=0;while(lead<256&&!bit(leads,lead))++lead;
  const std::string pending(1,static_cast<char>(lead));abc::TextDecoder incomplete(strict,false);
  check(incomplete.decode(pending,false,{}).empty(),"Strict decoder retains incomplete lead");
  try{incomplete.decode({},true,{0,12,3});check(false,"Strict final lead accepted");}catch(const abc::Error& e){check(e.code=="E_ENCODING"&&!e.russian.empty()&&e.location.line==12,"Strict final lead error");}
  abc::TextDecoder bom(normal,true);std::u16string switched;
  for(unsigned char b:std::string("\xef\xbb\xbf\xe6\x97\xa5"))switched+=bom.decode(std::string(1,static_cast<char>(b)),false,{});
  switched+=bom.decode({},true,{});check(switched==u"日","BOM switch split at every byte");
  char16_t wide=0;for(unsigned n=1;n<65536;++n)if(word(encoded,n*2)>255&&bit(encode_exact,n)){wide=static_cast<char16_t>(n);break;}
  check(wide!=0,"Representable two-byte character exists");
  try{abc::encode_text_bytes(std::u16string(524289,wide),normal,{0,13,1});check(false,"Two-byte output exceeded bound");}catch(const abc::Error& e){check(e.code=="E_LIMIT"&&!e.russian.empty()&&e.location.line==13,"Two-byte output bound");}
 }
 check(reader.input.peek()==std::char_traits<char>::eof(),"Full DBCS reference consumed");
 const auto names=abc::compile("begin var name:=ReadlnString;Writeln(Encoding.GetEncoding(name).CodePage);end.");std::ifstream aliasData(fixtures/"encoding-aliases-reference.tsv");std::string line;unsigned accepted=0;
 while(std::getline(aliasData,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line.front()=='#')continue;const auto tab=line.find('\t');const auto name=line.substr(0,tab);const auto cp=static_cast<unsigned>(std::stoul(line.substr(tab+1)));if(!abc::double_byte_codec(static_cast<std::uint16_t>(cp)))continue;++accepted;
  check(abc::double_byte_alias_code(abc::from_utf8(name))==cp,"DBCS alias lookup");std::istringstream input(name+"\n");std::ostringstream output;abc::execute(names,input,output);check(output.str()==std::to_string(cp)+"\n","DBCS Pascal name factory");
 }check(accepted==63,"All DBCS aliases checked");

}
std::string unhex(std::string_view value,bool utf16){
 if(value=="-")return {};std::string bytes;std::u16string text;const auto width=utf16?4U:2U;
 for(std::size_t n=0;n<value.size();n+=width){const auto c=std::stoul(std::string(value.substr(n,width)),nullptr,16);if(utf16)text+=static_cast<char16_t>(c);else bytes+=static_cast<char>(c);}
 return utf16?abc::to_utf8(text):bytes;
}
void files(){
 struct Temporary{
  std::filesystem::path directory=std::filesystem::temp_directory_path()/("ABC DBCS "+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  Temporary(){std::filesystem::create_directories(directory);}~Temporary(){std::error_code e;std::filesystem::remove_all(directory,e);}
 } temporary;
 const auto path=temporary.directory/std::filesystem::path(u8"данные.txt");const auto utf8=path.generic_u8string();std::string literal;
 for(auto c:utf8){literal+=static_cast<char>(c);if(c==u8'\'')literal+='\'';}
 const auto program=abc::compile("uses FilesOperations;var cp,op:integer;begin Read(cp,op);DefaultEncoding:=Encoding.GetEncoding(cp);var name:='"+literal+"';"
  "if op=0 then WriteStringToFile(name,'A'#13#10'Привет 日本語 中文 한국어 éＡ🌍'#0'Z');"
  "if (op=1) or (op=5) or (op=6) then Write(ReadAllText(name));if op=2 then Write(ReadFileToEnd(name));"
  "if op=3 then WriteStringToFile(name,StringOfChar('A',1023)+#55357#56832+StringOfChar('B',1024)+#55357);"
  "if op=4 then begin var f:Text;Assign(f,name);Rewrite(f);Write(f,StringOfChar('A',1023)+#55357);Write(f,#56832+StringOfChar('B',1024)+#55357);Close(f);end;end.");
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::ifstream data(fixtures/"dbcs-files-reference.tsv");check(bool(data),"DBCS file reference exists");
 std::ifstream legacy(fixtures/"dbcs-files-framework-reference.tsv");check(bool(legacy),"Legacy file reference retained");
 std::string line,oldLine;unsigned rows=0,differences=0;
 while(std::getline(data,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line.front()=='#')continue;
  do{if(!std::getline(legacy,oldLine))throw std::runtime_error("Truncated legacy file reference");if(!oldLine.empty()&&oldLine.back()=='\r')oldLine.pop_back();}while(oldLine.empty()||oldLine.front()=='#');
  const auto first=line.find('\t'),second=line.find('\t',first+1),third=line.find('\t',second+1);const auto cp=line.substr(0,first);const auto op=std::stoi(line.substr(first+1,second-first-1));
  const bool write=op==0||op==3||op==4;
  if(op==6){
   const auto tail=oldLine.rfind('\t');const auto* codec=abc::double_byte_codec(static_cast<std::uint16_t>(std::stoul(cp)));
   const auto oldOutput=unhex(std::string_view(oldLine).substr(tail+1),true),modernOutput=unhex(std::string_view(line).substr(third+1),true);
   check(oldLine.substr(0,tail)==line.substr(0,third)&&oldOutput+abc::to_utf8(std::u16string(1,codec->replacement_character))==modernOutput,"Recorded Framework EOF difference "+cp);++differences;
  }else check(line==oldLine,"Other legacy and modern file recipes agree");
  if(write){std::error_code e;std::filesystem::remove(path,e);}
  else {std::ofstream file(path,std::ios::binary);const auto input=unhex(std::string_view(line).substr(second+1,third-second-1),false);file.write(input.data(),input.size());}
  auto output=run(program,cp+" "+std::to_string(op));const auto expected=unhex(std::string_view(line).substr(third+1),!write);
  if(write){std::ifstream file(path,std::ios::binary);std::ostringstream bytes;bytes<<file.rdbuf();check(output.empty()&&bytes.str()==expected,"DefaultEncoding write, close and surrogate boundary "+cp);}
  else {if(op==2)for(std::size_t n=0;(n=output.find("\r\n",n))!=output.npos;)output.erase(n,1);check(output==expected,"ReadAllText/ReadFileToEnd/BOM with DefaultEncoding "+cp+" operation "+std::to_string(op)+" lengths "+std::to_string(output.size())+"/"+std::to_string(expected.size()));}
  ++rows;
 }
 check(rows==161,"All 23 double-byte file recipes checked");
 check(differences==23,"All pending legacy StreamReader EOF differences recorded");
}
}
int main(){try{reference();files();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" DBCS checks, "<<failures<<" failures\n";return failures?1:0;}

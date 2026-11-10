#include "abc/core.hpp"
#include "../src/file_encoding.hpp"
#include "../src/text_file.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
namespace {
std::uint64_t checks=0,failures=0;
void check(bool ok,std::string_view label){++checks;if(!ok){++failures;if(failures<=15)std::cerr<<"FAIL "<<label<<'\n';}}
struct Reader{
 std::ifstream file;explicit Reader(const std::filesystem::path& p):file(p,std::ios::binary){if(!file)throw std::runtime_error("Missing unpacked ISO reference");}
 unsigned byte(){int n=file.get();if(n<0)throw std::runtime_error("Truncated ISO reference");return static_cast<unsigned char>(n);}
 unsigned word(){auto a=byte();return a|(byte()<<8);}
 unsigned number(){auto a=word();return a|(word()<<16);}
 std::string bytes(unsigned n){std::string s;for(unsigned k=0;k<n;++k)s+=static_cast<char>(byte());return s;}
 std::string text(){unsigned n=0,s=0,b;do{b=byte();n|=(b&127)<<s;s+=7;if(s>28)throw std::runtime_error("Invalid length");}while(b&128);return bytes(n);}
 std::pair<std::u16string,bool> result(){auto flags=byte();std::u16string value;for(unsigned n=0;n<(flags&127);++n)value+=static_cast<char16_t>(word());return {value,(flags&128)!=0};}
 template<class F>void runs(F&& callback){auto count=number();unsigned pos=0;for(unsigned k=0;k<count;++k){auto size=number();const bool exact=byte()!=0;auto value=bytes(byte());if(!size||pos+size>65536)throw std::runtime_error("Invalid BMP runs");for(unsigned n=pos;n<pos+size;++n)callback(n,value,exact);pos+=size;}check(pos==65536,"All BMP inputs consumed");}
};
void reference(const std::filesystem::path& path){
 Reader r(path);check(r.bytes(8)=="ABCISO01"&&r.number()==5,"ISO reference header");
 const auto metadata=abc::compile("begin var cp:integer;Read(cp);var e:=Encoding.GetEncoding(cp);Writeln(e.CodePage);Writeln(e.GetType.FullName);Writeln(e=Encoding.GetEncoding(cp));end.");
 for(unsigned row=0;row<5;++row){const auto cp=r.number();const auto runtime=r.text();abc::TextEncoding normal{static_cast<abc::TextEncodingKind>(cp),false,false},strict=normal;strict.strict=true;std::vector<bool> exacts(65536);
  r.runs([&](unsigned n,const std::string& expected,bool exact){exacts[n]=exact;auto input=std::u16string(1,static_cast<char16_t>(n));check(abc::encode_text_bytes(input,normal,{})==expected,"Every ISO BMP encoding");try{auto actual=abc::encode_text_bytes(input,strict,{0,27,2});check(exact&&actual==expected,"Strict ISO encoding");}catch(const abc::Error& e){check(!exact&&e.code=="E_ENCODING"&&!e.russian.empty()&&e.location.line==27,"Strict ISO fallback");}});
  for(unsigned map=r.number();map>0;--map){const auto prefix=r.bytes(r.byte());for(unsigned n=0;n<65536;++n){const auto [expected,exact]=r.result();std::string input;input+=static_cast<char>(n>>8);input+=static_cast<char>(n);abc::TextDecoder decoder(normal,false);auto actual=decoder.decode(prefix,false,{});actual+=decoder.decode(input,true,{});check(actual==expected,"Controlled-mode lookup matrix");
   try{abc::TextDecoder decoderStrict(strict,false);auto actualStrict=decoderStrict.decode(prefix,false,{0,27,2});actualStrict+=decoderStrict.decode(input,true,{0,27,2});check(exact&&actualStrict==expected,"Strict controlled lookup");}catch(const abc::Error& e){check(!exact&&e.code=="E_ENCODING"&&!e.russian.empty(),"Strict controlled fallback");}
  }}
  for(unsigned count=r.number();count>0;--count){std::u16string prefix;for(unsigned n=r.byte();n>0;--n)prefix+=static_cast<char16_t>(r.word());const bool flush=r.byte()!=0;const auto prefixExpected=r.bytes(r.byte());abc::TextEncoder encoder(normal),encoderStrict(strict);check(encoder.encode(prefix,flush,{})==prefixExpected,"Encoder prefix and final reset");check(encoderStrict.encode(prefix,flush,{})==prefixExpected,"Strict encoder prefix");
   r.runs([&](unsigned n,const std::string& expected,bool exact){const auto input=std::u16string(1,static_cast<char16_t>(n));auto next=encoder;check(next.encode(input,true,{})==expected,"Every contextual BMP suffix");if(exacts[n]||(n>=0xd800&&n<=0xdfff)||n%257==0){try{auto nextStrict=encoderStrict;auto actual=nextStrict.encode(input,true,{0,27,2});check(exact&&actual==expected,"Strict contextual encoder");}catch(const abc::Error& e){check(!exact&&e.code=="E_ENCODING"&&!e.russian.empty(),"Strict contextual encoder fallback");}}});
  }
  for(unsigned count=r.number();count>0;--count){const auto prefix=r.bytes(r.byte());const auto prefixResult=r.result().first;
   for(unsigned n=0;n<65536;++n){const auto [expected,exact]=r.result();std::string input;input+=static_cast<char>(n>>8);input+=static_cast<char>(n);
    for(unsigned cut=0;cut<=2;++cut){abc::TextDecoder decoder(normal,false);auto actual=decoder.decode(prefix,false,{});check(actual==prefixResult,"Decoder pending prefix output");actual+=decoder.decode(std::string_view(input).substr(0,cut),false,{});actual+=decoder.decode(std::string_view(input).substr(cut),true,{});check(actual==expected,"Every decoder state/suffix/split");}
    try{abc::TextDecoder decoderStrict(strict,false);auto actual=decoderStrict.decode(prefix,false,{0,27,2});actual+=decoderStrict.decode(input,true,{0,27,2});check(exact&&actual==expected,"Strict decoder state");}catch(const abc::Error& e){check(!exact&&e.code=="E_ENCODING"&&!e.russian.empty()&&e.location.line==27,"Strict decoder state fallback");}
   }
  }
  std::istringstream input(std::to_string(cp));std::ostringstream output;abc::execute(metadata,input,output);check(output.str()==std::to_string(cp)+"\n"+runtime+"\nTrue\n","Pascal ISO factory/cache/runtime type");
 }
 check(r.file.peek()==std::char_traits<char>::eof(),"Complete ISO reference consumed");
 const auto names=abc::compile("begin var name:=ReadlnString;var e:=Encoding.GetEncoding(name);Writeln(e.CodePage,' ',e.GetType.Name);Writeln(e=Encoding.GetEncoding(e.CodePage));end.");
 std::ifstream aliases(std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures/encoding-aliases-reference.tsv");std::string line;unsigned aliasCount=0;while(std::getline(aliases,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line[0]=='#')continue;const auto tab=line.find('\t');const auto cp=std::stoul(line.substr(tab+1));if(!abc::iso2022_code_page(static_cast<std::uint16_t>(cp)))continue;++aliasCount;auto name=line.substr(0,tab);for(unsigned variant=0;variant<2;++variant){std::istringstream input(name+"\n");std::ostringstream output;abc::execute(names,input,output);check(output.str()==std::to_string(cp)+" ISO2022Encoding\nTrue\n","All validated ISO aliases/case/cache");for(auto& ch:name)if(ch>='a'&&ch<='z')ch=static_cast<char>(ch-32);}}check(aliasCount==7,"All seven ISO aliases");
}
std::string unhex(std::string_view value,bool utf16){if(value=="-")return {};std::string raw;std::u16string text;const auto width=utf16?4U:2U;for(std::size_t n=0;n<value.size();n+=width){auto v=std::stoul(std::string(value.substr(n,width)),nullptr,16);if(utf16)text+=static_cast<char16_t>(v);else raw+=static_cast<char>(v);}return utf16?abc::to_utf8(text):raw;}
void files(){
 struct Temporary{std::filesystem::path path=std::filesystem::temp_directory_path()/("ABC ISO "+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));Temporary(){std::filesystem::create_directories(path);}~Temporary(){std::error_code e;std::filesystem::remove_all(path,e);}} temporary;
 const auto path=temporary.path/std::filesystem::path(u8"данные.txt");std::string literal;for(auto ch:path.generic_u8string()){literal+=static_cast<char>(ch);if(ch==u8'\'')literal+='\'';}
 const auto program=abc::compile("uses FilesOperations;var cp,op:integer;begin Read(cp,op);var e:=Encoding.GetEncoding(cp);DefaultEncoding:=e;var name:='"+literal+"';var glyph:string;if cp=50225 then glyph:='한' else if cp=52936 then glyph:='中' else glyph:='日';"
  "if op=0 then WriteStringToFile(name,'A'#13#10'日本語 ｶﾀｶﾅ 中文 한국어 éＡ🌍'#0'Z');"
  "if op=1 then Write(ReadAllText(name));if op=2 then Write(ReadFileToEnd(name));"
  "if op=3 then WriteStringToFile(name,StringOfChar('A',1023)+glyph+glyph+StringOfChar('B',1024)+'ｶ'+#55357);"
  "if op>=4 then begin var f:Text;Assign(f,name);Rewrite(f);"
  "if op=4 then begin Write(f,StringOfChar('A',1023)+glyph+#55357);Write(f,#56832+glyph+StringOfChar('B',1024)+'ｶ'+#55357);end;"
  "if op=5 then begin Write(f,glyph);Flush(f);Write(f,glyph);end;"
  "if op=6 then begin Write(f,glyph+'ｶ');Flush(f);Write(f,glyph+'ｶ');end;"
  "if op=7 then Write(f,glyph);if op=8 then begin Write(f,glyph);DefaultEncoding:=Encoding.UTF8;Write(f,'A');Write(f,glyph);end;"
  "if op=9 then begin Write(f,StringOfChar(glyph[1],1025));Rewrite(f);Write(f,glyph);end;Close(f);"
  "if op=7 then begin Append(f);Write(f,glyph);Close(f);end;end;end.");
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::ifstream data(fixtures/"iso2022-files-reference.tsv"),legacy(fixtures/"iso2022-files-framework-reference.tsv");check(bool(data)&&bool(legacy),"Both ISO file corpora retained");std::string line,oldLine;unsigned rows=0,differences=0;
 while(std::getline(data,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line[0]=='#')continue;do{if(!std::getline(legacy,oldLine))throw std::runtime_error("Truncated legacy ISO file corpus");if(!oldLine.empty()&&oldLine.back()=='\r')oldLine.pop_back();}while(oldLine.empty()||oldLine[0]=='#');if(line!=oldLine)++differences;
  const auto a=line.find('\t'),b=line.find('\t',a+1),c=line.find('\t',b+1);const auto cp=std::stoul(line.substr(0,a));const auto op=std::stoi(line.substr(a+1,b-a-1));const bool write=op==0||op>=3;
  if(write){std::error_code e;std::filesystem::remove(path,e);}else{std::ofstream file(path,std::ios::binary);auto raw=unhex(std::string_view(line).substr(b+1,c-b-1),false);file.write(raw.data(),raw.size());}
  std::istringstream input(std::to_string(cp)+" "+std::to_string(op));std::ostringstream output;
  if(op==10){abc::TextFile file;file.filename=path;file.encoding=abc::TextEncoding{static_cast<abc::TextEncodingKind>(cp),false,false};file.encoder.emplace(*file.encoding);file.writer=std::make_shared<abc::FileWriter>(path,false,abc::SourceLocation{});const auto glyph=cp==50225?u'한':cp==52936?u'中':u'日';abc::text_write(file,std::u16string(1025,glyph),{});
   try{abc::text_write(file,std::u16string(1'048'500,u'A'),{0,29,2});check(false,"Oversize ISO preflight accepted");}catch(const abc::Error& e){check(e.code=="E_LIMIT"&&!e.russian.empty()&&e.location.line==29,"ISO rejected preview retains modes/designator");}file.close({});
  }else abc::execute(program,input,output);
  const auto expected=unhex(std::string_view(line).substr(c+1),!write);if(write){std::ifstream file(path,std::ios::binary);std::ostringstream raw;raw<<file.rdbuf();check(output.str().empty()&&raw.str()==expected,"ISO buffered/Flush/append/Rewrite write row "+std::to_string(rows));}
  else{auto actual=output.str();if(op==2)for(std::size_t n=0;(n=actual.find("\r\n",n))!=actual.npos;)actual.erase(n,1);check(actual==expected,"ISO complete/line/BOM/escape EOF read row "+std::to_string(rows));}++rows;
 }check(rows==9725,"All ISO file recipes checked");check(differences==4062,"Explicit legacy ISO StreamReader EOF differences");
}
}
int main(int argc,char** argv){try{if(argc!=2)throw std::runtime_error("Pass the unpacked ISO reference filename");reference(std::filesystem::path(argv[1]));files();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" ISO-2022/HZ checks, "<<failures<<" failures\n";return failures?1:0;}

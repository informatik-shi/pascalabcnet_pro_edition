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
 std::ifstream file;explicit Reader(const std::filesystem::path& path):file(path,std::ios::binary){if(!file)throw std::runtime_error("Missing ISCII reference");}
 unsigned byte(){int n=file.get();if(n<0)throw std::runtime_error("Truncated ISCII reference");return static_cast<unsigned char>(n);}
 unsigned word(){auto a=byte();return a|(byte()<<8);}
 std::uint32_t number(){auto a=word();return a|(word()<<16);}
 std::string bytes(unsigned n){std::string value;for(unsigned k=0;k<n;++k)value+=static_cast<char>(byte());return value;}
 std::string text(){unsigned n=0,s=0,b;do{b=byte();n|=(b&127)<<s;s+=7;if(s>28)throw std::runtime_error("Invalid string length");}while(b&128);return bytes(n);}
 template<class F>void runs(F&& callback){const auto count=number();unsigned position=0;for(unsigned row=0;row<count;++row){const auto length=number();const bool exact=byte()!=0;const auto expected=bytes(byte());if(!length||position+length>65536)throw std::runtime_error("Invalid exhaustive runs");for(unsigned n=position;n<position+length;++n)callback(n,expected,exact);position+=length;}check(position==65536,"Complete contextual BMP runs");}
};
bool bit(std::string_view bits,unsigned n){return (static_cast<unsigned char>(bits[n>>3])>>(n&7))&1;}
struct Codec{abc::TextEncoding encoding;std::vector<char16_t> singles;std::vector<std::u16string> pairs;std::string singleExact;std::vector<bool> pairExact;};
void reference(){
 Reader reader(std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures/iscii-reference.bin");check(reader.bytes(8)=="ABCISC01"&&reader.number()==10,"ISCII header");std::vector<Codec> codecs;
 const auto metadata=abc::compile("begin var n:integer;Read(n);var e:=Encoding.GetEncoding(n);Writeln(e.CodePage);Writeln(e.GetType.FullName);Writeln(e=Encoding.GetEncoding(n));end.");
 for(unsigned row=0;row<10;++row){const auto cp=reader.number();const auto runtime=reader.text();check(cp==57002+row,"All ten ISCII pages");Codec c;c.encoding={static_cast<abc::TextEncodingKind>(cp),false,false};auto strict=c.encoding;strict.strict=true;std::vector<bool> exacts(65536);
  reader.runs([&](unsigned n,const std::string& expected,bool exact){exacts[n]=exact;const auto input=std::u16string(1,static_cast<char16_t>(n));check(abc::encode_text_bytes(input,c.encoding,{})==expected,"Every ISCII BMP default mapping");try{const auto actual=abc::encode_text_bytes(input,strict,{0,21,2});check(exact&&actual==expected,"Every strict ISCII BMP mapping");}catch(const abc::Error& e){check(!exact&&e.code=="E_ENCODING"&&!e.russian.empty()&&e.location.line==21,"Strict ISCII BMP fallback");}});
  c.singles.resize(256);for(auto& v:c.singles)v=static_cast<char16_t>(reader.word());c.singleExact=reader.bytes(32);const auto modes=reader.bytes(256);for(unsigned n=0;n<256;++n)(void)reader.word();
  for(unsigned n=0;n<256;++n){const auto input=std::string(1,static_cast<char>(n));const auto expected=std::u16string(1,c.singles[n]);check(abc::decode_text_bytes(input,c.encoding,false,{})==expected,"All final single bytes");abc::TextDecoder decoder(c.encoding,false);const auto prefix=decoder.decode(input,false,{});const auto mode=static_cast<unsigned char>(modes[n]);check(prefix.size()==(mode==1||mode==3||mode==4?0:1),"Actual single-byte retention classification");check(prefix+decoder.decode({},true,{})==expected,"Every single final flush");}
  c.pairs.resize(65536);c.pairExact.resize(65536);
  for(unsigned n=0;n<65536;++n){const auto flags=reader.byte();const bool exact=flags&128;c.pairExact[n]=exact;auto& expected=c.pairs[n];for(unsigned k=0;k<(flags&127);++k)expected+=static_cast<char16_t>(reader.word());std::string input;input+=static_cast<char>(n>>8);input+=static_cast<char>(n);
   check(abc::decode_text_bytes(input,c.encoding,false,{})==expected,"Every ISCII byte pair");try{const auto actual=abc::decode_text_bytes(input,strict,false,{0,21,2});check(exact&&actual==expected,"Strict pair decode");}catch(const abc::Error& e){check(!exact&&e.code=="E_ENCODING"&&!e.russian.empty(),"Strict pair rejection");}
   for(unsigned cut=1;cut<=2;++cut){abc::TextDecoder decoder(c.encoding,false);auto actual=decoder.decode(std::string_view(input).substr(0,cut),false,{});actual+=decoder.decode(std::string_view(input).substr(cut),true,{});check(actual==expected,"Every byte-pair split and final flush");}
  }
  const auto contexts=reader.number();check(contexts==20,"All encoder contexts");for(unsigned context=0;context<contexts;++context){std::u16string prefix;const auto length=reader.byte();for(unsigned n=0;n<length;++n)prefix+=static_cast<char16_t>(reader.word());const auto prefixExpected=reader.bytes(reader.byte());abc::TextEncoder encoder(c.encoding);check(encoder.encode(prefix,false,{})==prefixExpected,"Context prefix streaming bytes");abc::TextEncoder strictEncoder(strict);check(strictEncoder.encode(prefix,false,{})==prefixExpected,"Strict context prefix bytes");
   reader.runs([&](unsigned n,const std::string& expected,bool exact){const auto input=std::u16string(1,static_cast<char16_t>(n));auto state=encoder;check(state.encode(input,true,{})==expected,"Every contextual BMP suffix/font reset");if(exacts[n]||n==0x200c||n==0x200d||(n>=0xd800&&n<=0xdfff)||n%257==0){try{auto stateStrict=strictEncoder;const auto actual=stateStrict.encode(input,true,{0,21,2});check(exact&&actual==expected,"Strict contextual suffix");}catch(const abc::Error& e){check(!exact&&e.code=="E_ENCODING"&&!e.russian.empty(),"Strict contextual rejection");}}});
  }
  std::istringstream in(std::to_string(cp));std::ostringstream out;abc::execute(metadata,in,out);check(out.str()==std::to_string(cp)+"\n"+runtime+"\nTrue\n","Pascal factory, cache and ISCII runtime identity");codecs.push_back(std::move(c));
 }
 check(reader.file.peek()==std::char_traits<char>::eof(),"Complete ISCII fixture consumed");
 const auto names=abc::compile("begin var s:=ReadlnString;var e:=Encoding.GetEncoding(s);Writeln(e.CodePage,' ',e.GetType.Name);Writeln(e=Encoding.GetEncoding(e.CodePage));end.");
 std::ifstream aliases(std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures/encoding-aliases-reference.tsv");std::string line;unsigned aliasCount=0;
 while(std::getline(aliases,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line.front()=='#')continue;const auto tab=line.find('\t');const auto cp=std::stoul(line.substr(tab+1));if(cp<57002||cp>57011)continue;++aliasCount;auto name=line.substr(0,tab);for(unsigned variant=0;variant<2;++variant){std::istringstream input(name+"\n");std::ostringstream output;abc::execute(names,input,output);check(output.str()==std::to_string(cp)+" ISCIIEncoding\nTrue\n","Validated ISCII alias/case/cache");for(auto& c:name)if(c>='a'&&c<='z')c=static_cast<char>(c-32);}}
 check(aliasCount==10,"All ten ISCII aliases");
 for(const auto& c:codecs)for(unsigned font=0;font<10;++font){std::string selector;selector+=static_cast<char>(0xef);selector+=static_cast<char>(0x42+font);
  for(unsigned n=0;n<65536;++n){abc::TextDecoder decoder(c.encoding,false);check(decoder.decode(selector,false,{}).empty(),"Font selection emits no character");std::string input;input+=static_cast<char>(n>>8);input+=static_cast<char>(n);check(decoder.decode(input,true,{})==codecs[font].pairs[n],"Every selected-script pair");}
  for(unsigned reset=0x40;reset<=0x41;++reset)for(unsigned n=0;n<256;++n){abc::TextDecoder decoder(c.encoding,false);decoder.decode(selector,false,{});std::string input;input+=static_cast<char>(0xef);input+=static_cast<char>(reset);input+=static_cast<char>(n);check(decoder.decode(input,true,{})==std::u16string(1,c.singles[n]),"ATR Roman/default restore initial mapping");}
 }
}
std::string unhex(std::string_view value,bool utf16){if(value=="-")return {};std::string raw;std::u16string text;const auto width=utf16?4U:2U;for(std::size_t n=0;n<value.size();n+=width){auto v=std::stoul(std::string(value.substr(n,width)),nullptr,16);if(utf16)text+=static_cast<char16_t>(v);else raw+=static_cast<char>(v);}return utf16?abc::to_utf8(text):raw;}
void files(){
 struct Temporary{std::filesystem::path path=std::filesystem::temp_directory_path()/("ABC ISCII "+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));Temporary(){std::filesystem::create_directories(path);}~Temporary(){std::error_code e;std::filesystem::remove_all(path,e);}} temporary;
 const auto path=temporary.path/std::filesystem::path(u8"данные.txt");std::string literal;for(auto c:path.generic_u8string()){literal+=static_cast<char>(c);if(c==u8'\'')literal+='\'';}
 const auto program=abc::compile("uses FilesOperations;var cp,op:integer;begin Read(cp,op);var e:=Encoding.GetEncoding(cp);DefaultEncoding:=e;var name:='"+literal+"';"
  "if op=0 then WriteStringToFile(name,'कকகకಕകકକਕ '#2381#8204#2381#8205' '#2386#2416' 🌍'#13#10);"
  "if op=1 then Write(ReadAllText(name));if op=2 then Write(ReadFileToEnd(name));"
  "if op=3 then WriteStringToFile(name,StringOfChar('A',1023)+'कকக'+StringOfChar('B',1024)+#2381#8205#55357);"
  "if op>=4 then begin var f:Text;Assign(f,name);Rewrite(f);"
  "if op=4 then begin Write(f,StringOfChar('A',1023)+'कক'+#55357);Write(f,#56832'க'+StringOfChar('B',1024)+#2381#8205#55357);end;"
  "if (op=5) or (op=6) then begin if op=5 then Write(f,e.GetString(Arr(byte(232)))[1]) else if cp=57002 then Write(f,#2509) else Write(f,#2381);Flush(f);Write(f,#8205);end;"
  "if op=7 then Write(f,'ক');if op=8 then begin Write(f,'ক');DefaultEncoding:=Encoding.GetEncoding(57004);Write(f,'க');end;"
  "if op=9 then begin if cp=57003 then Write(f,StringOfChar('க',1025)) else Write(f,StringOfChar('ক',1025));Rewrite(f);Write(f,'क');end;Close(f);"
  "if op=7 then begin Append(f);Write(f,'क');Close(f);end;end;end.");
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";std::ifstream data(fixtures/"iscii-files-reference.tsv"),legacy(fixtures/"iscii-files-framework-reference.tsv");check(bool(data)&&bool(legacy),"Both ISCII file corpora retained");std::string line,oldLine;unsigned rows=0,differences=0;
 while(std::getline(data,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line.front()=='#')continue;do{if(!std::getline(legacy,oldLine))throw std::runtime_error("Truncated legacy ISCII corpus");if(!oldLine.empty()&&oldLine.back()=='\r')oldLine.pop_back();}while(oldLine.empty()||oldLine.front()=='#');if(line!=oldLine)++differences;
  const auto a=line.find('\t'),b=line.find('\t',a+1),c=line.find('\t',b+1);const auto cp=line.substr(0,a);const auto op=std::stoi(line.substr(a+1,b-a-1));const bool write=op==0||op>=3;
  if(write){std::error_code e;std::filesystem::remove(path,e);}else{std::ofstream file(path,std::ios::binary);const auto raw=unhex(std::string_view(line).substr(b+1,c-b-1),false);file.write(raw.data(),raw.size());}
  std::istringstream input(cp+" "+std::to_string(op));std::ostringstream output;
  if(op==10){
   abc::TextFile file;file.filename=path;file.encoding=abc::TextEncoding{static_cast<abc::TextEncodingKind>(std::stoul(cp)),false,false};file.encoder.emplace(*file.encoding);file.writer=std::make_shared<abc::FileWriter>(path,false,abc::SourceLocation{});
   const auto glyph=cp=="57003"?u'க':u'ক';abc::text_write(file,std::u16string(1025,glyph),{});
   try{abc::text_write(file,std::u16string(1'047'550,glyph),{0,23,2});check(false,"Oversize writer preview accepted");}catch(const abc::Error& e){check(e.code=="E_LIMIT"&&!e.russian.empty()&&e.location.line==23,"Rejected preview retains writer state");}
   file.close({});
  }else abc::execute(program,input,output);
  const auto expected=unhex(std::string_view(line).substr(c+1),!write);
  if(write){std::ifstream file(path,std::ios::binary);std::ostringstream raw;raw<<file.rdbuf();check(output.str().empty()&&raw.str()==expected,"ISCII buffered/font/surrogate/flush/append write row "+std::to_string(rows));}
  else{auto actual=output.str();if(op==2)for(std::size_t n=0;(n=actual.find("\r\n",n))!=actual.npos;)actual.erase(n,1);check(actual==expected,"ISCII complete/post-line/BOM/state read row "+std::to_string(rows));}
  ++rows;
 }
 check(rows==9490,"All ISCII file recipes checked");check(differences==958,"Explicit legacy ISCII StreamReader EOF differences");
}
}
int main(){try{reference();files();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" ISCII checks, "<<failures<<" failures\n";return failures?1:0;}

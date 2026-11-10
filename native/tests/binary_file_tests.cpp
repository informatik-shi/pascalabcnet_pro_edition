#include "abc/core.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
unsigned checks=0,failures=0;
std::filesystem::path folder;
void check(bool ok,std::string_view message){++checks;if(!ok){++failures;if(failures<=30)std::cerr<<"FAIL "<<message<<'\n';}}
std::string contents(const std::filesystem::path& path){std::ifstream in(path,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
std::string quoted(std::string s){std::string r="'";for(char c:s){r+=c;if(c=='\'')r+=c;}return r+"'";}
std::string directory(){return "ChDir("+quoted(abc::to_utf8(folder.u16string()))+");";}
std::string body(std::string_view statements,std::string_view declarations={}){return std::string(declarations)+"begin "+directory()+std::string(statements)+" end.";}
std::string run(std::string_view source,std::string_view input={}){std::istringstream in{std::string(input)};std::ostringstream out;abc::execute(abc::compile(source),in,out);return out.str();}
void result(std::string_view source,std::string_view expected){try{const auto actual=run(source);check(actual==expected,std::string(source)+" actual="+actual);}catch(const std::exception& e){check(false,std::string(source)+" "+e.what());}}
void error(std::string_view source,std::string_view expected,std::string_view output={}){std::istringstream in;std::ostringstream out;try{abc::execute(abc::compile(source),in,out);check(false,"Expected "+std::string(expected)+" "+std::string(source));}catch(const abc::Error& e){check(e.code==expected&&out.str()==output&&!e.russian.empty()&&*e.what()&&e.location.line>0,std::string(e.what())+" actual="+e.code+" output="+out.str());}}
std::vector<std::string> split(std::string_view s,char delimiter='\t'){std::vector<std::string> r;std::size_t a=0;for(;;){const auto b=s.find(delimiter,a);r.emplace_back(s.substr(a,b==s.npos?b:b-a));if(b==s.npos)return r;a=b+1;}}
std::string unhex(std::string_view s){std::string r;if(s!="-")for(std::size_t n=0;n<s.size();n+=2)r+=static_cast<char>(std::stoul(std::string(s.substr(n,2)),nullptr,16));return r;}
void reference(){
 const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";
 auto source=contents(fixtures/"binary-file-reference.pas");const auto main=source.find("begin");check(main!=source.npos,"Typed-file Pascal reference main");if(main==source.npos)return;source.insert(main+5," "+directory());
 const auto program=abc::compile(source);std::ifstream input(fixtures/"binary-file-reference.tsv");check(bool(input),"Actual typed-file public API reference");std::string line;unsigned rows=0,accepted=0;
 const auto target=folder/"native-packed-reference.tmp";
 while(std::getline(input,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line[0]=='#')continue;const auto f=split(line);if(f.size()!=4)throw std::runtime_error("Invalid typed-file reference width");std::error_code ignored;std::filesystem::remove(target,ignored);std::istringstream in(f[0]);std::ostringstream out;std::string code="ok";try{abc::execute(program,in,out);}catch(const abc::Error& e){code=e.code;check(!e.russian.empty()&&*e.what()&&e.location.line>0,"Typed-file encoding exception is bilingual");}
  if(f[1]=="True"){++accepted;std::ostringstream expected;expected<<"9 9\n";const auto units=split(f[3],',');for(std::size_t n=0;n<units.size();n+=2)expected<<std::stoul(units[n],nullptr,16)<<' '<<std::stoul(units.at(n+1),nullptr,16)<<'\n';expected<<"True\n10 False\n";check(code=="ok"&&out.str()==expected.str(),"Typed primitive/enum/nested record read cp="+f[0]+" actual="+code+" "+out.str());check(contents(target)==unhex(f[2]),"Packed source byte layout, 53-byte records cp="+f[0]);}
  else check(code=="E_IO"&&out.str().empty()&&!std::filesystem::exists(target),"Multibyte typed-file encoding rejected before create cp="+f[0]);++rows;
 }
 check(rows==140&&accepted>90,"All 140 known encodings checked for typed files");
}
void semantics(){
 const std::string setup="DefaultEncoding:=Encoding.GetEncoding(1251);var f:file of integer;Rewrite(f,'values.tmp');";
 result(body(setup+"Write(f,1,2,3);Writeln(FileSize(f),' ',FilePos(f));Seek(f,1);Write(f,99);Reset(f);var a,b,c:integer;Read(f,a,b,c);Writeln(a,' ',b,' ',c,' ',Eof(f));Close(f);Erase(f);"),"3 3\n1 99 3 True\n");
 result(body(setup+"Write(f,1);Seek(f,3);Writeln(FilePos(f),' ',FileSize(f),' ',Eof(f));Write(f,7);Seek(f,1);var a,b,c:integer;Read(f,a,b,c);Writeln(a,' ',b,' ',c);Close(f);"),"3 1 False\n0 0 7\n");
 result(body(setup+"Write(f,1,2,3);Seek(f,1);Truncate(f);Writeln(FileSize(f),' ',FilePos(f),' ',Eof(f));Rewrite(f);Writeln(FileSize(f),' ',FilePos(f));Close(f);"),"1 1 True\n0 0\n");
 result(body(setup+"var alias:=f;Write(alias,17);Reset(f);var n:integer;Read(alias,n);Writeln(n,' ',FilePos(f));Close(alias);"),"17 1\n");
 result(body("var f:file;Rewrite(f,'raw.tmp',Encoding.UTF8);Write(f,42,'Привет',true,real(1.25));f.Position:=0;Writeln(f.ReadInteger,' ',f.ReadString,' ',f.ReadBoolean,' ',f.ReadReal,' ',f.Eof);Close(f);Erase(f);"),"42 Привет True 1.25 True\n");
 result(body("var f:file;Rewrite(f,'raw.tmp');f.WriteBytes(Arr(byte(1),byte(2),byte(255)));f.Position:=1;var a:=f.ReadBytes(10);Writeln(a.Length,' ',a[0],' ',a[1],' ',f.Position);var b:=f.ReadBytes(0);Writeln(b.Length);Close(f);"),"2 2 255 3\n0\n");
 result(body("var f:file;Rewrite(f,'raw.tmp');Write(f,Arr(11,22,33));Reset(f);var a,b,c:integer;Read(f,a,b,c);Writeln(a,' ',b,' ',c);Close(f);"),"11 22 33\n");
 result(body("var f:file;Rewrite(f,'raw.tmp',Encoding.UTF8);Write(f,'Ж');Reset(f,Encoding.ASCII);var c:char;Read(f,c);Writeln(Ord(c));Rewrite(f,Encoding.ASCII);Write(f,'Ж');Close(f);var g:file;Reset(g,'raw.tmp',Encoding.UTF8);Read(g,c);Writeln(Ord(c));Close(g);"),"1046\n1046\n");
 result(body("var f:file;Rewrite(f,'first.tmp');Assign(f,'second.tmp');Write(f,81);Close(f);var g:file;Reset(g,'first.tmp');Writeln(g.ReadInteger,' ',g.Name);Close(g);"),"81 first.tmp\n");
 result(body("var f:file;Rewrite(f,'raw.tmp');Write(f,1,2,3,4,5,6,7,8,9);Reset(f);for var i:=1 to 9 do assert(f.ReadInteger=i);Writeln(f.Size);Close(f);"),"36\n");
 result(body("var f:file;Rewrite(f,'raw.tmp');Write(f);Write(f,new integer[0]);Writeln(f.Size);Close(f);"),"0\n");
 result(body("var f:file of R;Rewrite(f,'raw.tmp',Encoding.ASCII);var r:R;r.n:=23;Write(f,r);Reset(f);var q:R;Read(f,q);Writeln(q.n);Close(f);","type R=record n:integer:=99;end;"),"23\n");
 error(body("var f:file;Close(f);"),"E_IO");error(body("var f:file;Assign(f,'raw.tmp');Write(f);"),"E_IO");
 error(body("var f:file;Writeln(f,123);"),"E_IO");
 error(body("var f:file;Rewrite(f,'raw.tmp');Seek(f,-1);"),"E_ARGUMENT");
 error(body("var f:file;Rewrite(f,'raw.tmp');f.ReadBytes(-1);"),"E_ARGUMENT");
 error(body("var f:file;Rewrite(f,'raw.tmp');f.ReadInteger;"),"E_EOF");
 error(body("var f:file;Rewrite(f,'raw.tmp');Write(f,Chr(55296));"),"E_ARGUMENT");
 error(body(setup+"Reset(f,Encoding.UTF8);"),"E_IO");
 error(body(setup+"f.ReadBytes(1);"),"E_TYPE");error(body(setup+"f.ReadInteger;"),"E_TYPE");
 error(body("var f:file of string;"),"E_TYPE");error(body("var f:file of array of integer;"),"E_TYPE");
 error(body("var f:file;var a:array of integer;Read(f,a);"),"E_TYPE");
 error(body("var f:file;Rewrite(f,'raw.tmp');var g:file;Rewrite(g,'raw.tmp');"),"E_IO");
 error(body("var f:file;Rewrite(f,'raw.tmp');var g:text;Rewrite(g,'raw.tmp');"),"E_IO");
 error(body("var f:text;Rewrite(f,'raw.tmp');var g:file;Rewrite(g,'raw.tmp');"),"E_IO");
 error(body("var f:file;Rewrite(f,'raw.tmp');WriteAllText('raw.tmp','bad');"),"E_IO");
 error(body("var f:file;Rewrite(f,'raw.tmp');ReadAllText('raw.tmp');"),"E_IO");
 {std::ofstream out(folder/"short.tmp",std::ios::binary);out.write("abc",3);}
 error(body("var f:file of integer;Reset(f,'short.tmp',Encoding.ASCII);FileSize(f);"),"E_IO");
 error(body("var f:file of integer;Reset(f,'short.tmp',Encoding.ASCII);var n:integer;Read(f,n);"),"E_EOF");
 result(body("var f:file;Rewrite(f,'raw.tmp');Seek(f,4294967299);Writeln(f.Position,' ',f.Size,' ',Eof(f));Close(f);"),"4294967299 0 False\n");
 auto program=abc::compile(body(setup+"Write(f,1);Close(f);"));
 for(auto& instruction:program.code)if(instruction.op==abc::Op::encoding_call){instruction.operand=255;break;}
 try{abc::verify(program);check(false,"Forged binary method rejected");}catch(const abc::Error& e){check(e.code=="E_BYTECODE","Forged binary method bytecode diagnostic");}
}
}
int main(){folder=std::filesystem::temp_directory_path()/("ABC native binary "+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directory(folder);try{reference();semantics();}catch(const std::exception& e){check(false,e.what());}std::error_code ignored;std::filesystem::remove_all(folder,ignored);std::cout<<checks<<" Pascal binary file checks, "<<failures<<" failures\n";return failures?1:0;}

#include "abc/core.hpp"
#include "../src/binary_stream.hpp"
#include <array>
#include <bit>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
namespace {
unsigned checks=0,failures=0;
void check(bool ok,std::string_view message){++checks;if(!ok){++failures;if(failures<16)std::cerr<<"FAIL "<<message<<'\n';}}
std::vector<std::string> split(std::string_view s){std::vector<std::string> r;std::size_t start=0;for(;;){const auto end=s.find('\t',start);r.emplace_back(s.substr(start,end==s.npos?end:end-start));if(end==s.npos)return r;start=end+1;}}
std::string unhex(std::string_view s){std::string r;if(s!="-")for(std::size_t n=0;n<s.size();n+=2)r+=static_cast<char>(std::stoul(std::string(s.substr(n,2)),nullptr,16));return r;}
std::string hex(std::string_view s){if(s.empty())return "-";std::string r;for(unsigned char c:s){r+="0123456789abcdef"[c>>4];r+="0123456789abcdef"[c&15];}return r;}
void reference(){
    struct Temporary{std::filesystem::path path=std::filesystem::temp_directory_path()/("ABC binary stream "+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));Temporary(){std::filesystem::create_directories(path);}~Temporary(){std::error_code e;std::filesystem::remove_all(path,e);}} temp;
    const auto path=temp.path/std::filesystem::path(u8"二进制-данные.bin");std::unique_ptr<abc::BinaryStream> stream;
    std::ifstream data(std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures/binary-stream-reference.tsv");check(bool(data),"Independent binary stream fixture");std::string line;unsigned rows=0;
    while(std::getline(data,line)){
        if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line[0]=='#')continue;const auto f=split(line);if(f.size()!=7)throw std::runtime_error("Binary trace width");
        const auto& op=f[1];const auto argument=std::stoll(f[2]);std::string actual="ok";const abc::SourceLocation location{0,47,3};
        try{
            if(op=="create"||op=="open")stream=std::make_unique<abc::BinaryStream>(path,op=="create",location);
            else if(op=="conflict"||op=="conflict-create"){abc::BinaryStream other(path,op=="conflict-create",location);}
            else if(op=="write"||op=="empty")stream->write(unhex(f[3]),location);
            else if(op=="seek"||op=="negative"||op=="big")stream->seek(argument,location);
            else if(op=="truncate")stream->truncate(location);
            else if(op=="close")stream->close();
            else if(op=="read"){std::string bytes(static_cast<std::size_t>(argument),'\0');const auto count=stream->read(bytes,location);bytes.resize(count);actual=hex(bytes);}
            else if(op=="i32"){std::array<char,4> bytes{};stream->read_exact(bytes,location);std::uint32_t n=0;for(unsigned k=0;k<4;++k)n|=static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[k]))<<(k*8);actual=std::to_string(std::bit_cast<std::int32_t>(n));}
            else if(op=="snapshot"){std::ifstream file(path,std::ios::binary);std::ostringstream bytes;bytes<<file.rdbuf();actual=hex(bytes.str());}
            else throw std::runtime_error("Unknown binary trace operation");
        }catch(const abc::Error& e){actual=e.code;check(!e.russian.empty()&&*e.what()&&e.location.line==47,"Bilingual binary stream exception with source");}
        auto expected=f[4];if(expected=="ArgumentOutOfRangeException")expected="E_ARGUMENT";else if(expected=="EndOfStreamException")expected="E_EOF";else if(expected=="IOException"||expected=="ObjectDisposedException")expected="E_IO";
        check(actual==expected,"Binary stream outcome row "+std::to_string(rows)+" "+op+" actual "+actual+" expected "+expected);
        if(f[5]=="-")check(!stream->is_open(),"Closed binary handle");else{check(stream->position(location)==std::stoll(f[5]),"Binary logical position including partial EOF and 64-bit seeks");check(stream->length(location)==std::stoll(f[6]),"Binary length including holes, failed create and truncation");}++rows;
    }check(rows==4320,"Every independent binary trace consumed");
    for(unsigned n=0;n<128;++n){try{abc::BinaryStream absent(temp.path/"missing-parent"/"x.bin",n%2!=0,{0,47,3});check(false,"Missing parent unexpectedly opened");}catch(const abc::Error& e){check(e.code=="E_IO"&&!e.russian.empty(),"Failed binary opens release handles");}}
    {abc::BinaryStream reopened(path,false,{});check(reopened.is_open(),"Binary handle can reopen after prior stream destruction");}
}
}
int main(){try{reference();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" binary stream checks, "<<failures<<" failures\n";return failures?1:0;}

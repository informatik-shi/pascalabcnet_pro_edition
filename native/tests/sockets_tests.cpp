#include "abc/core.hpp"
#include "../src/sockets.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
void require(bool condition,const char* message) {
    if(!condition)throw std::runtime_error(message);
}
template<class Action>
void expect_error(Action action,std::string_view code) {
    try { action(); }
    catch(const abc::Error& error) {
        require(error.code==code,"Unexpected socket diagnostic");
        require(*error.what() && !error.russian.empty(),"Socket diagnostic must be bilingual");
        return;
    }
    throw std::runtime_error("Missing socket diagnostic");
}
std::string fixture(const std::filesystem::path& path) {
    std::ifstream file(path);
    if(!file)throw std::runtime_error("Cannot read the socket fixture");
    return {std::istreambuf_iterator<char>{file},{}};
}
}
int main() {
    try {
        const auto path=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";
        const auto program=abc::compile(fixture(path/"sockets-compat.pas"));
        abc::SocketContext probe;
        const auto listener=probe.create_server(0,{});
        const auto port=probe.listener_port(listener,{});
        probe.close(listener);

        abc::ExecutionOptions options;
        options.arguments.push_back(abc::from_utf8(std::to_string(port)));
        std::istringstream input;
        std::ostringstream output;
        require(abc::execute(program,input,output,options)==0,"Socket fixture did not complete");
        auto expected=fixture(path/"sockets-compat.out");
        std::erase(expected,'\r');
        require(output.str()==expected,"Socket fixture differs from the original compiler");

        expect_error([] {abc::compile("begin CreateClientSocket;end.");},"E_NAME");
        expect_error([] {abc::compile("uses Sockets;begin ReceiveString(1,'literal');end.");},"E_TYPE");
        expect_error([] {abc::compile("uses Sockets;begin var x:integer; ReceiveString(1,x);end.");},"E_TYPE");
        const auto run=[](std::string_view source) {
            std::istringstream in;
            std::ostringstream out;
            abc::execute(abc::compile(source),in,out);
        };
        expect_error([&] {run("uses Sockets;begin Accept(-1);end.");},"E_SOCKET");
        expect_error([&] {run("uses Sockets;begin CreateServerSocket(65536);end.");},"E_RANGE");

        const auto blocked=probe.create_server(0,{});
        unsigned checks=0;
        options.cancelled=[&] {return ++checks>1;};
        expect_error([&] {probe.accept(blocked,{},options);},"E_CANCELLED");
        std::cout<<"Original socket fixture and typed diagnostics passed\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<error.what()<<'\n';
        return 1;
    }
}

#include "abc/core.hpp"
#include "../src/sockets.hpp"
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <sstream>
#include <thread>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#include <sys/select.h>
#include <unistd.h>
#endif

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

class PeerSocket {
public:
#ifdef _WIN32
    using Handle=SOCKET;
    static constexpr auto invalid=INVALID_SOCKET;
#else
    using Handle=int;
    static constexpr auto invalid=-1;
#endif
    const Handle handle;
    explicit PeerSocket(Handle value):handle(value){require(handle!=invalid,"Cannot open peer socket");}
    PeerSocket(const PeerSocket&)=delete;
    PeerSocket& operator=(const PeerSocket&)=delete;
    ~PeerSocket(){
#ifdef _WIN32
        ::closesocket(handle);
#else
        ::close(handle);
#endif
    }
};
using Deadline=std::chrono::steady_clock::time_point;
int wait_for_peer(PeerSocket::Handle handle,Deadline deadline) {
    while(std::chrono::steady_clock::now()<deadline){
        fd_set reads;FD_ZERO(&reads);FD_SET(handle,&reads);
        timeval interval{0,50'000};
#ifdef _WIN32
        constexpr int size=0;
#else
        const int size=handle+1;
#endif
        const auto ready=::select(size,&reads,nullptr,nullptr,&interval);
        if(ready)return ready;
    }
    return 0;
}
void expect_wire(PeerSocket::Handle handle,std::string_view bytes,Deadline deadline) {
    std::string received(bytes.size(),'\0');
    for(std::size_t n=0;n<received.size();){
        require(wait_for_peer(handle,deadline)>0,"Cannot read ClientServer peer payload");
        const auto count=::recv(handle,received.data()+n,static_cast<int>(received.size()-n),0);
        require(count>0,"ClientServer stream closed before the complete prefix or payload");
        n+=static_cast<std::size_t>(count);
    }
    require(received==bytes,"ClientServer emitted different wire bytes than the original");
}
void client_server_roundtrip(const std::filesystem::path& path) {
    using namespace std::chrono_literals;
    const auto program=abc::compile(fixture(path/"client-server-compat.pas"));
    const PeerSocket listener(::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP));
    sockaddr_in address{};
    address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    require(::bind(listener.handle,reinterpret_cast<const sockaddr*>(&address),sizeof address)==0
        && ::listen(listener.handle,3)==0,"Cannot bind ClientServer peer to loopback");
#ifdef _WIN32
    int address_size=sizeof address;
#else
    socklen_t address_size=sizeof address;
#endif
    require(::getsockname(listener.handle,reinterpret_cast<sockaddr*>(&address),&address_size)==0,"Cannot read ClientServer peer port");
    const auto deadline=std::chrono::steady_clock::now()+5s;
    auto peer=std::async(std::launch::async,[&]{
        std::string long_request="\x88\x01";
        for(int n=0;n<65;++n)long_request+="\xd0\x96";
        long_request.append("\xf0\x9f\x8c\x8d\0!",6);
        const std::array<std::string_view,3> requests{std::string_view("\0",1),long_request,
            "\x0eRequest \xd0\xaf\xf0\x9f\x8c\x8d"};
        for(std::size_t n=0;n<requests.size();++n){
            require(wait_for_peer(listener.handle,deadline)>0,"ClientServer did not connect to the configured port");
            const PeerSocket client(::accept(listener.handle,nullptr,nullptr));
            expect_wire(client.handle,requests[n],deadline);
            if(n+1==requests.size()){
#ifdef MSG_NOSIGNAL
                constexpr int flags=MSG_NOSIGNAL;
#else
                constexpr int flags=0;
#endif
#ifdef SO_NOSIGPIPE
                const int enabled=1;
                (void)::setsockopt(client.handle,SOL_SOCKET,SO_NOSIGPIPE,&enabled,sizeof enabled);
#endif
                require(::send(client.handle,"\x06\0\xd0",3,flags)==3,"Cannot send ClientServer response prefix");
                std::this_thread::sleep_for(50ms);
                require(::send(client.handle,"\x96\xef\xbb\xbf",4,flags)==4,"Cannot send remaining ClientServer response bytes");
            }
        }
    });
    abc::ExecutionOptions options;
    options.arguments.push_back(abc::from_utf8(std::to_string(ntohs(address.sin_port))));
    options.cancelled=[&] {return std::chrono::steady_clock::now()>=deadline;};
    std::istringstream input;
    std::ostringstream output;
    require(abc::execute(program,input,output,options)==0,"ClientServer fixture did not complete");
    peer.get();
    auto expected=fixture(path/"client-server-compat.out");
    std::erase(expected,'\r');
    require(output.str()==expected,"ClientServer received different text than the original compiler");

    auto invalid=program;
    invalid.client_server.reset();
    expect_error([&]{abc::verify(invalid);},"E_BYTECODE");
}

void client_server_listener(const std::filesystem::path& path,std::u16string mode) {
    using namespace std::chrono_literals;
    const auto program=abc::compile(fixture(path/"client-server-listener.pas"));
    abc::SocketContext peer;
    const auto spare=peer.create_server(0,{});
    const auto port=peer.listener_port(spare,{});
    peer.close(spare);
    const auto deadline=std::chrono::steady_clock::now()+5s;
    abc::ExecutionOptions options;
    options.arguments={abc::from_utf8(std::to_string(port)),mode};
    options.cancelled=[&] {return std::chrono::steady_clock::now()>=deadline;};
    std::ostringstream output;
    auto listener=std::async(std::launch::async,[&]{
        std::istringstream input;
        require(abc::execute(program,input,output,options)==0,"ClientServer listener did not halt successfully");
    });
    const abc::Value host=std::u16string{u"127.0.0.1"};
    const bool replying=mode==u"reply";
    const auto send=[&](std::u16string text,bool reply){
        return peer.command(host,port,abc::Value{std::move(text)},reply,{},options);
    };
    const auto first=[&]{
        for(;;){
            try {return send(u"Тест🌍",replying);}
            catch(const abc::Error& error){
                if(error.code!="E_SOCKET" || options.cancelled())throw;
                std::this_thread::sleep_for(10ms);
            }
        }
    }();
    const auto second=send(u"more",replying);
    (void)send(u"stop",false);
    listener.get();
    if(replying){
        using namespace std::string_view_literals;
        require(first==u"Тест🌍\0Ответ🌍"sv && second==u"more\0Ответ🌍"sv,
            "RunServer returned different reply bytes than PascalABC.NET");
        require(output.str().empty(),"RunServer reply handler should not print");
    }else{
        auto expected=fixture(path/"client-server-listener-print.out");
        std::erase(expected,'\r');
        require(output.str()==expected,"RunServer procedure output differs from PascalABC.NET");
    }

    auto invalid=program;
    invalid.client_server.reset();
    expect_error([&]{abc::verify(invalid);},"E_BYTECODE");
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
        client_server_roundtrip(path);
        client_server_listener(path,u"reply");
        client_server_listener(path,u"print");
        expect_error([] {abc::compile("begin SendCommand('x');end.");},"E_NAME");
        expect_error([] {abc::compile("uses ClientServer;begin port:='x';end.");},"E_TYPE");
        expect_error([] {abc::compile("uses ClientServer;begin RunServer((x:integer)->x);end.");},"E_TYPE");
        (void)abc::compile("uses ClientServer;begin RunServer(s -> '!');RunServer(s -> s[1]);end.");
        (void)abc::compile("uses ClientServer;function F(s:string):string:=s;function F(i:integer):string:=IntToStr(i);begin RunServer(F);end.");
        std::cout<<"Original socket and ClientServer fixtures and typed diagnostics passed\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<error.what()<<'\n';
        return 1;
    }
}

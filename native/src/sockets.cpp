// Native implementation of the socket handles and byte transfers from bin/Lib/Sockets.pas.
#include "sockets.hpp"
#include "binary_codec.hpp"
#include "diagnostics.hpp"
#include "file_encoding.hpp"
#include <array>
#include <limits>
#include <utility>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace abc {
namespace {
#ifdef _WIN32
using Descriptor=SOCKET;
using AddressLength=int;
using PollDescriptor=WSAPOLLFD;
constexpr Descriptor invalid=INVALID_SOCKET;
int last_error(){return ::WSAGetLastError();}
bool interrupted(int error){return error==WSAEINTR;}
bool pending(int error){return error==WSAEWOULDBLOCK || error==WSAEINPROGRESS;}
void release(Descriptor handle){::closesocket(handle);}
bool nonblocking(Descriptor handle){
    unsigned long mode=1;
    return ::ioctlsocket(handle,FIONBIO,&mode)==0;
}
int wait_io(PollDescriptor& descriptor){return ::WSAPoll(&descriptor,1,50);}
#else
using Descriptor=int;
using AddressLength=socklen_t;
using PollDescriptor=pollfd;
constexpr Descriptor invalid=-1;
int last_error(){return errno;}
bool interrupted(int error){return error==EINTR;}
bool pending(int error){return error==EAGAIN || error==EWOULDBLOCK || error==EINPROGRESS;}
void release(Descriptor handle){::close(handle);}
bool nonblocking(Descriptor handle){
    const auto flags=::fcntl(handle,F_GETFL);
    if(flags<0 || ::fcntl(handle,F_SETFL,flags|O_NONBLOCK)<0 || ::fcntl(handle,F_SETFD,FD_CLOEXEC)<0)return false;
#ifdef SO_NOSIGPIPE
    const int enabled=1;
    if(::setsockopt(handle,SOL_SOCKET,SO_NOSIGPIPE,&enabled,sizeof enabled)<0)return false;
#endif
    return true;
}
int wait_io(PollDescriptor& descriptor){return ::poll(&descriptor,1,50);}
#endif

[[noreturn]] void socket_error(SourceLocation location,std::string english,std::string russian) {
    throw Error("E_SOCKET",location,std::move(english),std::move(russian));
}
[[noreturn]] void invalid_handle(SourceLocation location) {
    socket_error(location,"Socket handle is closed or has the wrong role","Сокет закрыт или имеет неподходящее назначение");
}
void checkpoint(SourceLocation location,const ExecutionOptions& options) {
    if(options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,location);
}
void wait_ready(Descriptor handle,short events,SourceLocation location,const ExecutionOptions& options) {
    for(;;){
        checkpoint(location,options);
        PollDescriptor item{handle,events,0};
        const auto status=wait_io(item);
        if(status>0)return;
        if(status<0 && !interrupted(last_error()))
            socket_error(location,"Waiting for the socket failed","Не удалось дождаться готовности сокета");
    }
}
std::uint16_t port_number(std::int32_t port,SourceLocation location){
    if(port<0 || port>65535)throw Error("E_RANGE",location,"Socket port must be between 0 and 65535",
        "Порт сокета должен быть от 0 до 65535");
    return static_cast<std::uint16_t>(port);
}

class Socket {
public:
    explicit Socket(Descriptor handle=invalid):handle_(handle){}
    ~Socket(){close();}
    Socket(const Socket&)=delete;
    Socket& operator=(const Socket&)=delete;
    Socket(Socket&& other) noexcept :handle_(std::exchange(other.handle_,invalid)){}
    Socket& operator=(Socket&& other) noexcept {
        close();
        handle_=std::exchange(other.handle_,invalid);
        return *this;
    }
    void close(){
        if(handle_!=invalid)release(std::exchange(handle_,invalid));
    }
    Descriptor get() const{return handle_;}
private:
    Descriptor handle_;
};

Socket configured(Descriptor handle,SourceLocation location) {
    Socket result(handle);
    if(handle==invalid || !nonblocking(handle))
        socket_error(location,"Cannot open a native TCP socket","Не удалось открыть нативный TCP-сокет");
    return result;
}
Socket create_tcp(SourceLocation location){
    return configured(::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP),location);
}
enum class Role { listener, outgoing, incoming };
struct Entry {
    Socket socket;
    Role role;
    bool connected=false;
};

Socket connect_tcp(std::u16string_view host,std::int32_t port,int family,SourceLocation location,
    const ExecutionOptions& options) {
    const auto service=std::to_string(port_number(port,location));
    const auto name=to_utf8(host);
    if(name.empty() || name.find('\0')!=std::string::npos)
        socket_error(location,"Invalid socket host name","Недопустимое имя узла сокета");
    checkpoint(location,options);
    addrinfo hints{};
    hints.ai_family=family;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_protocol=IPPROTO_TCP;
    addrinfo* addresses=nullptr;
    if(::getaddrinfo(name.c_str(),service.c_str(),&hints,&addresses)!=0)
        socket_error(location,"Cannot resolve the socket host","Не удалось разрешить имя узла сокета");
    const std::unique_ptr<addrinfo,decltype(&::freeaddrinfo)> cleanup(addresses,&::freeaddrinfo);
    for(auto* address=addresses;address;address=address->ai_next){
        checkpoint(location,options);
        const auto descriptor=::socket(address->ai_family,address->ai_socktype,address->ai_protocol);
        if(descriptor==invalid)continue;
        auto candidate=configured(descriptor,location);
        if(::connect(descriptor,address->ai_addr,static_cast<AddressLength>(address->ai_addrlen))!=0){
            const auto failure=last_error();
            if(!pending(failure) && !interrupted(failure))continue;
            wait_ready(descriptor,POLLOUT,location,options);
            int error=0;
            AddressLength size=sizeof error;
            if(::getsockopt(descriptor,SOL_SOCKET,SO_ERROR,reinterpret_cast<char*>(&error),&size)!=0 || error!=0)continue;
        }
        return candidate;
    }
    socket_error(location,"Cannot connect the TCP client","Не удалось подключить TCP-клиент");
}

class SocketStream final:public BinaryDevice {
    Descriptor handle_;
    const ExecutionOptions& options_;
public:
    SocketStream(Descriptor handle,const ExecutionOptions& options):handle_(handle),options_(options){}
    std::int64_t position(SourceLocation location) const override {
        socket_error(location,"TCP streams do not have a file position","Поток TCP не имеет файловой позиции");
    }
    void seek(std::int64_t,SourceLocation location) override { (void)position(location); }
    std::size_t read(std::span<char> bytes,SourceLocation location) override {
        if(bytes.empty())return 0;
        for(;;){
            wait_ready(handle_,POLLIN,location,options_);
            const auto received=::recv(handle_,bytes.data(),static_cast<int>(bytes.size()),0);
            if(received>=0)return static_cast<std::size_t>(received);
            const auto error=last_error();
            if(!interrupted(error) && !pending(error))
                socket_error(location,"Cannot receive the socket string","Не удалось получить строку через сокет");
        }
    }
    void write(std::string_view bytes,SourceLocation location) override {
#ifdef MSG_NOSIGNAL
        constexpr int send_flags=MSG_NOSIGNAL;
#else
        constexpr int send_flags=0;
#endif
        while(!bytes.empty()){
            wait_ready(handle_,POLLOUT,location,options_);
            const auto sent=::send(handle_,bytes.data(),static_cast<int>(bytes.size()),send_flags);
            if(sent>0){bytes.remove_prefix(static_cast<std::size_t>(sent));continue;}
            const auto error=last_error();
            if(sent==0 || (!interrupted(error) && !pending(error)))
                socket_error(location,"Cannot send the socket string","Не удалось передать строку через сокет");
        }
    }
};
}

struct SocketContext::State {
    std::vector<Entry> entries;
    explicit State(SourceLocation location){
#ifdef _WIN32
        WSADATA data{};
        if(::WSAStartup(MAKEWORD(2,2),&data)!=0)
            socket_error(location,"Cannot initialize Windows Sockets","Не удалось инициализировать Windows Sockets");
#else
        (void)location;
#endif
    }
    ~State(){
#ifdef _WIN32
        entries.clear();
        ::WSACleanup();
#endif
    }
    std::int32_t add(Socket socket,Role role,SourceLocation location){
        if(entries.size()>=static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
            throw runtime_error(RuntimeError::allocation_limit,location);
        const auto handle=static_cast<std::int32_t>(entries.size());
        entries.push_back({std::move(socket),role});
        return handle;
    }
    Entry& get(std::int32_t handle,Role role,SourceLocation location){
        if(handle<0 || static_cast<std::size_t>(handle)>=entries.size())invalid_handle(location);
        auto& entry=entries[static_cast<std::size_t>(handle)];
        if(entry.role!=role || entry.socket.get()==invalid)invalid_handle(location);
        return entry;
    }
};

SocketContext::SocketContext()=default;
SocketContext::~SocketContext()=default;
SocketContext::State& SocketContext::state(SourceLocation location){
    if(!state_)state_=std::make_unique<State>(location);
    return *state_;
}
SocketContext::State& SocketContext::opened(SourceLocation location){
    if(!state_)invalid_handle(location);
    return *state_;
}
std::int32_t SocketContext::create_server(std::int32_t port,SourceLocation location){
    const auto checked=port_number(port,location);
    auto& context=state(location);
    auto socket=create_tcp(location);
    sockaddr_in address{};
    address.sin_family=AF_INET;
    address.sin_port=htons(checked);
    address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    if(::bind(socket.get(),reinterpret_cast<const sockaddr*>(&address),sizeof address)!=0
        || ::listen(socket.get(),SOMAXCONN)!=0)
        socket_error(location,"Cannot start the loopback listener","Не удалось запустить прослушивание на локальном адресе");
    return context.add(std::move(socket),Role::listener,location);
}
std::int32_t SocketContext::create_client(SourceLocation location){
    auto& context=state(location);
    return context.add(create_tcp(location),Role::outgoing,location);
}
std::int32_t SocketContext::listener_port(std::int32_t listener,SourceLocation location){
    const auto handle=opened(location).get(listener,Role::listener,location).socket.get();
    sockaddr_in address{};
    AddressLength length=sizeof address;
    if(::getsockname(handle,reinterpret_cast<sockaddr*>(&address),&length)!=0)
        socket_error(location,"Cannot read the listener address","Не удалось прочитать адрес прослушивания");
    return ntohs(address.sin_port);
}
std::int32_t SocketContext::accept(std::int32_t listener,SourceLocation location,const ExecutionOptions& options){
    auto& context=opened(location);
    const auto handle=context.get(listener,Role::listener,location).socket.get();
    for(;;){
        wait_ready(handle,POLLIN,location,options);
        const auto accepted=::accept(handle,nullptr,nullptr);
        if(accepted!=invalid)return context.add(configured(accepted,location),Role::incoming,location);
        const auto error=last_error();
        if(!interrupted(error) && !pending(error))
            socket_error(location,"Cannot accept the TCP connection","Не удалось принять TCP-соединение");
    }
}
bool SocketContext::connect(std::int32_t client,std::u16string_view host,std::int32_t port,
    SourceLocation location,const ExecutionOptions& options) {
    auto& entry=opened(location).get(client,Role::outgoing,location);
    if(entry.connected)
        socket_error(location,"Client socket is already connected","Клиентский сокет уже подключён");
    entry.socket=connect_tcp(host,port,AF_INET,location,options);
    entry.connected=true;
    return true;
}
void SocketContext::send(std::int32_t client,std::u16string_view text,SourceLocation location,const ExecutionOptions& options){
    const auto& entry=opened(location).get(client,Role::outgoing,location);
    if(!entry.connected)socket_error(location,"TCP client is not connected","TCP-клиент не подключён");
    const auto bytes=encode_text_bytes(text,{TextEncodingKind::ascii,false,false},location);
    SocketStream(entry.socket.get(),options).write(bytes,location);
}
std::u16string SocketContext::receive(std::int32_t client,SourceLocation location,const ExecutionOptions& options){
    const auto handle=opened(location).get(client,Role::incoming,location).socket.get();
    std::array<char,1024> bytes{};
    const auto received=SocketStream(handle,options).read(bytes,location);
    return decode_text_bytes(std::string_view(bytes.data(),received),
        {TextEncodingKind::ascii,false,false},false,location);
}

std::u16string SocketContext::command(const Value& host,std::int32_t port,const Value& text,bool reply,
    SourceLocation location,const ExecutionOptions& options) {
    if(std::holds_alternative<NilString>(host))throw runtime_error(RuntimeError::nil_reference,location);
    (void)state(location);
    auto socket=connect_tcp(std::get<std::u16string>(host),port,AF_UNSPEC,location,options);
    SocketStream stream(socket.get(),options);
    if(std::holds_alternative<NilString>(text))throw runtime_error(RuntimeError::nil_reference,location);
    BinaryCodec writer(stream,{TextEncodingKind::utf8,false,true});
    writer.write_string(std::get<std::u16string>(text),location,options);
    if(!reply)return {};
    BinaryCodec reader(stream,{TextEncodingKind::utf8,false,false},false,TextDecoding::framework);
    return reader.read_string(location,options);
}
void SocketContext::close(std::int32_t handle){
    if(state_ && handle>=0 && static_cast<std::size_t>(handle)<state_->entries.size())
        state_->entries[static_cast<std::size_t>(handle)].socket.close();
}
}

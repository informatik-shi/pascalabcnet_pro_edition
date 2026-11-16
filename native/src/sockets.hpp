#pragma once
#include "abc/core.hpp"

namespace abc {
class SocketContext {
public:
    SocketContext();
    ~SocketContext();
    SocketContext(const SocketContext&)=delete;
    SocketContext& operator=(const SocketContext&)=delete;

    std::int32_t create_server(std::int32_t port,SourceLocation);
    std::int32_t listener_port(std::int32_t listener,SourceLocation);
    std::int32_t create_client(SourceLocation);
    std::int32_t accept(std::int32_t listener,SourceLocation,const ExecutionOptions&);
    bool connect(std::int32_t client,std::u16string_view host,std::int32_t port,SourceLocation,const ExecutionOptions&);
    void send(std::int32_t client,std::u16string_view text,SourceLocation,const ExecutionOptions&);
    std::u16string receive(std::int32_t client,SourceLocation,const ExecutionOptions&);
    void close(std::int32_t handle);

private:
    struct State;
    std::unique_ptr<State> state_;
    State& state(SourceLocation);
    State& opened(SourceLocation);
};
}

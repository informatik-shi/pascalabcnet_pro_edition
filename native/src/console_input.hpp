#pragma once
#include "abc/core.hpp"

namespace abc {
// One cursor is shared by Read, Readln and Arrays.ReadArray during an execution.
class ConsoleInput {
public:
    ConsoleInput(std::istream& input, const ExecutionOptions& options) : input_(input), options_(options) {}
    Value read(Type type, SourceLocation location);
    std::u16string token(SourceLocation location);
    std::u16string read_line(SourceLocation location);
    void skip_line(SourceLocation location);
private:
    bool ensure(SourceLocation location);
    void check(SourceLocation location) const;
    std::istream& input_;
    const ExecutionOptions& options_;
    std::u16string line_;
    std::size_t cursor_=0;
    bool ended_=false;
};
} // namespace abc

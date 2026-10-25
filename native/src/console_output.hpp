#pragma once
#include "abc/core.hpp"

namespace abc {
class ConsoleOutput {
public:
    ConsoleOutput(std::ostream& output, const ExecutionOptions& options) : output_(output), limit_(options.output_limit) {}
    ConsoleOutput(const ConsoleOutput&)=delete;
    ConsoleOutput& operator=(const ConsoleOutput&)=delete;
    ~ConsoleOutput();
    void write(std::u16string_view text, SourceLocation location);
    void finish();
private:
    void emit(std::string_view bytes, SourceLocation location);
    std::ostream& output_;
    std::size_t limit_, used_=0;
    char16_t pending_=0;
    SourceLocation pending_location_{};
};
} // namespace abc

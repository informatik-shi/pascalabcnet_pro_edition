#pragma once
#include "abc/core.hpp"
namespace abc {
const std::u16string& process_executable_name();
class ProgramEnvironment {
    const ExecutionOptions& options_;
    bool redirected() const;
public:
    explicit ProgramEnvironment(const ExecutionOptions& options):options_(options){}
    std::int32_t count(SourceLocation) const;
    std::u16string parameter(std::int32_t,SourceLocation) const;
    std::u16string executable(SourceLocation) const;
};
}

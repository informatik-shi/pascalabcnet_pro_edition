#pragma once
#include "abc/core.hpp"
#include <functional>
#include <mutex>

namespace abc {
class TimeContext {
public:
    using Clock=std::function<std::uint64_t()>;
    using Pause=std::function<void(std::uint32_t)>;
    explicit TimeContext(Clock clock={},Pause pause={});
    std::int32_t milliseconds(SourceLocation location) const;
    std::int32_t delta(SourceLocation location);
    void sleep(std::int32_t duration,SourceLocation location,const ExecutionOptions& options) const;
private:
    Clock clock_;
    Pause pause_;
    std::uint64_t start_,last_;
    std::mutex delta_mutex_;
};
}

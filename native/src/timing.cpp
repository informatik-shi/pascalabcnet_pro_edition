#include "timing.hpp"
#include "diagnostics.hpp"
#include <algorithm>
#include <chrono>
#include <thread>

namespace abc {
namespace {
std::int32_t milliseconds_value(std::uint64_t elapsed,SourceLocation location) {
    const auto value=elapsed/1'000'000;
    if (value>INT32_MAX) throw runtime_error(RuntimeError::integer_overflow,location);
    return static_cast<std::int32_t>(value);
}
}
TimeContext::TimeContext(Clock clock,Pause pause):clock_(std::move(clock)),pause_(std::move(pause)) {
    if (!clock_) clock_=[] {
        return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
    };
    if (!pause_) pause_=[](std::uint32_t duration) {
        if (duration) std::this_thread::sleep_for(std::chrono::milliseconds(duration));
        else std::this_thread::yield();
    };
    start_=last_=clock_();
}
std::int32_t TimeContext::milliseconds(SourceLocation location) const {
    return milliseconds_value(clock_()-start_,location);
}
std::int32_t TimeContext::delta(SourceLocation location) {
    const std::lock_guard<std::mutex> lock(delta_mutex_);
    const auto now=clock_(),elapsed=now-last_;last_=now;
    return milliseconds_value(elapsed,location);
}
void TimeContext::sleep(std::int32_t duration,SourceLocation location,const ExecutionOptions& options) const {
    if (duration< -1) throw runtime_error(RuntimeError::sleep_duration,location);
    const auto check=[&] {if (options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,location);};
    if (!duration) {check();pause_(0);check();return;}
    const bool infinite=duration==-1;
    const auto begin=infinite ? 0 : clock_();
    const auto requested=infinite ? 0 : static_cast<std::uint64_t>(duration)*1'000'000;
    for (;;) {
        check();std::uint64_t remaining=20'000'000;
        if (!infinite) {
            const auto elapsed=clock_()-begin;if (elapsed>=requested) return;
            remaining=requested-elapsed;
        }
        const auto chunk=std::min<std::uint64_t>(20,(remaining+999'999)/1'000'000);
        pause_(static_cast<std::uint32_t>(chunk));
    }
}
}

#pragma once
#include "abc/core.hpp"

namespace abc {
struct Values;
struct RealRangePlan {
    std::uint64_t count=0;
    std::int32_t prefix=0;
    bool snap=false,direct=false;
};
RealRangePlan real_range_plan(Values& values,double first,double last,double step,const ExecutionOptions& options);
} // namespace abc

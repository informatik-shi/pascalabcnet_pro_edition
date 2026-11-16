#pragma once
#include "abc/core.hpp"

namespace abc {
struct Values;
Value utils_record_to_bytes(Values& values,Type result,const Value& object,const ExecutionOptions& options);
}

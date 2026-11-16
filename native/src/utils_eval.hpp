#pragma once
#include "abc/core.hpp"

namespace abc {
struct Values;
Value utils_eval(Values&,const Value& expression,Type result,const ExecutionOptions&);
}

#pragma once
#include "abc/core.hpp"
namespace abc {
struct Values;
struct ValueHashWork {
    Values& values;const ExecutionOptions& options;std::uint64_t done=0;
    void step();
};
std::size_t value_hash(ValueHashWork& work,const Value& value);
bool value_equal(ValueHashWork& work,const Value& left,const Value& right);
}

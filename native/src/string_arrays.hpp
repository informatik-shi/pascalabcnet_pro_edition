#pragma once
#include "abc/core.hpp"

namespace abc {
struct Values;
std::shared_ptr<Aggregate> make_string_array(Values& values, Type result, StringArray method,
    const Value& source, const Value& parameter, const ExecutionOptions& options);
} // namespace abc

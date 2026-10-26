#pragma once
#include "stdlib.hpp"

namespace abc {
struct Values;
struct SchoolContext { std::shared_ptr<Aggregate> primes; std::int32_t initial_prime_count=0; };
Value invoke_school_list(Values& values, SchoolContext& context, SchoolList method, Type list,
    std::span<const Value> arguments, const ExecutionOptions& options);
}

namespace abc::stdlib {
Value invoke_school(Function function, std::span<const Value> arguments, SourceLocation location, const ExecutionOptions& options, SchoolContext& school);
} // namespace abc::stdlib

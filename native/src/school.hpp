#pragma once
#include "stdlib.hpp"

namespace abc {
struct Values;
struct SchoolContext { std::shared_ptr<Aggregate> primes; std::int32_t initial_prime_count=0; };
Value invoke_school_list(Values& values, SchoolContext& context, SchoolList method, Type list,
    std::span<const Value> arguments, const ExecutionOptions& options);
Value school_replace_many(const Value& text, const std::shared_ptr<Aggregate>& source, const std::shared_ptr<Aggregate>& target,
    SourceLocation location, const ExecutionOptions& options);
void school_replace_last(Values& values, const std::shared_ptr<Cell>& receiver, const Value& source, const Value& target, const ExecutionOptions& options);
std::int64_t school_fold(const std::shared_ptr<Aggregate>& array, SchoolFold method, SourceLocation location, const ExecutionOptions& options);
std::int64_t school_pair(std::int64_t a, std::int64_t b, bool lcm, SourceLocation location);
}

namespace abc::stdlib {
Value invoke_school(Function function, std::span<const Value> arguments, SourceLocation location, const ExecutionOptions& options, SchoolContext& school);
Value invoke_school_text(Function function, std::span<const Value> arguments, SourceLocation location, const ExecutionOptions& options);
} // namespace abc::stdlib

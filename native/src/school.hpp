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
Value school_fold(Values& values,const std::shared_ptr<Aggregate>& array,SchoolFold method,const ExecutionOptions& options);
std::int64_t school_pair(std::int64_t a, std::int64_t b, bool lcm, SourceLocation location);
std::pair<std::int64_t,std::int64_t> school_pair_values(std::int64_t a,std::int64_t b,SourceLocation location);
std::shared_ptr<Aggregate> school_minmax(Values& values,const std::shared_ptr<Aggregate>& sequence,Type tuple,const ExecutionOptions& options);
void school_table_print(const std::shared_ptr<Aggregate>& table,std::int32_t filter,const Value& names,SourceLocation location,stdlib::Context& context);
}

namespace abc::stdlib {
Value invoke_school(Function function, std::span<const Value> arguments, SourceLocation location, const ExecutionOptions& options, SchoolContext& school);
Value invoke_school_text(Function function, std::span<const Value> arguments, SourceLocation location, const ExecutionOptions& options);
} // namespace abc::stdlib

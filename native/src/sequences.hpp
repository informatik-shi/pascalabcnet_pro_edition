#pragma once
#include "abc/core.hpp"

namespace abc {
struct Values;
std::shared_ptr<Aggregate> sequence_text(Values& values,Type type,Value text);
std::shared_ptr<Aggregate> sequence_create(Values& values,Type type,const std::shared_ptr<Aggregate>& source,SequenceProvider provider);
std::shared_ptr<Aggregate> sequence_begin(Values& values,const std::shared_ptr<Aggregate>& sequence);
void sequence_check(const std::shared_ptr<Aggregate>& sequence,SourceLocation location);
bool sequence_resume(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options);
std::optional<std::uint64_t> sequence_known_length(const std::shared_ptr<Aggregate>& sequence,SourceLocation location);
std::uint64_t sequence_length(const std::shared_ptr<Aggregate>& sequence,SourceLocation location);
Value sequence_at(Values& values,const std::shared_ptr<Aggregate>& sequence,std::int64_t index,const ExecutionOptions& options);
Value sequence_collect(Values& values,const std::shared_ptr<Aggregate>& sequence,Type collection,const ExecutionOptions& options);
Value sequence_query(Values& values,const std::shared_ptr<Aggregate>& sequence,SequenceQuery method,const Value& argument,const ExecutionOptions& options);
std::shared_ptr<Aggregate> sequence_snapshot(Values& values,Type type,const std::shared_ptr<Aggregate>& array,const ExecutionOptions& options);
std::shared_ptr<Aggregate> sequence_repeat(Values& values,Type type,std::int32_t count,const Value& value);
std::shared_ptr<Aggregate> sequence_range(Values& values,Type type,const Value& first,const Value& last);
std::shared_ptr<Aggregate> sequence_range_step(Values& values,Type type,const Value& first,const Value& last,std::int32_t step);
std::shared_ptr<Aggregate> sequence_range_big(Values& values,Type type,const BigInteger& first,const BigInteger& last,const BigInteger& step);
std::shared_ptr<Aggregate> sequence_partition(Values& values,Type type,double first,double last,std::int32_t intervals);
std::shared_ptr<Aggregate> sequence_range_real(Values& values,Type type,double first,double last,double step);
Value sequence_numeric_step(Values& values,Type type,const Value& left,const Value& right,bool product,const ExecutionOptions& options);
Value sequence_aggregate(Values& values,const std::shared_ptr<Aggregate>& sequence,SequenceAggregate method,const ExecutionOptions& options);
std::shared_ptr<Aggregate> sequence_partial(Values& values,Type type,const std::shared_ptr<Aggregate>& source,const ExecutionOptions& options={});
bool sequence_partial_resume(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options);
std::shared_ptr<Aggregate> sequence_slice(Values& values,Type type,const std::shared_ptr<Aggregate>& source,SequenceSlice method,std::int32_t count,const ExecutionOptions& options);
bool sequence_slice_resume(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options);
std::shared_ptr<Aggregate> sequence_concat(Values& values,Type type,const std::shared_ptr<Aggregate>& first,const std::shared_ptr<Aggregate>& second,const ExecutionOptions& options);
bool sequence_concat_resume(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options);
std::shared_ptr<Aggregate> sequence_transform(Values& values,Type type,const std::shared_ptr<Aggregate>& source,std::size_t callback,bool predicate,const ExecutionOptions& options,bool flatten=false);
bool sequence_transform_resume(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options);
Value sequence_predicate(Values& values,const std::shared_ptr<Aggregate>& source,std::size_t callback,PredicateQuery method,const ExecutionOptions& options);
bool sequence_equal(Values& values,const std::shared_ptr<Aggregate>& first,const std::shared_ptr<Aggregate>& second,const ExecutionOptions& options);
std::shared_ptr<Aggregate> sequence_default(Values& values,Type type,const std::shared_ptr<Aggregate>& source,Value fallback,const ExecutionOptions& options);
std::shared_ptr<Aggregate> sequence_zip(Values& values,Type type,std::vector<std::shared_ptr<Aggregate>> sources,std::size_t callback,const ExecutionOptions& options);
bool sequence_zip_resume(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options);
} // namespace abc

#pragma once
#include "abc/core.hpp"

namespace abc {
struct Values;
std::shared_ptr<Aggregate> sequence_create(Values& values,Type type,const std::shared_ptr<Aggregate>& source,SequenceProvider provider);
std::shared_ptr<Aggregate> sequence_begin(Values& values,const std::shared_ptr<Aggregate>& sequence);
void sequence_check(const std::shared_ptr<Aggregate>& sequence,SourceLocation location);
std::uint64_t sequence_length(const std::shared_ptr<Aggregate>& sequence,SourceLocation location);
Value sequence_at(Values& values,const std::shared_ptr<Aggregate>& sequence,std::int64_t index);
Value sequence_collect(Values& values,const std::shared_ptr<Aggregate>& sequence,Type collection,const ExecutionOptions& options);
} // namespace abc

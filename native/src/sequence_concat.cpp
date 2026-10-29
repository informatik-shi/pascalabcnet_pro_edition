#include "sequences.hpp"
#include "sequence_guard.hpp"
#include <algorithm>

namespace abc {
std::shared_ptr<Aggregate> sequence_concat(Values& values,Type type,const std::shared_ptr<Aggregate>& first,const std::shared_ptr<Aggregate>& second,const ExecutionOptions& options) {
    // LINQ Concat validates both references now, but opens each enumerator
    // only when the consumer reaches that source.
    if (!first || first->nil || !second || second->nil) throw runtime_error(RuntimeError::nil_reference,values.location);
    const auto depth=std::max(first->sequence ? first->sequence->nesting : 0,second->sequence ? second->sequence->nesting : 0);
    if (depth>=options.call_limit) throw runtime_error(RuntimeError::sequence_depth,values.location);
    auto result=sequence_composed(values,type,first,SequenceProvider::concat,options);
    result->sequence->second_source=second; result->sequence->nesting=depth+1; return result;
}
bool sequence_concat_resume(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options) {
    SequenceDepthGuard guard(values,options);
    const auto& state=*sequence->sequence;
    if (!state.concat) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    auto& cursor=*state.concat;
    if (cursor.positioned && index==cursor.index) return cursor.available;
    if ((!cursor.positioned && index) || (cursor.positioned && (!cursor.available || index<=cursor.index || index-cursor.index!=1)))
        throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    while (!cursor.ended) {
        if (cursor.work>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
        if ((cursor.work++&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
        if (!cursor.iterator) cursor.iterator=sequence_begin(values,cursor.second ? state.second_source : state.source);
        if (cursor.source_index>static_cast<std::uint64_t>(INT64_MAX)) throw runtime_error(RuntimeError::integer_overflow,values.location);
        if (sequence_resume(values,cursor.iterator,cursor.source_index,options)) {
            cursor.current=sequence_at(values,cursor.iterator,static_cast<std::int64_t>(cursor.source_index),options);
            ++cursor.source_index; cursor.positioned=true; cursor.index=index; cursor.available=true; return true;
        }
        cursor.iterator.reset(); cursor.source_index=0;
        if (cursor.second) cursor.ended=true;
        else cursor.second=true;
    }
    cursor.positioned=true; cursor.index=index; cursor.available=false; return false;
}
} // namespace abc

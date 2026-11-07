#include "sequences.hpp"
#include "sequence_guard.hpp"
#include <array>

namespace abc {
std::shared_ptr<Aggregate> sequence_transform(Values& values,Type type,const std::shared_ptr<Aggregate>& source,std::size_t callback,SequenceTransform method,const ExecutionOptions& options,std::shared_ptr<Aggregate> callable) {
    if (!source || source->nil) throw runtime_error(RuntimeError::nil_reference,values.location);
    if (callable) (void)values.callable_target(callable);
    auto result=sequence_composed(values,type,source,SequenceProvider::transform,options);
    result->sequence->callable=std::move(callable);
    result->sequence->callback=callback; result->sequence->transform_method=method; return result;
}
bool sequence_transform_resume(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options) {
    SequenceDepthGuard guard(values,options); const auto& state=*sequence->sequence;
    if (!state.transform || !values.invoke || (!state.callable && state.callback>=values.program.routines.size())) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    const bool indexed=state.callable ? type_info(values.program,state.callable->type).parameters.size()==2
        : values.program.routines[state.callback].parameters.size()==2;
    auto& cursor=*state.transform;
    if (cursor.positioned && index==cursor.index) return cursor.available;
    if ((!cursor.positioned && index) || (cursor.positioned && (!cursor.available || index<=cursor.index || index-cursor.index!=1)))
        throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    while (!cursor.ended) {
        if (cursor.work>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
        if ((cursor.work++&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
        if (state.transform_method==SequenceTransform::flatmap && cursor.inner) {
            if (cursor.inner_index>static_cast<std::uint64_t>(INT64_MAX)) throw runtime_error(RuntimeError::integer_overflow,values.location);
            if (sequence_resume(values,cursor.inner,cursor.inner_index,options)) {
                cursor.current=sequence_at(values,cursor.inner,static_cast<std::int64_t>(cursor.inner_index),options); ++cursor.inner_index;
                cursor.positioned=true; cursor.available=true; cursor.index=index; return true;
            }
            auto completed=std::move(cursor.inner);sequence_dispose(values,completed,options);
        }
        if (!cursor.iterator) cursor.iterator=sequence_begin(values,state.source,options);
        if (cursor.source_index>static_cast<std::uint64_t>(INT64_MAX)) throw runtime_error(RuntimeError::integer_overflow,values.location);
        if (!sequence_resume(values,cursor.iterator,cursor.source_index,options)) { cursor.ended=true; break; }
        auto item=sequence_at(values,cursor.iterator,static_cast<std::int64_t>(cursor.source_index),options);
        if (indexed && cursor.source_index>INT32_MAX) throw runtime_error(RuntimeError::integer_overflow,values.location);
        if (state.transform_method==SequenceTransform::skip_while && cursor.prefix_done) {
            ++cursor.source_index; cursor.current=std::move(item);
            cursor.positioned=true; cursor.available=true; cursor.index=index; return true;
        }
        const std::array<Value,2> arguments={item,static_cast<std::int32_t>(indexed ? cursor.source_index : 0)}; ++cursor.source_index;
        const auto actual=std::span<const Value>(arguments).first(indexed ? 2U : 1U);
        auto value=state.callable ? values.invoke_value(state.callable,actual) : values.invoke(state.callback,actual);
        if (state.transform_method==SequenceTransform::flatmap) {
            const auto returned=state.callable ? type_info(values.program,state.callable->type).element : values.program.routines[state.callback].result;
            std::shared_ptr<Aggregate> source;
            if (returned==Type::string) source=sequence_text(values,sequence->type,std::move(value));
            else {
                source=std::get<std::shared_ptr<Aggregate>>(std::move(value));
                const auto kind=type_info(values.program,returned).kind;
                if (kind!=CompoundKind::sequence) source=sequence_create(values,sequence->type,source,kind==CompoundKind::array ? SequenceProvider::array : SequenceProvider::list);
            }
            cursor.inner=sequence_begin(values,source,options); cursor.inner_index=0; continue;
        }
        if (predicate_transform(state.transform_method)) {
            const bool matched=std::get<bool>(value);
            if (state.transform_method==SequenceTransform::take_while && !matched) { cursor.ended=true; break; }
            if (state.transform_method==SequenceTransform::skip_while) {
                if (matched) continue;
                cursor.prefix_done=true;
            } else if (!matched) continue;
        }
        cursor.current=predicate_transform(state.transform_method) ? std::move(item) : std::move(value);
        cursor.positioned=true; cursor.available=true; cursor.index=index; return true;
    }
    cursor.positioned=true; cursor.available=false; cursor.index=index; return false;
}
} // namespace abc

#include "sequences.hpp"
#include "sequence_guard.hpp"
#include <array>

namespace abc {
std::shared_ptr<Aggregate> sequence_transform(Values& values,Type type,const std::shared_ptr<Aggregate>& source,std::size_t callback,bool predicate,const ExecutionOptions& options,bool flatten) {
    if (!source || source->nil) throw runtime_error(RuntimeError::nil_reference,values.location);
    auto result=sequence_composed(values,type,source,SequenceProvider::transform,options);
    result->sequence->callback=callback; result->sequence->predicate=predicate; result->sequence->flatten=flatten; return result;
}
bool sequence_transform_resume(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options) {
    SequenceDepthGuard guard(values,options); const auto& state=*sequence->sequence;
    if (!state.transform || !values.invoke || state.callback>=values.program.routines.size()) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    const bool indexed=values.program.routines[state.callback].parameters.size()==2;
    auto& cursor=*state.transform;
    if (cursor.positioned && index==cursor.index) return cursor.available;
    if ((!cursor.positioned && index) || (cursor.positioned && (!cursor.available || index<=cursor.index || index-cursor.index!=1)))
        throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    while (!cursor.ended) {
        if (cursor.work>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
        if ((cursor.work++&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
        if (state.flatten && cursor.inner) {
            if (cursor.inner_index>static_cast<std::uint64_t>(INT64_MAX)) throw runtime_error(RuntimeError::integer_overflow,values.location);
            if (sequence_resume(values,cursor.inner,cursor.inner_index,options)) {
                cursor.current=sequence_at(values,cursor.inner,static_cast<std::int64_t>(cursor.inner_index),options); ++cursor.inner_index;
                cursor.positioned=true; cursor.available=true; cursor.index=index; return true;
            }
            cursor.inner.reset();
        }
        if (!cursor.iterator) cursor.iterator=sequence_begin(values,state.source);
        if (cursor.source_index>static_cast<std::uint64_t>(INT64_MAX)) throw runtime_error(RuntimeError::integer_overflow,values.location);
        if (!sequence_resume(values,cursor.iterator,cursor.source_index,options)) { cursor.ended=true; break; }
        auto item=sequence_at(values,cursor.iterator,static_cast<std::int64_t>(cursor.source_index),options);
        if (indexed && cursor.source_index>INT32_MAX) throw runtime_error(RuntimeError::integer_overflow,values.location);
        const std::array<Value,2> arguments={item,static_cast<std::int32_t>(indexed ? cursor.source_index : 0)}; ++cursor.source_index;
        auto value=values.invoke(state.callback,std::span<const Value>(arguments).first(indexed ? 2U : 1U));
        if (state.flatten) {
            const auto returned=values.program.routines[state.callback].result;
            std::shared_ptr<Aggregate> source;
            if (returned==Type::string) source=sequence_text(values,sequence->type,std::move(value));
            else {
                source=std::get<std::shared_ptr<Aggregate>>(std::move(value));
                const auto kind=type_info(values.program,returned).kind;
                if (kind!=CompoundKind::sequence) source=sequence_create(values,sequence->type,source,kind==CompoundKind::array ? SequenceProvider::array : SequenceProvider::list);
            }
            cursor.inner=sequence_begin(values,source); cursor.inner_index=0; continue;
        }
        if (state.predicate && !std::get<bool>(value)) continue;
        cursor.current=state.predicate ? std::move(item) : std::move(value);
        cursor.positioned=true; cursor.available=true; cursor.index=index; return true;
    }
    cursor.positioned=true; cursor.available=false; cursor.index=index; return false;
}
} // namespace abc

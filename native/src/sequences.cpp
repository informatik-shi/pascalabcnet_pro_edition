#include "sequences.hpp"
#include "school_ip.hpp"
#include "runtime_values.hpp"

namespace abc {
namespace {
void require(const std::shared_ptr<Aggregate>& object,SourceLocation location) {
    if (!object || object->nil) throw runtime_error(RuntimeError::nil_reference,location);
}
const SequenceState& cursor(const std::shared_ptr<Aggregate>& sequence,SourceLocation location) {
    require(sequence,location);
    if (!sequence->sequence || !sequence->sequence->started) throw runtime_error(RuntimeError::invalid_bytecode,location);
    return *sequence->sequence;
}
void reserve_state(Values& values) {
    if (!values.remaining) throw runtime_error(RuntimeError::allocation_limit,values.location);
    --values.remaining;
}
}
std::shared_ptr<Aggregate> sequence_create(Values& values,Type type,const std::shared_ptr<Aggregate>& source,SequenceProvider provider) {
    if (!source || source->nil) return std::get<std::shared_ptr<Aggregate>>(values.zero(type));
    reserve_state(values);
    auto result=std::make_shared<Aggregate>(); result->type=type; result->sequence=std::make_unique<SequenceState>();
    result->sequence->source=source; result->sequence->provider=provider; return result;
}
std::shared_ptr<Aggregate> sequence_begin(Values& values,const std::shared_ptr<Aggregate>& sequence) {
    require(sequence,values.location);
    if (!sequence->sequence) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    reserve_state(values);
    auto result=std::make_shared<Aggregate>(); result->type=sequence->type;
    result->sequence=std::make_unique<SequenceState>(*sequence->sequence);
    auto& state=*result->sequence; state.started=true; state.first=state.count=0;
    require(state.source,values.location);
    if (collection_sequence(state.provider)) { state.count=state.source->items.size(); state.version=state.source->version; }
    else school_ip_sequence_start(state,values.location);
    return result;
}
void sequence_check(const std::shared_ptr<Aggregate>& sequence,SourceLocation location) {
    const auto& state=cursor(sequence,location);
    if (state.provider==SequenceProvider::list && state.version!=state.source->version) throw runtime_error(RuntimeError::collection_changed,location);
}
std::uint64_t sequence_length(const std::shared_ptr<Aggregate>& sequence,SourceLocation location) { return cursor(sequence,location).count; }
Value sequence_at(Values& values,const std::shared_ptr<Aggregate>& sequence,std::int64_t index) {
    sequence_check(sequence,values.location); const auto& state=*sequence->sequence;
    if (index<0 || static_cast<std::uint64_t>(index)>=state.count) throw runtime_error(RuntimeError::index_bounds,values.location);
    if (collection_sequence(state.provider)) return values.copy(state.source->items[static_cast<std::size_t>(index)]->value);
    return school_ip_sequence_value(values,type_info(values.program,sequence->type).element,state,index);
}
Value sequence_collect(Values& values,const std::shared_ptr<Aggregate>& sequence,Type array,const ExecutionOptions& options) {
    const auto iterator=sequence_begin(values,sequence); const auto count=sequence_length(iterator,values.location);
    const bool materialize=array!=Type::invalid;
    if (!materialize && collection_sequence(iterator->sequence->provider)) {
        if (count>INT32_MAX) throw runtime_error(RuntimeError::integer_overflow,values.location);
        return static_cast<std::int32_t>(count);
    }
    const auto element=type_info(values.program,sequence->type).element;
    std::shared_ptr<Aggregate> result;
    if (materialize) {
        const auto cost=iterator->sequence->provider==SequenceProvider::school_ip_hosts ? 4U : 1U;
        if (count>values.remaining/cost) throw runtime_error(RuntimeError::allocation_limit,values.location);
        result=values.array(array,0);
    }
    for (std::uint64_t i=0;i<count;++i) {
        if (i>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
        if ((i&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
        auto value=sequence_at(values,iterator,static_cast<std::int64_t>(i));
        if (materialize) result->items.push_back(values.cell(element,std::move(value)));
        else if (i>=INT32_MAX) throw runtime_error(RuntimeError::integer_overflow,values.location);
    }
    sequence_check(iterator,values.location);
    return materialize ? Value{result} : Value{static_cast<std::int32_t>(count)};
}
} // namespace abc

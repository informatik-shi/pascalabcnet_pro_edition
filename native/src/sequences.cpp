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
    auto& state=*result->sequence; state.started=true; state.first=0;
    if (state.provider==SequenceProvider::repeat || state.provider==SequenceProvider::range) {
        if (!state.seed || state.seed->type!=type_info(values.program,sequence->type).element)
            throw runtime_error(RuntimeError::invalid_bytecode,values.location);
        return result;
    }
    state.count=0;
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
    if (state.provider==SequenceProvider::repeat) return values.copy(state.seed->value);
    if (state.provider==SequenceProvider::range) {
        if (state.seed->type==Type::character) return static_cast<char16_t>(std::get<char16_t>(state.seed->value)+index);
        return static_cast<std::int32_t>(std::int64_t(std::get<std::int32_t>(state.seed->value))+index);
    }
    return school_ip_sequence_value(values,type_info(values.program,sequence->type).element,state,index);
}
Value sequence_collect(Values& values,const std::shared_ptr<Aggregate>& sequence,Type collection,const ExecutionOptions& options) {
    const auto iterator=sequence_begin(values,sequence); const auto count=sequence_length(iterator,values.location);
    const bool materialize=collection!=Type::invalid;
    const bool list=materialize && type_info(values.program,collection).kind==CompoundKind::list;
    if (!materialize && collection_sequence(iterator->sequence->provider)) {
        if (count>INT32_MAX) throw runtime_error(RuntimeError::integer_overflow,values.location);
        return static_cast<std::int32_t>(count);
    }
    const auto element=type_info(values.program,sequence->type).element;
    std::shared_ptr<Aggregate> result;
    if (materialize) {
        const auto cost=iterator->sequence->provider==SequenceProvider::school_ip_hosts ? 4U : 1U;
        if (count>values.remaining/cost) throw runtime_error(RuntimeError::allocation_limit,values.location);
        result=values.array(collection,0);
    }
    for (std::uint64_t i=0;i<count;++i) {
        if (i>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
        if ((i&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
        auto value=sequence_at(values,iterator,static_cast<std::int64_t>(i));
        if (materialize) {
            auto item=values.cell(element,std::move(value));
            if (list) values.own_list_cell(item,result);
            result->items.push_back(std::move(item));
        }
        else if (i>=INT32_MAX) throw runtime_error(RuntimeError::integer_overflow,values.location);
    }
    sequence_check(iterator,values.location);
    return materialize ? Value{result} : Value{static_cast<std::int32_t>(count)};
}
Value sequence_query(Values& values,const std::shared_ptr<Aggregate>& sequence,SequenceQuery method,const Value& argument,const ExecutionOptions& options) {
    require(sequence,values.location);
    const auto element=type_info(values.program,sequence->type).element;
    const bool indexed=sequence_query_index(method);
    const bool single=method==SequenceQuery::single || method==SequenceQuery::single_default;
    const bool defaults=method==SequenceQuery::first_default || method==SequenceQuery::last_default
        || method==SequenceQuery::element_default || method==SequenceQuery::single_default;
    const auto absent=[&]() -> Value {
        if (defaults) return values.zero(element,false,true);
        throw runtime_error(indexed ? RuntimeError::index_bounds : RuntimeError::sequence_empty,values.location);
    };
    const auto index=indexed ? std::get<std::int32_t>(argument) : 0;
    if (indexed && index<0) return absent();
    const auto iterator=sequence_begin(values,sequence); const auto count=sequence_length(iterator,values.location);
    if (!count) {
        if (method==SequenceQuery::any || method==SequenceQuery::contains) return false;
        return absent();
    }
    std::uint64_t work=0;
    const auto check=[&] {
        if (work>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
        if ((work++&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
        sequence_check(iterator,values.location);
    };
    // IList-backed views retain direct access. Generator queries visit and produce
    // each preceding value, so allocation, cancellation and work limits still apply.
    if (collection_sequence(iterator->sequence->provider) && method!=SequenceQuery::contains) {
        check();
        if (method==SequenceQuery::any) return true;
        if (single && count!=1) throw runtime_error(RuntimeError::sequence_multiple,values.location);
        const auto target=indexed ? static_cast<std::uint64_t>(index)
            : method==SequenceQuery::last || method==SequenceQuery::last_default ? count-1 : 0;
        if (target>=count) return absent();
        return sequence_at(values,iterator,static_cast<std::int64_t>(target));
    }
    Value last=std::int32_t{0};
    for (std::uint64_t i=0;i<count;++i) {
        check(); auto value=sequence_at(values,iterator,static_cast<std::int64_t>(i));
        if (method==SequenceQuery::any) return true;
        if (method==SequenceQuery::contains) { if (values.equal(value,argument)) return true; }
        else if ((indexed && i==static_cast<std::uint64_t>(index)) || method==SequenceQuery::first || method==SequenceQuery::first_default) return value;
        else if (single) {
            if (i) throw runtime_error(RuntimeError::sequence_multiple,values.location);
            last=std::move(value);
        }
        else if (method==SequenceQuery::last || method==SequenceQuery::last_default) last=std::move(value);
    }
    sequence_check(iterator,values.location);
    if (method==SequenceQuery::contains) return false;
    if (method==SequenceQuery::last || method==SequenceQuery::last_default || single) return last;
    return absent();
}
std::shared_ptr<Aggregate> sequence_snapshot(Values& values,Type type,const std::shared_ptr<Aggregate>& array,const ExecutionOptions& options) {
    require(array,values.location);
    if (array->items.size()>values.remaining) throw runtime_error(RuntimeError::allocation_limit,values.location);
    const auto element=type_info(values.program,type).element;
    auto snapshot=values.array(array->type,0);
    for (std::size_t i=0;i<array->items.size();++i) {
        if (i>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
        if ((i&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
        snapshot->items.push_back(values.cell(element,values.copy(array->items[i]->value)));
    }
    return sequence_create(values,type,snapshot,SequenceProvider::array);
}
std::shared_ptr<Aggregate> sequence_repeat(Values& values,Type type,std::int32_t count,const Value& value) {
    if (count<0) throw runtime_error(RuntimeError::sequence_count_range,values.location);
    reserve_state(values);
    auto result=std::make_shared<Aggregate>(); result->type=type; result->sequence=std::make_unique<SequenceState>();
    auto& state=*result->sequence; state.provider=SequenceProvider::repeat; state.count=static_cast<std::uint32_t>(count);
    state.seed=values.cell(type_info(values.program,type).element,values.copy(value)); return result;
}
std::shared_ptr<Aggregate> sequence_range(Values& values,Type type,const Value& first,const Value& last) {
    const bool characters=type_info(values.program,type).element==Type::character;
    const auto a=characters ? std::int64_t(std::get<char16_t>(first)) : std::int64_t(std::get<std::int32_t>(first));
    const auto b=characters ? std::int64_t(std::get<char16_t>(last)) : std::int64_t(std::get<std::int32_t>(last));
    const auto count=b<a ? 0 : b-a+1;
    if (count>INT32_MAX) throw runtime_error(RuntimeError::integer_overflow,values.location);
    if (!characters && !count && values.empty_integer_range && values.empty_integer_range->type==type) return values.empty_integer_range;
    auto result=sequence_repeat(values,type,static_cast<std::int32_t>(count),first);
    result->sequence->provider=SequenceProvider::range;
    if (!characters && !count) values.empty_integer_range=result;
    return result;
}
} // namespace abc

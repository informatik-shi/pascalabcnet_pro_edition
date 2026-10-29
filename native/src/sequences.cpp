#include "sequences.hpp"
#include "school_ip.hpp"
#include "runtime_values.hpp"
#include "big_integer_runtime.hpp"
#include "real_range.hpp"
#include <cmath>

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
double finite_partition(double value,SourceLocation location) {
    if (!std::isfinite(value)) throw runtime_error(RuntimeError::real_overflow,location);
    return value;
}
void reserve_state(Values& values) {
    if (!values.remaining) throw runtime_error(RuntimeError::allocation_limit,values.location);
    --values.remaining;
}
}
std::shared_ptr<Aggregate> sequence_text(Values& values,Type type,Value text) {
    if (std::holds_alternative<NilString>(text)) return std::get<std::shared_ptr<Aggregate>>(values.zero(type));
    const auto size=std::get<std::u16string>(text).size();
    if (size>values.remaining) throw runtime_error(RuntimeError::allocation_limit,values.location);
    values.remaining-=size; reserve_state(values);
    auto result=std::make_shared<Aggregate>(); result->type=type; result->sequence=std::make_unique<SequenceState>();
    auto& state=*result->sequence; state.provider=SequenceProvider::string; state.count=size; state.seed=values.cell(Type::string,std::move(text)); return result;
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
    if (state.provider==SequenceProvider::transform) { reserve_state(values); state.transform=std::make_shared<TransformState>(); return result; }
    if (state.provider==SequenceProvider::string) {
        if (!state.seed || state.seed->type!=Type::string || !std::holds_alternative<std::u16string>(state.seed->value)) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
        return result;
    }
    if (state.provider==SequenceProvider::concat) {
        reserve_state(values); state.concat=std::make_shared<ConcatState>(); return result;
    }
    if (state.provider==SequenceProvider::slice) {
        reserve_state(values); state.slice=std::make_shared<SliceState>(); return result;
    }
    if (state.provider==SequenceProvider::partial_sum) {
        reserve_state(values); state.partial=std::make_shared<PartialSumState>();
        state.partial->total=values.zero(type_info(values.program,sequence->type).element); return result;
    }
    if (state.provider==SequenceProvider::repeat || state.provider==SequenceProvider::range || state.provider==SequenceProvider::stepped_range || state.provider==SequenceProvider::big_range || state.provider==SequenceProvider::partition || state.provider==SequenceProvider::real_range) {
        if (!state.seed || state.seed->type!=type_info(values.program,sequence->type).element)
            throw runtime_error(RuntimeError::invalid_bytecode,values.location);
        if (state.provider==SequenceProvider::big_range) {
            if (!state.big_range || state.seed->type!=Type::big_integer) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
            reserve_state(values);
            state.big_range=std::make_shared<BigRangeState>(BigRangeState{state.big_range->last,state.big_range->step,std::get<BigInteger>(state.seed->value)});
        }
        if (state.provider==SequenceProvider::partition) {
            if (!state.partition || state.seed->type!=Type::real) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
            reserve_state(values);
            state.partition=std::make_shared<PartitionState>(PartitionState{state.partition->last,0,std::get<double>(state.seed->value)});
        }
        if (state.provider==SequenceProvider::real_range) {
            if (!state.real_range || state.seed->type!=Type::real) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
            reserve_state(values); state.count=0;
            state.real_range=std::make_shared<RealRangeState>(RealRangeState{state.real_range->last,state.real_range->step});
        }
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
bool sequence_resume(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options) {
    const auto location=values.location;
    sequence_check(sequence,location); auto& state=*sequence->sequence;
    if (state.provider==SequenceProvider::partial_sum) return sequence_partial_resume(values,sequence,index,options);
    if (state.provider==SequenceProvider::slice) return sequence_slice_resume(values,sequence,index,options);
    if (state.provider==SequenceProvider::concat) return sequence_concat_resume(values,sequence,index,options);
    if (state.provider==SequenceProvider::transform) return sequence_transform_resume(values,sequence,index,options);
    if (state.provider==SequenceProvider::real_range) {
        if (!state.real_range) throw runtime_error(RuntimeError::invalid_bytecode,location);
        auto& range=*state.real_range;
        if (!range.initialized) {
            if (index) throw runtime_error(RuntimeError::invalid_bytecode,location);
            const auto plan=real_range_plan(values,std::get<double>(state.seed->value),range.last,range.step,options);
            state.count=plan.count; range.prefix=plan.prefix; range.snap=plan.snap; range.direct=plan.direct; range.initialized=true;
        }
        return index<state.count;
    }
    if (state.provider==SequenceProvider::big_range) {
        if (!state.big_range) throw runtime_error(RuntimeError::invalid_bytecode,location);
        auto& range=*state.big_range; auto current=range.current;
        if (!range.positioned) {
            if (index) throw runtime_error(RuntimeError::invalid_bytecode,location);
            if (range.step.zero()) throw runtime_error(RuntimeError::sequence_step_zero,location);
        } else if (index==range.index) return range.available;
        else {
            if (!range.available || index<=range.index || index-range.index!=1) throw runtime_error(RuntimeError::invalid_bytecode,location);
            auto next=big_binary(Op::add,range.current,range.step,location,options);
            if (!next.shares_storage(range.current) && !next.shares_storage(range.step)) values.number(next);
            current=std::move(next);
        }
        big_work((current.bits()+range.last.bits())/28+2,location,options);
        const bool available=range.step.negative() ? current>=range.last : current<=range.last;
        range.current=std::move(current); range.available=available;
        range.positioned=true; range.index=index; return range.available;
    }
    if (state.provider==SequenceProvider::partition) {
        if (!state.partition) throw runtime_error(RuntimeError::invalid_bytecode,location);
        auto& partition=*state.partition;
        if (partition.positioned && index==partition.index) return partition.available;
        if (!partition.positioned) {
            if (index) throw runtime_error(RuntimeError::invalid_bytecode,location);
            if (!state.count) throw runtime_error(RuntimeError::partition_count,location);
            if (options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,location);
            partition.step=finite_partition(finite_partition(partition.last-partition.current,location)/static_cast<double>(state.count-1),location);
        } else {
            if (!partition.available || index<=partition.index || index-partition.index!=1) throw runtime_error(RuntimeError::invalid_bytecode,location);
            partition.current=finite_partition(partition.current+partition.step,location);
        }
        partition.index=index; partition.positioned=true; partition.available=index<state.count;
        return partition.available;
    }
    if (state.provider==SequenceProvider::stepped_range) {
        if (!state.step) throw runtime_error(RuntimeError::sequence_step_zero,location);
        // The source increments after yield. Early-return queries and break must
        // not execute this final checked increment; completing enumeration must.
        if (index>=state.count && state.exhaustion_overflow) throw runtime_error(RuntimeError::integer_overflow,location);
    }
    return index<state.count;
}
std::optional<std::uint64_t> sequence_known_length(const std::shared_ptr<Aggregate>& sequence,SourceLocation location) {
    const auto& state=cursor(sequence,location);
    if (state.provider==SequenceProvider::transform || state.provider==SequenceProvider::concat || state.provider==SequenceProvider::slice || state.provider==SequenceProvider::partial_sum || state.provider==SequenceProvider::big_range || (state.provider==SequenceProvider::real_range && !state.real_range->initialized)) return std::nullopt;
    return state.count;
}
std::uint64_t sequence_length(const std::shared_ptr<Aggregate>& sequence,SourceLocation location) {
    const auto count=sequence_known_length(sequence,location);
    if (!count) throw runtime_error(RuntimeError::invalid_bytecode,location);
    return *count;
}
Value sequence_at(Values& values,const std::shared_ptr<Aggregate>& sequence,std::int64_t index,const ExecutionOptions& options) {
    sequence_check(sequence,values.location); const auto& state=*sequence->sequence;
    if (index<0 || !sequence_resume(values,sequence,static_cast<std::uint64_t>(index),options)) throw runtime_error(RuntimeError::index_bounds,values.location);
    if (state.provider==SequenceProvider::real_range) {
        const auto& range=*state.real_range; const auto first=std::get<double>(state.seed->value);
        if (range.direct) return first;
        if (range.snap && index==range.prefix) return range.last;
        // Preserve the source's separately rounded multiply and add; no FMA.
        volatile double offset=static_cast<double>(index)*range.step;
        return finite_partition(first+finite_partition(offset,values.location),values.location);
    }
    if (state.provider==SequenceProvider::string) return std::get<std::u16string>(state.seed->value)[static_cast<std::size_t>(index)];
    if (state.provider==SequenceProvider::transform) return values.copy(state.transform->current);
    if (state.provider==SequenceProvider::concat) return values.copy(state.concat->current);
    if (state.provider==SequenceProvider::slice) return values.copy(state.slice->current);
    if (state.provider==SequenceProvider::partial_sum) return state.partial->total;
    if (state.provider==SequenceProvider::big_range) return state.big_range->current;
    if (state.provider==SequenceProvider::partition) return state.partition->current;
    if (collection_sequence(state.provider)) return values.copy(state.source->items[static_cast<std::size_t>(index)]->value);
    if (state.provider==SequenceProvider::repeat) return values.copy(state.seed->value);
    if (state.provider==SequenceProvider::range || state.provider==SequenceProvider::stepped_range) {
        const auto first=state.seed->type==Type::character ? std::int64_t(std::get<char16_t>(state.seed->value)) : std::int64_t(std::get<std::int32_t>(state.seed->value));
        const auto number=first+index*state.step;
        if (state.seed->type==Type::character) return static_cast<char16_t>(number);
        return static_cast<std::int32_t>(number);
    }
    return school_ip_sequence_value(values,type_info(values.program,sequence->type).element,state,index);
}
Value sequence_collect(Values& values,const std::shared_ptr<Aggregate>& sequence,Type collection,const ExecutionOptions& options) {
    const auto iterator=sequence_begin(values,sequence);
    (void)sequence_resume(values,iterator,0,options);
    const auto known_count=sequence_known_length(iterator,values.location);
    const bool materialize=collection!=Type::invalid;
    const bool list=materialize && type_info(values.program,collection).kind==CompoundKind::list;
    if (!materialize && collection_sequence(iterator->sequence->provider)) {
        const auto count=*known_count;
        if (count>INT32_MAX) throw runtime_error(RuntimeError::integer_overflow,values.location);
        return static_cast<std::int32_t>(count);
    }
    const auto element=type_info(values.program,sequence->type).element;
    std::shared_ptr<Aggregate> result;
    if (materialize) {
        const auto cost=iterator->sequence->provider==SequenceProvider::school_ip_hosts ? 4U : 1U;
        if (known_count && *known_count>values.remaining/cost) throw runtime_error(RuntimeError::allocation_limit,values.location);
        result=values.array(collection,0);
    }
    std::uint64_t count=0;
    for (;sequence_resume(values,iterator,count,options);++count) {
        const auto i=count;
        if (i>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
        if ((i&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
        auto value=sequence_at(values,iterator,static_cast<std::int64_t>(i),options);
        if (materialize) {
            auto item=values.cell(element,std::move(value));
            if (list) values.own_list_cell(item,result);
            result->items.push_back(std::move(item));
        }
        else if (i>=INT32_MAX) throw runtime_error(RuntimeError::integer_overflow,values.location);
    }
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
    const auto iterator=sequence_begin(values,sequence);
    if (!sequence_resume(values,iterator,0,options)) {
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
        const auto count=sequence_length(iterator,values.location);
        check();
        if (method==SequenceQuery::any) return true;
        if (single && count!=1) throw runtime_error(RuntimeError::sequence_multiple,values.location);
        const auto target=indexed ? static_cast<std::uint64_t>(index)
            : method==SequenceQuery::last || method==SequenceQuery::last_default ? count-1 : 0;
        if (target>=count) return absent();
        return sequence_at(values,iterator,static_cast<std::int64_t>(target),options);
    }
    Value last=std::int32_t{0};
    for (std::uint64_t i=0;sequence_resume(values,iterator,i,options);++i) {
        check(); auto value=sequence_at(values,iterator,static_cast<std::int64_t>(i),options);
        if (method==SequenceQuery::any) return true;
        if (method==SequenceQuery::contains) { if (values.equal(value,argument)) return true; }
        else if ((indexed && i==static_cast<std::uint64_t>(index)) || method==SequenceQuery::first || method==SequenceQuery::first_default) return value;
        else if (single) {
            if (i) throw runtime_error(RuntimeError::sequence_multiple,values.location);
            last=std::move(value);
        }
        else if (method==SequenceQuery::last || method==SequenceQuery::last_default) last=std::move(value);
    }
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
std::shared_ptr<Aggregate> sequence_range_step(Values& values,Type type,const Value& first,const Value& last,std::int32_t step) {
    const bool characters=type_info(values.program,type).element==Type::character;
    const auto a=characters ? std::int64_t(std::get<char16_t>(first)) : std::int64_t(std::get<std::int32_t>(first));
    const auto b=characters ? std::int64_t(std::get<char16_t>(last)) : std::int64_t(std::get<std::int32_t>(last));
    std::uint64_t count=0;
    if (step>0 && a<=b) count=static_cast<std::uint64_t>((b-a)/step)+1;
    if (step<0 && a>=b) count=static_cast<std::uint64_t>((a-b)/-std::int64_t(step))+1;
    auto result=sequence_repeat(values,type,0,first); auto& state=*result->sequence;
    state.provider=SequenceProvider::stepped_range; state.step=step; state.count=count;
    const auto next=a+static_cast<std::int64_t>(count)*step;
    state.exhaustion_overflow=count && (next<INT32_MIN || next>INT32_MAX);
    return result;
}
std::shared_ptr<Aggregate> sequence_range_big(Values& values,Type type,const BigInteger& first,const BigInteger& last,const BigInteger& step) {
    auto result=sequence_repeat(values,type,0,first);
    reserve_state(values); result->sequence->provider=SequenceProvider::big_range;
    result->sequence->big_range=std::make_shared<BigRangeState>(BigRangeState{last,step,first});
    return result;
}
std::shared_ptr<Aggregate> sequence_partition(Values& values,Type type,double first,double last,std::int32_t intervals) {
    auto result=sequence_repeat(values,type,0,first);
    reserve_state(values); auto& state=*result->sequence; state.provider=SequenceProvider::partition;
    state.count=intervals>0 ? static_cast<std::uint64_t>(intervals)+1 : 0;
    state.partition=std::make_shared<PartitionState>(PartitionState{last,0,first});
    return result;
}
std::shared_ptr<Aggregate> sequence_range_real(Values& values,Type type,double first,double last,double step) {
    auto result=sequence_repeat(values,type,0,first);
    reserve_state(values); result->sequence->provider=SequenceProvider::real_range;
    result->sequence->real_range=std::make_shared<RealRangeState>(RealRangeState{last,step});
    return result;
}
} // namespace abc

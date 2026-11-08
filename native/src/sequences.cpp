#include "sequences.hpp"
#include "equality.hpp"
#include "value_hash.hpp"
#include "list_operations.hpp"
#include "sequence_scope.hpp"
#include "enumerator.hpp"
#include "interfaces.hpp"
#include "termination.hpp"
#include "school_ip.hpp"
#include "runtime_values.hpp"
#include "big_integer_runtime.hpp"
#include "real_range.hpp"
#include "generators.hpp"
#include "random_sequence.hpp"
#include "directory_sequence.hpp"
#include "array_views.hpp"
#include "matrix_operations.hpp"
#include "file_lines.hpp"
#include "hash_set.hpp"
#include "dictionary.hpp"
#include "ordinal_range.hpp"
#include "new_set.hpp"
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
namespace {
struct CleanupMode {
    Values& values;bool active;
    CleanupMode(Values& v,bool enabled=true):values(v),active(enabled){if(active && values.cleanup_depth++==0 && values.cleanup_mode)values.cleanup_mode(true);}
    ~CleanupMode(){if(active && --values.cleanup_depth==0 && values.cleanup_mode)values.cleanup_mode(false);}
};
bool termination(const std::exception_ptr& error){if(!error)return false;try{std::rethrow_exception(error);}catch(const ProgramTermination&){return true;}catch(...){return false;}}
bool legacy_transform(const Values& values,const SequenceState& state){
    if(state.provider!=SequenceProvider::transform || (state.transform_method!=SequenceTransform::select && state.transform_method!=SequenceTransform::where))return false;
    return (state.callable ? type_info(values.program,state.callable->type).parameters.size() : values.program.routines.at(state.callback).parameters.size())==1;
}
void release_native(SequenceState& state) noexcept {
    if(state.file_line)file_sequence_dispose(*state.file_line);
    if(state.directory)directory_sequence_dispose(*state.directory);
    state.partial.reset();state.slice.reset();state.concat.reset();state.transform.reset();state.zip.reset();
    state.generator.reset();state.cartesian.reset();state.power.reset();state.combinator.reset();
    state.view.reset();state.matrix_line.reset();state.text_line.reset();state.hash_cursor.reset();
    state.random.reset();state.protocol_cursor.reset();
}
template<class F>void visit_children(SequenceState& state,F close){
    if(state.partial)close(state.partial->iterator);
    if(state.slice)close(state.slice->iterator);
    if(state.concat)close(state.concat->iterator);
    if(state.transform){close(state.transform->inner);close(state.transform->iterator);}
    if(state.zip)for(auto it=state.zip->iterators.rbegin();it!=state.zip->iterators.rend();++it)close(*it);
    if(state.cartesian)for(auto it=state.cartesian->iterators.rbegin();it!=state.cartesian->iterators.rend();++it)close(*it);
}
}
void sequence_dispose(const std::shared_ptr<Aggregate>& sequence) noexcept {
    if(!sequence || !sequence->sequence || !sequence->sequence->started || sequence->sequence->disposed)return;
    auto& state=*sequence->sequence;state.disposed=true;
    visit_children(state,[](auto& inner){sequence_dispose(inner);inner.reset();});
    release_native(state);
}
void sequence_dispose(Values& values,const std::shared_ptr<Aggregate>& sequence,const ExecutionOptions& options){
    if(!sequence || !sequence->sequence || !sequence->sequence->started || sequence->sequence->disposed)return;
    auto& state=*sequence->sequence;state.disposed=true;
    // Legacy unary Select/Where retain their enumerator when its Dispose throws.
    // A consuming finally then calls Dispose again; generated iterators do not.
    if(legacy_transform(values,state) && state.transform && state.transform->iterator){
        try{sequence_dispose(values,state.transform->iterator,options);}catch(...){state.disposed=false;throw;}
        release_native(state);return;
    }
    std::exception_ptr failure;
    visit_children(state,[&](auto& inner){try{sequence_finish(values,inner,options,failure);}catch(...){failure=std::current_exception();}inner.reset();});
    if(!termination(failure) && state.protocol_cursor && std::get<std::shared_ptr<Aggregate>>(state.protocol_cursor->receiver) && !std::get<std::shared_ptr<Aggregate>>(state.protocol_cursor->receiver)->nil)try{CleanupMode cleanup(values,bool(failure));enumerator_invoke(values,state.protocol_cursor->protocol,Type::invalid,EnumeratorMethod::dispose,state.protocol_cursor->receiver,options);}catch(...){failure=std::current_exception();}
    if(failure && state.protocol_cursor){state.disposed=false;std::rethrow_exception(failure);}
    release_native(state);
    if(failure)std::rethrow_exception(failure);
}
void sequence_finish(Values& values,const std::shared_ptr<Aggregate>& sequence,const ExecutionOptions& options,std::exception_ptr failure){
    if(!failure){sequence_dispose(values,sequence,options);return;}
    try{std::rethrow_exception(failure);}catch(const ProgramTermination&){sequence_dispose(sequence);throw;}catch(...){}
    CleanupMode cleanup(values);
    sequence_dispose(values,sequence,options);
    std::rethrow_exception(failure);
}

namespace {
std::shared_ptr<Aggregate> cursor_lease(std::shared_ptr<Aggregate> owner) {
    auto* pointer=owner.get();
    return std::shared_ptr<Aggregate>(pointer,[owner=std::move(owner)](Aggregate*) noexcept {sequence_dispose(owner);});
}
}
std::shared_ptr<Aggregate> sequence_begin(Values& values,const std::shared_ptr<Aggregate>& sequence,const ExecutionOptions& options) {
    require(sequence,values.location);
    if (!sequence->sequence) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    if(sequence->sequence->provider==SequenceProvider::ordinal_range)return sequence_begin(values,ordinal_range_begin(values,sequence->type,sequence->sequence->source),options);
    reserve_state(values);
    if(sequence->sequence->provider==SequenceProvider::file_lines)return cursor_lease(file_sequence_begin(values,sequence));
    if(sequence->sequence->provider==SequenceProvider::directory)return cursor_lease(directory_sequence_begin(values,sequence));
    auto result=std::make_shared<Aggregate>(); result->type=sequence->type;
    result->sequence=std::make_unique<SequenceState>(*sequence->sequence);
    auto& state=*result->sequence; state.started=true;state.disposed=false; state.first=0;
    if(state.provider==SequenceProvider::user_source){
        reserve_state(values);state.protocol_cursor=std::make_shared<ProtocolCursor>();
        state.protocol_cursor->protocol=interface_contract(values.program.types,sequence->type).at(0).result;
        state.protocol_cursor->receiver=interface_invoke(values,sequence->type,0,state.source,{},options);
        return cursor_lease(std::move(result));
    }
    if(state.provider==SequenceProvider::random){
        if(!state.random)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
        reserve_state(values);state.random=std::make_shared<RandomSequenceState>(*state.random);state.random->positioned=false;state.random->available=false;state.random->index=0;state.random->current=values.zero(type_info(values.program,sequence->type).element);return cursor_lease(std::move(result));
    }
    if(state.provider==SequenceProvider::text_lines){reserve_state(values);state.text_line=std::make_shared<TextLineState>();state.text_line->file=state.text_input;return cursor_lease(std::move(result));}
    if (state.provider==SequenceProvider::transform) { reserve_state(values); state.transform=std::make_shared<TransformState>(); return cursor_lease(std::move(result)); }
    if (state.provider==SequenceProvider::zip) { reserve_state(values); state.zip=std::make_shared<ZipState>(); return cursor_lease(std::move(result)); }
    if (state.provider==SequenceProvider::generated) { reserve_state(values); state.generator=std::make_shared<GeneratorState>(); return cursor_lease(std::move(result)); }
    if (state.provider==SequenceProvider::cartesian) { reserve_state(values); state.cartesian=std::make_shared<CartesianState>(); return cursor_lease(std::move(result)); }
    if (state.provider==SequenceProvider::cartesian_power) { reserve_state(values); state.power=std::make_shared<CartesianPowerState>(); return cursor_lease(std::move(result)); }
    if (state.provider==SequenceProvider::combinatorics) { reserve_state(values); state.combinator=std::make_shared<CombinatorState>(); return cursor_lease(std::move(result)); }
    if (state.provider==SequenceProvider::array_view) { reserve_state(values); state.view=std::make_shared<ArrayViewState>(); return cursor_lease(std::move(result)); }
    if (state.provider==SequenceProvider::matrix_lines) { reserve_state(values); state.matrix_line=std::make_shared<MatrixLineState>(); return cursor_lease(std::move(result)); }
    if (state.provider==SequenceProvider::string) {
        if (!state.seed || state.seed->type!=Type::string || !std::holds_alternative<std::u16string>(state.seed->value)) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
        return cursor_lease(std::move(result));
    }
    if (state.provider==SequenceProvider::concat) {
        reserve_state(values); state.concat=std::make_shared<ConcatState>(); return cursor_lease(std::move(result));
    }
    if (state.provider==SequenceProvider::slice) {
        reserve_state(values); state.slice=std::make_shared<SliceState>(); return cursor_lease(std::move(result));
    }
    if (state.provider==SequenceProvider::partial_sum) {
        reserve_state(values); state.partial=std::make_shared<PartialSumState>();
        state.partial->total=values.zero(type_info(values.program,sequence->type).element); return cursor_lease(std::move(result));
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
        return cursor_lease(std::move(result));
    }
    state.count=0;
    if(state.provider==SequenceProvider::new_set){if(!state.seed)throw runtime_error(RuntimeError::invalid_bytecode,values.location);state.source=new_set_hash(values,std::get<NewSetValue>(state.seed->value));}
    require(state.source,values.location);
    if(state.provider==SequenceProvider::hash_set || state.provider==SequenceProvider::new_set){reserve_state(values);state.count=state.source->count;state.version=state.source->version;state.hash_cursor=std::make_shared<HashSetCursor>();}
    else if(dictionary_provider(state.provider)){const auto owner=dictionary_sequence_source(state,values.location);reserve_state(values);state.count=owner->count;state.version=owner->version;state.hash_cursor=std::make_shared<HashSetCursor>();}
    else if (collection_sequence(state.provider)) { state.count=state.source->items.size(); state.version=state.source->version; }
    else school_ip_sequence_start(state,values.location);
    return cursor_lease(std::move(result));
}
void sequence_check(const std::shared_ptr<Aggregate>& sequence,SourceLocation location) {
    const auto& state=cursor(sequence,location);
    if(state.disposed)return;
    if(dictionary_provider(state.provider) && state.version!=dictionary_sequence_source(state,location)->version)throw runtime_error(RuntimeError::collection_changed,location);
    if ((state.provider==SequenceProvider::list || state.provider==SequenceProvider::hash_set || state.provider==SequenceProvider::new_set) && state.version!=state.source->version) throw runtime_error(RuntimeError::collection_changed,location);
}
static bool sequence_resume_impl(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options) {
    const auto location=values.location;
    sequence_check(sequence,location); auto& state=*sequence->sequence;
    if(state.disposed)return false;
    if(state.provider==SequenceProvider::user_source){
        if(!state.protocol_cursor)throw runtime_error(RuntimeError::invalid_bytecode,location);auto& c=*state.protocol_cursor;
        if(c.positioned && c.index==index)return c.available;
        if((!c.positioned && index) || (c.positioned && (!c.available || index<=c.index || index-c.index!=1)))throw runtime_error(RuntimeError::invalid_bytecode,location);
        c.available=std::get<bool>(enumerator_invoke(values,c.protocol,Type::invalid,EnumeratorMethod::move,c.receiver,options));c.index=index;c.positioned=true;return c.available;
    }
    if(state.provider==SequenceProvider::random)return random_sequence_resume(values,sequence,index,options);
    if(state.provider==SequenceProvider::directory)return directory_sequence_resume(values,sequence,index,options);
    if(dictionary_provider(state.provider))return dictionary_sequence_resume(values,sequence,index,options);
    if(state.provider==SequenceProvider::hash_set || state.provider==SequenceProvider::new_set)return hash_set_resume(values,sequence,index,options);
    if(state.provider==SequenceProvider::file_lines)return file_sequence_resume(values,sequence,index,options);
    if(state.provider==SequenceProvider::text_lines)return text_lines_resume(values,sequence,index);
    if (state.provider==SequenceProvider::partial_sum) return sequence_partial_resume(values,sequence,index,options);
    if (state.provider==SequenceProvider::slice) return sequence_slice_resume(values,sequence,index,options);
    if (state.provider==SequenceProvider::concat) return sequence_concat_resume(values,sequence,index,options);
    if (state.provider==SequenceProvider::transform) return sequence_transform_resume(values,sequence,index,options);
    if (state.provider==SequenceProvider::zip) return sequence_zip_resume(values,sequence,index,options);
    if (state.provider==SequenceProvider::generated) return generator_resume(values,sequence,index,options);
    if (state.provider==SequenceProvider::cartesian) return sequence_cartesian_resume(values,sequence,index,options);
    if (state.provider==SequenceProvider::cartesian_power) return sequence_cartesian_power_resume(values,sequence,index,options);
    if (state.provider==SequenceProvider::combinatorics) return sequence_combinatorics_resume(values,sequence,index,options);
    if (state.provider==SequenceProvider::array_view) return array_view_resume(values,sequence,index,options);
    if (state.provider==SequenceProvider::matrix_lines) return matrix_line_resume(values,sequence,index,options);
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
bool sequence_resume(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options) try {
    const auto available=sequence_resume_impl(values,sequence,index,options);
    const auto& s=*sequence->sequence;
    if(!available && (s.partial || s.slice || s.concat || s.transform || s.zip || s.cartesian))sequence_dispose(values,sequence,options);
    return available;
}catch(const Error& error){
    if(error.code=="E_CANCELLED" || error.code=="E_LIMIT" || error.code=="E_BYTECODE")throw;
    if(sequence && sequence->sequence){const auto& s=*sequence->sequence;if((s.partial || s.slice || s.concat || s.transform || s.zip || s.cartesian) && !legacy_transform(values,s))sequence_finish(values,sequence,options,std::current_exception());}
    throw;
}
std::optional<std::uint64_t> sequence_known_length(const std::shared_ptr<Aggregate>& sequence,SourceLocation location) {
    const auto& state=cursor(sequence,location);
    if(state.disposed)return std::uint64_t{0};
    if(state.provider==SequenceProvider::random || state.provider==SequenceProvider::directory || state.provider==SequenceProvider::user_source)return std::nullopt;
    if (state.provider==SequenceProvider::text_lines || state.provider==SequenceProvider::file_lines || state.provider==SequenceProvider::matrix_lines || state.provider==SequenceProvider::array_view || state.provider==SequenceProvider::combinatorics || state.provider==SequenceProvider::cartesian || state.provider==SequenceProvider::cartesian_power || state.provider==SequenceProvider::generated || state.provider==SequenceProvider::zip || state.provider==SequenceProvider::transform || state.provider==SequenceProvider::concat || state.provider==SequenceProvider::slice || state.provider==SequenceProvider::partial_sum || state.provider==SequenceProvider::big_range || (state.provider==SequenceProvider::real_range && !state.real_range->initialized)) return std::nullopt;
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
    if(state.provider==SequenceProvider::user_source){if(!state.protocol_cursor)throw runtime_error(RuntimeError::invalid_bytecode,values.location);return enumerator_invoke(values,state.protocol_cursor->protocol,Type::invalid,EnumeratorMethod::current,state.protocol_cursor->receiver,options);}
    if (state.provider==SequenceProvider::real_range) {
        const auto& range=*state.real_range; const auto first=std::get<double>(state.seed->value);
        if (range.direct) return first;
        if (range.snap && index==range.prefix) return range.last;
        // Preserve the source's separately rounded multiply and add; no FMA.
        volatile double offset=static_cast<double>(index)*range.step;
        return finite_partition(first+finite_partition(offset,values.location),values.location);
    }
    if(state.provider==SequenceProvider::hash_set || state.provider==SequenceProvider::new_set || dictionary_provider(state.provider))return values.copy(state.hash_cursor->current);
    if(state.provider==SequenceProvider::random)return values.copy(state.random->current);
    if(state.provider==SequenceProvider::directory)return directory_sequence_current(sequence);
    if(state.provider==SequenceProvider::file_lines)return state.file_line->current;
    if(state.provider==SequenceProvider::text_lines)return state.text_line->current;
    if (state.provider==SequenceProvider::string) return std::get<std::u16string>(state.seed->value)[static_cast<std::size_t>(index)];
    if (state.provider==SequenceProvider::transform) return values.copy(state.transform->current);
    if (state.provider==SequenceProvider::zip) return values.copy(state.zip->current);
    if (state.provider==SequenceProvider::generated) return values.copy(state.generator->current);
    if (state.provider==SequenceProvider::cartesian) return values.copy(state.cartesian->current);
    if (state.provider==SequenceProvider::cartesian_power) return values.copy(state.power->current);
    if (state.provider==SequenceProvider::combinatorics) return values.copy(state.combinator->current);
    if (state.provider==SequenceProvider::array_view) return values.copy(state.view->current);
    if (state.provider==SequenceProvider::matrix_lines) return values.copy(state.matrix_line->current);
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
    if(sequence && !sequence->nil && sequence->sequence && sequence->sequence->provider==SequenceProvider::ordinal_range)return ordinal_range_collect(values,sequence->sequence->source,collection,options);
    const auto iterator=sequence_begin(values,sequence,options);
    return sequence_finally(values,iterator,options,[&]() -> Value {
    if(iterator->sequence->provider!=SequenceProvider::hash_set && !dictionary_provider(iterator->sequence->provider))(void)sequence_resume(values,iterator,0,options);
    const auto known_count=sequence_known_length(iterator,values.location);
    const bool materialize=collection!=Type::invalid;
    const bool list=materialize && type_info(values.program,collection).kind==CompoundKind::list;
    if (!materialize && (collection_sequence(iterator->sequence->provider) || iterator->sequence->provider==SequenceProvider::hash_set || dictionary_provider(iterator->sequence->provider))) {
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
        if (list && known_count && (collection_sequence(iterator->sequence->provider) || iterator->sequence->provider==SequenceProvider::hash_set || dictionary_provider(iterator->sequence->provider))) values.set_list_capacity(result,static_cast<std::size_t>(*known_count));
    }
    std::uint64_t count=0;
    for (;sequence_resume(values,iterator,count,options);++count) {
        const auto i=count;
        if (i>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
        if ((i&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
        if (materialize) {
            auto value=sequence_at(values,iterator,static_cast<std::int64_t>(i),options);
            auto item=values.cell(element,std::move(value));
            if (list) { values.grow_list(result,result->items.size()+1);values.own_list_cell(item,result); }
            if(list)values.list_append_cell(result,std::move(item));else result->items.push_back(std::move(item));
        }
        // Native providers may defer producing a yield value until sequence_at.
        // A user IEnumerator Count must call MoveNext without requesting Current.
        else {if(iterator->sequence->provider!=SequenceProvider::user_source)(void)sequence_at(values,iterator,static_cast<std::int64_t>(i),options);if(i>=INT32_MAX)throw runtime_error(RuntimeError::integer_overflow,values.location);}
    }
    return materialize ? Value{result} : Value{static_cast<std::int32_t>(count)};
    });
}
bool sequence_query_signature_valid(const Program& p,Type owner,std::size_t operand){
 if(!compound(owner) || !valid_value_type(p,owner) || type_info(p,owner).kind!=CompoundKind::sequence)return false;const auto method=static_cast<SequenceQuery>(operand&255U);const auto raw=operand>>8U;if(method>SequenceQuery::contains_comparer)return false;
 if(method!=SequenceQuery::contains_comparer)return raw==0;
 if(raw<64 || raw>=64+p.types.size())return false;const auto payload=static_cast<Type>(raw);return type_info(p,payload).kind==CompoundKind::equality_comparer_interface && type_info(p,payload).element==type_info(p,owner).element;
}
Value sequence_query(Values& values,const std::shared_ptr<Aggregate>& sequence,SequenceQuery method,const Value& argument,const ExecutionOptions& options,const std::shared_ptr<Aggregate>& comparer) {
    require(sequence,values.location);
    const auto element=type_info(values.program,sequence->type).element;
    if(method==SequenceQuery::contains && sequence->sequence){
        const auto& state=*sequence->sequence;const auto source=state.source;
        if(state.provider==SequenceProvider::array && source && type_info(values.program,source->type).kind==CompoundKind::array){require(source,values.location);ValueHashWork work{values,options};for(const auto& cell:source->items){if(value_equal(work,cell->value,argument))return true;}return false;}
        if(state.provider==SequenceProvider::list && source && type_info(values.program,source->type).element==element)return list_contains(values,source,argument,options);
        if(state.provider==SequenceProvider::hash_set && source && type_info(values.program,source->type).element==element){require(source,values.location);const std::array<Value,2> args{source,argument};return hash_set_invoke(values,source->type,Type::invalid,HashSetMethod::contains,args,options);}
        if(dictionary_provider(state.provider) && source && type_info(values.program,source->type).element==element){require(source,values.location);const std::array<Value,2> args{source,argument};return dictionary_invoke(values,source->type,Type::invalid,state.provider==SequenceProvider::dictionary_entries ? DictionaryMethod::pair_contains : DictionaryMethod::view_contains,args,options);}
    }
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
    const auto iterator=sequence_begin(values,sequence,options);
    return sequence_finally(values,iterator,options,[&]() -> Value {
    if (!sequence_resume(values,iterator,0,options)) {
        if (method==SequenceQuery::any || sequence_query_contains(method)) return false;
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
    if (collection_sequence(iterator->sequence->provider) && !sequence_query_contains(method)) {
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
        check();
        if (method==SequenceQuery::any){if(iterator->sequence->provider!=SequenceProvider::user_source)(void)sequence_at(values,iterator,static_cast<std::int64_t>(i),options);return true;}
        if(indexed && i!=static_cast<std::uint64_t>(index)){if(iterator->sequence->provider!=SequenceProvider::user_source)(void)sequence_at(values,iterator,static_cast<std::int64_t>(i),options);continue;}
        if(single && i){if(iterator->sequence->provider!=SequenceProvider::user_source)(void)sequence_at(values,iterator,static_cast<std::int64_t>(i),options);throw runtime_error(RuntimeError::sequence_multiple,values.location);}
        auto value=sequence_at(values,iterator,static_cast<std::int64_t>(i),options);
        if (sequence_query_contains(method)) { if (equality_policy_value(values,element,comparer,value,argument,options)) return true; }
        else if ((indexed && i==static_cast<std::uint64_t>(index)) || method==SequenceQuery::first || method==SequenceQuery::first_default) return value;
        else if (single) {
            if (i) throw runtime_error(RuntimeError::sequence_multiple,values.location);
            last=std::move(value);
        }
        else if (method==SequenceQuery::last || method==SequenceQuery::last_default) last=std::move(value);
    }
    if (sequence_query_contains(method)) return false;
    if (method==SequenceQuery::last || method==SequenceQuery::last_default || single) return last;
    return absent();
    });
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

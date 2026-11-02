#include "sequences.hpp"
#include "sequence_guard.hpp"
#include <algorithm>
#include <numeric>
#include <span>

namespace abc {
namespace {
void work(Values& values,std::uint64_t& count,const ExecutionOptions& options) {
    if (count>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
    if ((count&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
    ++count;
}
template<class Read,class Swap,class Tick>
bool advance_permutation(std::size_t size,Read read,Swap swap,Tick tick,SourceLocation location) {
    // Source accesses a[-2] for an empty array; completion must retain that error.
    if (!size) throw runtime_error(RuntimeError::index_bounds,location);
    auto j=size-1;
    while (j) { tick(); if (read(j-1)<read(j)) break; --j; }
    if (!j) return false; // Source leaves the last permutation unchanged.
    auto k=size-1;
    while (read(j-1)>=read(k)) { tick(); --k; }
    tick(); swap(j-1,k);
    for (auto left=j,right=size-1;left<right;++left,--right) { tick(); swap(left,right); }
    return true;
}
void indices(Values& values,std::vector<std::size_t>& target,std::size_t size,std::uint64_t& used,const ExecutionOptions& options) {
    if (size>values.remaining) throw runtime_error(RuntimeError::allocation_limit,values.location);
    values.remaining-=size; target.resize(size);
    for (std::size_t i=0;i<size;++i) { work(values,used,options); target[i]=i; }
}
bool permutation(Values& values,std::vector<std::size_t>& order,std::uint64_t& used,const ExecutionOptions& options) {
    return advance_permutation(order.size(),[&](std::size_t i){return order[i];},
        [&](std::size_t a,std::size_t b){std::swap(order[a],order[b]);},[&]{work(values,used,options);},values.location);
}
bool combination(Values& values,CombinatorState& cursor,const ExecutionOptions& options) {
    auto i=cursor.width;
    while (i) {
        work(values,cursor.work,options); --i;
        if (cursor.indices[i]<cursor.source_count-cursor.width+i) {
            ++cursor.indices[i];
            // Retain the source helper's extra lookahead index at width.
            for (auto j=i;j<cursor.width;++j) { work(values,cursor.work,options); cursor.indices[j+1]=cursor.indices[j]+1; }
            return true;
        }
    }
    return false;
}
std::shared_ptr<Aggregate> copy_array(Values& values,Type type,const std::shared_ptr<Aggregate>& source,std::uint64_t& used,const ExecutionOptions& options) {
    auto result=values.array(type,source->items.size());
    for (std::size_t i=0;i<source->items.size();++i) { work(values,used,options); values.write(result->items[i],source->items[i]->value); }
    return result;
}
void select(Values& values,const SequenceState& state,CombinatorState& cursor,const std::shared_ptr<Aggregate>& target,const ExecutionOptions& options) {
    for (std::size_t i=0;i<cursor.width;++i) {
        work(values,cursor.work,options);
        if (i>=cursor.indices.size() || cursor.indices[i]>=cursor.source_count) throw runtime_error(RuntimeError::index_bounds,values.location);
        if (state.seed) target->items[i]->value=std::get<std::u16string>(state.seed->value)[cursor.indices[i]];
        else values.write(target->items[i],state.source->items[cursor.indices[i]]->value);
    }
}
}
bool next_permutation(Values& values,const std::shared_ptr<Aggregate>& array,const ExecutionOptions& options) {
    if (!array || array->nil) throw runtime_error(RuntimeError::nil_reference,values.location);
    std::uint64_t used=0;
    return advance_permutation(array->items.size(),[&](std::size_t i){return std::get<std::int32_t>(array->items[i]->value);},
        [&](std::size_t a,std::size_t b){std::swap(array->items[a]->value,array->items[b]->value);},
        [&]{work(values,used,options);},values.location);
}
std::shared_ptr<Aggregate> sequence_combinatorics(Values& values,Type type,Type array_type,Type source_type,Value source,std::int32_t count,CombinatorMethod method,const ExecutionOptions& options) {
    auto result=sequence_cartesian_power(values,type,array_type,source_type,std::move(source),count,options);
    result->sequence->provider=SequenceProvider::combinatorics; result->sequence->combinator_method=method; return result;
}
bool sequence_combinatorics_resume(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options) {
    SequenceDepthGuard guard(values,options); const auto& state=*sequence->sequence;
    if (!state.combinator) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    auto& cursor=*state.combinator;
    if (cursor.positioned && index==cursor.index) return cursor.available;
    if ((!cursor.positioned && index) || (cursor.positioned && (!cursor.available || index<=cursor.index || index-cursor.index!=1)))
        throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    if (index>static_cast<std::uint64_t>(INT64_MAX)) throw runtime_error(RuntimeError::integer_overflow,values.location);
    work(values,cursor.work,options);
    const bool full=state.combinator_method==CombinatorMethod::permutations;
    const bool partial=state.combinator_method==CombinatorMethod::partial_permutations;
    const auto finish=[&](bool available){cursor.positioned=true;cursor.available=available;cursor.index=index;return available;};
    const auto source_count=[&] {
        if (!state.seed && (!state.source || state.source->nil)) throw runtime_error(RuntimeError::nil_reference,values.location);
        return state.seed ? std::get<std::u16string>(state.seed->value).size() : state.source->items.size();
    };
    const auto begin_selection=[&] {
        select(values,state,cursor,cursor.combination,options);
        cursor.selected=copy_array(values,state.power_array_type,cursor.combination,cursor.work,options);
        cursor.result=values.array(state.power_array_type,cursor.width);
        indices(values,cursor.order,cursor.width,cursor.work,options);
    };
    if (!cursor.positioned) {
        if (full) {
            cursor.source_count=source_count(); cursor.width=cursor.source_count;
            cursor.result=values.array(state.power_array_type,cursor.width);
        } else {
            if (state.power_count<0) throw runtime_error(RuntimeError::array_length,values.location);
            cursor.width=static_cast<std::size_t>(state.power_count);
            // Combinations allocates its result buffer before reading Self.Length.
            cursor.combination=values.array(state.power_array_type,cursor.width);
            cursor.source_count=source_count();
        }
        indices(values,cursor.indices,cursor.source_count,cursor.work,options);
        if (partial) begin_selection();
    } else {
        if (full) {
            if (!permutation(values,cursor.indices,cursor.work,options)) return finish(false);
        } else if (partial) {
            if (!permutation(values,cursor.order,cursor.work,options)) {
                if (!combination(values,cursor,options)) return finish(false);
                begin_selection();
            }
        } else if (!combination(values,cursor,options)) return finish(false);
    }
    if (partial) {
        for (std::size_t i=0;i<cursor.width;++i) {
            work(values,cursor.work,options); values.write(cursor.result->items[i],cursor.selected->items[cursor.order[i]]->value);
        }
    } else select(values,state,cursor,full ? cursor.result : cursor.combination,options);
    auto result=copy_array(values,state.power_array_type,full || partial ? cursor.result : cursor.combination,cursor.work,options);
    if (state.seed) {
        if (cursor.width>1'048'576) throw runtime_error(RuntimeError::string_limit,values.location);
        if (cursor.width>values.remaining) throw runtime_error(RuntimeError::allocation_limit,values.location);
        values.remaining-=cursor.width; std::u16string text(cursor.width,char16_t{0});
        for (std::size_t i=0;i<cursor.width;++i) { work(values,cursor.work,options); text[i]=std::get<char16_t>(result->items[i]->value); }
        cursor.current=std::move(text);
    } else cursor.current=std::move(result);
    return finish(true);
}
}

#include "sequences.hpp"
#include "sequence_guard.hpp"
#include <algorithm>

namespace abc {
namespace {
void work(Values& values,std::uint64_t& count,const ExecutionOptions& options) {
    if (count>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
    if ((count&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
    ++count;
}
template<class Cursor> bool positioned(Cursor& cursor,std::uint64_t index,SourceLocation location) {
    if ((!cursor.positioned && index) || (cursor.positioned && index!=cursor.index
        && (!cursor.available || index<=cursor.index || index-cursor.index!=1)))
        throw runtime_error(RuntimeError::invalid_bytecode,location);
    if (index>static_cast<std::uint64_t>(INT64_MAX)) throw runtime_error(RuntimeError::integer_overflow,location);
    return cursor.positioned && index==cursor.index;
}
template<class Cursor> bool finish(Cursor& cursor,std::uint64_t index,bool available) {
    cursor.index=index; cursor.positioned=true; cursor.available=available; return available;
}
}
std::shared_ptr<Aggregate> sequence_cartesian(Values& values,Type type,std::vector<std::shared_ptr<Aggregate>> sources,std::size_t callback,const ExecutionOptions& options,std::shared_ptr<Aggregate> callable) {
    if (sources.size()<2 || sources.size()>5) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    std::size_t nesting=0;
    for (const auto& source:sources) if (source && source->sequence) nesting=std::max(nesting,source->sequence->nesting);
    if (nesting>=options.call_limit) throw runtime_error(RuntimeError::sequence_depth,values.location);
    auto result=sequence_composed(values,type,sources.front(),SequenceProvider::cartesian,options);
    auto& state=*result->sequence; state.nesting=nesting+1; state.sources=std::move(sources);
    state.callback=callback; state.callable=std::move(callable); return result;
}
bool sequence_cartesian_resume(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options) {
    SequenceDepthGuard guard(values,options); const auto& state=*sequence->sequence;
    if (!state.cartesian || state.sources.size()<2 || state.sources.size()>5) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    auto& cursor=*state.cartesian;
    if (positioned(cursor,index,values.location)) return cursor.available;
    if (!cursor.initialized) {
        // The source iterator checks its function before opening the outer source.
        if (state.callable) (void)values.callable_target(state.callable);
        cursor.initialized=true;
    }
    const auto count=state.sources.size();
    for (;;) {
        work(values,cursor.work,options);
        const auto depth=cursor.depth;
        if (!cursor.iterators[depth]) {
            cursor.iterators[depth]=sequence_begin(values,state.sources[depth]);
            cursor.indices[depth]=0;
        }
        const auto source_index=cursor.indices[depth];
        if (!sequence_resume(values,cursor.iterators[depth],source_index,options)) {
            cursor.iterators[depth].reset();
            if (!depth) return finish(cursor,index,false);
            --cursor.depth; continue;
        }
        // Each nested foreach saves Current before entering the next loop.
        cursor.values[depth]=sequence_at(values,cursor.iterators[depth],static_cast<std::int64_t>(source_index),options);
        ++cursor.indices[depth];
        if (depth+1<count) { ++cursor.depth; cursor.iterators[depth+1].reset(); continue; }
        if (state.callable) cursor.current=values.invoke_value(state.callable,std::span<const Value>(cursor.values).first(count));
        else if (state.callback) {
            if (!values.invoke || state.callback-1>=values.program.routines.size()) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
            cursor.current=values.invoke(state.callback-1,std::span<const Value>(cursor.values).first(count));
        } else {
            const auto type=type_info(values.program,sequence->type).element;
            const auto& fields=type_info(values.program,type).fields;
            auto tuple=std::make_shared<Aggregate>(); tuple->type=type;
            for (std::size_t n=0;n<count;++n) tuple->items.push_back(values.cell(fields[n].type,values.copy(cursor.values[n])));
            cursor.current=std::move(tuple);
        }
        return finish(cursor,index,true);
    }
}
std::shared_ptr<Aggregate> sequence_cartesian_power(Values& values,Type type,Type array_type,Type source_type,Value source,std::int32_t count,const ExecutionOptions& options) {
    std::shared_ptr<Aggregate> array;
    const bool text=source_type==Type::string;
    if (text) {
        if (std::holds_alternative<NilString>(source)) throw runtime_error(RuntimeError::nil_reference,values.location);
        const auto size=std::get<std::u16string>(source).size();
        if (size>values.remaining) throw runtime_error(RuntimeError::allocation_limit,values.location);
        values.remaining-=size; // Source ToCharArray is eager even when Take(0) follows.
    } else {
        array=std::get<std::shared_ptr<Aggregate>>(source);
        if (type_info(values.program,source_type).kind==CompoundKind::sequence)
            array=std::get<std::shared_ptr<Aggregate>>(sequence_collect(values,array,array_type,options));
    }
    auto result=sequence_composed(values,type,array,SequenceProvider::cartesian_power,options);
    auto& state=*result->sequence; state.power_count=count; state.power_array_type=array_type;
    if (text) state.seed=values.cell(Type::string,std::move(source));
    return result;
}
bool sequence_cartesian_power_resume(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options) {
    SequenceDepthGuard guard(values,options); const auto& state=*sequence->sequence;
    if (!state.power) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    auto& cursor=*state.power;
    if (positioned(cursor,index,values.location)) return cursor.available;
    work(values,cursor.work,options);
    const bool text=bool(state.seed);
    const auto count=state.power_count;
    if (!cursor.positioned) {
        if (count<0) throw runtime_error(RuntimeError::cartesian_power_count,values.location);
        if (!text && (!state.source || state.source->nil)) throw runtime_error(RuntimeError::nil_reference,values.location);
        cursor.source_count=text ? std::get<std::u16string>(state.seed->value).size() : state.source->items.size();
        // Length is read before n=0, as in the original iterator.
        if (count && !cursor.source_count) return finish(cursor,index,false);
        const auto size=static_cast<std::size_t>(count);
        if (size>values.remaining) throw runtime_error(RuntimeError::allocation_limit,values.location);
        values.remaining-=size; cursor.digits.assign(size,0);
    } else {
        if (!count) return finish(cursor,index,false);
        auto position=cursor.digits.size();
        for (;;) {
            work(values,cursor.work,options);
            if (!position) return finish(cursor,index,false);
            auto& digit=cursor.digits[--position];
            if (++digit<cursor.source_count) break;
            digit=0;
        }
    }
    const auto size=static_cast<std::size_t>(count);
    if (text) {
        if (size>1'048'576) throw runtime_error(RuntimeError::string_limit,values.location);
        if (size>values.remaining) throw runtime_error(RuntimeError::allocation_limit,values.location);
        values.remaining-=size;
        std::u16string result(size,char16_t{0}); const auto& source=std::get<std::u16string>(state.seed->value);
        for (std::size_t n=0;n<size;++n) { work(values,cursor.work,options); result[n]=source[cursor.digits[n]]; }
        cursor.current=std::move(result);
    } else {
        auto result=values.array(state.power_array_type,size);
        for (std::size_t n=0;n<size;++n) {
            work(values,cursor.work,options); values.write(result->items[n],state.source->items[cursor.digits[n]]->value);
        }
        cursor.current=std::move(result);
    }
    return finish(cursor,index,true);
}
}

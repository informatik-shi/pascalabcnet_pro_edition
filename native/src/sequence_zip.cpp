#include "sequences.hpp"
#include "sequence_guard.hpp"
#include <array>
#include <algorithm>

namespace abc {
std::shared_ptr<Aggregate> sequence_zip(Values& values,Type type,std::vector<std::shared_ptr<Aggregate>> sources,std::size_t callback,const ExecutionOptions& options) {
    if (sources.size()<2 || sources.size()>5) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    std::size_t nesting=0;
    for (const auto& source:sources) if (source && source->sequence) nesting=std::max(nesting,source->sequence->nesting);
    if (nesting>=options.call_limit) throw runtime_error(RuntimeError::sequence_depth,values.location);
    auto result=sequence_composed(values,type,sources.front(),SequenceProvider::zip,options);
    result->sequence->nesting=nesting+1; result->sequence->sources=std::move(sources); result->sequence->callback=callback;
    return result;
}
bool sequence_zip_resume(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options) {
    SequenceDepthGuard guard(values,options);
    const auto& state=*sequence->sequence;
    if (!state.zip) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    auto& cursor=*state.zip;
    if (cursor.positioned && index==cursor.index) return cursor.available;
    if ((!cursor.positioned && index) || (cursor.positioned && (!cursor.available || index<=cursor.index || index-cursor.index!=1)))
        throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    if (index>static_cast<std::uint64_t>(INT64_MAX)) throw runtime_error(RuntimeError::integer_overflow,values.location);
    if (index>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
    if ((index&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
    // Pascal's iterator obtains every enumerator before the first MoveNext.
    if (cursor.iterators.empty())
        for (const auto& source:state.sources) cursor.iterators.push_back(sequence_begin(values,source));
    std::array<Value,5> arguments{};
    for (std::size_t i=0;i<cursor.iterators.size();++i) {
        const auto& iterator=cursor.iterators[i];
        if (!sequence_resume(values,iterator,index,options)) {
            cursor.positioned=true; cursor.index=index; cursor.available=false; return false;
        }
        // List<T>.Enumerator caches Current during MoveNext. Array enumerators
        // read their element when Current is requested after all MoveNext calls.
        if (iterator->sequence->provider==SequenceProvider::list)
            arguments[i]=sequence_at(values,iterator,static_cast<std::int64_t>(index),options);
    }
    for (std::size_t i=0;i<cursor.iterators.size();++i)
        if (cursor.iterators[i]->sequence->provider!=SequenceProvider::list)
            arguments[i]=sequence_at(values,cursor.iterators[i],static_cast<std::int64_t>(index),options);
    const auto count=cursor.iterators.size();
    if (state.callback) {
        if (!values.invoke || state.callback-1>=values.program.routines.size()) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
        cursor.current=values.invoke(state.callback-1,std::span<const Value>(arguments).first(count));
    } else {
        const auto type=type_info(values.program,sequence->type).element;
        const auto& fields=type_info(values.program,type).fields;
        auto tuple=std::make_shared<Aggregate>(); tuple->type=type;
        for (std::size_t i=0;i<count;++i) tuple->items.push_back(values.cell(fields[i].type,values.copy(arguments[i])));
        cursor.current=std::move(tuple);
    }
    cursor.positioned=true; cursor.index=index; cursor.available=true; return true;
}
}

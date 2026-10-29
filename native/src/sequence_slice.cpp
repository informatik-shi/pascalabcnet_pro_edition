#include "sequences.hpp"
#include "sequence_guard.hpp"
#include <utility>

namespace abc {
std::shared_ptr<Aggregate> sequence_slice(Values& values,Type type,const std::shared_ptr<Aggregate>& source,SequenceSlice method,std::int32_t count,const ExecutionOptions& options) {
    // Enumerable.Take/Skip validate source at the call; the Pascal tail
    // iterators defer that access, and TakeLast(n<=0) never opens it.
    if ((method==SequenceSlice::take || method==SequenceSlice::skip) && (!source || source->nil)) throw runtime_error(RuntimeError::nil_reference,values.location);
    auto result=sequence_composed(values,type,source,SequenceProvider::slice,options);
    result->sequence->slice_method=method; result->sequence->slice_count=count;
    return result;
}
bool sequence_slice_resume(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options) {
    SequenceDepthGuard guard(values,options);
    const auto& state=*sequence->sequence;
    if (!state.slice) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    auto& slice=*state.slice;
    if (slice.positioned && index==slice.index) return slice.available;
    if (!slice.positioned) {
        if (index) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    } else if (!slice.available || index<=slice.index || index-slice.index!=1) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    const auto method=state.slice_method;
    const auto count=state.slice_count>0 ? static_cast<std::uint32_t>(state.slice_count) : 0U;
    const auto finish=[&](bool available) {
        slice.positioned=true; slice.index=index; slice.available=available; return available;
    };
    // Take stops before requesting the next source value, including its checked
    // post-yield arithmetic and version check. Empty TakeLast also avoids nil.
    if ((method==SequenceSlice::take && index>=count) || (method==SequenceSlice::take_last && !count)) return finish(false);
    const auto element=type_info(values.program,sequence->type).element;
    const auto pull=[&](Value& item) {
        if (slice.ended) return false;
        if (slice.work>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
        if ((slice.work++&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
        if (!slice.iterator) slice.iterator=sequence_begin(values,state.source);
        if (slice.source_index>static_cast<std::uint64_t>(INT64_MAX)) throw runtime_error(RuntimeError::integer_overflow,values.location);
        if (!sequence_resume(values,slice.iterator,slice.source_index,options)) { slice.ended=true; return false; }
        item=sequence_at(values,slice.iterator,static_cast<std::int64_t>(slice.source_index),options);
        ++slice.source_index; return true;
    };
    const auto enqueue=[&](Value item) {
        auto cell=std::move(slice.spare);
        if (cell) values.write(cell,item);
        else cell=values.cell(element,std::move(item));
        slice.buffer.push_back(std::move(cell));
    };
    if (method==SequenceSlice::take) return finish(pull(slice.current));
    if (method==SequenceSlice::skip) {
        if (!slice.positioned) {
            Value ignored;
            for (std::uint32_t skipped=0;skipped<count;++skipped) if (!pull(ignored)) return finish(false);
        }
        return finish(pull(slice.current));
    }
    if (method==SequenceSlice::take_last) {
        if (!slice.buffered) {
            Value item;
            while (pull(item)) {
                if (slice.buffer.size()==count) { slice.spare=std::move(slice.buffer.front()); slice.buffer.pop_front(); }
                enqueue(std::move(item));
            }
            slice.buffered=true;
        }
        if (slice.buffer.empty()) return finish(false);
        slice.current=slice.buffer.front()->value; slice.buffer.pop_front();
        return finish(true);
    }
    if (method==SequenceSlice::skip_last) {
        if (!count) return finish(pull(slice.current));
        Value item;
        while (pull(item)) {
            enqueue(std::move(item));
            if (slice.buffer.size()>count) {
                slice.spare=std::move(slice.buffer.front()); slice.buffer.pop_front();
                slice.current=slice.spare->value; return finish(true);
            }
        }
        return finish(false);
    }
    throw runtime_error(RuntimeError::invalid_bytecode,values.location);
}
} // namespace abc

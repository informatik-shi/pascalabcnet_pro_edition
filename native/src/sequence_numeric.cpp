#include "sequences.hpp"
#include "runtime_values.hpp"
#include "big_integer_runtime.hpp"
#include "sequence_guard.hpp"
#include <cmath>
#include <limits>

namespace abc {
namespace {
std::int64_t add64(std::int64_t a,std::int64_t b,SourceLocation location) {
    if ((b>0 && a>INT64_MAX-b) || (b<0 && a<INT64_MIN-b)) throw runtime_error(RuntimeError::integer_overflow,location);
    return a+b;
}
std::int64_t multiply64(std::int64_t a,std::int64_t b,SourceLocation location) {
    const auto magnitude=[](std::int64_t n){return n<0 ? std::uint64_t{0}-static_cast<std::uint64_t>(n) : static_cast<std::uint64_t>(n);};
    const bool negative=(a<0)!=(b<0); const auto left=magnitude(a),right=magnitude(b);
    const auto limit=negative ? std::uint64_t{1}<<63 : static_cast<std::uint64_t>(INT64_MAX);
    if (right && left>limit/right) throw runtime_error(RuntimeError::integer_overflow,location);
    const auto product=left*right;
    if (negative && product==(std::uint64_t{1}<<63)) return INT64_MIN;
    return negative ? -static_cast<std::int64_t>(product) : static_cast<std::int64_t>(product);
}
double finite(double n,SourceLocation location) {
    if (!std::isfinite(n)) throw runtime_error(RuntimeError::real_overflow,location);
    return n;
}
}
Value sequence_numeric_step(Values& values,Type type,const Value& left,const Value& right,bool product,const ExecutionOptions& options) {
    if (type==Type::big_integer) {
        const auto& a=std::get<BigInteger>(left); const auto& b=std::get<BigInteger>(right);
        auto result=big_binary(product ? Op::multiply : Op::add,a,b,values.location,options);
        if (!result.shares_storage(a) && !result.shares_storage(b)) values.number(result);
        return result;
    }
    if (type==Type::real) {
        const auto a=std::get<double>(left),b=std::get<double>(right);
        return finite(product ? a*b : a+b,values.location);
    }
    if (type==Type::int64) {
        const auto a=std::get<std::int64_t>(left),b=std::get<std::int64_t>(right);
        return product ? multiply64(a,b,values.location) : add64(a,b,values.location);
    }
    if (type==Type::integer && !product) {
        const auto sum=std::int64_t(std::get<std::int32_t>(left))+std::get<std::int32_t>(right);
        if (sum<INT32_MIN || sum>INT32_MAX) throw runtime_error(RuntimeError::integer_overflow,values.location);
        return static_cast<std::int32_t>(sum);
    }
    throw runtime_error(RuntimeError::invalid_bytecode,values.location);
}
Value sequence_aggregate(Values& values,const std::shared_ptr<Aggregate>& sequence,SequenceAggregate method,const ExecutionOptions& options) {
    const auto iterator=sequence_begin(values,sequence);
    const auto element=type_info(values.program,sequence->type).element;
    const bool average=method==SequenceAggregate::average,product=method==SequenceAggregate::product;
    const auto accumulator=(average || product) && element==Type::integer ? Type::int64 : element;
    auto total=values.zero(accumulator);
    if (product) {
        if (accumulator==Type::big_integer) { total=checked_big(values.location,[]{return BigInteger::from_signed(1);}); values.number(total); }
        else if (accumulator==Type::real) total=1.0;
        else total=std::int64_t{1};
    }
    std::uint64_t count=0;
    for (;sequence_resume(values,iterator,count,options);++count) {
        if (count>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
        if ((count&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
        if (count>static_cast<std::uint64_t>(INT64_MAX)) throw runtime_error(RuntimeError::integer_overflow,values.location);
        auto item=sequence_at(values,iterator,static_cast<std::int64_t>(count),options);
        if (accumulator==Type::int64 && element==Type::integer) item=std::int64_t(std::get<std::int32_t>(item));
        total=sequence_numeric_step(values,accumulator,total,item,product,options);
        if (average && count>=static_cast<std::uint64_t>(element==Type::big_integer ? INT32_MAX : INT64_MAX)) throw runtime_error(RuntimeError::integer_overflow,values.location);
    }
    if (!average) return total;
    if (!count) {
        if (element==Type::big_integer) return 0.0;
        throw runtime_error(RuntimeError::sequence_empty,values.location);
    }
    const auto sum=element==Type::big_integer ? std::get<double>(big_cast(total,Type::real,values.location))
        : accumulator==Type::real ? std::get<double>(total) : static_cast<double>(std::get<std::int64_t>(total));
    return finite(sum/static_cast<double>(count),values.location);
}
std::shared_ptr<Aggregate> sequence_partial(Values& values,Type type,const std::shared_ptr<Aggregate>& source,const ExecutionOptions& options) {
    return sequence_composed(values,type,source,SequenceProvider::partial_sum,options);
}
bool sequence_partial_resume(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options) {
    SequenceDepthGuard guard(values,options);
    auto& state=*sequence->sequence;
    if (!state.partial) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    auto& partial=*state.partial;
    if (partial.positioned && index==partial.index) return partial.available;
    if (!partial.positioned) {
        if (index) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
        partial.iterator=sequence_begin(values,state.source);
    } else if (!partial.available || index<=partial.index || index-partial.index!=1) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    if (index>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
    if ((index&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
    const bool available=sequence_resume(values,partial.iterator,index,options);
    if (available) {
        if (index>static_cast<std::uint64_t>(INT64_MAX)) throw runtime_error(RuntimeError::integer_overflow,values.location);
        const auto item=sequence_at(values,partial.iterator,static_cast<std::int64_t>(index),options);
        partial.total=sequence_numeric_step(values,type_info(values.program,sequence->type).element,partial.total,item,false,options);
    }
    partial.index=index; partial.available=available; partial.positioned=true;
    return available;
}
} // namespace abc

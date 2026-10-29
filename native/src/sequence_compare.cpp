#include "sequences.hpp"
#include "runtime_values.hpp"

namespace abc {
bool sequence_equal(Values& values,const std::shared_ptr<Aggregate>& first,const std::shared_ptr<Aggregate>& second,const ExecutionOptions& options) {
    if (!first || first->nil || !second || second->nil) throw runtime_error(RuntimeError::nil_reference,values.location);
    const auto left=sequence_begin(values,first),right=sequence_begin(values,second);
    for (std::uint64_t index=0;;++index) {
        if (index>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
        if ((index&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
        if (index>static_cast<std::uint64_t>(INT64_MAX)) throw runtime_error(RuntimeError::integer_overflow,values.location);
        if (!sequence_resume(values,left,index,options)) return !sequence_resume(values,right,index,options);
        if (!sequence_resume(values,right,index,options)) return false;
        const auto a=sequence_at(values,left,static_cast<std::int64_t>(index),options);
        const auto b=sequence_at(values,right,static_cast<std::int64_t>(index),options);
        if (!values.equal(a,b)) return false;
    }
}
} // namespace abc

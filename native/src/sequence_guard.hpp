#pragma once
#include "runtime_values.hpp"

namespace abc {
class SequenceDepthGuard {
    Values& values_;
public:
    SequenceDepthGuard(Values& values,const ExecutionOptions& options):values_(values) {
        if (values_.sequence_depth>=options.call_limit) throw runtime_error(RuntimeError::sequence_depth,values_.location);
        ++values_.sequence_depth;
    }
    ~SequenceDepthGuard(){--values_.sequence_depth;}
    SequenceDepthGuard(const SequenceDepthGuard&)=delete;
    SequenceDepthGuard& operator=(const SequenceDepthGuard&)=delete;
};
inline std::shared_ptr<Aggregate> sequence_composed(Values& values,Type type,const std::shared_ptr<Aggregate>& source,SequenceProvider provider,const ExecutionOptions& options) {
    const auto nesting=source && source->sequence ? source->sequence->nesting : 0;
    if (nesting>=options.call_limit) throw runtime_error(RuntimeError::sequence_depth,values.location);
    if (!values.remaining) throw runtime_error(RuntimeError::allocation_limit,values.location);
    --values.remaining;
    auto result=std::make_shared<Aggregate>(); result->type=type; result->sequence=std::make_unique<SequenceState>();
    result->sequence->provider=provider; result->sequence->source=source; result->sequence->nesting=nesting+1;
    return result;
}
} // namespace abc

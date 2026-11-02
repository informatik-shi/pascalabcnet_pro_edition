#pragma once
#include "sequences.hpp"
#include "runtime_values.hpp"

namespace abc {
bool array_view_signature_valid(const Program& program,Type source,Type result,Type callback,ArrayViewMethod method);
std::shared_ptr<Aggregate> array_view(Values& values,Type type,const std::shared_ptr<Aggregate>& source,ArrayViewMethod method,const ExecutionOptions& options,std::shared_ptr<Aggregate> callable={});
bool array_view_resume(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options);
}

#pragma once
#include "runtime_values.hpp"

namespace abc {
bool collection_signature_valid(const Program& program,Type source,Type result,Type callback,CollectionOperation method);
Value collection_operation(Values& values,Type type,const std::shared_ptr<Aggregate>& source,CollectionOperation method,const Value& argument,const ExecutionOptions& options);
}

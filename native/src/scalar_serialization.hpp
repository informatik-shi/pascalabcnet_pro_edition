#pragma once

#include "binary_stream.hpp"

namespace abc {
struct SerializedScalar {
    Type type;
    Value value;
};

void serialize_scalar(BinaryDevice&,Type,const Value&,SourceLocation,const ExecutionOptions&);
SerializedScalar deserialize_scalar(BinaryDevice&,SourceLocation,const ExecutionOptions&);
}

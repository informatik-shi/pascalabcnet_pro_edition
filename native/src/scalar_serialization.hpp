#pragma once

#include "binary_stream.hpp"

namespace abc {
struct Values;
struct SerializedValue {
    Type type;
    Value value;
};

bool nrbf_primitive_type(Type);
bool nrbf_primitive_array(const std::vector<TypeInfo>&,Type);
void serialize_value(BinaryDevice&,const Program&,Type,const Value&,SourceLocation,const ExecutionOptions&);
SerializedValue deserialize_value(BinaryDevice&,Values&,SourceLocation,const ExecutionOptions&);
}

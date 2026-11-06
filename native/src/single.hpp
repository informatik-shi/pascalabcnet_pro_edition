#pragma once
#include "abc/core.hpp"
namespace abc {
float single_number(const Value&,SourceLocation);
Value floating_integer_cast(Type,const Value&,SourceLocation);
float single_binary(Op,float,float,SourceLocation);
}

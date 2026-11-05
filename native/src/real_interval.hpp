#pragma once
#include "abc/core.hpp"
#include <span>

namespace abc {
struct Values;
enum class RealIntervalMethod { construct,low,high,size,is_empty,equal,not_equal,to_string,hash_code,membership };
bool real_interval_signature(const Program& program,Type owner,RealIntervalMethod method);
std::vector<Type> real_interval_parameters(Type owner,RealIntervalMethod method);
Type real_interval_result(Type owner,RealIntervalMethod method);
std::u16string real_interval_text(const std::shared_ptr<Aggregate>& value,SourceLocation location);
std::int32_t real_interval_hash(const std::shared_ptr<Aggregate>& value);
Value real_interval_invoke(Values& values,Type owner,RealIntervalMethod method,std::span<const Value> arguments);
}

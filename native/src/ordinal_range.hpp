#pragma once
#include "abc/core.hpp"
#include <span>

namespace abc {
struct Values;
enum class OrdinalRangeMethod { construct,low,high,count,is_empty,contains,contains_real,equal,not_equal,to_string,hash_code,copy_to,to_array,step,reverse,membership,membership_real,contains_longword,contains_uint64,membership_longword,membership_uint64 };
bool ordinal_range_signature(const Program& program,Type owner,OrdinalRangeMethod method);
std::vector<Type> ordinal_range_parameters(const std::vector<TypeInfo>& types,Type owner,OrdinalRangeMethod method);
Type ordinal_range_result(const std::vector<TypeInfo>& types,Type owner,OrdinalRangeMethod method);
Type ordinal_range_array(const std::vector<TypeInfo>& types,Type owner);
std::int32_t ordinal_range_count(const std::shared_ptr<Aggregate>& value);
std::u16string ordinal_range_text(const std::shared_ptr<Aggregate>& value);
std::int32_t ordinal_range_hash(const std::shared_ptr<Aggregate>& value);
std::shared_ptr<Aggregate> ordinal_range_begin(Values& values,Type sequence,const std::shared_ptr<Aggregate>& value);
Value ordinal_range_collect(Values& values,const std::shared_ptr<Aggregate>& value,Type collection,const ExecutionOptions& options);
Value ordinal_range_invoke(Values& values,Type owner,OrdinalRangeMethod method,std::span<const Value> arguments,const ExecutionOptions& options);
}

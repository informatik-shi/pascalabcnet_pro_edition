#pragma once
#include "abc/core.hpp"
#include <span>

namespace abc {
struct Values;
enum class SqlGuidMethod : std::uint8_t {
    null_value, from_guid, from_text, from_bytes, parse,
    value, is_null, to_string, to_bytes, compare, equal, hash
};
inline bool sql_guid_record(const TypeInfo& type) {return type.native_record==NativeRecord::sql_guid;}
bool sql_guid_shape(std::span<const TypeInfo>,const TypeInfo&);
std::vector<Type> sql_guid_parameters(Type owner,const TypeInfo&,SqlGuidMethod);
Type sql_guid_result(Type owner,const TypeInfo&,SqlGuidMethod);
Value sql_guid_invoke(Values&,Type,SqlGuidMethod,std::span<const Value>);
std::u16string sql_guid_text(const Value&,SourceLocation);
int compare_sql_guids(const Value&,const Value&);
std::int32_t sql_guid_hash(const Value&);
}

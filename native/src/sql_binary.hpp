#pragma once
#include "abc/core.hpp"
#include <span>

namespace abc {
struct Values;
enum class SqlBinaryMethod : std::uint8_t {
    null_value, from_bytes, is_null, value, length, item, to_string, compare, equal, hash, concat, to_guid
};
inline bool sql_binary_record(const TypeInfo& type) {return type.native_record==NativeRecord::sql_binary;}
bool sql_binary_shape(std::span<const TypeInfo>,const TypeInfo&);
std::vector<Type> sql_binary_parameters(Type owner,const TypeInfo&,SqlBinaryMethod);
Type sql_binary_result(Type owner,const TypeInfo&,SqlBinaryMethod);
Value sql_binary_invoke(Values&,Type,SqlBinaryMethod,std::span<const Value>);
std::u16string sql_binary_text(const Value&);
int compare_sql_binaries(const Value&,const Value&);
std::int32_t sql_binary_hash(const Value&);
}

#pragma once
#include "abc/core.hpp"
#include <span>

namespace abc {
struct Values;
enum class SqlDateTimeMethod : std::uint8_t {
    null_value, minimum, maximum, from_date, from_sql_ticks, parse,
    is_null, value, day_ticks, time_ticks, to_string, compare, add, subtract
};
inline bool sql_date_time_record(const TypeInfo& type) {return type.native_record==NativeRecord::sql_date_time;}
bool sql_date_time_shape(std::span<const TypeInfo>,const TypeInfo&);
std::vector<Type> sql_date_time_parameters(Type owner,const TypeInfo&,SqlDateTimeMethod);
Type sql_date_time_result(Type owner,const TypeInfo&,SqlDateTimeMethod);
Value sql_date_time_invoke(Values&,Type,SqlDateTimeMethod,std::span<const Value>);
std::u16string sql_date_time_text(const Value&,SourceLocation);
}

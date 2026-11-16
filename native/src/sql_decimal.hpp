#pragma once
#include "abc/core.hpp"
#include <span>

namespace abc {
struct Values;
enum class SqlDecimalMethod : std::uint8_t {
    null_value, from_integer, from_decimal, parse, minimum, maximum, is_null,
    value, precision, scale, is_positive, data, to_double, to_string,
    add, subtract, multiply, divide, compare, negate
};
inline bool sql_decimal_record(const TypeInfo& type) {return type.native_record==NativeRecord::sql_decimal;}
bool sql_decimal_shape(std::span<const TypeInfo>,const TypeInfo&);
std::vector<Type> sql_decimal_parameters(Type owner,const TypeInfo&,SqlDecimalMethod);
Type sql_decimal_result(Type owner,const TypeInfo&,SqlDecimalMethod);
Value sql_decimal_invoke(Values&,Type,SqlDecimalMethod,std::span<const Value>);
Value sql_decimal_convert(Values&,Type owner,Type source,const Value&);
int compare_sql_decimals(const Value&,const Value&,SourceLocation);
std::u16string sql_decimal_text(const Value&);
}

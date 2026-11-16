#pragma once
#include "abc/core.hpp"
#include <span>

namespace abc {
struct Values;
enum class SqlMoneyMethod : std::uint8_t {
    null_value, zero, minimum, maximum, from_integer, from_decimal, parse, is_null,
    decimal_value, to_string, to_double, to_int32, to_int64, add, subtract, multiply, divide, compare, negate
};
inline bool sql_money_record(const TypeInfo& type) {return type.native_record==NativeRecord::sql_money;}
bool sql_money_shape(std::span<const TypeInfo>,const TypeInfo&);
std::vector<Type> sql_money_parameters(Type owner,const TypeInfo&,SqlMoneyMethod);
Type sql_money_result(Type owner,const TypeInfo&,SqlMoneyMethod);
Value sql_money_invoke(Values&,Type,SqlMoneyMethod,std::span<const Value>);
Value sql_money_convert(Values&,Type owner,Type source,const Value&);
std::u16string sql_money_text(const Value&);
}

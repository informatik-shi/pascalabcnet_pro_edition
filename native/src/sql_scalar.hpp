#pragma once
#include "abc/core.hpp"
#include <span>

namespace abc {
struct Values;
struct SqlScalarType {
    std::string_view name;
    std::u16string_view framework_name;
    Type underlying;
};
enum class SqlScalarMethod : std::uint8_t {
    create, null_value, value, is_null, to_string, false_value, true_value,
    is_false, is_true, zero, minimum, maximum
};
inline bool sql_scalar_record(const TypeInfo& type) {return type.native_record==NativeRecord::sql_scalar;}
std::span<const SqlScalarType> sql_scalar_types();
const SqlScalarType* sql_scalar_type(std::string_view name);
bool sql_scalar_shape(const TypeInfo&);
bool sql_scalar_method(Type underlying,SqlScalarMethod);
std::vector<Type> sql_scalar_parameters(Type owner,Type underlying,SqlScalarMethod);
Type sql_scalar_result(Type owner,Type underlying,SqlScalarMethod);
Value sql_scalar_invoke(Values&,Type owner,SqlScalarMethod,std::span<const Value>);
Value sql_scalar_convert(Values&,Type owner,Type source,const Value&);
std::u16string sql_scalar_text(const Value&,SourceLocation);
}

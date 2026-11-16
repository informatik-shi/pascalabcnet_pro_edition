#include "sql_scalar.hpp"
#include "formatting.hpp"
#include "runtime_values.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>

namespace abc {
namespace {
constexpr SqlScalarType types[]{
    {"sqlboolean",u"SqlBoolean",Type::boolean},{"sqlbyte",u"SqlByte",Type::byte},
    {"sqlint16",u"SqlInt16",Type::smallint},{"sqlint32",u"SqlInt32",Type::integer},
    {"sqlint64",u"SqlInt64",Type::int64},{"sqlsingle",u"SqlSingle",Type::single},
    {"sqldouble",u"SqlDouble",Type::real},{"sqlstring",u"SqlString",Type::string}
};
const Aggregate& record(const Value& value) {
    return *std::get<std::shared_ptr<Aggregate>>(value);
}
bool defined(const Aggregate& value) {
    return std::get<bool>(value.items[0]->value);
}
Value make_sql(Values& values,Type owner,Value value) {
    auto result=std::get<std::shared_ptr<Aggregate>>(values.zero(owner));
    if(std::holds_alternative<NilString>(value))return result;
    const auto real=std::get_if<double>(&value);
    const auto single=std::get_if<float>(&value);
    if((real && !std::isfinite(*real)) || (single && !std::isfinite(*single)))
        throw Error("E_OVERFLOW",values.location,"SQL floating-point values must be finite",
            "Вещественное значение SQL должно быть конечным");
    result->items[0]->value=true;
    result->items[1]->value=std::move(value);
    return result;
}
unsigned numeric_rank(Type type) {
    switch(type){
    case Type::byte:return 1;
    case Type::smallint:return 2;
    case Type::word:return 3;
    case Type::integer:return 4;
    case Type::longword:return 5;
    case Type::int64:return 6;
    case Type::uint64:return 7;
    case Type::single:return 8;
    case Type::real:return 9;
    default:return 0;
    }
}
Value convert_number(Type target,const Value& value,SourceLocation location) {
    return std::visit([&](const auto& item)->Value{
        using T=std::decay_t<decltype(item)>;
        if constexpr(std::is_arithmetic_v<T>){
            switch(target){
            case Type::smallint:return static_cast<std::int16_t>(item);
            case Type::integer:return static_cast<std::int32_t>(item);
            case Type::int64:return static_cast<std::int64_t>(item);
            case Type::single:return static_cast<float>(item);
            case Type::real:return static_cast<double>(item);
            default:break;
            }
        }
        throw runtime_error(RuntimeError::invalid_bytecode,location);
    },value);
}
Value endpoint(Type type,bool upper,SourceLocation location) {
    switch(type){
    case Type::byte:return static_cast<std::uint8_t>(upper ? UINT8_MAX : 0);
    case Type::smallint:return static_cast<std::int16_t>(upper ? INT16_MAX : INT16_MIN);
    case Type::integer:return upper ? INT32_MAX : INT32_MIN;
    case Type::int64:return upper ? INT64_MAX : INT64_MIN;
    case Type::single:return upper ? std::numeric_limits<float>::max() : -std::numeric_limits<float>::max();
    case Type::real:return upper ? std::numeric_limits<double>::max() : -std::numeric_limits<double>::max();
    default:throw runtime_error(RuntimeError::invalid_bytecode,location);
    }
}
} // namespace

std::span<const SqlScalarType> sql_scalar_types() {return types;}
const SqlScalarType* sql_scalar_type(std::string_view name) {
    const auto found=std::find_if(std::begin(types),std::end(types),[&](const auto& type){return type.name==name;});
    return found==std::end(types) ? nullptr : found;
}
bool sql_scalar_shape(const TypeInfo& shape) {
    const auto* type=sql_scalar_type(shape.name);
    return sql_scalar_record(shape) && shape.kind==CompoundKind::record && type && shape.element==type->underlying
        && shape.key==Type::invalid && shape.fields.size()==2
        && shape.fields[0].name=="_defined" && shape.fields[0].type==Type::boolean && !shape.fields[0].global
        && shape.fields[1].name=="_value" && shape.fields[1].type==shape.element && !shape.fields[1].global;
}
bool sql_scalar_method(Type underlying,SqlScalarMethod method) {
    using M=SqlScalarMethod;
    if(method>=M::false_value && method<=M::is_true)return underlying==Type::boolean;
    if(method>=M::zero && method<=M::maximum)return numeric_rank(underlying)>0;
    return method<=M::to_string;
}
std::vector<Type> sql_scalar_parameters(Type owner,Type underlying,SqlScalarMethod method) {
    using M=SqlScalarMethod;
    if(method==M::create)return {underlying};
    if(method==M::value || method==M::is_null || method==M::to_string || method==M::is_false || method==M::is_true)
        return {owner};
    return {};
}
Type sql_scalar_result(Type owner,Type underlying,SqlScalarMethod method) {
    using M=SqlScalarMethod;
    if(method==M::value)return underlying;
    if(method==M::to_string)return Type::string;
    if(method==M::is_null || method==M::is_false || method==M::is_true)return Type::boolean;
    return owner;
}
std::u16string sql_scalar_text(const Value& value,SourceLocation location) {
    const auto& data=record(value);
    return defined(data) ? format_field(data.items[1]->value,0,{},location) : u"Null";
}
Value sql_scalar_invoke(Values& values,Type owner,SqlScalarMethod method,std::span<const Value> arguments) {
    using M=SqlScalarMethod;
    const auto underlying=type_info(values.program,owner).element;
    switch(method){
    case M::create:return make_sql(values,owner,arguments[0]);
    case M::null_value:return values.zero(owner);
    case M::false_value:case M::true_value:return make_sql(values,owner,method==M::true_value);
    case M::zero:return make_sql(values,owner,values.zero(underlying));
    case M::minimum:case M::maximum:return make_sql(values,owner,endpoint(underlying,method==M::maximum,values.location));
    case M::is_null:return !defined(record(arguments[0]));
    case M::is_false:case M::is_true:{
        const auto& item=record(arguments[0]);
        return defined(item) && std::get<bool>(item.items[1]->value)==(method==M::is_true);
    }
    case M::to_string:return sql_scalar_text(arguments[0],values.location);
    case M::value: {
        const auto& item=record(arguments[0]);
        if(!defined(item))
            throw Error("E_NIL",values.location,"Cannot read the value of SQL Null","Нельзя прочитать значение SQL Null");
        return item.items[1]->value;
    }
    }
    throw runtime_error(RuntimeError::invalid_bytecode,values.location);
}
Value sql_scalar_convert(Values& values,Type owner,Type source,const Value& value) {
    if(source==owner)return value;
    const auto target=type_info(values.program,owner).element;
    const Value* input=&value;
    bool missing=false;
    if(compound(source)){
        const auto& shape=type_info(values.program,source);
        if(sql_scalar_record(shape)){
            const auto& data=record(value);
            missing=!defined(data);
            input=&data.items[1]->value;
            source=shape.element;
        }
    }
    const auto rank=numeric_rank(source);
    if(source==target || (rank && rank<=numeric_rank(target))){
        if(missing)return values.zero(owner);
        return make_sql(values,owner,source==target ? *input : convert_number(target,*input,values.location));
    }
    throw Error("E_TYPE",values.location,"Value cannot be converted to the requested SQL type",
        "Значение нельзя преобразовать к запрошенному типу SQL");
}
} // namespace abc

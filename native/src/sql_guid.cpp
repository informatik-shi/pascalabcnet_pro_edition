#include "sql_guid.hpp"
#include "guid.hpp"
#include "runtime_values.hpp"
#include <array>

namespace abc {
namespace {
using Method=SqlGuidMethod;
const Aggregate& record(const Value& value) {
    return *std::get<std::shared_ptr<Aggregate>>(value);
}
bool missing(const Value& value) {
    return !std::get<bool>(record(value).items[0]->value);
}
const Value& guid(const Value& value) {
    return record(value).items[1]->value;
}
Value store(Values& values,Type owner,Value value) {
    auto result=std::get<std::shared_ptr<Aggregate>>(values.zero(owner,false,true));
    result->items[0]->value=true;
    result->items[1]->value=std::move(value);
    return result;
}
}
bool sql_guid_shape(std::span<const TypeInfo> types,const TypeInfo& shape) {
    if(!sql_guid_record(shape) || shape.kind!=CompoundKind::record || shape.name!="sqlguid"
        || !compound(shape.element) || !compound(shape.key))return false;
    const auto guid=static_cast<std::size_t>(shape.element)-64,array=static_cast<std::size_t>(shape.key)-64;
    return guid<types.size() && array<types.size() && guid_record_type(types[guid]) && types[guid].element==shape.key
        && types[array].kind==CompoundKind::array && types[array].rank==1 && types[array].element==Type::byte
        && shape.fields.size()==2 && shape.fields[0].name=="_defined" && shape.fields[0].type==Type::boolean
        && shape.fields[1].name=="_guid" && shape.fields[1].type==shape.element
        && !shape.fields[0].global && !shape.fields[1].global;
}
std::vector<Type> sql_guid_parameters(Type owner,const TypeInfo& shape,Method method) {
    switch(method){
    case Method::null_value:return {};
    case Method::from_guid:return {shape.element};
    case Method::from_text:case Method::parse:return {Type::string};
    case Method::from_bytes:return {shape.key};
    case Method::compare:case Method::equal:return {owner,owner};
    default:return {owner};
    }
}
Type sql_guid_result(Type owner,const TypeInfo& shape,Method method) {
    switch(method){
    case Method::value:return shape.element;
    case Method::is_null:case Method::equal:return Type::boolean;
    case Method::to_string:return Type::string;
    case Method::to_bytes:return shape.key;
    case Method::compare:case Method::hash:return Type::integer;
    default:return owner;
    }
}
std::u16string sql_guid_text(const Value& value,SourceLocation location) {
    return missing(value) ? u"Null" : guid_text(guid(value),{},location);
}
int compare_sql_guids(const Value& left,const Value& right) {
    const bool a=missing(left),b=missing(right);
    if(a || b)return b-a;
    constexpr std::array<std::size_t,16> order{10,11,12,13,14,15,8,9,6,7,4,5,0,1,2,3};
    const auto x=guid_bytes(guid(left)),y=guid_bytes(guid(right));
    for(const auto i:order){
        if(x[i]!=y[i])return x[i]>y[i] ? 1 : -1;
    }
    return 0;
}
std::int32_t sql_guid_hash(const Value& value) {
    return missing(value) ? 0 : guid_hash_code(guid(value));
}
Value sql_guid_invoke(Values& values,Type owner,Method method,std::span<const Value> arguments) {
    const auto clr=type_info(values.program,owner).element;
    switch(method){
    case Method::null_value:return values.zero(owner);
    case Method::from_guid:return store(values,owner,arguments[0]);
    case Method::parse:case Method::from_text:{
        const auto* s=std::get_if<std::u16string>(&arguments[0]);
        if(method==Method::parse && s && *s==u"Null")return values.zero(owner);
        return store(values,owner,guid_invoke(values,clr,GuidMethod::parse,arguments));
    }
    case Method::from_bytes:return store(values,owner,guid_invoke(values,clr,GuidMethod::from_bytes,arguments));
    case Method::is_null:return missing(arguments[0]);
    case Method::to_string:return sql_guid_text(arguments[0],values.location);
    case Method::compare:return static_cast<std::int32_t>(compare_sql_guids(arguments[0],arguments[1]));
    case Method::equal:return compare_sql_guids(arguments[0],arguments[1])==0;
    case Method::hash:return sql_guid_hash(arguments[0]);
    case Method::value:case Method::to_bytes:{
        const auto& value=arguments[0];
        if(missing(value))throw Error("E_NIL",values.location,"Cannot read the value of SQL Null","Нельзя прочитать значение SQL Null");
        if(method==Method::value)return guid(value);
        const std::array<Value,1> payload{guid(value)};
        return guid_invoke(values,clr,GuidMethod::to_bytes,payload);
    }
    }
    throw runtime_error(RuntimeError::invalid_bytecode,values.location);
}
} // namespace abc

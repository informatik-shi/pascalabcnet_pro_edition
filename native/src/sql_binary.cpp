#include "sql_binary.hpp"
#include "runtime_values.hpp"
#include "sql_guid.hpp"
#include <algorithm>
#include <array>
#include <bit>

namespace abc {
namespace {
using Method=SqlBinaryMethod;
std::shared_ptr<Aggregate> bytes(const Value& value) {
    const auto& record=*std::get<std::shared_ptr<Aggregate>>(value);
    const auto& data=std::get<std::shared_ptr<Aggregate>>(record.items[0]->value);
    return data && !data->nil ? data : nullptr;
}
std::uint8_t element(const Aggregate& array,std::size_t index) {
    return std::get<std::uint8_t>(array.items[index]->value);
}
std::size_t unpadded_size(const Aggregate& array) {
    auto size=array.items.size();
    while(size && element(array,size-1)==0)--size;
    return size;
}
Value store(Values& values,Type owner,std::shared_ptr<Aggregate> payload) {
    auto result=std::get<std::shared_ptr<Aggregate>>(values.zero(owner,false,true));
    result->items[0]->value=std::move(payload);
    return result;
}
void copy_bytes(Aggregate& target,std::size_t offset,const Aggregate& source) {
    for(std::size_t i=0;i<source.items.size();++i)target.items[offset+i]->value=element(source,i);
}
const Aggregate& required_bytes(const Value& value,SourceLocation location) {
    const auto& data=bytes(value);
    if(!data)throw Error("E_NIL",location,"Cannot read the value of SQL Null","Нельзя прочитать значение SQL Null");
    return *data;
}
}
bool sql_binary_shape(std::span<const TypeInfo> types,const TypeInfo& shape) {
    if(!sql_binary_record(shape) || shape.kind!=CompoundKind::record || shape.name!="sqlbinary"
        || !compound(shape.element) || !compound(shape.key))return false;
    const auto array=static_cast<std::size_t>(shape.element)-64,guid=static_cast<std::size_t>(shape.key)-64;
    return array<types.size() && guid<types.size() && sql_guid_record(types[guid]) && types[guid].key==shape.element
        && types[array].kind==CompoundKind::array && types[array].rank==1 && types[array].element==Type::byte
        && shape.fields.size()==1 && shape.fields[0].name=="_bytes" && shape.fields[0].type==shape.element && !shape.fields[0].global;
}
std::vector<Type> sql_binary_parameters(Type owner,const TypeInfo& shape,Method method) {
    switch(method){
    case Method::null_value:return {};
    case Method::from_bytes:return {shape.element};
    case Method::item:return {owner,Type::integer};
    case Method::compare:case Method::equal:case Method::concat:return {owner,owner};
    default:return {owner};
    }
}
Type sql_binary_result(Type owner,const TypeInfo& shape,Method method) {
    switch(method){
    case Method::value:return shape.element;
    case Method::length:case Method::compare:case Method::hash:return Type::integer;
    case Method::is_null:case Method::equal:return Type::boolean;
    case Method::item:return Type::byte;
    case Method::to_string:return Type::string;
    case Method::to_guid:return shape.key;
    default:return owner;
    }
}
std::u16string sql_binary_text(const Value& value) {
    const auto& data=bytes(value);
    if(!data)return u"Null";
    return u"SqlBinary("+from_utf8(std::to_string(data->items.size()))+u")";
}
int compare_sql_binaries(const Value& left,const Value& right) {
    const auto& a=bytes(left);
    const auto& b=bytes(right);
    if(!a || !b)return static_cast<int>(static_cast<bool>(a))-static_cast<int>(static_cast<bool>(b));
    const auto size_a=unpadded_size(*a),size_b=unpadded_size(*b);
    for(std::size_t i=0;i<std::min(size_a,size_b);++i){
        const auto x=element(*a,i),y=element(*b,i);
        if(x!=y)return x>y ? 1 : -1;
    }
    return (size_a>size_b)-(size_a<size_b);
}
std::int32_t sql_binary_hash(const Value& value) {
    const auto& data=bytes(value);
    if(!data)return 0;
    const auto size=unpadded_size(*data);
    std::uint32_t hash=0;
    for(std::size_t i=0;i<size;++i){
        const auto upper=static_cast<std::uint32_t>(std::bit_cast<std::int32_t>(hash)>>28)&255U;
        hash=(hash<<4)^element(*data,i)^upper;
    }
    return std::bit_cast<std::int32_t>(hash);
}
Value sql_binary_invoke(Values& values,Type owner,Method method,std::span<const Value> arguments) {
    const auto& shape=type_info(values.program,owner);
    switch(method){
    case Method::null_value:return values.zero(owner);
    case Method::from_bytes:{
        const auto& source=std::get<std::shared_ptr<Aggregate>>(arguments[0]);
        if(!source || source->nil)return values.zero(owner);
        auto payload=values.array(shape.element,source->items.size());
        copy_bytes(*payload,0,*source);
        return store(values,owner,std::move(payload));
    }
    case Method::to_string:return sql_binary_text(arguments[0]);
    case Method::is_null:return !bytes(arguments[0]);
    case Method::compare:return static_cast<std::int32_t>(compare_sql_binaries(arguments[0],arguments[1]));
    case Method::equal:return compare_sql_binaries(arguments[0],arguments[1])==0;
    case Method::hash:return sql_binary_hash(arguments[0]);
    case Method::concat:{
        const auto& a=bytes(arguments[0]);
        const auto& b=bytes(arguments[1]);
        if(!a || !b)return values.zero(owner);
        constexpr auto limit=static_cast<std::size_t>(INT32_MAX);
        if(b->items.size()>limit || a->items.size()>limit-b->items.size())
            throw runtime_error(RuntimeError::allocation_limit,values.location);
        auto payload=values.array(shape.element,a->items.size()+b->items.size());
        copy_bytes(*payload,0,*a);
        copy_bytes(*payload,a->items.size(),*b);
        return store(values,owner,std::move(payload));
    }
    case Method::to_guid:{
        const auto& data=bytes(arguments[0]);
        if(!data)return values.zero(shape.key);
        const std::array<Value,1> args{data};
        return sql_guid_invoke(values,shape.key,SqlGuidMethod::from_bytes,args);
    }
    default:break;
    }
    const auto& data=required_bytes(arguments[0],values.location);
    switch(method){
    case Method::length:return static_cast<std::int32_t>(data.items.size());
    case Method::value:{
        auto result=values.array(shape.element,data.items.size());
        copy_bytes(*result,0,data);
        return result;
    }
    case Method::item:{
        const auto index=std::get<std::int32_t>(arguments[1]);
        if(index<0 || static_cast<std::size_t>(index)>=data.items.size())
            throw runtime_error(RuntimeError::index_bounds,values.location);
        return element(data,static_cast<std::size_t>(index));
    }
    default:throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    }
}
} // namespace abc

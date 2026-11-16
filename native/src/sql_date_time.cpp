#include "sql_date_time.hpp"
#include "calendar.hpp"
#include "runtime_values.hpp"
#include <array>

namespace abc {
namespace {
using Method=SqlDateTimeMethod;
constexpr std::int64_t ticks_per_millisecond=10000;
constexpr std::int64_t ticks_per_day=864000000000;
constexpr std::int64_t base_date=599266080000000000;
constexpr std::int64_t max_date=3155378975999999999;
constexpr std::int32_t minimum_day=-53690,maximum_day=2958463,high_time=25919999;

struct SqlDateTime {std::int32_t day;std::int32_t time;};
const Aggregate& record(const Value& value) {
    return *std::get<std::shared_ptr<Aggregate>>(value);
}
bool missing(const Value& value) {
    return !std::get<bool>(record(value).items[0]->value);
}
Error bad_range(SourceLocation location) {
    return Error("E_RANGE",location,"DateTime is outside the SqlDateTime range","Дата выходит за диапазон SqlDateTime");
}
Error overflow(SourceLocation location) {
    return Error("E_OVERFLOW",location,"SqlDateTime day or time ticks overflow",
        "Переполнение числа тиков даты или времени SqlDateTime");
}
SqlDateTime data(const Value& value,SourceLocation location) {
    if(missing(value))throw Error("E_NIL",location,"Cannot read the value of SQL Null","Нельзя прочитать значение SQL Null");
    const auto& fields=record(value).items;
    return {std::get<std::int32_t>(fields[1]->value),std::get<std::int32_t>(fields[2]->value)};
}
Value store(Values& values,Type owner,SqlDateTime time) {
    if(time.day<minimum_day || time.day>maximum_day || time.time<0 || time.time>high_time)throw overflow(values.location);
    auto result=std::get<std::shared_ptr<Aggregate>>(values.zero(owner,false,true));
    result->items[0]->value=true;
    result->items[1]->value=time.day;
    result->items[2]->value=time.time;
    return result;
}
SqlDateTime from_ticks(std::int64_t ticks,SourceLocation location) {
    if(ticks==max_date)return {maximum_day,high_time};
    if(ticks<base_date+minimum_day*ticks_per_day || ticks>max_date)throw bad_range(location);
    auto days=(ticks-base_date)/ticks_per_day;
    auto within=(ticks-base_date)%ticks_per_day;
    if(within<0){--days;within+=ticks_per_day;}
    const volatile double sql_ticks=static_cast<double>(within)/ticks_per_millisecond*0.3;
    auto time=static_cast<std::int32_t>(sql_ticks+0.5);
    if(time>high_time){++days;time=0;}
    return {static_cast<std::int32_t>(days),time};
}
std::int64_t to_ticks(SqlDateTime date) {
    const auto milliseconds=(static_cast<std::int64_t>(date.time)*10+1)/3;
    return base_date+date.day*ticks_per_day+milliseconds*ticks_per_millisecond;
}
std::int64_t date_ticks(const Value& date) {
    return static_cast<std::int64_t>(date_time_binary_bits(date)&date_time_ticks_mask);
}
}
bool sql_date_time_shape(std::span<const TypeInfo> types,const TypeInfo& shape) {
    if(!sql_date_time_record(shape) || shape.kind!=CompoundKind::record || shape.name!="sqldatetime"
        || !compound(shape.element) || !compound(shape.key))return false;
    const auto date=static_cast<std::size_t>(shape.element)-64,index=static_cast<std::size_t>(shape.key)-64;
    return date<types.size() && index<types.size() && types[date].native_record==NativeRecord::date_time
        && types[date].element==shape.key && types[index].native_record==NativeRecord::time_span
        && shape.fields.size()==3 && shape.fields[0].name=="_defined" && shape.fields[0].type==Type::boolean
        && shape.fields[1].name=="_day" && shape.fields[1].type==Type::integer
        && shape.fields[2].name=="_time" && shape.fields[2].type==Type::integer
        && !shape.fields[0].global && !shape.fields[1].global && !shape.fields[2].global;
}
std::vector<Type> sql_date_time_parameters(Type owner,const TypeInfo& shape,Method method) {
    switch(method){
    case Method::null_value:case Method::minimum:case Method::maximum:return {};
    case Method::from_date:return {shape.element};
    case Method::from_sql_ticks:return {Type::integer,Type::integer};
    case Method::parse:return {Type::string};
    case Method::compare:return {owner,owner};
    case Method::add:case Method::subtract:return {owner,shape.key};
    default:return {owner};
    }
}
Type sql_date_time_result(Type owner,const TypeInfo& shape,Method method) {
    switch(method){
    case Method::is_null:return Type::boolean;
    case Method::value:return shape.element;
    case Method::day_ticks:case Method::time_ticks:case Method::compare:return Type::integer;
    case Method::to_string:return Type::string;
    default:return owner;
    }
}
std::u16string sql_date_time_text(const Value& value,SourceLocation location) {
    return missing(value) ? u"Null" : unzoned_date_text(to_ticks(data(value,location)),location);
}
Value sql_date_time_invoke(Values& values,Type owner,Method method,std::span<const Value> arguments) {
    using C=CalendarMethod;
    const auto clr=type_info(values.program,owner).element;
    switch(method){
    case Method::null_value:return values.zero(owner);
    case Method::minimum:return store(values,owner,{minimum_day,0});
    case Method::maximum:return store(values,owner,{maximum_day,high_time});
    case Method::from_date:return store(values,owner,from_ticks(date_ticks(arguments[0]),values.location));
    case Method::from_sql_ticks:
        return store(values,owner,{std::get<std::int32_t>(arguments[0]),std::get<std::int32_t>(arguments[1])});
    case Method::parse:{
        const auto* s=std::get_if<std::u16string>(&arguments[0]);
        if(!s)throw Error("E_NIL",values.location,"SqlDateTime.Parse requires a string","Для SqlDateTime.Parse нужна строка");
        if(*s==u"Null")return values.zero(owner);
        const std::array<Value,1> input{normalize_date_text(values,clr,*s)};
        const auto date=calendar_invoke(values,clr,static_cast<std::size_t>(C::parse),input);
        return store(values,owner,from_ticks(date_ticks(date),values.location));
    }
    case Method::is_null:return missing(arguments[0]);
    case Method::to_string:return sql_date_time_text(arguments[0],values.location);
    case Method::compare:{
        const auto a=missing(arguments[0]),b=missing(arguments[1]);
        if(a || b)return static_cast<std::int32_t>(b-a);
        const auto x=to_ticks(data(arguments[0],values.location)),y=to_ticks(data(arguments[1],values.location));
        return static_cast<std::int32_t>((x>y)-(x<y));
    }
    default:break;
    }
    if((method==Method::add || method==Method::subtract) && missing(arguments[0]))return values.zero(owner);
    const auto date=data(arguments[0],values.location);
    switch(method){
    case Method::day_ticks:return date.day;
    case Method::time_ticks:return date.time;
    case Method::value:return date_time_from_binary_bits(values,clr,static_cast<std::uint64_t>(to_ticks(date)));
    case Method::add:case Method::subtract:{
        const auto left=date_time_from_binary_bits(values,clr,static_cast<std::uint64_t>(to_ticks(date)));
        const std::array<Value,2> args{left,arguments[1]};
        const auto op=method==Method::add ? C::add : C::subtract;
        const auto result=calendar_invoke(values,clr,static_cast<std::size_t>(op),args);
        return store(values,owner,from_ticks(date_ticks(result),values.location));
    }
    default:throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    }
}
} // namespace abc

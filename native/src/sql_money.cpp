#include "sql_money.hpp"
#include "decimal.hpp"
#include "integer_values.hpp"
#include "runtime_values.hpp"
#include "sql_scalar.hpp"
#include <array>
#include <stdexcept>

namespace abc {
namespace {
using Method=SqlMoneyMethod;
constexpr unsigned money_scale=4;
constexpr std::int64_t ticks_per_unit=10000;
Error overflow(SourceLocation location) {
    return Error("E_OVERFLOW",location,"SqlMoney arithmetic or conversion overflow",
        "Переполнение арифметики или преобразования SqlMoney");
}
Error format_error(SourceLocation location) {
    return Error("E_FORMAT",location,"Invalid SqlMoney text","Некорректная строка SqlMoney");
}
const Aggregate& record(const Value& value) {
    return *std::get<std::shared_ptr<Aggregate>>(value);
}
bool missing(const Value& value) {
    return !std::get<bool>(record(value).items[0]->value);
}
std::int64_t ticks(const Value& value,SourceLocation location) {
    if(missing(value))throw Error("E_NIL",location,"Cannot read the value of SQL Null","Нельзя прочитать значение SQL Null");
    return std::get<std::int64_t>(record(value).items[1]->value);
}
Value store(Values& values,Type owner,std::int64_t raw) {
    auto result=std::get<std::shared_ptr<Aggregate>>(values.zero(owner,false,true));
    result->items[0]->value=true;
    result->items[1]->value=raw;
    return result;
}
std::uint64_t magnitude(std::int64_t raw) {
    const auto words=static_cast<std::uint64_t>(raw);
    return raw<0 ? 0-words : words;
}
std::int64_t signed_ticks(const BigInteger& scaled,SourceLocation location) {
    try{return scaled.to_signed();}
    catch(const std::overflow_error&){throw overflow(location);}
}
std::int64_t from_decimal(const Value& value,SourceLocation location) {
    const auto bits=decimal_bits(value);
    auto coefficient=BigInteger::from_unsigned(bits[0]+(static_cast<std::uint64_t>(bits[1])<<32));
    coefficient=coefficient.add(BigInteger::from_unsigned(bits[2]).shift_left(64));
    const auto scale=(bits[3]>>16)&0xffU;
    const auto ten=BigInteger::from_unsigned(10);
    if(scale<money_scale)coefficient=coefficient.multiply(ten.power(money_scale-scale));
    if(scale>money_scale){
        const auto divisor=ten.power(scale-money_scale);
        auto [part,discarded]=coefficient.divide(divisor);
        if(discarded.shift_left(1)>=divisor)part=part.add(BigInteger::from_unsigned(1));
        coefficient=std::move(part);
    }
    if(bits[3]&0x80000000U)coefficient=coefficient.negated();
    return signed_ticks(coefficient,location);
}
Value decimal(Values& values,Type type,std::int64_t raw) {
    const auto coefficient=magnitude(raw);
    return decimal_from_bits(values,type,{static_cast<std::uint32_t>(coefficient),
        static_cast<std::uint32_t>(coefficient>>32),0,(money_scale<<16)|(raw<0 ? 0x80000000U : 0)});
}
bool numeric_space(char16_t c) {
    return c==u' ' || (c>=u'\t' && c<=u'\r');
}
void trim(std::u16string_view& s) {
    while(!s.empty() && numeric_space(s.front()))s.remove_prefix(1);
    while(!s.empty() && numeric_space(s.back()))s.remove_suffix(1);
}
std::u16string normalized_money(std::u16string_view source,SourceLocation location) {
    trim(source);
    bool negative=false,has_sign=false,has_currency=false,invariant_currency=false;
    if(source.starts_with(u'(') && source.ends_with(u')')){
        negative=true;
        has_sign=true;
        source=source.substr(1,source.size()-2);
        trim(source);
    }
    while(!source.empty()){
        if(source.front()==u'$' || source.front()==u'¤'){
            if(has_currency)throw format_error(location);
            invariant_currency=source.front()==u'¤';has_currency=true;
            source.remove_prefix(1);
            trim(source);
        }else if(source.front()==u'+' || source.front()==u'-'){
            if(has_sign)throw format_error(location);
            negative=source.front()==u'-';has_sign=true;
            source.remove_prefix(1);
            if(has_currency)trim(source);
        }else if(source.back()==u'$' || source.back()==u'¤'){
            if(has_currency)throw format_error(location);
            invariant_currency=source.back()==u'¤';has_currency=true;
            source.remove_suffix(1);
            trim(source);
        }else if(source.back()==u'+' || source.back()==u'-'){
            if(has_sign)throw format_error(location);
            negative=source.back()==u'-';has_sign=true;
            source.remove_suffix(1);
            trim(source);
        }else break;
    }
    std::u16string cleaned;
    if(negative)cleaned.push_back(u'-');
    bool digit=false,point=false;
    for(const auto c:source){
        if(c>=u'0' && c<=u'9'){digit=true;cleaned.push_back(c);}
        else if(c==u'.' && !point){point=true;cleaned.push_back(c);}
        else if(c!=u',' || point || !digit || invariant_currency)throw format_error(location);
    }
    if(!digit)throw format_error(location);
    return cleaned;
}
std::int64_t to_int64(std::int64_t raw) {
    const auto tenths=raw/(ticks_per_unit/10);
    return tenths/10+(tenths%10>=5 ? 1 : 0);
}
}
bool sql_money_shape(std::span<const TypeInfo> types,const TypeInfo& shape) {
    if(!sql_money_record(shape) || shape.kind!=CompoundKind::record || shape.name!="sqlmoney"
        || !compound(shape.element) || shape.key!=Type::invalid)return false;
    const auto decimal_index=static_cast<std::size_t>(shape.element)-64;
    return decimal_index<types.size() && decimal_record_shape(types[decimal_index])
        && shape.fields.size()==2 && shape.fields[0].name=="_defined" && shape.fields[0].type==Type::boolean
        && shape.fields[1].name=="_scaled" && shape.fields[1].type==Type::int64
        && !shape.fields[0].global && !shape.fields[1].global;
}
std::vector<Type> sql_money_parameters(Type owner,const TypeInfo& shape,Method method) {
    switch(method){
    case Method::null_value:case Method::zero:case Method::minimum:case Method::maximum:return {};
    case Method::from_integer:return {Type::int64};
    case Method::from_decimal:return {shape.element};
    case Method::parse:return {Type::string};
    case Method::add:case Method::subtract:case Method::multiply:case Method::divide:case Method::compare:return {owner,owner};
    default:return {owner};
    }
}
Type sql_money_result(Type owner,const TypeInfo& shape,Method method) {
    switch(method){
    case Method::is_null:return Type::boolean;
    case Method::decimal_value:return shape.element;
    case Method::to_string:return Type::string;
    case Method::to_double:return Type::real;
    case Method::to_int32:case Method::compare:return Type::integer;
    case Method::to_int64:return Type::int64;
    default:return owner;
    }
}
std::u16string sql_money_text(const Value& value) {
    if(missing(value))return u"Null";
    const auto raw=ticks(value,{});
    auto text=std::to_string(magnitude(raw));
    if(text.size()<=money_scale)text.insert(0,money_scale+1-text.size(),'0');
    text.insert(text.size()-money_scale,1,'.');
    for(unsigned i=0;i<2 && text.back()=='0';++i)text.pop_back();
    if(raw<0)text.insert(0,1,'-');
    return std::u16string(text.begin(),text.end());
}
Value sql_money_invoke(Values& values,Type owner,Method method,std::span<const Value> arguments) {
    const auto clr=type_info(values.program,owner).element;
    const auto invoke=[&](DecimalMethod operation,std::span<const Value> args) {
        return decimal_invoke(values,clr,static_cast<std::size_t>(operation),args);
    };
    switch(method){
    case Method::null_value:return values.zero(owner);
    case Method::zero:return store(values,owner,0);
    case Method::minimum:return store(values,owner,INT64_MIN);
    case Method::maximum:return store(values,owner,INT64_MAX);
    case Method::from_integer:{
        const auto number=BigInteger::from_signed(std::get<std::int64_t>(arguments[0]));
        return store(values,owner,signed_ticks(number.multiply(BigInteger::from_unsigned(ticks_per_unit)),values.location));
    }
    case Method::from_decimal:return store(values,owner,from_decimal(arguments[0],values.location));
    case Method::parse:{
        const auto* s=std::get_if<std::u16string>(&arguments[0]);
        if(!s)throw Error("E_NIL",values.location,"SqlMoney.Parse requires a string","Для SqlMoney.Parse нужна строка");
        if(*s==u"Null")return values.zero(owner);
        const std::array<Value,1> text{normalized_money(*s,values.location)};
        return store(values,owner,from_decimal(invoke(DecimalMethod::parse,text),values.location));
    }
    case Method::is_null:return missing(arguments[0]);
    case Method::to_string:return sql_money_text(arguments[0]);
    case Method::compare:{
        const auto a=missing(arguments[0]),b=missing(arguments[1]);
        if(a || b)return static_cast<std::int32_t>(b-a);
        const auto x=ticks(arguments[0],values.location),y=ticks(arguments[1],values.location);
        return static_cast<std::int32_t>((x>y)-(x<y));
    }
    case Method::add:case Method::subtract:case Method::multiply:case Method::divide:{
        if(missing(arguments[0]) || missing(arguments[1]))return values.zero(owner);
        const auto a=ticks(arguments[0],values.location),b=ticks(arguments[1],values.location);
        if(method==Method::add || method==Method::subtract){
            const auto x=BigInteger::from_signed(a),y=BigInteger::from_signed(b);
            return store(values,owner,signed_ticks(method==Method::add ? x.add(y) : x.subtract(y),values.location));
        }
        const std::array<Value,2> operands{decimal(values,clr,a),decimal(values,clr,b)};
        const auto result=invoke(method==Method::multiply ? DecimalMethod::multiply : DecimalMethod::divide,operands);
        return store(values,owner,from_decimal(result,values.location));
    }
    case Method::negate:{
        if(missing(arguments[0]))return arguments[0];
        const auto x=ticks(arguments[0],values.location);
        if(x==INT64_MIN)throw overflow(values.location);
        return store(values,owner,-x);
    }
    default:break;
    }
    const auto raw=ticks(arguments[0],values.location);
    switch(method){
    case Method::decimal_value:return decimal(values,clr,raw);
    case Method::to_double:{
        const std::array<Value,1> value{decimal(values,clr,raw)};
        return invoke(DecimalMethod::to_real,value);
    }
    case Method::to_int64:return to_int64(raw);
    case Method::to_int32:return checked_signed_integer(Type::integer,Value{to_int64(raw)},values.location);
    default:throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    }
}
Value sql_money_convert(Values& values,Type owner,Type source,const Value& value) {
    if(source==owner)return value;
    if(source==type_info(values.program,owner).element)return store(values,owner,from_decimal(value,values.location));
    if(compound(source) && sql_scalar_record(type_info(values.program,source))){
        const auto base=type_info(values.program,source).element;
        if(integer_type(base)){
            const std::array<Value,1> input{value};
            if(std::get<bool>(sql_scalar_invoke(values,source,SqlScalarMethod::is_null,input)))return values.zero(owner);
            return sql_money_convert(values,owner,base,sql_scalar_invoke(values,source,SqlScalarMethod::value,input));
        }
    }
    if(integer_type(source) && source!=Type::shortint){
        const auto is_unsigned=wide_unsigned(source) || source==Type::byte || source==Type::word;
        const auto number=is_unsigned ? BigInteger::from_unsigned(unsigned_number(value,values.location))
            : BigInteger::from_signed(integral_number(value,values.location));
        return store(values,owner,signed_ticks(number.multiply(BigInteger::from_unsigned(ticks_per_unit)),values.location));
    }
    throw Error("E_TYPE",values.location,"Value cannot be converted to SqlMoney","Значение нельзя преобразовать к SqlMoney");
}
} // namespace abc

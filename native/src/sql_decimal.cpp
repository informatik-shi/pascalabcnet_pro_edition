#include "sql_decimal.hpp"
#include "decimal.hpp"
#include "integer_values.hpp"
#include "runtime_values.hpp"
#include "sql_scalar.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>

namespace abc {
namespace {
using Method=SqlDecimalMethod;
constexpr unsigned maximum_precision=38;

struct SqlDecimal {
    BigInteger coefficient;
    unsigned precision=1;
    unsigned scale=0;
    bool positive=true;
};

const BigInteger& limit() {
    static const auto power=BigInteger::from_unsigned(10).power(maximum_precision);
    return power;
}
Error format_error(SourceLocation location) {
    return Error("E_FORMAT",location,"Invalid SqlDecimal text","Некорректная строка SqlDecimal");
}
Error overflow(SourceLocation location) {
    return Error("E_OVERFLOW",location,"SqlDecimal arithmetic or conversion overflow",
        "Переполнение арифметики или преобразования SqlDecimal");
}
const Aggregate& record(const Value& value) {
    return *std::get<std::shared_ptr<Aggregate>>(value);
}
bool missing(const Value& value) {
    return !std::get<bool>(record(value).items[0]->value);
}
SqlDecimal number(const Value& value,SourceLocation location) {
    if(missing(value))throw Error("E_NIL",location,"Cannot read the value of SQL Null","Нельзя прочитать значение SQL Null");
    const auto& fields=record(value).items;
    return {std::get<BigInteger>(fields[1]->value),std::get<std::uint8_t>(fields[2]->value),
        std::get<std::uint8_t>(fields[3]->value),std::get<bool>(fields[4]->value)};
}
Value store(Values& values,Type owner,SqlDecimal number) {
    if(number.coefficient>=limit() || number.precision>maximum_precision || number.scale>number.precision)
        throw overflow(values.location);
    auto result=std::get<std::shared_ptr<Aggregate>>(values.zero(owner,false,true));
    result->items[0]->value=true;
    result->items[1]->value=std::move(number.coefficient);
    result->items[2]->value=static_cast<std::uint8_t>(number.precision);
    result->items[3]->value=static_cast<std::uint8_t>(number.scale);
    result->items[4]->value=number.positive;
    return result;
}
unsigned required_precision(const BigInteger& coefficient,unsigned scale) {
    return std::max(scale,static_cast<unsigned>(coefficient.text().size()));
}
SqlDecimal integer(BigInteger coefficient) {
    const auto positive=!coefficient.negative();
    coefficient=coefficient.absolute();
    const auto precision=required_precision(coefficient,0);
    return {std::move(coefficient),precision,0,positive};
}
SqlDecimal from_decimal(const Value& value) {
    const auto data=decimal_bits(value);
    auto coefficient=BigInteger::from_unsigned(data[0]+(static_cast<std::uint64_t>(data[1])<<32));
    coefficient=coefficient.add(BigInteger::from_unsigned(data[2]).shift_left(64));
    const auto scale=(data[3]>>16)&0xffU;
    const auto precision=required_precision(coefficient,scale);
    return {std::move(coefficient),precision,scale,(data[3]&0x80000000U)==0};
}
std::array<std::uint32_t,4> words(const BigInteger& coefficient) {
    const auto mask=BigInteger::from_unsigned(UINT32_MAX);
    std::array<std::uint32_t,4> result{};
    for(std::size_t i=0;i<result.size();++i)
        result[i]=static_cast<std::uint32_t>(coefficient.shift_right(static_cast<std::int32_t>(32*i)).bit_and(mask).to_unsigned());
    return result;
}
SqlDecimal parse(std::u16string_view s,SourceLocation location) {
    while(!s.empty() && s.front()==u' ')s.remove_prefix(1);
    while(!s.empty() && s.back()==u' ')s.remove_suffix(1);
    const auto negative=s.starts_with(u'-');
    if(negative || s.starts_with(u'+'))s.remove_prefix(1);
    while(s.size()>2 && s.front()==u'0')s.remove_prefix(1);
    if(s==u"0.")s=u".0";
    if(s.empty() || s.size()>maximum_precision+1)throw format_error(location);
    while(s.size()>1 && s.front()==u'0')s.remove_prefix(1);
    std::string digits;
    auto point=std::string::npos;
    for(const auto c:s){
        if(c>=u'0' && c<=u'9')digits.push_back(static_cast<char>(c));
        else if(c==u'.' && point==std::string::npos)point=digits.size();
        else throw format_error(location);
    }
    if(digits.empty())throw format_error(location);
    const auto precision=static_cast<unsigned>(digits.size());
    const auto scale=point==std::string::npos ? 0U : static_cast<unsigned>(digits.size()-point);
    auto coefficient=BigInteger::parse(digits);
    if(coefficient>=limit())throw overflow(location);
    if(precision>maximum_precision)throw format_error(location);
    const auto positive=!negative || coefficient.zero();
    return {std::move(coefficient),precision,scale,positive};
}
std::u16string text(const SqlDecimal& value) {
    auto digits=value.coefficient.text();
    if(value.scale){
        if(digits.size()<=value.scale)digits.insert(0,value.scale+1-digits.size(),'0');
        digits.insert(digits.size()-value.scale,1,'.');
    }
    if(!value.positive)digits.insert(0,1,'-');
    return std::u16string(digits.begin(),digits.end());
}
BigInteger scale_coefficient(const BigInteger& coefficient,int scale_change,bool round) {
    static const auto ten=BigInteger::from_unsigned(10);
    if(scale_change==0)return coefficient;
    const auto factor=ten.power(static_cast<unsigned>(std::abs(scale_change)));
    if(scale_change>0)return coefficient.multiply(factor);
    auto [part,discarded]=coefficient.divide(factor);
    if(round && discarded.shift_left(1)>=factor)part=part.add(BigInteger::from_unsigned(1));
    return part;
}
BigInteger signed_coefficient(const SqlDecimal& value,int scale,bool round) {
    auto coefficient=scale_coefficient(value.coefficient,scale-static_cast<int>(value.scale),round);
    return value.positive ? coefficient : coefficient.negated();
}
SqlDecimal decimal_sum(const SqlDecimal& x,const SqlDecimal& y,bool subtract) {
    const auto integral=std::max(x.precision-x.scale,y.precision-y.scale);
    const auto scale=std::min(std::max(x.scale,y.scale),maximum_precision-integral);
    const auto precision=std::min(integral+scale+1,maximum_precision);
    const auto a=signed_coefficient(x,static_cast<int>(scale),true);
    const auto b=signed_coefficient(y,static_cast<int>(scale),true);
    auto combined=subtract ? a.subtract(b) : a.add(b);
    return {combined.absolute(),precision,scale,!combined.negative()};
}
SqlDecimal decimal_product(const SqlDecimal& x,const SqlDecimal& y) {
    const auto actual=static_cast<int>(x.scale+y.scale);
    const auto integral=static_cast<int>(x.precision-x.scale+y.precision-y.scale+1);
    const auto precision=std::min(actual+integral,static_cast<int>(maximum_precision));
    const auto scale=std::max(std::min(precision-integral,actual),std::min(actual,6));
    auto coefficient=scale_coefficient(x.coefficient.multiply(y.coefficient),scale-actual,true);
    const bool positive=coefficient.zero() || x.positive==y.positive;
    return {std::move(coefficient),static_cast<unsigned>(precision),static_cast<unsigned>(scale),positive};
}
SqlDecimal decimal_quotient(const SqlDecimal& x,const SqlDecimal& y,SourceLocation location) {
    if(y.coefficient.zero())throw runtime_error(RuntimeError::division_zero,location);
    const auto wanted_scale=std::max(x.scale+y.precision+1,6U);
    const auto integral=std::min(x.precision-x.scale+y.scale,maximum_precision);
    const auto precision=std::min(integral+wanted_scale,maximum_precision);
    const auto scale=std::max(6U,precision-integral);
    const auto adjusted_scale=scale+y.scale;
    if(adjusted_scale>maximum_precision)throw overflow(location);
    const auto dividend=scale_coefficient(x.coefficient,static_cast<int>(adjusted_scale)-static_cast<int>(x.scale),true);
    if(dividend>=limit())throw overflow(location);
    auto coefficient=dividend.divide(y.coefficient).first;
    const bool positive=coefficient.zero() || x.positive==y.positive;
    return {std::move(coefficient),precision,scale,positive};
}
}

bool sql_decimal_shape(std::span<const TypeInfo> types,const TypeInfo& shape) {
    if(!sql_decimal_record(shape) || shape.kind!=CompoundKind::record || shape.name!="sqldecimal"
        || !compound(shape.element) || !compound(shape.key))return false;
    const auto decimal=static_cast<std::size_t>(shape.element)-64, array=static_cast<std::size_t>(shape.key)-64;
    if(decimal>=types.size() || array>=types.size() || !decimal_record_shape(types[decimal]))return false;
    if(types[array].kind!=CompoundKind::array || types[array].rank!=1 || types[array].element!=Type::integer)return false;
    const std::pair<std::string_view,Type> fields[]{
        {"_defined",Type::boolean},{"_coefficient",Type::big_integer},{"_precision",Type::byte},
        {"_scale",Type::byte},{"_positive",Type::boolean}
    };
    return std::ranges::equal(shape.fields,fields,[](const auto& found,const auto& expected){
        return found.name==expected.first && found.type==expected.second && !found.global;
    });
}
std::vector<Type> sql_decimal_parameters(Type owner,const TypeInfo& shape,Method method) {
    switch(method){
    case Method::from_integer:return {Type::int64};
    case Method::from_decimal:return {shape.element};
    case Method::parse:return {Type::string};
    case Method::null_value:case Method::minimum:case Method::maximum:return {};
    case Method::add:case Method::subtract:case Method::multiply:case Method::divide:case Method::compare:
        return {owner,owner};
    default:return {owner};
    }
}
Type sql_decimal_result(Type owner,const TypeInfo& shape,Method method) {
    switch(method){
    case Method::is_null:case Method::is_positive:return Type::boolean;
    case Method::precision:case Method::scale:return Type::byte;
    case Method::value:return shape.element;
    case Method::data:return shape.key;
    case Method::to_double:return Type::real;
    case Method::to_string:return Type::string;
    case Method::compare:return Type::integer;
    default:return owner;
    }
}
std::u16string sql_decimal_text(const Value& value) {
    return missing(value) ? u"Null" : text(number(value,{}));
}
int compare_sql_decimals(const Value& left,const Value& right,SourceLocation location) {
    if(missing(left))return missing(right) ? 0 : -1;
    if(missing(right))return 1;
    const auto a=number(left,location),b=number(right,location);
    const auto scale=static_cast<int>(std::max(a.scale,b.scale));
    return signed_coefficient(a,scale,false).compare(signed_coefficient(b,scale,false));
}
Value sql_decimal_invoke(Values& values,Type owner,Method method,std::span<const Value> arguments) {
    switch(method){
    case Method::null_value:return values.zero(owner);
    case Method::from_integer:return store(values,owner,integer(BigInteger::from_signed(std::get<std::int64_t>(arguments[0]))));
    case Method::from_decimal:return store(values,owner,from_decimal(arguments[0]));
    case Method::parse: {
        const auto* s=std::get_if<std::u16string>(&arguments[0]);
        if(!s)throw Error("E_NIL",values.location,"SqlDecimal.Parse requires a string","Для SqlDecimal.Parse нужна строка");
        if(*s==u"Null")return values.zero(owner);
        return store(values,owner,parse(*s,values.location));
    }
    case Method::minimum:case Method::maximum:
        return store(values,owner,{limit().subtract(BigInteger::from_unsigned(1)),maximum_precision,0,method==Method::maximum});
    case Method::is_null:return missing(arguments[0]);
    case Method::to_string:return sql_decimal_text(arguments[0]);
    case Method::compare:return static_cast<std::int32_t>(compare_sql_decimals(arguments[0],arguments[1],values.location));
    case Method::negate:{
        if(missing(arguments[0]))return arguments[0];
        auto data=number(arguments[0],values.location);
        data.positive=data.coefficient.zero() || !data.positive;
        return store(values,owner,std::move(data));
    }
    case Method::add:case Method::subtract:case Method::multiply:case Method::divide:{
        if(missing(arguments[0]) || missing(arguments[1]))return values.zero(owner);
        const auto a=number(arguments[0],values.location),b=number(arguments[1],values.location);
        if(method==Method::multiply)return store(values,owner,decimal_product(a,b));
        if(method==Method::divide)return store(values,owner,decimal_quotient(a,b,values.location));
        return store(values,owner,decimal_sum(a,b,method==Method::subtract));
    }
    default:break;
    }
    const auto data=number(arguments[0],values.location);
    switch(method){
    case Method::precision:return static_cast<std::uint8_t>(data.precision);
    case Method::scale:return static_cast<std::uint8_t>(data.scale);
    case Method::is_positive:return data.positive;
    case Method::value: {
        auto bits=words(data.coefficient);
        if(bits[3]!=0 || data.scale>28)throw overflow(values.location);
        bits[3]=(data.scale<<16)|(data.positive ? 0 : 0x80000000U);
        return decimal_from_bits(values,type_info(values.program,owner).element,bits);
    }
    case Method::data: {
        const auto bits=words(data.coefficient);
        auto array=values.array(type_info(values.program,owner).key,4);
        for(std::size_t i=0;i<bits.size();++i)array->items[i]->value=std::bit_cast<std::int32_t>(bits[i]);
        return array;
    }
    case Method::to_double: {
        const auto bits=words(data.coefficient);
        double result=0;
        for(auto word=bits.rbegin();word!=bits.rend();++word)result=result*0x1p32+*word;
        result/=std::pow(10.0,data.scale);
        return data.positive ? result : -result;
    }
    default:throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    }
}
Value sql_decimal_convert(Values& values,Type owner,Type source,const Value& value) {
    if(source==owner)return value;
    if(source==type_info(values.program,owner).element)return store(values,owner,from_decimal(value));
    if(compound(source) && sql_scalar_record(type_info(values.program,source))){
        const auto base=type_info(values.program,source).element;
        if(integer_type(base)){
            const std::array<Value,1> input{value};
            if(std::get<bool>(sql_scalar_invoke(values,source,SqlScalarMethod::is_null,input)))return values.zero(owner);
            return sql_decimal_convert(values,owner,base,sql_scalar_invoke(values,source,SqlScalarMethod::value,input));
        }
    }
    if(integer_type(source) && source!=Type::shortint){
        const auto is_unsigned=wide_unsigned(source) || source==Type::byte || source==Type::word;
        const auto coefficient=is_unsigned ? BigInteger::from_unsigned(unsigned_number(value,values.location))
            : BigInteger::from_signed(integral_number(value,values.location));
        return store(values,owner,integer(coefficient));
    }
    throw Error("E_TYPE",values.location,"Value cannot be converted to SqlDecimal","Значение нельзя преобразовать к SqlDecimal");
}
} // namespace abc

#include "guid.hpp"
#include "runtime_values.hpp"
#include "stdlib.hpp"
#include <compare>

namespace abc {
namespace {
struct GuidParts {
    std::uint32_t first=0;
    std::uint16_t second=0,third=0;
    std::uint64_t tail=0;
    auto operator<=>(const GuidParts&) const=default;
};
constexpr std::array<std::pair<std::string_view,Type>,4> fields{{
    {"_first",Type::longword},{"_second",Type::word},{"_third",Type::word},{"_tail",Type::uint64}}};

Error invalid_guid(SourceLocation location) {
    return Error("E_FORMAT",location,"Invalid Guid text","Некорректная строка Guid");
}
std::u16string_view trim(std::u16string_view value) {
    while(!value.empty() && stdlib::unicode_whitespace(value.front()))value.remove_prefix(1);
    while(!value.empty() && stdlib::unicode_whitespace(value.back()))value.remove_suffix(1);
    return value;
}
unsigned hex_digit(char16_t unit) {
    if(unit>=u'0' && unit<=u'9')return unit-u'0';
    if(unit>=u'A' && unit<=u'F')return unit-u'A'+10;
    if(unit>=u'a' && unit<=u'f')return unit-u'a'+10;
    return 16;
}
std::uint64_t hex(std::u16string_view text,SourceLocation location,std::uint64_t limit=UINT64_MAX) {
    if(text.empty())throw invalid_guid(location);
    std::uint64_t result=0;
    for(const auto unit:text){
        const auto digit=hex_digit(unit);
        if(digit>=16)throw invalid_guid(location);
        if(result>(limit-digit)/16)
            throw Error("E_OVERFLOW",location,"Guid hexadecimal component is too large","Шестнадцатеричный компонент Guid слишком велик");
        result=result*16+digit;
    }
    return result;
}
GuidParts parse_hex_tuple(std::u16string_view input,SourceLocation location) {
    std::u16string text;
    text.reserve(input.size());
    for(const auto unit:input)if(!stdlib::unicode_whitespace(unit))text.push_back(unit);
    std::u16string_view remaining(text);
    const auto expect=[&](char16_t unit){
        if(!remaining.starts_with(unit))throw invalid_guid(location);
        remaining.remove_prefix(1);
    };
    const auto part=[&](char16_t trailing){
        if(!remaining.starts_with(u"0x") && !remaining.starts_with(u"0X"))throw invalid_guid(location);
        remaining.remove_prefix(2);
        const auto end=remaining.find(trailing);
        if(end==remaining.npos)throw invalid_guid(location);
        const auto value=hex(remaining.substr(0,end),location,UINT32_MAX);
        remaining.remove_prefix(end+1);
        return value;
    };
    GuidParts result;
    expect(u'{');
    result.first=static_cast<std::uint32_t>(part(u','));
    result.second=static_cast<std::uint16_t>(part(u','));
    result.third=static_cast<std::uint16_t>(part(u','));
    expect(u'{');
    for(unsigned i=0;i<8;++i){
        const auto byte=part(i==7 ? u'}' : u',');
        if(byte>UINT8_MAX)throw invalid_guid(location);
        result.tail=(result.tail<<8)|byte;
    }
    expect(u'}');
    if(!remaining.empty())throw invalid_guid(location);
    return result;
}
GuidParts parse_guid(std::u16string_view input,SourceLocation location) {
    auto text=trim(input);
    if(text.starts_with(u'{') && text.find(u',')!=text.npos)return parse_hex_tuple(text,location);
    const bool wrapped=text.starts_with(u'{') || text.starts_with(u'(');
    if(wrapped){
        if(text.size()!=38 || (text.front()==u'{' ? text.back()!=u'}' : text.back()!=u')'))throw invalid_guid(location);
        text=text.substr(1,36);
    }
    if(text.size()==32 && !wrapped)
        return {static_cast<std::uint32_t>(hex(text.substr(0,8),location)),
            static_cast<std::uint16_t>(hex(text.substr(8,4),location)),static_cast<std::uint16_t>(hex(text.substr(12,4),location)),
            hex(text.substr(16),location)};
    if(text.size()!=36 || text[8]!=u'-' || text[13]!=u'-' || text[18]!=u'-' || text[23]!=u'-')throw invalid_guid(location);
    return {static_cast<std::uint32_t>(hex(text.substr(0,8),location)),
        static_cast<std::uint16_t>(hex(text.substr(9,4),location)),static_cast<std::uint16_t>(hex(text.substr(14,4),location)),
        (hex(text.substr(19,4),location)<<48)|hex(text.substr(24),location)};
}
GuidParts parts(const Value& value) {
    const auto& record=*std::get<std::shared_ptr<Aggregate>>(value);
    return {std::get<std::uint32_t>(record.items[0]->value),std::get<std::uint16_t>(record.items[1]->value),
        std::get<std::uint16_t>(record.items[2]->value),std::get<std::uint64_t>(record.items[3]->value)};
}
Value make_guid(Values& values,Type type,GuidParts data) {
    auto record=std::get<std::shared_ptr<Aggregate>>(values.zero(type));
    record->items[0]->value=data.first;
    record->items[1]->value=data.second;
    record->items[2]->value=data.third;
    record->items[3]->value=data.tail;
    return record;
}
GuidParts from_bytes(const Value& value,SourceLocation location) {
    const auto& data=*std::get<std::shared_ptr<Aggregate>>(value);
    if(data.nil)throw runtime_error(RuntimeError::nil_reference,location);
    if(data.items.size()!=16)
        throw Error("E_RANGE",location,"Guid requires sixteen bytes","Для Guid требуется шестнадцать байт");
    const auto byte=[&](std::size_t n){return static_cast<std::uint64_t>(std::get<std::uint8_t>(data.items[n]->value));};
    const auto little=[&](std::size_t first,std::size_t count){
        std::uint64_t result=0;
        for(std::size_t i=0;i<count;++i)result|=byte(first+i)<<(8*i);
        return result;
    };
    std::uint64_t tail=0;
    for(std::size_t i=8;i<16;++i)tail=(tail<<8)|byte(i);
    return {static_cast<std::uint32_t>(little(0,4)),static_cast<std::uint16_t>(little(4,2)),
        static_cast<std::uint16_t>(little(6,2)),tail};
}
void append_hex(std::u16string& result,std::uint64_t value,unsigned digits) {
    constexpr std::u16string_view alphabet=u"0123456789abcdef";
    for(unsigned i=digits;i>0;--i)result.push_back(alphabet[(value>>(4*(i-1)))&15U]);
}
} // namespace

bool guid_record_shape(std::span<const TypeInfo> types,const TypeInfo& shape) {
    if(!guid_record_type(shape) || shape.name!="guid" || shape.kind!=CompoundKind::record || shape.key!=Type::invalid
        || shape.fields.size()!=fields.size() || !compound(shape.element)
        || static_cast<unsigned>(shape.element)-64U>=types.size())return false;
    const auto& array=types[static_cast<unsigned>(shape.element)-64U];
    if(array.kind!=CompoundKind::array || array.rank!=1 || array.element!=Type::byte)return false;
    for(std::size_t i=0;i<fields.size();++i){
        const auto& field=shape.fields[i];
        if(field.global || field.name!=fields[i].first || field.type!=fields[i].second)return false;
    }
    return true;
}
std::vector<Type> guid_parameters(Type owner,Type byte_array,GuidMethod method) {
    switch(method){
    case GuidMethod::parse:return {Type::string};
    case GuidMethod::from_bytes:return {byte_array};
    case GuidMethod::compare_to:case GuidMethod::equals:return {owner,owner};
    case GuidMethod::to_string_format:return {owner,Type::string};
    default:return {owner};
    }
}
Type guid_result(Type owner,Type byte_array,GuidMethod method) {
    switch(method){
    case GuidMethod::parse:case GuidMethod::from_bytes:return owner;
    case GuidMethod::compare_to:return Type::integer;
    case GuidMethod::equals:return Type::boolean;
    case GuidMethod::to_bytes:return byte_array;
    default:return Type::string;
    }
}
int compare_guid(const Value& left,const Value& right) {
    const auto result=parts(left)<=>parts(right);
    return (result>0)-(result<0);
}
std::array<std::uint8_t,16> guid_bytes(const Value& value) {
    const auto data=parts(value);
    std::array<std::uint8_t,16> bytes{};
    const auto little=[&](std::size_t first,unsigned count,std::uint32_t word){
        for(unsigned i=0;i<count;++i)bytes[first+i]=static_cast<std::uint8_t>(word>>(8*i));
    };
    little(0,4,data.first);
    little(4,2,data.second);
    little(6,2,data.third);
    for(unsigned i=0;i<8;++i)bytes[8+i]=static_cast<std::uint8_t>(data.tail>>(8*(7-i)));
    return bytes;
}
std::u16string guid_text(const Value& value,std::u16string_view format,SourceLocation location) {
    auto style=format.empty() ? u'd' : format.front();
    if(style>=u'A' && style<=u'Z')style=static_cast<char16_t>(style+32);
    if(format.size()>1 || std::u16string_view(u"ndbpx").find(style)==std::u16string_view::npos)
        throw Error("E_FORMAT",location,"Invalid Guid format","Недопустимый формат Guid");
    const auto data=parts(value);
    std::u16string text;
    if(style==u'x'){
        text=u"{0x";
        append_hex(text,data.first,8);text+=u",0x";
        append_hex(text,data.second,4);text+=u",0x";
        append_hex(text,data.third,4);text+=u",{";
        for(unsigned i=0;i<8;++i){
            if(i)text.push_back(u',');
            text+=u"0x";
            append_hex(text,(data.tail>>(8*(7-i)))&0xffU,2);
        }
        text+=u"}}";
        return text;
    }
    if(style==u'b' || style==u'p')text.push_back(style==u'b' ? u'{' : u'(');
    const auto separator=[&]{if(style!=u'n')text.push_back(u'-');};
    append_hex(text,data.first,8);separator();
    append_hex(text,data.second,4);separator();
    append_hex(text,data.third,4);separator();
    append_hex(text,data.tail>>48,4);separator();
    append_hex(text,data.tail&0x0000ffffffffffffULL,12);
    if(style==u'b' || style==u'p')text.push_back(style==u'b' ? u'}' : u')');
    return text;
}
Value guid_invoke(Values& values,Type type,GuidMethod method,std::span<const Value> arguments) {
    using M=GuidMethod;
    switch(method){
    case M::parse:
        if(std::holds_alternative<NilString>(arguments[0]))throw runtime_error(RuntimeError::nil_reference,values.location);
        return make_guid(values,type,parse_guid(string_text(arguments[0]),values.location));
    case M::from_bytes:return make_guid(values,type,from_bytes(arguments[0],values.location));
    case M::to_string:return guid_text(arguments[0],{},values.location);
    case M::to_string_format:return guid_text(arguments[0],string_text(arguments[1]),values.location);
    case M::compare_to:return static_cast<std::int32_t>(compare_guid(arguments[0],arguments[1]));
    case M::equals:return compare_guid(arguments[0],arguments[1])==0;
    case M::to_bytes:{
        const auto bytes=guid_bytes(arguments[0]);
        auto result=values.array(type_info(values.program,type).element,bytes.size());
        for(std::size_t i=0;i<bytes.size();++i)result->items[i]->value=bytes[i];
        return result;
    }
    }
    throw runtime_error(RuntimeError::invalid_bytecode,values.location);
}
} // namespace abc

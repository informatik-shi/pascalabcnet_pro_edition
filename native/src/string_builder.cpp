#include "string_builder.hpp"
#include "formatting.hpp"
#include "runtime_values.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>

namespace abc {
struct StringBuilderState {
    std::u16string text;
    std::size_t capacity=16;
    std::size_t max_capacity=std::numeric_limits<std::int32_t>::max();
};
namespace {
constexpr std::size_t text_limit=1'048'576;
constexpr std::size_t default_capacity=16;
constexpr std::size_t growth_limit=8000;

Error out_of_range(SourceLocation location) {
    return Error("E_ARGUMENT",location,"StringBuilder index, length or capacity is out of range",
        "Индекс, длина или ёмкость StringBuilder вне допустимого диапазона");
}
std::size_t nonnegative(const Value& input,SourceLocation location) {
    const auto number=std::get<std::int32_t>(input);
    if (number<0) throw out_of_range(location);
    return static_cast<std::size_t>(number);
}
std::u16string_view slice(std::u16string_view text,std::size_t start,std::size_t count,SourceLocation location) {
    if (start>text.size() || count>text.size()-start) throw out_of_range(location);
    return text.substr(start,count);
}
std::u16string scalar_text(const Value& value,SourceLocation location) {
    if (!std::holds_alternative<float>(value) && !std::holds_alternative<double>(value))
        return format_field(value,0,std::nullopt,location);
    const auto single=std::holds_alternative<float>(value);
    const auto number=single?static_cast<double>(std::get<float>(value)):std::get<double>(value);
    if (number==0) return u"0";
    if (std::isnan(number)) return u"NaN";
    if (std::isinf(number)) return number<0?u"-Infinity":u"Infinity";
    char buffer[64];
    const auto result=std::to_chars(buffer,buffer+sizeof buffer,number,std::chars_format::general,single?7:15);
    if (result.ec!=std::errc{}) throw runtime_error(RuntimeError::number_format,location);
    std::replace(buffer,result.ptr,'e','E');
    return std::u16string(buffer,result.ptr);
}
std::size_t appended_capacity(const StringBuilderState& builder,std::size_t count,SourceLocation location) {
    if (count>text_limit-builder.text.size()) throw runtime_error(RuntimeError::string_limit,location);
    const auto length=builder.text.size()+count;
    if (length<=builder.capacity) return builder.capacity;
    if (length>builder.max_capacity) throw out_of_range(location);
    return builder.capacity+std::max(length-builder.capacity,std::min(builder.capacity,growth_limit));
}
void append_text(StringBuilderState& builder,std::u16string_view text,SourceLocation location) {
    const auto capacity=appended_capacity(builder,text.size(),location);
    builder.text.append(text);
    builder.capacity=capacity;
}
void append_repeated(StringBuilderState& builder,char16_t character,std::size_t count,SourceLocation location) {
    const auto capacity=appended_capacity(builder,count,location);
    builder.text.append(count,character);
    builder.capacity=capacity;
}
void append_array(StringBuilderState& builder,const Value& argument,std::size_t start,
    std::optional<std::size_t> count,SourceLocation location) {
    const auto& array=std::get<std::shared_ptr<Aggregate>>(argument);
    if (!array || array->nil) {
        if (start || count.value_or(0)) throw runtime_error(RuntimeError::nil_reference,location);
        return;
    }
    const auto size=array->items.size();
    if (start>size || count.value_or(size)>size-start) throw out_of_range(location);
    const auto amount=count.value_or(size);
    const auto capacity=appended_capacity(builder,amount,location);
    builder.text.reserve(builder.text.size()+amount);
    for (std::size_t i=0;i<amount;++i)
        builder.text.push_back(std::get<char16_t>(array->items[start+i]->value));
    builder.capacity=capacity;
}
void set_capacity(StringBuilderState& builder,std::size_t capacity,SourceLocation location) {
    if (capacity<builder.text.size() || capacity>builder.max_capacity) throw out_of_range(location);
    if (capacity>text_limit) throw runtime_error(RuntimeError::string_limit,location);
    builder.capacity=capacity;
}
StringBuilderState& state(const std::shared_ptr<Aggregate>& value,SourceLocation location) {
    if (!value || value->nil) throw runtime_error(RuntimeError::nil_reference,location);
    if (!value->builder) throw runtime_error(RuntimeError::invalid_bytecode,location);
    return *value->builder;
}
Value construct(Values& values,Type owner,StringBuilderMethod method,std::span<const Value> args) {
    const auto location=values.location;
    std::size_t capacity=default_capacity;
    std::size_t maximum=std::numeric_limits<std::int32_t>::max();
    std::u16string_view text;
    switch (method) {
    case StringBuilderMethod::create_limited:
        maximum=nonnegative(args[1],location);
        if (maximum==0) throw out_of_range(location);
        [[fallthrough]];
    case StringBuilderMethod::create_capacity: capacity=nonnegative(args[0],location); break;
    case StringBuilderMethod::create_text: text=string_text(args[0]); break;
    case StringBuilderMethod::create_text_capacity:
        text=string_text(args[0]);
        capacity=nonnegative(args[1],location);
        break;
    case StringBuilderMethod::create_slice:
        text=slice(string_text(args[0]),nonnegative(args[1],location),nonnegative(args[2],location),location);
        capacity=nonnegative(args[3],location);
        break;
    default: break;
    }
    if (capacity>maximum) throw out_of_range(location);
    if (capacity==0) capacity=std::min(default_capacity,maximum);
    capacity=std::max(capacity,text.size());
    if (capacity>text_limit) throw runtime_error(RuntimeError::string_limit,location);
    if (!values.remaining) throw runtime_error(RuntimeError::allocation_limit,location);
    auto object=std::make_shared<Aggregate>();
    object->type=owner;
    object->builder=std::make_shared<StringBuilderState>(StringBuilderState{std::u16string(text),capacity,maximum});
    --values.remaining;
    return object;
}
}

bool string_builder_signature(const Program& program,Type owner,std::size_t operation) {
    using M=StringBuilderMethod;
    if (!compound(owner) || static_cast<unsigned>(owner)-64U>=program.types.size()
        || type_info(program,owner).kind!=CompoundKind::string_builder
        || (operation>>8U)>std::numeric_limits<std::uint16_t>::max()) return false;
    const auto method=static_cast<M>(operation&255U);
    const auto payload=static_cast<Type>(operation>>8U);
    if (method==M::append_scalar)
        return integer_type(payload) || payload==Type::single || payload==Type::real || payload==Type::boolean;
    if (method==M::append_array || method==M::append_array_range || method==M::copy_to) {
        if (!compound(payload) || static_cast<unsigned>(payload)-64U>=program.types.size()) return false;
        const auto& shape=type_info(program,payload);
        return shape.kind==CompoundKind::array && shape.rank==1 && shape.element==Type::character;
    }
    return method<=M::replace_char_range && (operation>>8U)==0;
}
std::vector<Type> string_builder_parameters(Type owner,StringBuilderMethod method,Type payload) {
    using M=StringBuilderMethod;
    switch (method) {
    case M::create_empty: return {};
    case M::create_capacity: return {Type::integer};
    case M::create_limited: return {Type::integer,Type::integer};
    case M::create_text: return {Type::string};
    case M::create_text_capacity: return {Type::string,Type::integer};
    case M::create_slice: return {Type::string,Type::integer,Type::integer,Type::integer};
    case M::ensure_capacity: case M::set_length: case M::set_capacity: case M::get_char:
        return {owner,Type::integer};
    case M::set_char: return {owner,Type::integer,Type::character};
    case M::text_range: return {owner,Type::integer,Type::integer};
    case M::append_text: case M::append_line_text: return {owner,Type::string};
    case M::append_char: return {owner,Type::character};
    case M::append_repeated: return {owner,Type::character,Type::integer};
    case M::append_slice: return {owner,Type::string,Type::integer,Type::integer};
    case M::equals: case M::append_builder: return {owner,owner};
    case M::append_scalar: case M::append_array: return {owner,payload};
    case M::append_array_range: return {owner,payload,Type::integer,Type::integer};
    case M::copy_to: return {owner,Type::integer,payload,Type::integer,Type::integer};
    case M::replace_char: return {owner,Type::character,Type::character};
    case M::replace_char_range: return {owner,Type::character,Type::character,Type::integer,Type::integer};
    default: return {owner};
    }
}
Type string_builder_result(Type owner,StringBuilderMethod method) {
    using M=StringBuilderMethod;
    switch (method) {
    case M::length: case M::capacity: case M::max_capacity: case M::ensure_capacity: return Type::integer;
    case M::set_length: case M::set_capacity: case M::set_char: case M::copy_to: return Type::unit;
    case M::text: case M::text_range: return Type::string;
    case M::get_char: return Type::character;
    case M::equals: return Type::boolean;
    default: return owner;
    }
}
Value string_builder_invoke(Values& values,Type owner,StringBuilderMethod method,std::span<const Value> args) {
    using M=StringBuilderMethod;
    if (method<=M::create_slice) return construct(values,owner,method,args);
    const auto location=values.location;
    auto& builder=state(std::get<std::shared_ptr<Aggregate>>(args[0]),location);
    const auto number=[&](std::size_t n){return nonnegative(args[n],location);};
    const auto length=builder.text.size();
    switch (method) {
    case M::length: return static_cast<std::int32_t>(length);
    case M::capacity: return static_cast<std::int32_t>(builder.capacity);
    case M::max_capacity: return static_cast<std::int32_t>(builder.max_capacity);
    case M::ensure_capacity:
        if (const auto requested=number(1); requested>builder.capacity) set_capacity(builder,requested,location);
        return static_cast<std::int32_t>(builder.capacity);
    case M::set_capacity: set_capacity(builder,number(1),location); return false;
    case M::set_length: {
        const auto proposed=number(1);
        if (proposed>builder.max_capacity) throw out_of_range(location);
        if (proposed>length) append_repeated(builder,0,proposed-length,location);
        else builder.text.resize(proposed);
        return false;
    }
    case M::get_char: {
        const auto index=std::get<std::int32_t>(args[1]);
        if (index<0 || static_cast<std::size_t>(index)>=length)
            throw runtime_error(RuntimeError::index_bounds,location);
        return builder.text[static_cast<std::size_t>(index)];
    }
    case M::set_char: {
        const auto index=number(1);
        if (index>=length) throw out_of_range(location);
        builder.text[index]=std::get<char16_t>(args[2]);
        return false;
    }
    case M::clear: builder.text.clear(); break;
    case M::text: return builder.text;
    case M::text_range: return std::u16string(slice(builder.text,number(1),number(2),location));
    case M::append_text: append_text(builder,string_text(args[1]),location); break;
    case M::append_char: append_repeated(builder,std::get<char16_t>(args[1]),1,location); break;
    case M::append_repeated: append_repeated(builder,std::get<char16_t>(args[1]),number(2),location); break;
    case M::append_slice:
        append_text(builder,slice(string_text(args[1]),number(2),number(3),location),location);
        break;
    case M::append_scalar: append_text(builder,scalar_text(args[1],location),location); break;
    case M::append_array: append_array(builder,args[1],0,std::nullopt,location); break;
    case M::append_array_range: append_array(builder,args[1],number(2),number(3),location); break;
    case M::append_builder: {
        const auto& other=std::get<std::shared_ptr<Aggregate>>(args[1]);
        if (other && !other->nil) {
            const auto text=state(other,location).text;
            append_text(builder,text,location);
        }
        break;
    }
    case M::copy_to: {
        const auto& target=std::get<std::shared_ptr<Aggregate>>(args[2]);
        if (!target || target->nil) throw runtime_error(RuntimeError::nil_reference,location);
        const auto start=number(1);
        const auto destination=number(3);
        const auto count=number(4);
        const auto content=slice(builder.text,start,count,location);
        if (destination>target->items.size() || count>target->items.size()-destination)
            throw out_of_range(location);
        for (std::size_t i=0;i<count;++i) values.write(target->items[destination+i],content[i]);
        return false;
    }
    case M::replace_char: case M::replace_char_range: {
        const auto start=method==M::replace_char?0U:number(3);
        const auto count=method==M::replace_char?length:number(4);
        (void)slice(builder.text,start,count,location);
        auto begin=builder.text.begin()+static_cast<std::ptrdiff_t>(start);
        std::replace(begin,begin+static_cast<std::ptrdiff_t>(count),
            std::get<char16_t>(args[1]),std::get<char16_t>(args[2]));
        break;
    }
    case M::append_line: case M::append_line_text:
        if (method==M::append_line_text) append_text(builder,string_text(args[1]),location);
#ifdef _WIN32
        append_text(builder,u"\r\n",location);
#else
        append_text(builder,u"\n",location);
#endif
        break;
    case M::equals: {
        const auto& other=std::get<std::shared_ptr<Aggregate>>(args[1]);
        if (!other || other->nil) return false;
        const auto& compared=state(other,location);
        return builder.capacity==compared.capacity && builder.max_capacity==compared.max_capacity && builder.text==compared.text;
    }
    default: throw runtime_error(RuntimeError::invalid_bytecode,location);
    }
    return args[0];
}
}

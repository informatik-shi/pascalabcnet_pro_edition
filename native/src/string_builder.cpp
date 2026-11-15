#include "string_builder.hpp"
#include "runtime_values.hpp"
#include <algorithm>
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

bool string_builder_signature(const Program& program,Type owner,std::size_t method) {
    return compound(owner) && static_cast<unsigned>(owner)-64U<program.types.size()
        && type_info(program,owner).kind==CompoundKind::string_builder
        && method<=static_cast<std::size_t>(StringBuilderMethod::equals);
}
std::vector<Type> string_builder_parameters(Type owner,StringBuilderMethod method) {
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
    case M::equals: return {owner,owner};
    default: return {owner};
    }
}
Type string_builder_result(Type owner,StringBuilderMethod method) {
    using M=StringBuilderMethod;
    switch (method) {
    case M::length: case M::capacity: case M::max_capacity: case M::ensure_capacity: return Type::integer;
    case M::set_length: case M::set_capacity: case M::set_char: return Type::unit;
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

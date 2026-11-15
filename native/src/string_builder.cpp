#include "string_builder.hpp"
#include "composite_format.hpp"
#include "number_format.hpp"
#include "runtime_values.hpp"
#include <algorithm>
#include <limits>

namespace abc {
struct StringBuilderState {
    struct Chunk {std::size_t length,capacity;};
    std::u16string text;
    std::vector<Chunk> chunks;
    std::size_t max_capacity;

    std::size_t capacity() const {
        return text.size()-chunks.back().length+chunks.back().capacity;
    }
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
    const auto capacity=builder.capacity();
    if (length<=capacity) return capacity;
    if (length>builder.max_capacity) throw out_of_range(location);
    return capacity+std::max(length-capacity,std::min(capacity,growth_limit));
}
template<class Append>
void extend(StringBuilderState& builder,std::size_t amount,SourceLocation location,Append append) {
    const auto capacity=builder.capacity();
    const auto next_capacity=appended_capacity(builder,amount,location);
    const auto current=std::min(amount,capacity-builder.text.size());
    if (next_capacity!=capacity) builder.chunks.reserve(builder.chunks.size()+1);
    append();
    builder.chunks.back().length+=current;
    if (next_capacity!=capacity) builder.chunks.push_back({amount-current,next_capacity-capacity});
}
void append_text(StringBuilderState& builder,std::u16string_view text,SourceLocation location) {
    extend(builder,text.size(),location,[&]{builder.text.append(text);});
}
void append_repeated(StringBuilderState& builder,char16_t character,std::size_t count,SourceLocation location) {
    extend(builder,count,location,[&]{builder.text.append(count,character);});
}
std::u16string array_text(const Value& argument,std::size_t start,std::optional<std::size_t> count,
    SourceLocation location) {
    const auto& array=std::get<std::shared_ptr<Aggregate>>(argument);
    if (!array || array->nil) {
        if (start || count.value_or(0)) throw runtime_error(RuntimeError::nil_reference,location);
        return {};
    }
    const auto size=array->items.size();
    if (start>size || count.value_or(size)>size-start) throw out_of_range(location);
    const auto amount=count.value_or(size);
    if (amount>text_limit) throw runtime_error(RuntimeError::string_limit,location);
    std::u16string text(amount,0);
    for (std::size_t i=0;i<amount;++i) text[i]=std::get<char16_t>(array->items[start+i]->value);
    return text;
}
void set_capacity(StringBuilderState& builder,std::size_t capacity,SourceLocation location) {
    if (capacity<builder.text.size() || capacity>builder.max_capacity) throw out_of_range(location);
    if (capacity>text_limit) throw runtime_error(RuntimeError::string_limit,location);
    builder.chunks.back().capacity=capacity-builder.text.size()+builder.chunks.back().length;
}
struct Position {std::size_t chunk,offset;};
Position locate(const std::vector<StringBuilderState::Chunk>& chunks,std::size_t length,std::size_t index) {
    auto offset=length;
    auto chunk=chunks.size();
    do {
        --chunk;
        offset-=chunks[chunk].length;
    } while (offset>index);
    return {chunk,offset};
}
void shrink(StringBuilderState& builder,std::size_t length) {
    const auto capacity=builder.capacity();
    const auto position=locate(builder.chunks,builder.text.size(),length);
    builder.chunks.resize(position.chunk+1);
    builder.chunks.back()={length-position.offset,capacity-position.offset};
    builder.text.resize(length);
}
void insert_text(StringBuilderState& builder,std::size_t index,std::u16string_view text,
    std::size_t repetitions,SourceLocation location) {
    if (index>builder.text.size()) throw out_of_range(location);
    if (text.empty() || !repetitions) return;
    if (repetitions>(text_limit-builder.text.size())/text.size())
        throw runtime_error(RuntimeError::string_limit,location);
    const auto count=text.size()*repetitions;
    if (builder.text.size()+count>builder.max_capacity) throw out_of_range(location);
    const auto position=locate(builder.chunks,builder.text.size(),index);
    const auto chunk=builder.chunks[position.chunk];
    const auto in_place=chunk.length<=32 && count<=chunk.capacity-chunk.length;
    if (!in_place) builder.chunks.reserve(builder.chunks.size()+1);
    if (repetitions==1) builder.text.insert(index,text);
    else {
        std::u16string repeated;
        repeated.reserve(count);
        for (std::size_t n=0;n<repetitions;++n) repeated.append(text);
        builder.text.insert(index,repeated);
    }
    if (in_place) builder.chunks[position.chunk].length+=count;
    else builder.chunks.insert(builder.chunks.begin()+static_cast<std::ptrdiff_t>(position.chunk),
        {count,std::max(count,default_capacity)});
}
void remove_chunks(std::vector<StringBuilderState::Chunk>& chunks,std::size_t length,std::size_t index,std::size_t count) {
    const auto first=locate(chunks,length,index);
    const auto last=locate(chunks,length,index+count);
    if (first.chunk==last.chunk) chunks[first.chunk].length-=count;
    else {
        chunks[first.chunk].length=index-first.offset;
        chunks[last.chunk].length-=index+count-last.offset;
        const auto erase_from=first.chunk+(chunks[first.chunk].length!=0);
        chunks.erase(chunks.begin()+static_cast<std::ptrdiff_t>(erase_from),
            chunks.begin()+static_cast<std::ptrdiff_t>(last.chunk));
    }
}
void remove_text(StringBuilderState& builder,std::size_t index,std::size_t count,SourceLocation location) {
    (void)slice(builder.text,index,count,location);
    if (!count) return;
    if (count==builder.text.size()) {
        shrink(builder,0);
        return;
    }
    remove_chunks(builder.chunks,builder.text.size(),index,count);
    builder.text.erase(index,count);
}
std::vector<std::size_t> matching_spans(std::u16string_view text,std::u16string_view needle) {
    if (needle.size()>text.size()) return {};
    std::vector<std::size_t> prefix(needle.size());
    for (std::size_t i=1,j=0;i<needle.size();++i) {
        while (j && needle[i]!=needle[j]) j=prefix[j-1];
        if (needle[i]==needle[j]) ++j;
        prefix[i]=j;
    }
    std::vector<std::size_t> spans;
    for (std::size_t i=0,j=0;i<text.size();++i) {
        while (j && text[i]!=needle[j]) j=prefix[j-1];
        if (text[i]==needle[j]) ++j;
        if (j==needle.size()) {
            spans.push_back(i+1-j);
            j=0;
        }
    }
    return spans;
}
void replace_text(StringBuilderState& builder,std::size_t start,std::size_t count,
    const Value& old_value,std::u16string_view replacement,SourceLocation location) {
    const auto& text=builder.text;
    const auto region=slice(text,start,count,location);
    if (std::holds_alternative<NilString>(old_value)) throw runtime_error(RuntimeError::nil_reference,location);
    const auto& needle=string_text(old_value);
    if (needle.empty())
        throw Error("E_ARGUMENT",location,"The substring to replace must not be empty",
            "Подстрока для замены должна быть непустой");
    const auto positions=matching_spans(region,needle);
    if (positions.empty()) return;

    auto chunks=builder.chunks;
    auto length=text.size();
    const auto grows=replacement.size()>needle.size();
    const auto width=grows?replacement.size()-needle.size():needle.size()-replacement.size();
    // Matches beginning in the same chunk change its layout as one edit.
    for (std::size_t batch=0;width && batch<positions.size();) {
        const auto adjust=[&](std::size_t index) {
            return grows?index+(length-text.size()):index-(text.size()-length);
        };
        const auto first=adjust(start+positions[batch]);
        const auto position=locate(chunks,length,first);
        const auto end=position.offset+chunks[position.chunk].length;
        auto next=batch+1;
        while (next<positions.size() && adjust(start+positions[next])<end) ++next;
        const auto matches=next-batch;
        if (grows) {
            if (matches>(text_limit-length)/width) throw runtime_error(RuntimeError::string_limit,location);
            const auto extra=matches*width;
            if (length+extra>builder.max_capacity) throw out_of_range(location);
            chunks.insert(chunks.begin()+static_cast<std::ptrdiff_t>(position.chunk),
                {extra,std::max(extra,default_capacity)});
            length+=extra;
        } else {
            const auto removed=matches*width;
            const auto tail=adjust(start+positions[next-1])+needle.size();
            remove_chunks(chunks,length,tail-removed,removed);
            length-=removed;
        }
        batch=next;
    }
    std::u16string result;
    result.reserve(length);
    std::size_t copied=0;
    for (const auto relative:positions) {
        const auto position=start+relative;
        result.append(text,copied,position-copied);
        result.append(replacement);
        copied=position+needle.size();
    }
    result.append(text,copied);
    builder.text=std::move(result);
    builder.chunks=std::move(chunks);
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
    object->builder=std::make_shared<StringBuilderState>(StringBuilderState{std::u16string(text),{{text.size(),capacity}},maximum});
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
    if (method==M::append_scalar || method==M::insert_scalar)
        return integer_type(payload) || payload==Type::single || payload==Type::real || payload==Type::boolean;
    if (method==M::append_array || method==M::append_array_range || method==M::copy_to
        || method==M::insert_array || method==M::insert_array_range) {
        if (!compound(payload) || static_cast<unsigned>(payload)-64U>=program.types.size()) return false;
        const auto& shape=type_info(program,payload);
        return shape.kind==CompoundKind::array && shape.rank==1 && shape.element==Type::character;
    }
    if (method==M::append_format || method==M::format || method==M::append_format_provider) {
        if (method==M::append_format_provider && known_number_format_type(program)==Type::invalid) return false;
        if (!compound(payload) || static_cast<unsigned>(payload)-64U>=program.types.size()) return false;
        const auto& shape=type_info(program,payload);
        return shape.kind==CompoundKind::array && shape.rank==1 && compound(shape.element)
            && static_cast<unsigned>(shape.element)-64U<program.types.size()
            && type_info(program,shape.element).kind==CompoundKind::boxed_object;
    }
    return method<=M::replace_text_range && (operation>>8U)==0;
}
std::vector<Type> string_builder_parameters(Type owner,StringBuilderMethod method,Type payload,Type provider) {
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
    case M::insert_text: return {owner,Type::integer,Type::string};
    case M::insert_repeated: return {owner,Type::integer,Type::string,Type::integer};
    case M::insert_scalar: case M::insert_array: return {owner,Type::integer,payload};
    case M::insert_array_range: return {owner,Type::integer,payload,Type::integer,Type::integer};
    case M::remove: return {owner,Type::integer,Type::integer};
    case M::replace_text: return {owner,Type::string,Type::string};
    case M::replace_text_range: return {owner,Type::string,Type::string,Type::integer,Type::integer};
    case M::append_format: return {owner,Type::string,payload};
    case M::append_format_provider: return {owner,provider,Type::string,payload};
    case M::format: return {Type::string,payload};
    default: return {owner};
    }
}
Type string_builder_result(Type owner,StringBuilderMethod method) {
    using M=StringBuilderMethod;
    switch (method) {
    case M::length: case M::capacity: case M::max_capacity: case M::ensure_capacity: return Type::integer;
    case M::set_length: case M::set_capacity: case M::set_char: case M::copy_to: return Type::unit;
    case M::text: case M::text_range: case M::format: return Type::string;
    case M::get_char: return Type::character;
    case M::equals: return Type::boolean;
    default: return owner;
    }
}
Value string_builder_invoke(Values& values,Type owner,StringBuilderMethod method,std::span<const Value> args) {
    using M=StringBuilderMethod;
    if (method<=M::create_slice) return construct(values,owner,method,args);
    const auto location=values.location;
    if (method==M::format) {
        static const auto pabc=[] {
            NumberFormatSettings settings;
            settings.group=u".";
            return settings;
        }();
        std::u16string text;
        composite_format(values.program,args[0],args[1],pabc,location,[&](std::u16string_view part) {
            if (part.size()>text_limit-text.size()) throw runtime_error(RuntimeError::string_limit,location);
            text.append(part);
        });
        return text;
    }
    auto& builder=state(std::get<std::shared_ptr<Aggregate>>(args[0]),location);
    const auto number=[&](std::size_t n){return nonnegative(args[n],location);};
    const auto length=builder.text.size();
    switch (method) {
    case M::length: return static_cast<std::int32_t>(length);
    case M::capacity: return static_cast<std::int32_t>(builder.capacity());
    case M::max_capacity: return static_cast<std::int32_t>(builder.max_capacity);
    case M::ensure_capacity:
        if (const auto requested=number(1); requested>builder.capacity()) set_capacity(builder,requested,location);
        return static_cast<std::int32_t>(builder.capacity());
    case M::set_capacity: set_capacity(builder,number(1),location); return false;
    case M::set_length: {
        const auto proposed=number(1);
        if (proposed>builder.max_capacity) throw out_of_range(location);
        if (proposed>length) append_repeated(builder,0,proposed-length,location);
        else shrink(builder,proposed);
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
    case M::clear: shrink(builder,0); break;
    case M::text: return builder.text;
    case M::text_range: return std::u16string(slice(builder.text,number(1),number(2),location));
    case M::append_text: append_text(builder,string_text(args[1]),location); break;
    case M::append_char: append_repeated(builder,std::get<char16_t>(args[1]),1,location); break;
    case M::append_repeated: append_repeated(builder,std::get<char16_t>(args[1]),number(2),location); break;
    case M::append_slice:
        append_text(builder,slice(string_text(args[1]),number(2),number(3),location),location);
        break;
    case M::append_scalar:
        append_text(builder,localize_number(format_scalar(args[1],location),number_settings(values),location),location);
        break;
    case M::append_array: append_text(builder,array_text(args[1],0,std::nullopt,location),location); break;
    case M::append_array_range: append_text(builder,array_text(args[1],number(2),number(3),location),location); break;
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
    case M::insert_text: case M::insert_repeated:
        insert_text(builder,number(1),string_text(args[2]),method==M::insert_text?1U:number(3),location);
        break;
    case M::insert_scalar:
        insert_text(builder,number(1),localize_number(format_scalar(args[2],location),number_settings(values),location),1,location);
        break;
    case M::insert_array:
        insert_text(builder,number(1),array_text(args[2],0,std::nullopt,location),1,location);
        break;
    case M::insert_array_range:
        insert_text(builder,number(1),array_text(args[2],number(3),number(4),location),1,location);
        break;
    case M::remove:
        remove_text(builder,number(1),number(2),location);
        break;
    case M::replace_text: case M::replace_text_range: {
        const auto start=method==M::replace_text?0U:number(3);
        const auto count=method==M::replace_text?length:number(4);
        replace_text(builder,start,count,args[1],string_text(args[2]),location);
        break;
    }
    case M::append_format: case M::append_format_provider: {
        const auto provided=method==M::append_format_provider;
        const auto& settings=number_settings(values,provided?&args[1]:nullptr);
        const auto at=provided?2U:1U;
        composite_format(values.program,args[at],args[at+1],settings,location,[&](std::u16string_view part) {
            append_text(builder,part,location);
        });
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
        return builder.capacity()==compared.capacity() && builder.max_capacity==compared.max_capacity && builder.text==compared.text;
    }
    default: throw runtime_error(RuntimeError::invalid_bytecode,location);
    }
    return args[0];
}
}

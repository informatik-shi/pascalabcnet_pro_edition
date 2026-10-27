#include "formatting.hpp"
#include "diagnostics.hpp"
#include "stdlib.hpp"
#include "big_integer_runtime.hpp"
#include <array>
#include <charconv>
#include <type_traits>
#include <algorithm>

namespace abc {
namespace {
bool supports(const std::vector<TypeInfo>& types, Type type, unsigned depth) {
    if (scalar(type) || type==Type::nil) return true;
    if (!compound(type) || depth>32) return false;
    const auto index=static_cast<std::size_t>(type)-static_cast<std::size_t>(Type::first_compound);
    if (index>=types.size()) return false;
    const auto& info=types[index];
    if (info.kind==CompoundKind::record || info.kind==CompoundKind::tuple) {
        for (const auto& field:info.fields) if (!supports(types,field.type,depth+1)) return false;
        return true;
    }
    return (info.kind==CompoundKind::array || info.kind==CompoundKind::list) && supports(types,info.element,depth+1);
}
class ObjectWriter {
    const Program& program;
    SourceLocation location;
    const ExecutionOptions& options;
    std::u16string text;
    std::vector<const Aggregate*> path;
    void append(std::u16string_view part) {
        if (part.size()>1'048'576-text.size()) throw runtime_error(RuntimeError::string_limit,location);
        text+=part;
    }
    void value(const Value& data, Type type) {
        if (options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,location);
        if (!compound(type)) { append(type==Type::nil ? u"nil" : format_field(data,0,std::nullopt,location)); return; }
        const auto& object=std::get<std::shared_ptr<Aggregate>>(data);
        if (!object || object->nil) { append(u"nil"); return; }
        if (std::find(path.begin(),path.end(),object.get())!=path.end()) { append(u"(...)"); return; }
        path.push_back(object.get());
        const auto& info=program.types[static_cast<std::size_t>(type)-static_cast<std::size_t>(Type::first_compound)];
        if (info.kind==CompoundKind::record || info.kind==CompoundKind::tuple) {
            append(u"(");
            for (std::size_t i=0;i<info.fields.size();++i) {
                if (i) append(u",");
                value(object->items[i]->value,info.fields[i].type);
            }
            append(u")");
        } else if (info.kind==CompoundKind::array && info.rank==2) {
            if (object->items.empty()) append(u"[[]]");
            else sequence(object->rows,99,[&](std::size_t row) {
                sequence(object->columns,99,[&](std::size_t column) { value(object->items[row*object->columns+column]->value,info.element); });
            });
        } else sequence(object->items.size(),info.kind==CompoundKind::list ? 100 : 99,
            [&](std::size_t i) { value(object->items[i]->value,info.element); });
        path.pop_back();
    }
    template<class F> void sequence(std::size_t size, std::size_t cutoff, F element) {
        append(u"[");
        const bool trimmed=size>cutoff+1;
        const auto count=trimmed ? cutoff : size;
        for (std::size_t i=0;i<count;++i) { if (i) append(u","); element(i); }
        if (trimmed) append(u",...");
        append(u"]");
    }
public:
    ObjectWriter(const Program& p, SourceLocation l, const ExecutionOptions& o):program(p),location(l),options(o) {}
    std::u16string render(const Value& data, Type type) { value(data,type); return std::move(text); }
};
}
bool object_text_supported(const std::vector<TypeInfo>& types, Type type) { return supports(types,type,0); }
std::u16string object_text(const Program& program, const Value& value, Type type, SourceLocation location, const ExecutionOptions& options) {
    return ObjectWriter(program,location,options).render(value,type);
}
std::u16string pad_field(std::u16string text, std::int32_t width, SourceLocation location) {
    if (width<0) throw runtime_error(RuntimeError::field_width,location);
    if (static_cast<std::size_t>(width)>1'048'576) throw runtime_error(RuntimeError::string_limit,location);
    if (static_cast<std::size_t>(width)>text.size()) text.insert(0,static_cast<std::size_t>(width)-text.size(),u' ');
    return text;
}
std::u16string format_field(const Value& value, std::int32_t width, std::optional<std::int32_t> digits, SourceLocation location) {
    constexpr std::size_t limit=1'048'576;
    if (!digits && width<0) throw runtime_error(RuntimeError::field_width,location);
    const auto size=static_cast<std::uint64_t>(width<0 ? -std::int64_t(width) : width);
    if (size>limit) throw runtime_error(RuntimeError::string_limit,location);
    std::u16string text;
    if (digits) {
        if (*digits==INT32_MIN) throw runtime_error(RuntimeError::integer_overflow,location);
        const auto precision=*digits<0 ? -*digits : *digits;
        if (static_cast<std::size_t>(precision)>limit-2) throw runtime_error(RuntimeError::string_limit,location);
        std::string bytes(static_cast<std::size_t>(precision)+512,'\0');
        const auto converted=std::to_chars(bytes.data(),bytes.data()+bytes.size(),std::get<double>(value),std::chars_format::fixed,precision);
        if (converted.ec!=std::errc{}) throw runtime_error(RuntimeError::number_format,location);
        text=from_utf8(std::string_view(bytes.data(),static_cast<std::size_t>(converted.ptr-bytes.data())));
    } else if (const auto* big=std::get_if<BigInteger>(&value)) text=checked_big(location,[&]{return from_utf8(big->text());});
    else if (const auto* string=std::get_if<std::u16string>(&value)) text=*string;
    else if (std::holds_alternative<NilString>(value)) text=u"nil";
    else if (const auto* ch=std::get_if<char16_t>(&value)) text+=*ch;
    else if (const auto* flag=std::get_if<bool>(&value)) text=*flag ? u"True" : u"False";
    else if (std::holds_alternative<double>(value)) {
        const std::array<Value,1> arguments{value};
        text=std::get<std::u16string>(*stdlib::invoke_text(stdlib::Function::float_to_str,arguments,location));
    } else text=std::visit([&](const auto& n)->std::u16string {
        using T=std::decay_t<decltype(n)>;
        if constexpr (std::is_integral_v<T> && !std::is_same_v<T,bool> && !std::is_same_v<T,char16_t>) {
            char bytes[32]; const auto converted=std::to_chars(bytes,bytes+sizeof bytes,n);
            if (converted.ec!=std::errc{}) throw runtime_error(RuntimeError::number_format,location);
            return from_utf8(std::string_view(bytes,static_cast<std::size_t>(converted.ptr-bytes)));
        } else throw runtime_error(RuntimeError::invalid_bytecode,location);
    },value);
    if (text.size()>limit) throw runtime_error(RuntimeError::string_limit,location);
    if (size>text.size()) {
        const auto count=static_cast<std::size_t>(size)-text.size();
        if (width<0) text.append(count,u' '); else text.insert(0,count,u' ');
    }
    return text;
}
} // namespace abc

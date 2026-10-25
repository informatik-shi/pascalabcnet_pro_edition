#include "console_input.hpp"
#include "diagnostics.hpp"
#include "number.hpp"
#include <charconv>
#include <type_traits>

namespace abc {
void ConsoleInput::check(SourceLocation location) const {
    if (options_.cancelled && options_.cancelled()) throw runtime_error(RuntimeError::cancelled,location);
}
bool ConsoleInput::ensure(SourceLocation location) {
    if (cursor_<line_.size()) return true;
    if (ended_) return false;
    std::string bytes;
    char c;
    while (input_.get(c)) {
        if ((bytes.size()&255U)==0) check(location);
        if (c=='\n') { bytes+=c; break; }
        if (bytes.size()==1'048'576) throw runtime_error(RuntimeError::input_limit,location);
        bytes+=c;
    }
    if (input_.bad()) throw runtime_error(RuntimeError::input_stream,location);
    ended_=input_.eof();
    try { line_=from_utf8(bytes); }
    catch (const std::invalid_argument&) { throw runtime_error(RuntimeError::input_encoding,location); }
    cursor_=0;
    return !line_.empty();
}
void ConsoleInput::skip_line(SourceLocation location) {
    // PABCSystem.Readln consumes through END_OF_LINE_SYMBOL (#10), or EOF.
    while (ensure(location)) {
        if ((cursor_&255U)==0) check(location);
        if (line_[cursor_++]==u'\n') break;
    }
}
std::u16string ConsoleInput::read_line(SourceLocation location) {
    check(location);
    if (!ensure(location)) return {};
    auto result=std::get<std::u16string>(read(Type::string,location));
    if (ensure(location)) {
        const auto end=line_[cursor_++];
        if (end==u'\r' && ensure(location) && line_[cursor_]==u'\n') ++cursor_;
    }
    return result;
}
std::u16string ConsoleInput::token(SourceLocation location) {
    check(location);
    const auto space=[](char16_t c){return c==u' ' || c==u'\t' || c==u'\r' || c==u'\n';};
    while (ensure(location) && space(line_[cursor_])) {
        if ((cursor_&255U)==0) check(location);
        ++cursor_;
    }
    if (!ensure(location)) throw runtime_error(RuntimeError::input_end,location);
    std::u16string result;
    while (ensure(location) && !space(line_[cursor_])) {
        if ((cursor_&255U)==0) check(location);
        result+=line_[cursor_++];
    }
    return result;
}
Value ConsoleInput::read(Type type, SourceLocation location) {
    check(location);
    if (type==Type::character) return ensure(location) ? line_[cursor_++] : char16_t{65535};
    if (type==Type::string) {
        if (!ensure(location)) return std::u16string(1,char16_t{65535}); // Source Read(string) appends its EOF sentinel once.
        std::u16string result;
        while (ensure(location) && line_[cursor_]!=u'\r' && line_[cursor_]!=u'\n') {
            if ((cursor_&255U)==0) check(location);
            result+=line_[cursor_++];
        }
        return result; // Read(string) leaves the line terminator for Readln.
    }
    auto bytes=to_utf8(token(location),true);
    auto text=std::string_view(bytes);
    if (type==Type::boolean) {
        for (auto& ch:bytes) if (ch>='A' && ch<='Z') ch=static_cast<char>(ch+32);
        if (bytes=="true") return true;
        if (bytes=="false") return false;
        throw runtime_error(RuntimeError::input_boolean,location);
    }
    if (text.starts_with('+')) text.remove_prefix(1);
    const auto parse=[&]<class T>()->Value {
        if constexpr (std::is_floating_point_v<T>) {
            double value=0;
            if (!parse_real(text,value)) throw runtime_error(RuntimeError::input_real,location);
            return value;
        } else {
            T value{};
            const auto converted=std::from_chars(text.data(),text.data()+text.size(),value);
            if (converted.ec!=std::errc{} || converted.ptr!=text.data()+text.size()) throw runtime_error(RuntimeError::input_range,location,type_name(type));
            return value;
        }
    };
    switch (type) {
    case Type::integer: return parse.operator()<std::int32_t>();
    case Type::int64: return parse.operator()<std::int64_t>();
    case Type::real: return parse.operator()<double>();
    case Type::byte: return parse.operator()<std::uint8_t>();
    case Type::shortint: return parse.operator()<std::int8_t>();
    case Type::smallint: return parse.operator()<std::int16_t>();
    case Type::word: return parse.operator()<std::uint16_t>();
    case Type::longword: return parse.operator()<std::uint32_t>();
    case Type::uint64: return parse.operator()<std::uint64_t>();
    default: throw runtime_error(RuntimeError::invalid_bytecode,location);
    }
}
} // namespace abc

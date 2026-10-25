#include "formatting.hpp"
#include "diagnostics.hpp"
#include "stdlib.hpp"
#include <array>
#include <charconv>
#include <type_traits>

namespace abc {
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
    } else if (const auto* string=std::get_if<std::u16string>(&value)) text=*string;
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

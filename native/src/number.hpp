#pragma once

#include <charconv>
#include <cmath>
#include <locale>
#include <sstream>
#include <string_view>

namespace abc {

inline bool parse_real(std::string_view text, double& value) {
    if (text.empty() || text.front() == '+') return false;
#ifdef ABC_HAS_FLOAT_FROM_CHARS
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size() && std::isfinite(value);
#else
    // Older libc++ releases lack floating from_chars. Do not use global locale.
    std::istringstream input{std::string(text)};
    input.imbue(std::locale::classic());
    input >> std::noskipws >> value;
    if (input.fail() || input.peek() != std::char_traits<char>::eof() || !std::isfinite(value)) return false;
    if (value == 0) {
        const auto mantissa = text.substr(0, text.find_first_of("eE"));
        if (mantissa.find_first_of("123456789") != std::string_view::npos) return false;
    }
    return true;
#endif
}

} // namespace abc

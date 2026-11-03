#include "cp1251.hpp"
#include <algorithm>
#include <array>

namespace abc {
namespace {
#include "cp1251_data.inc"
}
char16_t windows1251_character(std::uint8_t byte) {
    return byte<128 ? static_cast<char16_t>(byte) : windows1251_high[byte-128];
}
std::uint8_t windows1251_ordinal(char16_t character) {
    if (character<128) return static_cast<std::uint8_t>(character);
    const auto item=std::lower_bound(windows1251_encode.begin(),windows1251_encode.end(),character,
        [](const auto& entry,char16_t value){return entry.character<value;});
    return item!=windows1251_encode.end() && item->character==character ? item->byte : std::uint8_t{63};
}
}

#pragma once
#include <cstdint>
#include <span>
#include <string_view>
namespace abc {
struct SingleByteCodec {
    std::uint16_t code_page;
    std::uint8_t replacement;
    bool hold_high_surrogate;
    const char16_t* decode;
    const std::uint8_t* decode_strict;
    std::span<const std::uint32_t> encode;
};
const SingleByteCodec* single_byte_codec(std::uint16_t code_page);
std::uint16_t encoding_alias_code(std::u16string_view name);
std::uint16_t single_byte_encode(const SingleByteCodec&,char16_t);
}

#pragma once
#include <cstdint>
#include <span>
#include <string_view>
namespace abc {
struct DoubleByteCodec {
    std::uint16_t code_page;
    std::uint8_t replacement_byte;
    char16_t replacement_character;
    const std::uint8_t* leads;
    const char16_t* single;
    const std::uint8_t* single_exact;
    std::span<const std::uint32_t> pairs;
    const std::uint8_t* pair_exact;
    std::span<const std::uint32_t> encode;
    const std::uint8_t* encode_exact;
};
const DoubleByteCodec* double_byte_codec(std::uint16_t);
std::uint16_t double_byte_alias_code(std::u16string_view);
bool double_byte_lead(const DoubleByteCodec&,std::uint8_t);
std::uint32_t double_byte_encode(const DoubleByteCodec&,char16_t);
std::uint32_t double_byte_decode_pair(const DoubleByteCodec&,std::uint16_t);
}

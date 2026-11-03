#pragma once
#include <cstdint>
namespace abc {
char16_t windows1251_character(std::uint8_t byte);
std::uint8_t windows1251_ordinal(char16_t character);
}

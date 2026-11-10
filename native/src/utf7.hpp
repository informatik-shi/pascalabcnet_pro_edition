#pragma once
#include "abc/core.hpp"
#include <string_view>
namespace abc {
struct Utf7EncoderState {std::uint32_t bits=0;std::int8_t count=-1;};
struct Utf7DecoderState {std::uint32_t bits=0;std::int8_t count=-1;bool first=false;};
std::string utf7_encode(std::u16string_view,Utf7EncoderState&,bool final,SourceLocation);
std::u16string utf7_decode(std::string_view,Utf7DecoderState&,bool final,bool strict,SourceLocation);
}

#pragma once
#include "abc/core.hpp"
#include <array>
#include <string_view>
namespace abc {
struct IsoEncoderState {std::uint8_t mode=0,shift=0;bool stamped=false;};
struct IsoDecoderState {std::uint8_t mode=0,shift=0,count=0;std::array<std::uint8_t,4> pending{};};
inline bool iso2022_code_page(std::uint16_t cp){return (cp>=50220&&cp<=50222)||cp==50225||cp==52936;}
std::string iso2022_encode(std::u16string_view,std::uint16_t,IsoEncoderState&,bool final,bool strict,SourceLocation);
std::u16string iso2022_decode(std::string_view,std::uint16_t,IsoDecoderState&,bool final,bool strict,SourceLocation);
}

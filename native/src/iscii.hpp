#pragma once
#include "abc/core.hpp"
#include <string_view>
namespace abc {
struct IsciiEncoderState {std::uint8_t script=0;bool virama=false;};
struct IsciiDecoderState {std::uint8_t script=0,mode=0,pending=0;};
inline bool iscii_code_page(std::uint16_t code){return code>=57002&&code<=57011;}
std::string iscii_encode(std::u16string_view,std::uint16_t,IsciiEncoderState&,bool final,bool strict,SourceLocation);
std::u16string iscii_decode(std::string_view,std::uint16_t,IsciiDecoderState&,bool final,bool strict,SourceLocation);
}

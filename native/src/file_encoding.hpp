#pragma once
#include "abc/core.hpp"
#include <string_view>

namespace abc {
enum class TextEncodingKind : std::uint16_t { windows1251=1251,utf16_le=1200,utf16_be=1201,utf32_le=12000,utf32_be=12001,ascii=20127,utf8=65001 };
struct TextEncoding {
    TextEncodingKind kind=TextEncodingKind::utf8;
    bool preamble=false,strict=false;
    bool operator==(const TextEncoding&) const=default;
};
std::string_view text_encoding_preamble(TextEncoding encoding);
std::u16string decode_text_bytes(std::string_view bytes,TextEncoding encoding,bool detect_preamble,SourceLocation location);
std::string encode_text_bytes(std::u16string_view text,TextEncoding encoding,SourceLocation location);
class TextDecoder {
    TextEncoding encoding_;
    bool detect_,finished_=false;
    std::string carry_;
    std::size_t received_=0,produced_=0;
public:
    explicit TextDecoder(TextEncoding encoding,bool detect_preamble=true):encoding_(encoding),detect_(detect_preamble){}
    std::u16string decode(std::string_view bytes,bool final,SourceLocation location);
};
// Legacy internal default; execution-bound file calls use FileContext DefaultEncoding.
std::u16string decode_file_bytes(std::string_view bytes,SourceLocation location);
}

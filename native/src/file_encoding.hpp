#pragma once
#include "abc/core.hpp"
#include "iscii.hpp"
#include "iso2022.hpp"
#include "utf7.hpp"
#include <string_view>

namespace abc {
enum class TextEncodingKind : std::uint16_t { windows1251=1251,utf16_le=1200,utf16_be=1201,utf32_le=12000,utf32_be=12001,ascii=20127,gb18030=54936,utf7=65000,utf8=65001 };
struct TextEncoding {
    TextEncodingKind kind=TextEncodingKind::utf8;
    bool preamble=false,strict=false;
    bool operator==(const TextEncoding&) const=default;
};
std::string_view text_encoding_preamble(TextEncoding encoding);
std::u16string decode_text_bytes(std::string_view bytes,TextEncoding encoding,bool detect_preamble,SourceLocation location);
std::string encode_text_bytes(std::u16string_view text,TextEncoding encoding,SourceLocation location);
class TextEncoder {
    TextEncoding encoding_;
    char16_t carry_=0;
    IsciiEncoderState iscii_;
    IsoEncoderState iso_;
    Utf7EncoderState utf7_;
public:
    explicit TextEncoder(TextEncoding encoding):encoding_(encoding){}
    std::string encode(std::u16string_view text,bool final,SourceLocation location);
};
class TextDecoder {
    TextEncoding encoding_;
    bool detect_,finished_=false;
    std::string carry_;
    std::size_t received_=0,produced_=0;
    IsciiDecoderState iscii_;
    IsoDecoderState iso_;
    Utf7DecoderState utf7_;
public:
    explicit TextDecoder(TextEncoding encoding,bool detect_preamble=true):encoding_(encoding),detect_(detect_preamble){}
    std::u16string decode(std::string_view bytes,bool final,SourceLocation location);
    void resume_after_final() noexcept{finished_=false;}
    // BinaryReader retains codec state across fields and seeks; each bounded
    // decoding operation accounts its own work instead of limiting file size.
    void reset_operation_budget() noexcept{received_=produced_=0;}
};
// Legacy internal default; execution-bound file calls use FileContext DefaultEncoding.
std::u16string decode_file_bytes(std::string_view bytes,SourceLocation location);
}

#include "file_encoding.hpp"
#include "sbcs.hpp"
#include "diagnostics.hpp"
namespace abc {
std::string TextEncoder::encode(std::u16string_view input,bool final,SourceLocation location){
    (void)text_encoding_preamble(encoding_);
    if(input.size()>1'048'576)throw runtime_error(RuntimeError::encoding_limit,location);
    if(encoding_.kind==TextEncodingKind::utf7){auto state=utf7_;auto result=utf7_encode(input,state,final,location);utf7_=state;return result;}
    std::u16string text;if(carry_)text+=carry_;text+=input;char16_t nextCarry=0;
    const auto* single=single_byte_codec(static_cast<std::uint16_t>(encoding_.kind));
    if(!final&&(!single||single->hold_high_surrogate)&&!text.empty()&&text.back()>=0xd800&&text.back()<=0xdbff){nextCarry=text.back();text.pop_back();}
    auto state=iscii_;auto iso=iso_;const auto cp=static_cast<std::uint16_t>(encoding_.kind);
    auto result=iscii_code_page(cp)?iscii_encode(text,cp,state,final,encoding_.strict,location)
        :iso2022_code_page(cp)?iso2022_encode(text,cp,iso,final,encoding_.strict,location):encode_text_bytes(text,encoding_,location);
    carry_=nextCarry;iscii_=state;iso_=iso;return result;
}
}

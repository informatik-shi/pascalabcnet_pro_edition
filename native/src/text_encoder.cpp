#include "file_encoding.hpp"
#include "sbcs.hpp"
#include "diagnostics.hpp"
namespace abc {
std::string TextEncoder::encode(std::u16string_view input,bool final,SourceLocation location){
    (void)text_encoding_preamble(encoding_);
    if(input.size()>1'048'576)throw runtime_error(RuntimeError::encoding_limit,location);
    std::u16string text;if(carry_)text+=carry_;text+=input;char16_t nextCarry=0;
    const auto* single=single_byte_codec(static_cast<std::uint16_t>(encoding_.kind));
    if(!final&&(!single||single->hold_high_surrogate)&&!text.empty()&&text.back()>=0xd800&&text.back()<=0xdbff){nextCarry=text.back();text.pop_back();}
    auto state=iscii_;auto result=iscii_code_page(static_cast<std::uint16_t>(encoding_.kind))
        ?iscii_encode(text,static_cast<std::uint16_t>(encoding_.kind),state,final,encoding_.strict,location)
        :encode_text_bytes(text,encoding_,location);
    carry_=nextCarry;iscii_=state;return result;
}
}

#include "sbcs.hpp"
#include <algorithm>
#include <iterator>
namespace abc {
namespace {
struct EncodingAlias {std::u16string_view name;std::uint16_t code_page;};
#include "sbcs_data.inc"
}
const SingleByteCodec* single_byte_codec(std::uint16_t code_page){
    const auto found=std::lower_bound(std::begin(sbcs_codecs),std::end(sbcs_codecs),code_page,[](const auto& codec,auto value){return codec.code_page<value;});
    return found!=std::end(sbcs_codecs) && found->code_page==code_page ? found : nullptr;
}
std::uint16_t encoding_alias_code(std::u16string_view name){
    const auto found=std::lower_bound(std::begin(encoding_aliases),std::end(encoding_aliases),name,[](const auto& alias,auto value){return alias.name<value;});
    return found!=std::end(encoding_aliases) && found->name==name ? found->code_page : 0;
}
std::uint16_t single_byte_encode(const SingleByteCodec& codec,char16_t character){
    const auto key=static_cast<std::uint32_t>(character)<<16;
    const auto found=std::lower_bound(codec.encode.begin(),codec.encode.end(),key);
    return found!=codec.encode.end() && (*found&0xffff0000U)==key ? static_cast<std::uint16_t>(*found) : codec.replacement;
}
}

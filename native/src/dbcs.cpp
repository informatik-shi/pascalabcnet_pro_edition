#include "dbcs.hpp"
#include <algorithm>
#include <iterator>
namespace abc {
namespace {
struct DbcsAlias{std::u16string_view name;std::uint16_t code_page;};
#include "dbcs_data.inc"
std::uint16_t lookup(std::span<const std::uint32_t> table,std::uint16_t key,std::uint16_t replacement){
    const auto target=static_cast<std::uint32_t>(key)<<16;const auto found=std::lower_bound(table.begin(),table.end(),target);
    return found!=table.end()&&(*found&0xffff0000U)==target ? static_cast<std::uint16_t>(*found) : replacement;
}
bool exact(const std::uint8_t* bits,std::uint16_t key){return (bits[key>>3]>>(key&7))&1U;}
}
const DoubleByteCodec* double_byte_codec(std::uint16_t code){
    const auto found=std::lower_bound(std::begin(dbcs_codecs),std::end(dbcs_codecs),code,[](const auto& codec,auto value){return codec.code_page<value;});
    return found!=std::end(dbcs_codecs)&&found->code_page==code ? found : nullptr;
}
std::uint16_t double_byte_alias_code(std::u16string_view name){
    const auto found=std::lower_bound(std::begin(dbcs_aliases),std::end(dbcs_aliases),name,[](const auto& alias,auto value){return alias.name<value;});
    return found!=std::end(dbcs_aliases)&&found->name==name ? found->code_page : 0;
}
bool double_byte_lead(const DoubleByteCodec& codec,std::uint8_t value){return exact(codec.leads,value);}
std::uint32_t double_byte_encode(const DoubleByteCodec& codec,char16_t value){return lookup(codec.encode,value,codec.replacement_byte)|(exact(codec.encode_exact,value)?0x10000U:0U);}
std::uint32_t double_byte_decode_pair(const DoubleByteCodec& codec,std::uint16_t value){return lookup(codec.pairs,value,codec.replacement_character)|(exact(codec.pair_exact,value)?0x10000U:0U);}
}

#include "iscii.hpp"
#include "diagnostics.hpp"
namespace abc {
namespace {
struct IsciiCodec {const char16_t* base;const std::uint8_t* exact;const std::uint8_t* mode;const char16_t* alternate;};
#include "iscii_data.inc"
}
std::string iscii_encode(std::u16string_view input,std::uint16_t cp,IsciiEncoderState& state,bool final,bool strict,SourceLocation location){
    if(!iscii_code_page(cp))throw runtime_error(RuntimeError::encoding_kind,location);
    if(!state.script)state.script=static_cast<std::uint8_t>(cp-57000);std::string result;result.reserve(input.size());
    const auto byte=[&](unsigned b){if(result.size()>=1'048'576)throw runtime_error(RuntimeError::encoding_limit,location);result+=static_cast<char>(b);};
    for(char16_t unit:input){
        if(unit<160){byte(unit);state.virama=false;continue;}
        if(state.virama&&(unit==0x200c||unit==0x200d)){byte(unit==0x200c?0xe8:0xe9);state.virama=false;continue;}
        const auto entry=unit>=0x0901&&unit<=0x0d6f?iscii_encode_map[unit-0x0901]:0;
        if(!entry){if(strict)throw runtime_error(RuntimeError::encoding_data,location);byte(63);state.virama=false;continue;}
        const auto script=entry>>16,first=(entry>>8)&255,second=entry&255;
        if(state.script!=script){byte(0xef);byte(script+0x40);state.script=static_cast<std::uint8_t>(script);}
        byte(first);state.virama=first==0xe8;if(second)byte(second);
    }
    // A final call resets a changed font; source keeps a default-font Virama flag.
    if(final&&state.script!=cp-57000U){byte(0xef);byte(cp-57000U+0x40);state.script=static_cast<std::uint8_t>(cp-57000);state.virama=false;}
    return result;
}
std::u16string iscii_decode(std::string_view input,std::uint16_t cp,IsciiDecoderState& state,bool final,bool strict,SourceLocation location){
    if(!iscii_code_page(cp))throw runtime_error(RuntimeError::encoding_kind,location);
    if(!state.script)state.script=static_cast<std::uint8_t>(cp-57000);std::u16string result;result.reserve(input.size()+1);
    const auto fallback=[&]{if(strict)throw runtime_error(RuntimeError::encoding_data,location);result+=u'?';};
    for(unsigned char b:input){
        if(state.mode){const auto previous=state.mode;state.mode=0;
            if(previous==1){if(b>=0x40&&b<=0x4b){state.script=b<=0x41?static_cast<std::uint8_t>(cp-57000):static_cast<std::uint8_t>(b-0x40);continue;}fallback();}
            else if(previous==2){if(b==0xe8||b==0xe9){result+=static_cast<char16_t>(b==0xe8?0x200c:0x200d);continue;}}
            else if(previous==3){if(b==0xb8||b==0xbf){result+=static_cast<char16_t>(b==0xb8?0x0952:0x0970);continue;}fallback();}
            else if(previous==4){const auto& codec=iscii_codecs[state.script-2];result+=b==0xe9?codec.alternate[state.pending]:codec.base[state.pending];if(b==0xe9)continue;}
            else throw runtime_error(RuntimeError::invalid_bytecode,location);
        }
        const auto& codec=iscii_codecs[state.script-2];const auto mode=codec.mode[b];
        if(mode==0||mode==2){if(!(codec.exact[b>>3]&(1U<<(b&7))))fallback();else result+=codec.base[b];}
        if(mode){state.mode=mode;state.pending=b;}
    }
    if(final){if(state.mode==1||state.mode==3)fallback();else if(state.mode==4)result+=iscii_codecs[state.script-2].base[state.pending];state={};}
    return result;
}
}

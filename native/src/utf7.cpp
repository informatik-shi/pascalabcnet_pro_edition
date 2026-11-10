#include "utf7.hpp"
#include "diagnostics.hpp"
namespace abc {
namespace {
constexpr std::string_view digits="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
constexpr std::u16string_view direct=u"\t\n\r '(),-./0123456789:?ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
}
std::string utf7_encode(std::u16string_view input,Utf7EncoderState& state,bool final,SourceLocation location){
    std::string result;result.reserve(input.size());
    const auto byte=[&](char value){if(result.size()>=1'048'576)throw runtime_error(RuntimeError::encoding_limit,location);result+=value;};
    const auto finish=[&]{if(state.count>=0){if(state.count)byte(digits[(state.bits<<(6-state.count))&63]);byte('-');state={};}};
    // UTF-7 transforms UTF-16 units, including individual surrogates, without encoder fallback.
    for(char16_t ch:input){
        if(direct.find(ch)!=direct.npos){finish();byte(static_cast<char>(ch));}
        else if(state.count<0&&ch==u'+'){byte('+');byte('-');}
        else{if(state.count<0){byte('+');state.count=0;}state.bits=(state.bits<<16)|ch;state.count+=16;
            while(state.count>=6){state.count-=6;byte(digits[(state.bits>>state.count)&63]);}}
    }
    if(final)finish();return result;
}
std::u16string utf7_decode(std::string_view input,Utf7DecoderState& state,bool final,bool strict,SourceLocation location){
    std::u16string result;result.reserve(input.size());
    const auto fallback=[&](unsigned char value){if(strict)throw runtime_error(RuntimeError::encoding_data,location);if(value)result+=static_cast<char16_t>(value);};
    for(unsigned char value:input){
        if(state.count>=0){
            const auto digit=digits.find(static_cast<char>(value));
            if(digit!=digits.npos){state.first=false;state.bits=(state.bits<<6)|static_cast<std::uint32_t>(digit);state.count+=6;
                if(state.count>=16){state.count-=16;result+=static_cast<char16_t>((state.bits>>state.count)&65535U);}continue;}
            state.count=-1;if(value=='-'){if(state.first)result+=u'+';}else fallback(value);
        }else if(value=='+'){state.count=0;state.first=true;}
        else if(value>=128)fallback(value);else result+=static_cast<char16_t>(value);
    }
    if(final)state={};return result;
}
}

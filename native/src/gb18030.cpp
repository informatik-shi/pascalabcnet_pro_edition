#include "gb18030.hpp"
#include "diagnostics.hpp"
#include <array>
namespace abc {
namespace {
#include "gb18030_data.inc"
bool lead(unsigned b){return b>=0x81&&b<=0xfe;}
bool digit(unsigned b){return b>=0x30&&b<=0x39;}
bool trail(unsigned b){return b>=0x40&&b<=0xfe&&b!=0x7f;}
char16_t two(unsigned a,unsigned b){return gb_decode_two[(a-0x81)*190+b-0x40-(b>=0x80?1:0)];}
unsigned four(unsigned a,unsigned b,unsigned c,unsigned d){return ((a-0x81)*10+b-0x30)*1260+(c-0x81)*10+d-0x30;}
}
std::string gb18030_encode(std::u16string_view text,bool strict,SourceLocation location){
    std::string result;result.reserve(text.size());
    const auto byte=[&](unsigned b){if(result.size()>=1'048'576)throw runtime_error(RuntimeError::encoding_limit,location);result+=static_cast<char>(b);};
    for(std::size_t n=0;n<text.size();++n){
        const auto value=text[n];std::uint32_t encoded;
        if(value>=0xd800&&value<=0xdbff&&n+1<text.size()&&text[n+1]>=0xdc00&&text[n+1]<=0xdfff){
            auto offset=0x2e248U+((value-0xd800U)<<10)+(text[++n]-0xdc00U);
            const auto d=offset%10+0x30;offset/=10;const auto c=offset%126+0x81;offset/=126;const auto b=offset%10+0x30;offset/=10;
            byte(offset+0x81);byte(b);byte(c);byte(d);continue;
        }
        if(value>=0xd800&&value<=0xdfff&&strict)throw runtime_error(RuntimeError::encoding_data,location);
        encoded=gb_encode_bmp[value];
        if(encoded>65535){byte(encoded>>24);byte((encoded>>16)&255);byte((encoded>>8)&255);}else if(encoded>255)byte(encoded>>8);
        byte(encoded&255);
    }
    return result;
}
std::u16string gb18030_decode(std::string_view input,std::size_t pending,bool final,bool strict,std::string& retained,SourceLocation location){
    std::u16string result;result.reserve(input.size());retained.clear();std::size_t position=0;
    const auto at=[&](std::size_t n){return static_cast<unsigned char>(input[n]);};
    const auto fallback=[&]{if(strict)throw runtime_error(RuntimeError::encoding_data,location);result+=u'?';};
    const auto scalar=[&](unsigned offset){
        if(offset<=0x99fb)result+=gb_decode_four_bmp[offset];
        else if(offset>=0x2e248&&offset<=0x12e247){offset-=0x2e248;result+=static_cast<char16_t>(0xd800+(offset>>10));result+=static_cast<char16_t>(0xdc00+(offset&1023));}
        else fallback();
    };
    if(pending){
        if(pending>3||pending>input.size())throw runtime_error(RuntimeError::invalid_bytecode,location);
        std::array<unsigned,4> queue{};std::size_t count=pending;for(;position<pending;++position)queue[position]=at(position);
        const auto remove=[&](std::size_t length){for(std::size_t n=length;n<count;++n)queue[n-length]=queue[n];count-=length;};
        while(count){
            if(!lead(queue[0])){if(queue[0]<=0x7f)result+=static_cast<char16_t>(queue[0]);else fallback();remove(1);continue;}
            while((count<2||(digit(queue[1])&&count<4))&&position<input.size())queue[count++]=at(position++);
            if((count<2||(digit(queue[1])&&count<4))&&!final){for(std::size_t n=0;n<count;++n)retained+=static_cast<char>(queue[n]);return result;}
            if(count>=2&&trail(queue[1])){result+=two(queue[0],queue[1]);remove(2);}
            else if(count==4&&digit(queue[1])&&lead(queue[2])&&digit(queue[3])){scalar(four(queue[0],queue[1],queue[2],queue[3]));remove(4);}
            else {fallback();remove(1);}
        }
    }
    while(position<input.size()){
        const auto first=at(position);
        if(first<=0x7f){result+=static_cast<char16_t>(first);++position;continue;}
        if(!lead(first)){fallback();++position;continue;}
        const auto available=input.size()-position;
        if(available>=2&&trail(at(position+1))){result+=two(first,at(position+1));position+=2;continue;}
        if(available>=2&&digit(at(position+1))){
            if(available>=4){
                if(lead(at(position+2))&&digit(at(position+3))){scalar(four(first,at(position+1),at(position+2),at(position+3)));position+=4;continue;}
            }else if(!final){retained.assign(input.substr(position));return result;}
            else {fallback();position+=2;continue;}
        }else if(available==1&&!final){retained.assign(input.substr(position));return result;}
        fallback();++position;
    }
    return result;
}
}

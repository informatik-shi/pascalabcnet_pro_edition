#include "iso2022.hpp"
#include "diagnostics.hpp"
#include <algorithm>
#include <span>
namespace abc {
namespace {
struct IsoCodec {std::uint16_t cp;char16_t replacement;std::span<const std::uint32_t> encode;const std::uint8_t* encode_exact;const char16_t* base;const std::uint8_t* base_exact;const char16_t* pair;const std::uint8_t* pair_exact;};
#include "iso2022_data.inc"
bool exact(const std::uint8_t* bits,unsigned n){return (bits[n>>3]>>(n&7))&1U;}
const IsoCodec& codec(std::uint16_t cp,SourceLocation location){for(const auto& c:iso_codecs)if(c.cp==cp)return c;throw runtime_error(RuntimeError::encoding_kind,location);}
unsigned symbol(const IsoCodec& c,char16_t ch){const auto key=static_cast<std::uint32_t>(ch)<<16;const auto it=std::lower_bound(c.encode.begin(),c.encode.end(),key);return it!=c.encode.end()&&(*it&0xffff0000U)==key?*it&65535U:63U;}
// -1 incomplete, -2 invalid; nonnegative values are the selected JP mode.
int escape(const IsoDecoderState& s,bool jp){
    const auto& q=s.pending;
    if(jp){if(s.count<3)return -1;if(q[1]=='('){if(q[2]=='B'||q[2]=='H'||q[2]=='J')return 0;if(q[2]=='I')return 2;}
        else if(q[1]=='$'){if(q[2]=='@'||q[2]=='B')return 1;if(s.count<4)return -1;if(q[2]=='('&&q[3]=='D')return 1;}
        else if(q[1]=='&'&&q[2]=='@')return 0;return -2;
    }
    if(s.count<4)return -1;return q[1]=='$'&&q[2]==')'&&q[3]=='C'?0:-2;
}
}
std::string iso2022_encode(std::u16string_view input,std::uint16_t cp,IsoEncoderState& state,bool final,bool strict,SourceLocation location){
    const auto& c=codec(cp,location);const bool jp=cp<50225;std::string result;result.reserve(input.size());
    const auto byte=[&](unsigned n){if(result.size()>=1'048'576)throw runtime_error(RuntimeError::encoding_limit,location);result+=static_cast<char>(n);};
    const auto ascii=[&]{if(jp){if(cp==50222&&state.mode==2){byte(15);state.mode=state.shift;}if(state.mode){byte(27);byte('(');byte('B');state.mode=0;}}
        else if(state.mode){if(cp==50225)byte(15);else{byte('~');byte('}');}state.mode=0;}};
    for(char16_t ch:input){if(strict&&!exact(c.encode_exact,ch))throw runtime_error(RuntimeError::encoding_data,location);const auto code=symbol(c,ch);
        if(jp&&(code>>8)==0x10){if(state.mode!=2){if(cp==50222){byte(14);state.shift=state.mode;}else{byte(27);byte('(');byte('I');}state.mode=2;}byte(code&127);}
        else if(code>255){
            if(jp){if(cp==50222&&state.mode==2){byte(15);state.mode=state.shift;}if(state.mode!=1){byte(27);byte('$');byte('B');state.mode=1;}}
            else if(cp==50225){if(!state.stamped){byte(27);byte('$');byte(')');byte('C');state.stamped=true;}if(!state.mode){byte(14);state.mode=1;}}
            else if(!state.mode){byte('~');byte('{');state.mode=1;}
            byte(code>>8);byte(code&255);
        }else{ascii();if(cp==52936&&code=='~')byte('~');byte(code);}
    }
    if(final){ascii();if(cp==50225)state.stamped=false;}return result;
}
std::u16string iso2022_decode(std::string_view input,std::uint16_t cp,IsoDecoderState& state,bool final,bool strict,SourceLocation location){
    const auto& c=codec(cp,location);const bool jp=cp<50225;std::u16string result;result.reserve(input.size());
    const auto fallback=[&]{if(strict)throw runtime_error(RuntimeError::encoding_data,location);result+=c.replacement;};
    const auto single=[&](unsigned b){if(strict&&!exact(c.base_exact,b))throw runtime_error(RuntimeError::encoding_data,location);result+=c.base[b];};
    const auto pair=[&](unsigned code){if(strict&&!exact(c.pair_exact,code))throw runtime_error(RuntimeError::encoding_data,location);result+=c.pair[code];};
    if(cp==52936){
        std::string data;if(state.count)data+=static_cast<char>(state.pending[0]);data+=input;state.count=0;std::size_t position=0;
        while(position<data.size()){
            auto first=static_cast<unsigned char>(data[position++]);
            if(first=='~'){
                if(position==data.size()){if(!final){state.pending[0]=first;state.count=1;}else fallback();break;}
                const auto next=static_cast<unsigned char>(data[position++]);
                if(next=='~'&&!state.mode){single('~');continue;}if(next=='{'){state.mode=1;continue;}if(next=='}'){state.mode=0;continue;}if(next=='\n')continue;--position;
            }
            if(state.mode&&first>=32){if(position==data.size()){if(!final){state.pending[0]=first;state.count=1;}else fallback();break;}pair((static_cast<unsigned>(first)<<8)|static_cast<unsigned char>(data[position++]));}
            else single(first);
        }
        if(final&&!state.count)state={};return result;
    }
    std::size_t position=0;
    const auto take=[&]{const auto b=state.pending[0];for(unsigned n=1;n<state.count;++n)state.pending[n-1]=state.pending[n];--state.count;return b;};
    while(position<input.size()||state.count){
        std::uint8_t first;
        if(state.count){
            if(state.pending[0]==27){if(position==input.size()){if(!final)break;}
                else {if(state.count>=4)throw runtime_error(RuntimeError::invalid_bytecode,location);state.pending[state.count++]=static_cast<std::uint8_t>(input[position++]);const auto mode=escape(state,jp);if(mode!=-2){if(mode>=0){state.count=0;if(jp)state.mode=state.shift=static_cast<std::uint8_t>(mode);}continue;}}
            }first=take();
        }else{first=static_cast<std::uint8_t>(input[position++]);if(first==27){state.pending[0]=first;state.count=1;continue;}}
        if(first==14){if(jp){state.shift=state.mode;state.mode=2;}else state.mode=1;continue;}
        if(first==15){state.mode=jp?state.shift:0;continue;}
        if(state.mode==1&&(jp||(first!=32&&first!=9&&first!=10))){
            bool have=false;unsigned code=first;
            if(state.count){if(state.pending[0]!=27){code=(code<<8)|take();have=true;}}
            else if(position<input.size()){code=(code<<8)|static_cast<std::uint8_t>(input[position++]);have=true;}
            else {if(!final){state.pending[0]=first;state.count=1;}else fallback();break;}
            if(have)pair(code);else single(first);
        }else if(jp&&(state.mode==2||(first>=0xa1&&first<=0xdf)))pair(0x1000U|((first>=0xa1&&first<=0xdf)?first&127U:first));
        else single(first);
    }
    if(final&&!state.count)state={};return result;
}
}

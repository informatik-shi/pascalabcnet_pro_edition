#include "file_encoding.hpp"
#include "sbcs.hpp"
#include "dbcs.hpp"
#include "gb18030.hpp"
#include "diagnostics.hpp"

namespace abc {
namespace {
constexpr std::size_t data_limit=1'048'576;
constexpr char16_t replacement=0xfffd;
bool valid(TextEncodingKind kind) {
    return kind==TextEncodingKind::utf8 || kind==TextEncodingKind::gb18030 || kind==TextEncodingKind::utf16_le || kind==TextEncodingKind::utf16_be
        || kind==TextEncodingKind::utf32_le || kind==TextEncodingKind::utf32_be || single_byte_codec(static_cast<std::uint16_t>(kind)) || double_byte_codec(static_cast<std::uint16_t>(kind)) || iscii_code_page(static_cast<std::uint16_t>(kind));
}
void validate(TextEncoding encoding,SourceLocation location) {
    if (!valid(encoding.kind)) throw runtime_error(RuntimeError::encoding_kind,location);
}
char16_t invalid(TextEncoding encoding,SourceLocation location) {
    if (encoding.strict) throw runtime_error(RuntimeError::encoding_data,location);
    return replacement;
}
std::uint32_t byte(std::string_view bytes,std::size_t index) {
    return static_cast<unsigned char>(bytes[index]);
}
std::uint32_t unit(std::string_view bytes,std::size_t index,unsigned width,bool big_endian) {
    std::uint32_t result=0;
    for (unsigned n=0;n<width;++n) result=(result<<8)|byte(bytes,index+(big_endian ? n : width-1-n));
    return result;
}
void scalar(std::u16string& result,std::uint32_t value) {
    if (value<=0xffff) result.push_back(static_cast<char16_t>(value));
    else {
        result.push_back(static_cast<char16_t>(0xd800+((value-0x10000)>>10)));
        result.push_back(static_cast<char16_t>(0xdc00+((value-0x10000)&0x3ff)));
    }
}
std::u16string decode_utf8(std::string_view bytes,TextEncoding encoding,SourceLocation location) {
    std::u16string result;result.reserve(bytes.size());
    for(std::size_t index=0;index<bytes.size();) {
        const auto lead=byte(bytes,index);
        if (lead<128) {result.push_back(static_cast<char16_t>(lead));++index;continue;}
        const unsigned width=lead>=0xc2 && lead<=0xdf ? 2U : lead>=0xe0 && lead<=0xef ? 3U : lead>=0xf0 && lead<=0xf4 ? 4U : 0U;
        if (!width) {result.push_back(invalid(encoding,location));++index;continue;}
        auto value=lead & (width==2 ? 0x1fU : width==3 ? 0x0fU : 0x07U);
        unsigned consumed=1;
        while(consumed<width && index+consumed<bytes.size()) {
            const auto next=byte(bytes,index+consumed);
            if(next<0x80 || next>0xbf || (consumed==1 && ((lead==0xe0 && next<0xa0) || (lead==0xed && next>0x9f)
                || (lead==0xf0 && next<0x90) || (lead==0xf4 && next>0x8f))))break;
            value=(value<<6)|(next&0x3f);++consumed;
        }
        if(consumed!=width)result.push_back(invalid(encoding,location));else scalar(result,value);
        index+=consumed;
    }
    return result;
}
std::u16string decode_utf16(std::string_view bytes,TextEncoding encoding,bool big_endian,SourceLocation location) {
    std::u16string result;result.reserve((bytes.size()+1)/2);
    char16_t high=0;
    for (std::size_t index=0;index+1<bytes.size();index+=2) {
        const auto value=static_cast<char16_t>(unit(bytes,index,2,big_endian));
        if (high) {
            if (value>=0xdc00 && value<=0xdfff) {
                result.push_back(high);result.push_back(value);high=0;continue;
            }
            result.push_back(invalid(encoding,location));high=0;
        }
        if (value>=0xd800 && value<=0xdbff) high=value;
        else result.push_back(value>=0xdc00 && value<=0xdfff ? invalid(encoding,location) : value);
    }
    if (high) result.push_back(invalid(encoding,location));
    if (bytes.size()%2) result.push_back(invalid(encoding,location));
    return result;
}
std::u16string decode_utf32(std::string_view bytes,TextEncoding encoding,bool big_endian,SourceLocation location) {
    std::u16string result;result.reserve((bytes.size()+3)/4);
    for (std::size_t index=0;index+3<bytes.size();index+=4) {
        const auto value=unit(bytes,index,4,big_endian);
        if (value>0x10ffff || (value>=0xd800 && value<=0xdfff)) result.push_back(invalid(encoding,location));
        else scalar(result,value);
    }
    if (bytes.size()%4) result.push_back(invalid(encoding,location));
    return result;
}
}

std::string_view text_encoding_preamble(TextEncoding encoding) {
    validate(encoding,{});
    if(!encoding.preamble)return {};
    switch(encoding.kind) {
    case TextEncodingKind::utf8:return {"\xef\xbb\xbf",3};
    case TextEncodingKind::utf16_le:return {"\xff\xfe",2};
    case TextEncodingKind::utf16_be:return {"\xfe\xff",2};
    case TextEncodingKind::utf32_le:return {"\xff\xfe\0\0",4};
    case TextEncodingKind::utf32_be:return {"\0\0\xfe\xff",4};
    default:return {};
    }
}
namespace {
void detect_encoding(std::string_view& bytes,TextEncoding& encoding) {
    const auto expected=text_encoding_preamble(encoding);
    // StreamReader keeps an incomplete configured preamble for the decoder at EOF.
    // In particular, FF FE alone is incomplete UTF-32LE, not a UTF-16LE switch.
    if(!expected.empty() && bytes.size()<expected.size() && expected.starts_with(bytes))return;
    if(!expected.empty() && bytes.starts_with(expected))bytes.remove_prefix(expected.size());
    else for(auto kind:{TextEncodingKind::utf32_le,TextEncodingKind::utf32_be,TextEncodingKind::utf16_le,TextEncodingKind::utf16_be,TextEncodingKind::utf8}) {
        const auto mark=text_encoding_preamble({kind,true,false});
        if(bytes.starts_with(mark)){encoding={kind,true,false};bytes.remove_prefix(mark.size());break;}
    }
}
}
std::u16string decode_text_bytes(std::string_view bytes,TextEncoding encoding,bool detect_preamble,SourceLocation location) {
    validate(encoding,location);
    if(bytes.size()>data_limit)throw runtime_error(RuntimeError::encoding_limit,location);
    if(detect_preamble)detect_encoding(bytes,encoding);
    if(iscii_code_page(static_cast<std::uint16_t>(encoding.kind))){IsciiDecoderState state;return iscii_decode(bytes,static_cast<std::uint16_t>(encoding.kind),state,true,encoding.strict,location);}
    if(encoding.kind==TextEncodingKind::gb18030){std::string retained;return gb18030_decode(bytes,0,true,encoding.strict,retained,location);}
    if(encoding.kind==TextEncodingKind::utf8)return decode_utf8(bytes,encoding,location);
    if(encoding.kind==TextEncodingKind::utf16_le || encoding.kind==TextEncodingKind::utf16_be)return decode_utf16(bytes,encoding,encoding.kind==TextEncodingKind::utf16_be,location);
    if(encoding.kind==TextEncodingKind::utf32_le || encoding.kind==TextEncodingKind::utf32_be)return decode_utf32(bytes,encoding,encoding.kind==TextEncodingKind::utf32_be,location);
    std::u16string result;result.reserve(bytes.size());
    if(const auto* codec=double_byte_codec(static_cast<std::uint16_t>(encoding.kind))){
        for(std::size_t index=0;index<bytes.size();){
            const auto first=static_cast<std::uint8_t>(bytes[index++]);std::uint32_t value;
            if(double_byte_lead(*codec,first) && index<bytes.size()){
                const auto pair=static_cast<std::uint16_t>((static_cast<unsigned>(first)<<8)|static_cast<std::uint8_t>(bytes[index++]));value=double_byte_decode_pair(*codec,pair);
            }else value=codec->single[first]|(((codec->single_exact[first>>3]>>(first&7))&1U)?0x10000U:0U);
            if(encoding.strict && !(value&0x10000U))throw runtime_error(RuntimeError::encoding_data,location);
            result.push_back(static_cast<char16_t>(value));
        }
        return result;
    }
    const auto& codec=*single_byte_codec(static_cast<std::uint16_t>(encoding.kind));
    for(unsigned char value:bytes){
        if(encoding.strict && !(codec.decode_strict[value>>3]&(1U<<(value&7))))throw runtime_error(RuntimeError::encoding_data,location);
        result.push_back(codec.decode[value]);
    }
    return result;
}
std::u16string TextDecoder::decode(std::string_view bytes,bool final,SourceLocation location) {
    validate(encoding_,location);
    if(finished_) {
        if(!bytes.empty())throw runtime_error(RuntimeError::invalid_bytecode,location);
        return {};
    }
    if(bytes.size()>data_limit-received_)throw runtime_error(RuntimeError::encoding_limit,location);
    const auto pending=detect_?0:carry_.size();
    received_+=bytes.size();std::string data=std::move(carry_);data.append(bytes);carry_.clear();
    if(detect_ && data.size()<4 && !final){
        bool wait=encoding_.kind!=TextEncodingKind::gb18030;
        for(auto kind:{TextEncodingKind::utf8,TextEncodingKind::utf16_le,TextEncodingKind::utf16_be,TextEncodingKind::utf32_le,TextEncodingKind::utf32_be})
            if(text_encoding_preamble({kind,true,false}).starts_with(data))wait=true;
        if(wait){carry_=std::move(data);return {};}
    }
    std::string_view view=data;
    if(detect_){detect_encoding(view,encoding_);detect_=false;}
    if(iscii_code_page(static_cast<std::uint16_t>(encoding_.kind))){
        auto state=iscii_;auto result=iscii_decode(view,static_cast<std::uint16_t>(encoding_.kind),state,final,encoding_.strict,location);
        if(result.size()>data_limit-produced_)throw runtime_error(RuntimeError::encoding_limit,location);
        produced_+=result.size();iscii_=state;finished_=final;return result;
    }
    if(encoding_.kind==TextEncodingKind::gb18030){
        auto result=gb18030_decode(view,pending,final,encoding_.strict,carry_,location);
        if(result.size()>data_limit-produced_)throw runtime_error(RuntimeError::encoding_limit,location);
        produced_+=result.size();finished_=final;return result;
    }
    auto end=view.size();
    if(!final) {
        if(encoding_.kind==TextEncodingKind::utf8) {
            for(std::size_t length=1;length<=3 && length<=view.size();++length) {
                const auto pos=view.size()-length;const auto lead=byte(view,pos);
                if(lead>=0x80 && lead<=0xbf)continue;
                const unsigned width=lead>=0xc2 && lead<=0xdf ? 2U : lead>=0xe0 && lead<=0xef ? 3U : lead>=0xf0 && lead<=0xf4 ? 4U : 0U;
                bool prefix=width && length<width;
                for(std::size_t n=1;prefix && n<length;++n) {
                    const auto next=byte(view,pos+n);
                    prefix=next>=0x80 && next<=0xbf && !(n==1 && ((lead==0xe0 && next<0xa0) || (lead==0xed && next>0x9f)
                        || (lead==0xf0 && next<0x90) || (lead==0xf4 && next>0x8f)));
                }
                if(prefix)end=pos;
                break;
            }
        } else if(encoding_.kind==TextEncodingKind::utf16_le || encoding_.kind==TextEncodingKind::utf16_be) {
            end-=end%2;
            if(end>=2){const auto last=unit(view,end-2,2,encoding_.kind==TextEncodingKind::utf16_be);if(last>=0xd800 && last<=0xdbff)end-=2;}
        } else if(encoding_.kind==TextEncodingKind::utf32_le || encoding_.kind==TextEncodingKind::utf32_be)end-=end%4;
        else if(const auto* codec=double_byte_codec(static_cast<std::uint16_t>(encoding_.kind))){
            for(std::size_t index=0;index<view.size();){
                if(double_byte_lead(*codec,static_cast<std::uint8_t>(view[index]))){if(index+1==view.size()){end=index;break;}index+=2;}
                else ++index;
            }
        }
    }
    auto result=decode_text_bytes(view.substr(0,end),encoding_,false,location);
    if(result.size()>data_limit-produced_)throw runtime_error(RuntimeError::encoding_limit,location);
    produced_+=result.size();carry_.assign(view.substr(end));finished_=final;return result;
}
std::string encode_text_bytes(std::u16string_view text,TextEncoding encoding,SourceLocation location) {
    validate(encoding,location);
    if(text.size()>data_limit)throw runtime_error(RuntimeError::encoding_limit,location);
    if(iscii_code_page(static_cast<std::uint16_t>(encoding.kind))){IsciiEncoderState state;return iscii_encode(text,static_cast<std::uint16_t>(encoding.kind),state,true,encoding.strict,location);}
    if(encoding.kind==TextEncodingKind::gb18030)return gb18030_encode(text,encoding.strict,location);
    std::string result;result.reserve(text.size());
    auto append=[&](std::uint32_t value){if(result.size()>=data_limit)throw runtime_error(RuntimeError::encoding_limit,location);result.push_back(static_cast<char>(value));};
    if(const auto* codec=double_byte_codec(static_cast<std::uint16_t>(encoding.kind))){
        for(char16_t value:text){
            const auto encoded=double_byte_encode(*codec,value);
            if(encoding.strict && !(encoded&0x10000U))throw runtime_error(RuntimeError::encoding_data,location);
            const auto code=encoded&65535U;if(code>255)append(code>>8);append(code&255U);
        }
        return result;
    }
    if(const auto* codec=single_byte_codec(static_cast<std::uint16_t>(encoding.kind))) {
        for(char16_t value:text) {
            const auto encoded=single_byte_encode(*codec,value);
            if(encoding.strict && !(encoded&256U))throw runtime_error(RuntimeError::encoding_data,location);
            append(encoded&255U);
        }
        return result;
    }
    const bool big_endian=encoding.kind==TextEncodingKind::utf16_be || encoding.kind==TextEncodingKind::utf32_be;
    auto append_unit=[&](std::uint32_t value,unsigned width){for(unsigned n=0;n<width;++n)append((value>>((big_endian ? width-1-n : n)*8))&255);};
    for(std::size_t index=0;index<text.size();++index) {
        std::uint32_t value=text[index];
        if(value>=0xd800 && value<=0xdbff && index+1<text.size() && text[index+1]>=0xdc00 && text[index+1]<=0xdfff) {
            value=0x10000+((value-0xd800)<<10)+(text[++index]-0xdc00);
        } else if(value>=0xd800 && value<=0xdfff)value=invalid(encoding,location);
        if(encoding.kind==TextEncodingKind::utf8) {
            if(value<0x80)append(value);
            else if(value<0x800){append(0xc0|(value>>6));append(0x80|(value&0x3f));}
            else if(value<0x10000){append(0xe0|(value>>12));append(0x80|((value>>6)&0x3f));append(0x80|(value&0x3f));}
            else {append(0xf0|(value>>18));append(0x80|((value>>12)&0x3f));append(0x80|((value>>6)&0x3f));append(0x80|(value&0x3f));}
        } else if(encoding.kind==TextEncodingKind::utf32_le || encoding.kind==TextEncodingKind::utf32_be)append_unit(value,4);
        else if(value<=0xffff)append_unit(value,2);
        else {append_unit(0xd800+((value-0x10000)>>10),2);append_unit(0xdc00+((value-0x10000)&0x3ff),2);}
    }
    return result;
}
std::u16string decode_file_bytes(std::string_view bytes,SourceLocation location) {
    if (bytes.size()>data_limit) throw runtime_error(RuntimeError::file_limit,location);
    try {return decode_text_bytes(bytes,{TextEncodingKind::utf8,true,true},true,location);}
    catch(const Error& error) {
        if(error.code!="E_ENCODING")throw;
        throw runtime_error(RuntimeError::file_encoding,location);
    }
}
} // namespace abc

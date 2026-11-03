#include "file_encoding.hpp"
#include "cp1251.hpp"
#include "diagnostics.hpp"

namespace abc {
namespace {
constexpr std::size_t data_limit=1'048'576;
constexpr char16_t replacement=0xfffd;
bool valid(TextEncodingKind kind) {
    return kind==TextEncodingKind::utf8 || kind==TextEncodingKind::utf16_le || kind==TextEncodingKind::utf16_be
        || kind==TextEncodingKind::utf32_le || kind==TextEncodingKind::utf32_be || kind==TextEncodingKind::windows1251 || kind==TextEncodingKind::ascii;
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
std::u16string decode_text_bytes(std::string_view bytes,TextEncoding encoding,bool detect_preamble,SourceLocation location) {
    validate(encoding,location);
    if(bytes.size()>data_limit)throw runtime_error(RuntimeError::encoding_limit,location);
    if(detect_preamble) {
        const auto expected=text_encoding_preamble(encoding);
        if(!expected.empty() && bytes.starts_with(expected))bytes.remove_prefix(expected.size());
        else for(auto kind:{TextEncodingKind::utf32_le,TextEncodingKind::utf32_be,TextEncodingKind::utf16_le,TextEncodingKind::utf16_be,TextEncodingKind::utf8}) {
            const auto mark=text_encoding_preamble({kind,true,false});
            if(bytes.starts_with(mark)){encoding={kind,true,false};bytes.remove_prefix(mark.size());break;}
        }
    }
    if(encoding.kind==TextEncodingKind::utf8)return decode_utf8(bytes,encoding,location);
    if(encoding.kind==TextEncodingKind::utf16_le || encoding.kind==TextEncodingKind::utf16_be)return decode_utf16(bytes,encoding,encoding.kind==TextEncodingKind::utf16_be,location);
    if(encoding.kind==TextEncodingKind::utf32_le || encoding.kind==TextEncodingKind::utf32_be)return decode_utf32(bytes,encoding,encoding.kind==TextEncodingKind::utf32_be,location);
    std::u16string result;result.reserve(bytes.size());
    for(unsigned char value:bytes)result.push_back(encoding.kind==TextEncodingKind::windows1251 ? windows1251_character(value)
        : value<128 ? static_cast<char16_t>(value) : encoding.strict ? invalid(encoding,location) : char16_t{u'?'});
    return result;
}
std::string encode_text_bytes(std::u16string_view text,TextEncoding encoding,SourceLocation location) {
    validate(encoding,location);
    if(text.size()>data_limit)throw runtime_error(RuntimeError::encoding_limit,location);
    std::string result;result.reserve(text.size());
    auto append=[&](std::uint32_t value){if(result.size()>=data_limit)throw runtime_error(RuntimeError::encoding_limit,location);result.push_back(static_cast<char>(value));};
    if(encoding.kind==TextEncodingKind::ascii || encoding.kind==TextEncodingKind::windows1251) {
        for(char16_t value:text) {
            const auto encoded=encoding.kind==TextEncodingKind::windows1251 ? windows1251_ordinal(value) : value<128 ? static_cast<std::uint8_t>(value) : std::uint8_t{63};
            if(encoding.strict && (encoding.kind==TextEncodingKind::ascii ? value>=128 : windows1251_character(encoded)!=value))throw runtime_error(RuntimeError::encoding_data,location);
            append(encoded);
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

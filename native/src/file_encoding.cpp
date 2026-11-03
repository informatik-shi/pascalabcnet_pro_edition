#include "file_encoding.hpp"
#include "diagnostics.hpp"

namespace abc {
namespace {
constexpr std::size_t file_limit=1'048'576;
constexpr char16_t replacement=0xfffd;
std::uint32_t byte(std::string_view bytes,std::size_t index) {
    return static_cast<unsigned char>(bytes[index]);
}
std::uint32_t unit(std::string_view bytes,std::size_t index,unsigned width,bool big_endian) {
    std::uint32_t result=0;
    for (unsigned n=0;n<width;++n) result=(result<<8)|byte(bytes,index+(big_endian ? n : width-1-n));
    return result;
}
std::u16string decode_utf16(std::string_view bytes,bool big_endian) {
    std::u16string result;result.reserve((bytes.size()+1)/2);
    char16_t high=0;
    for (std::size_t index=0;index+1<bytes.size();index+=2) {
        const auto value=static_cast<char16_t>(unit(bytes,index,2,big_endian));
        if (high) {
            if (value>=0xdc00 && value<=0xdfff) {
                result.push_back(high);result.push_back(value);high=0;continue;
            }
            result.push_back(replacement);high=0;
        }
        if (value>=0xd800 && value<=0xdbff) high=value;
        else result.push_back(value>=0xdc00 && value<=0xdfff ? replacement : value);
    }
    if (high) result.push_back(replacement);
    if (bytes.size()%2) result.push_back(replacement);
    return result;
}
std::u16string decode_utf32(std::string_view bytes,bool big_endian) {
    std::u16string result;result.reserve((bytes.size()+3)/4);
    for (std::size_t index=0;index+3<bytes.size();index+=4) {
        const auto value=unit(bytes,index,4,big_endian);
        if (value>0x10ffff || (value>=0xd800 && value<=0xdfff)) result.push_back(replacement);
        else if (value<=0xffff) result.push_back(static_cast<char16_t>(value));
        else {
            result.push_back(static_cast<char16_t>(0xd800+((value-0x10000)>>10)));
            result.push_back(static_cast<char16_t>(0xdc00+((value-0x10000)&0x3ff)));
        }
    }
    if (bytes.size()%4) result.push_back(replacement);
    return result;
}
}

std::u16string decode_file_bytes(std::string_view bytes,SourceLocation location) {
    if (bytes.size()>file_limit) throw runtime_error(RuntimeError::file_limit,location);
    // UTF-32LE shares the first two bytes of UTF-16LE, so test the longer mark first.
    if (bytes.starts_with(std::string_view{"\xff\xfe\0\0",4})) return decode_utf32(bytes.substr(4),false);
    if (bytes.starts_with(std::string_view{"\0\0\xfe\xff",4})) return decode_utf32(bytes.substr(4),true);
    if (bytes.starts_with(std::string_view{"\xff\xfe",2})) return decode_utf16(bytes.substr(2),false);
    if (bytes.starts_with(std::string_view{"\xfe\xff",2})) return decode_utf16(bytes.substr(2),true);
    if (bytes.starts_with(std::string_view{"\xef\xbb\xbf",3})) bytes.remove_prefix(3);
    try { return from_utf8(bytes); }
    catch (const std::invalid_argument&) { throw runtime_error(RuntimeError::file_encoding,location); }
}
} // namespace abc

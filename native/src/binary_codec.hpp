#pragma once
#include "binary_stream.hpp"
#include "file_encoding.hpp"
namespace abc {
class BinaryCodec {
    BinaryDevice& stream_;
    TextEncoding encoding_;
    TextEncoder encoder_;
    TextDecoder decoder_;
    std::u16string decode(std::string_view,std::size_t capacity,SourceLocation);
public:
    BinaryCodec(BinaryDevice& stream,TextEncoding encoding):stream_(stream),encoding_(encoding),encoder_(encoding),decoder_(encoding,false){}
    Value read(Type,SourceLocation,const ExecutionOptions&);
    void write(Type,const Value&,SourceLocation,const ExecutionOptions&);
    char16_t read_character(SourceLocation,const ExecutionOptions&);
    std::u16string read_string(SourceLocation,const ExecutionOptions&);
    void write_character(char16_t,SourceLocation);
    void write_string(std::u16string_view,SourceLocation,const ExecutionOptions&);
};
}

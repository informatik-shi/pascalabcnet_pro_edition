#pragma once
#include "abc/core.hpp"
#include "binary_codec.hpp"
#include <span>
namespace abc {
struct Values;class FileContext;
enum class BinaryMethod:std::uint8_t {assign,reset,rewrite,reset_named,rewrite_named,reset_encoding,rewrite_encoding,reset_named_encoding,rewrite_named_encoding,close,truncate,eof,position,size,seek,name,full_name,to_string,erase,rename,read_data,write_one,write_many,read_bytes,write_bytes,writeln,writeln_data,write_empty};
struct BinaryFile {
    std::filesystem::path filename;
    std::unique_ptr<BinaryStream> stream;
    std::unique_ptr<BinaryCodec> codec;
    Type element=Type::invalid;
    std::int64_t element_size=1;
    bool is_open() const noexcept{return stream&&stream->is_open();}
};
bool binary_value_supported(const std::vector<TypeInfo>&,Type,bool strings,bool arrays=false);
std::int64_t binary_element_size(const Program&,Type,SourceLocation);
void binary_element_probe(Values&,Type);
std::vector<Type> binary_parameters(const std::vector<TypeInfo>&,Type,std::size_t);
Type binary_result(const std::vector<TypeInfo>&,Type,std::size_t);
bool binary_signature_valid(const Program&,Type,std::size_t);
Value binary_invoke(Values&,FileContext&,Type,std::size_t,std::span<const Value>);
}

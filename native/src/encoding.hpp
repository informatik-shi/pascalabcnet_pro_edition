#pragma once
#include "abc/core.hpp"
#include <map>
#include <span>

namespace abc {
struct Values;
class FileContext;
enum class EncodingMethod : std::uint8_t { from_code_page,from_name,utf8,utf16_le,utf16_be,utf32_le,ascii,code_page,preamble,bytes_string,bytes_characters,string_bytes,characters_bytes,file_read,file_write,text_open_read,text_open_write,text_open_append,text_reset,text_rewrite,text_append,text_reset_named,text_rewrite_named,text_append_named };
const TextEncoding& encoding_value(const Value& value,SourceLocation location);
bool encoding_signature_valid(const Program& program,Type encoding,EncodingMethod method);
std::vector<Type> encoding_parameters(const TypeInfo& type,Type encoding,EncodingMethod method);
std::vector<Type> encoding_parameters(const Program& program,Type encoding,EncodingMethod method);
Type encoding_result(const TypeInfo& type,Type encoding,EncodingMethod method);
Type encoding_result(const Program& program,Type encoding,EncodingMethod method);
struct EncodingContext {
    std::map<std::pair<Type,std::int32_t>,std::shared_ptr<Aggregate>> cache;
    Value invoke(Values& values,FileContext& files,Type encoding,EncodingMethod method,std::span<const Value> arguments);
};
}

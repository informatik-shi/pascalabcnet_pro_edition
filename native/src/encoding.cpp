#include "encoding.hpp"
#include "file_encoding.hpp"
#include "files.hpp"
#include "text_file.hpp"
#include "runtime_values.hpp"
#include "sbcs.hpp"
#include "dbcs.hpp"

namespace abc {
namespace {
TextEncoding descriptor(std::int32_t code,SourceLocation location) {
    switch(code) {
    case 65001:return {TextEncodingKind::utf8,true,false};
    case 1200:return {TextEncodingKind::utf16_le,true,false};
    case 1201:return {TextEncodingKind::utf16_be,true,false};
    case 12000:return {TextEncodingKind::utf32_le,true,false};
    case 12001:return {TextEncodingKind::utf32_be,true,false};
    case 1251:return {TextEncodingKind::windows1251,false,false};
    case 20127:return {TextEncodingKind::ascii,false,false};
    case 54936:return {TextEncodingKind::gb18030,false,false};
    default:
        if(code>0 && code<=65535 && (single_byte_codec(static_cast<std::uint16_t>(code)) || double_byte_codec(static_cast<std::uint16_t>(code)) || iscii_code_page(static_cast<std::uint16_t>(code)) || iso2022_code_page(static_cast<std::uint16_t>(code))))return {static_cast<TextEncodingKind>(code),false,false};
        throw runtime_error(RuntimeError::encoding_kind,location,std::to_string(code));
    }
}
std::int32_t named_code(const Value& name,SourceLocation location) {
    if(std::holds_alternative<NilString>(name))throw runtime_error(RuntimeError::nil_reference,location);
    auto text=string_text(name);for(auto& ch:text)if(ch>=u'A' && ch<=u'Z')ch=static_cast<char16_t>(ch+32);
    if(const auto code=encoding_alias_code(text))return code;
    if(const auto code=double_byte_alias_code(text))return code;
    if(text==u"gb18030")return 54936;
    if(text==u"csiso2022jp")return 50221;
    if(text==u"csiso2022kr")return 50225;
    if(text==u"hz-gb-2312")return 52936;
    if(text==u"iso-2022-jp")return 50220;
    if(text==u"iso-2022-kr")return 50225;
    if(text==u"iso-2022-kr-7")return 50225;
    if(text==u"iso-2022-kr-7bit")return 50225;
    if(text==u"x-iscii-as")return 57006;
    if(text==u"x-iscii-be")return 57003;
    if(text==u"x-iscii-de")return 57002;
    if(text==u"x-iscii-gu")return 57010;
    if(text==u"x-iscii-ka")return 57008;
    if(text==u"x-iscii-ma")return 57009;
    if(text==u"x-iscii-or")return 57007;
    if(text==u"x-iscii-pa")return 57011;
    if(text==u"x-iscii-ta")return 57004;
    if(text==u"x-iscii-te")return 57005;
    throw runtime_error(RuntimeError::encoding_kind,location,to_utf8(text,true));
}
} // namespace
const TextEncoding& encoding_value(const Value& value,SourceLocation location) {
    const auto& object=std::get<std::shared_ptr<Aggregate>>(value);
    if(!object || object->nil)throw runtime_error(RuntimeError::nil_reference,location);
    if(!object->encoding)throw runtime_error(RuntimeError::invalid_bytecode,location);
    return *object->encoding;
}
namespace {
std::shared_ptr<Aggregate> byte_array(Values& values,Type type,std::string_view bytes) {
    auto result=values.array(type,bytes.size());
    for(std::size_t i=0;i<bytes.size();++i)result->items[i]->value=static_cast<std::uint8_t>(static_cast<unsigned char>(bytes[i]));
    return result;
}
}
std::vector<Type> encoding_parameters(const Program& program,Type encoding,EncodingMethod method) { return encoding_parameters(type_info(program,encoding),encoding,method); }
std::vector<Type> encoding_parameters(const TypeInfo& type,Type encoding,EncodingMethod method) {
    switch(method) {
    case EncodingMethod::from_code_page:return {Type::integer};
    case EncodingMethod::from_name:return {Type::string};
    case EncodingMethod::utf8:case EncodingMethod::utf16_le:case EncodingMethod::utf16_be:case EncodingMethod::utf32_le:case EncodingMethod::ascii:return {};
    case EncodingMethod::code_page:case EncodingMethod::preamble:return {encoding};
    case EncodingMethod::bytes_string:return {encoding,Type::string};
    case EncodingMethod::bytes_characters:return {encoding,type.key};
    case EncodingMethod::string_bytes:case EncodingMethod::characters_bytes:return {encoding,type.element};
    case EncodingMethod::file_read:return {Type::string,encoding};
    case EncodingMethod::file_write:return {Type::string,Type::string,encoding};
    case EncodingMethod::text_open_read:case EncodingMethod::text_open_write:case EncodingMethod::text_open_append:return {Type::string,encoding};
    case EncodingMethod::text_reset:case EncodingMethod::text_rewrite:case EncodingMethod::text_append:return {Type::text_file,encoding};
    case EncodingMethod::text_reset_named:case EncodingMethod::text_rewrite_named:case EncodingMethod::text_append_named:return {Type::text_file,Type::string,encoding};
    }
    throw runtime_error(RuntimeError::invalid_bytecode,{});
}
Type encoding_result(const Program& program,Type encoding,EncodingMethod method) { return encoding_result(type_info(program,encoding),encoding,method); }
Type encoding_result(const TypeInfo& type,Type encoding,EncodingMethod method) {
    switch(method) {
    case EncodingMethod::from_code_page:case EncodingMethod::from_name:case EncodingMethod::utf8:case EncodingMethod::utf16_le:case EncodingMethod::utf16_be:case EncodingMethod::utf32_le:case EncodingMethod::ascii:return encoding;
    case EncodingMethod::code_page:return Type::integer;
    case EncodingMethod::preamble:case EncodingMethod::bytes_string:case EncodingMethod::bytes_characters:return type.element;
    case EncodingMethod::string_bytes:case EncodingMethod::file_read:return Type::string;
    case EncodingMethod::characters_bytes:return type.key;
    case EncodingMethod::file_write:case EncodingMethod::text_reset:case EncodingMethod::text_rewrite:case EncodingMethod::text_append:
    case EncodingMethod::text_reset_named:case EncodingMethod::text_rewrite_named:case EncodingMethod::text_append_named:return Type::unit;
    case EncodingMethod::text_open_read:case EncodingMethod::text_open_write:case EncodingMethod::text_open_append:return Type::text_file;
    }
    throw runtime_error(RuntimeError::invalid_bytecode,{});
}
bool encoding_signature_valid(const Program& program,Type encoding,EncodingMethod method) {
    if(static_cast<unsigned>(method)>static_cast<unsigned>(EncodingMethod::text_append_named) || !compound(encoding) || !valid_value_type(program,encoding))return false;
    const auto& type=type_info(program,encoding);
    if(type.kind!=CompoundKind::encoding)return false;
    const auto array=[&](Type value,Type element) {
        if(!compound(value) || !valid_value_type(program,value))return false;
        const auto& info=type_info(program,value);return info.kind==CompoundKind::array && info.rank==1 && info.element==element;
    };
    return array(type.element,Type::byte) && array(type.key,Type::character);
}
Value EncodingContext::invoke(Values& values,FileContext& files,Type encoding,EncodingMethod method,std::span<const Value> arguments) {
    const auto location=values.location;const auto& type=type_info(values.program,encoding);
    if(method<=EncodingMethod::ascii) {
        const auto code=method==EncodingMethod::from_code_page ? std::get<std::int32_t>(arguments[0]) : method==EncodingMethod::from_name ? named_code(arguments[0],location)
            : method==EncodingMethod::utf8 ? 65001 : method==EncodingMethod::utf16_le ? 1200 : method==EncodingMethod::utf16_be ? 1201 : method==EncodingMethod::utf32_le ? 12000 : 20127;
        const auto data=descriptor(code,location);const auto key=std::pair{encoding,code};
        if(const auto found=cache.find(key);found!=cache.end())return found->second;
        if(!values.remaining)throw runtime_error(RuntimeError::allocation_limit,location);
        --values.remaining;auto result=std::make_shared<Aggregate>();result->type=encoding;result->encoding=std::make_shared<const TextEncoding>(data);cache.emplace(key,result);return result;
    }
    if(method==EncodingMethod::file_read || method==EncodingMethod::file_write) {
        // Validate the path before the encoding, as the source File overload does.
        const auto& path=string_text(arguments[0]);(void)files.resolve(path,location);
        const auto& data=encoding_value(arguments.back(),location);
        if(method==EncodingMethod::file_read)return read_text_file(files,path,location,&data);
        write_text_file(files,path,string_text(arguments[1]),location,&data);return false;
    }
    if(method>=EncodingMethod::text_open_read) {
        const auto method_number=static_cast<unsigned>(method)-static_cast<unsigned>(EncodingMethod::text_open_read);
        const auto mode=method_number%3;const bool create=method_number<3,named=method_number>=6;
        auto file=create ? std::make_shared<TextFile>() : std::get<std::shared_ptr<TextFile>>(arguments[0]);
        (void)text_file(Value{file},location);
        if(create || named)text_assign(files,*file,arguments[create ? 0 : 1],location);
        text_open(files,file,mode,location,&arguments.back());
        if(create)return file;return false;
    }
    const auto& data=encoding_value(arguments[0],location);
    if(method==EncodingMethod::code_page)return static_cast<std::int32_t>(data.kind);
    if(method==EncodingMethod::preamble)return byte_array(values,type.element,text_encoding_preamble(data));
    if(method==EncodingMethod::bytes_string || method==EncodingMethod::bytes_characters) {
        std::u16string text;
        if(method==EncodingMethod::bytes_string) {
            if(std::holds_alternative<NilString>(arguments[1]))throw runtime_error(RuntimeError::nil_reference,location);
            text=string_text(arguments[1]);
        } else {
            const auto& source=std::get<std::shared_ptr<Aggregate>>(arguments[1]);
            if(!source || source->nil)throw runtime_error(RuntimeError::nil_reference,location);
            text.reserve(source->items.size());for(const auto& item:source->items)text.push_back(std::get<char16_t>(item->value));
        }
        return byte_array(values,type.element,encode_text_bytes(text,data,location));
    }
    const auto& source=std::get<std::shared_ptr<Aggregate>>(arguments[1]);
    if(!source || source->nil)throw runtime_error(RuntimeError::nil_reference,location);
    std::string bytes;bytes.reserve(source->items.size());for(const auto& item:source->items)bytes.push_back(static_cast<char>(std::get<std::uint8_t>(item->value)));
    auto text=decode_text_bytes(bytes,data,false,location);
    if(method==EncodingMethod::string_bytes)return text;
    auto result=values.array(type.key,text.size());for(std::size_t i=0;i<text.size();++i)result->items[i]->value=text[i];return result;
}
}

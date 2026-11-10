#include "binary_codec.hpp"
#include "encoding_metadata.hpp"
#include "diagnostics.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <limits>
namespace abc {
namespace {
void checkpoint(std::uint64_t& work,SourceLocation location,const ExecutionOptions& options){
    if(work>=options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,location);
    if((work++&255U)==0&&options.cancelled&&options.cancelled())throw runtime_error(RuntimeError::cancelled,location);
}
unsigned width(Type type){
    switch(type){case Type::boolean:case Type::byte:case Type::shortint:return 1;
    case Type::word:case Type::smallint:return 2;
    case Type::integer:case Type::longword:case Type::single:return 4;
    case Type::int64:case Type::uint64:case Type::real:return 8;
    default:throw runtime_error(RuntimeError::invalid_bytecode,{});}
}
}
std::u16string BinaryCodec::decode(std::string_view bytes,std::size_t capacity,SourceLocation location){
    auto next=decoder_;next.reset_operation_budget();auto value=next.decode(bytes,false,location);
    if(value.size()>capacity)throw runtime_error(RuntimeError::binary_character_buffer,location);
    decoder_=std::move(next);return value;
}
char16_t BinaryCodec::read_character(SourceLocation location,const ExecutionOptions& options){
    const auto start=stream_.position(location);std::uint64_t work=0;
    const bool two=encoding_.kind==TextEncodingKind::utf16_le||encoding_.kind==TextEncodingKind::utf16_be;
    for(;;){checkpoint(work,location,options);std::array<char,2> bytes{};auto count=stream_.read(std::span<char>(bytes).first(1),location);if(!count)throw runtime_error(RuntimeError::binary_end,location);
        if(two)count+=stream_.read(std::span<char>(bytes).subspan(1,1),location);
        std::u16string text;
        try{text=decode(std::string_view(bytes.data(),count),1,location);}catch(...){stream_.seek(start,location);throw;}
        if(!text.empty())return text.front();
    }
}
std::u16string BinaryCodec::read_string(SourceLocation location,const ExecutionOptions& options){
    std::uint32_t length=0;unsigned shift=0;std::uint64_t work=0;
    for(;;){checkpoint(work,location,options);if(shift==35)throw runtime_error(RuntimeError::binary_prefix,location);std::array<char,1> b{};stream_.read_exact(b,location);const auto n=static_cast<std::uint8_t>(b[0]);length|=static_cast<std::uint32_t>(n&127U)<<shift;shift+=7;if(!(n&128U))break;}
    if(length&0x80000000U)throw runtime_error(RuntimeError::binary_string_length,location);
    const auto maximum=std::get<std::int32_t>(encoding_metadata_value(static_cast<std::uint16_t>(encoding_.kind),EncodingMethod::max_char_count,128,location));
    std::u16string result;std::array<char,128> bytes{};
    while(length){checkpoint(work,location,options);const auto count=stream_.read(std::span<char>(bytes).first(std::min<std::size_t>(bytes.size(),length)),location);if(!count)throw runtime_error(RuntimeError::binary_end,location);
        auto part=decode(std::string_view(bytes.data(),count),static_cast<std::size_t>(maximum),location);
        if(part.size()>1'048'576-result.size())throw runtime_error(RuntimeError::string_limit,location);result+=part;length-=static_cast<std::uint32_t>(count);
    }return result;
}
void BinaryCodec::write_character(char16_t value,SourceLocation location){
    if(value>=0xd800&&value<=0xdfff)throw runtime_error(RuntimeError::binary_character,location);
    auto next=encoder_;const auto bytes=next.encode(std::u16string_view(&value,1),true,location);stream_.write(bytes,location);encoder_=std::move(next);
}
void BinaryCodec::write_string(std::u16string_view text,SourceLocation location,const ExecutionOptions& options){
    if(text.size()>1'048'576)throw runtime_error(RuntimeError::string_limit,location);
    // Source BinaryWriter counts with a fresh encoding, but keeps a separate
    // encoder for Write(char) and strings whose byte length exceeds 256.
    TextEncoder fresh(encoding_);std::uint32_t length=0;std::uint64_t work=0;
    for(std::size_t p=0;p<text.size();){checkpoint(work,location,options);const auto count=std::min<std::size_t>(1024,text.size()-p);length+=static_cast<std::uint32_t>(fresh.encode(text.substr(p,count),p+count==text.size(),location).size());p+=count;}
    auto prefix=length;do{const char b=static_cast<char>((prefix&127U)|(prefix>=128?128U:0U));stream_.write(std::string_view(&b,1),location);prefix>>=7;}while(prefix);
    if(length<=256){stream_.write(encode_text_bytes(text,encoding_,location),location);return;}
    const auto maximum=std::get<std::int32_t>(encoding_metadata_value(static_cast<std::uint16_t>(encoding_.kind),EncodingMethod::max_byte_count,1,location));
    const auto chunk=256U/static_cast<unsigned>(maximum);
    for(std::size_t p=0;p<text.size();){checkpoint(work,location,options);const auto count=std::min<std::size_t>(chunk,text.size()-p);auto next=encoder_;const auto bytes=next.encode(text.substr(p,count),p+count==text.size(),location);stream_.write(bytes,location);encoder_=std::move(next);p+=count;}
}
Value BinaryCodec::read(Type type,SourceLocation location,const ExecutionOptions& options){
    if(type==Type::character)return read_character(location,options);
    if(type==Type::string)return read_string(location,options);
    std::array<char,8> bytes{};const auto count=width(type);stream_.read_exact(std::span<char>(bytes).first(count),location);std::uint64_t bits=0;
    for(unsigned k=0;k<count;++k)bits|=static_cast<std::uint64_t>(static_cast<std::uint8_t>(bytes[k]))<<(k*8);
    switch(type){case Type::boolean:return bits!=0;case Type::byte:return static_cast<std::uint8_t>(bits);case Type::shortint:return std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(bits));
    case Type::word:return static_cast<std::uint16_t>(bits);case Type::smallint:return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(bits));
    case Type::integer:return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(bits));case Type::longword:return static_cast<std::uint32_t>(bits);
    case Type::int64:return std::bit_cast<std::int64_t>(bits);case Type::uint64:return bits;
    case Type::single:return std::bit_cast<float>(static_cast<std::uint32_t>(bits));case Type::real:return std::bit_cast<double>(bits);
    default:throw runtime_error(RuntimeError::invalid_bytecode,location);}
}
void BinaryCodec::write(Type type,const Value& value,SourceLocation location,const ExecutionOptions& options){
    if(type==Type::character){write_character(std::get<char16_t>(value),location);return;}
    if(type==Type::string){if(std::holds_alternative<NilString>(value))throw runtime_error(RuntimeError::nil_reference,location);write_string(std::get<std::u16string>(value),location,options);return;}
    std::uint64_t bits=0;const auto count=width(type);
    switch(type){case Type::boolean:bits=std::get<bool>(value)?1U:0U;break;
    case Type::byte:bits=std::get<std::uint8_t>(value);break;case Type::shortint:bits=static_cast<std::uint8_t>(std::get<std::int8_t>(value));break;
    case Type::word:bits=std::get<std::uint16_t>(value);break;case Type::smallint:bits=static_cast<std::uint16_t>(std::get<std::int16_t>(value));break;
    case Type::integer:bits=static_cast<std::uint32_t>(std::get<std::int32_t>(value));break;case Type::longword:bits=std::get<std::uint32_t>(value);break;
    case Type::int64:bits=static_cast<std::uint64_t>(std::get<std::int64_t>(value));break;case Type::uint64:bits=std::get<std::uint64_t>(value);break;
    case Type::single:bits=std::bit_cast<std::uint32_t>(std::get<float>(value));break;case Type::real:bits=std::bit_cast<std::uint64_t>(std::get<double>(value));break;
    default:throw runtime_error(RuntimeError::invalid_bytecode,location);}
    std::array<char,8> bytes{};for(unsigned k=0;k<count;++k)bytes[k]=static_cast<char>(bits>>(k*8));stream_.write(std::string_view(bytes.data(),count),location);
}
}

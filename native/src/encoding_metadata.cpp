#include "encoding_metadata.hpp"
#include "diagnostics.hpp"
#include <algorithm>
#include <iterator>
#include <limits>
#include <string_view>
namespace abc {
namespace {
struct Metadata {
    std::uint16_t code_page,windows_code_page;
    std::uint8_t flags,byte_multiplier,byte_extra,char_divisor,char_offset,char_extra,char_zero;
    std::u16string_view web,body,header;
};
#include "encoding_metadata_data.inc"
}
Value encoding_metadata_value(std::uint16_t code,EncodingMethod method,std::int32_t count,SourceLocation location){
    const auto found=std::lower_bound(std::begin(metadata),std::end(metadata),code,[](const auto& row,unsigned page){return row.code_page<page;});
    if(found==std::end(metadata)||found->code_page!=code)throw runtime_error(RuntimeError::encoding_kind,location);
    const auto& row=*found;
    switch(method){
    case EncodingMethod::web_name:return std::u16string(row.web);
    case EncodingMethod::body_name:return std::u16string(row.body);
    case EncodingMethod::header_name:return std::u16string(row.header);
    case EncodingMethod::windows_code_page:return static_cast<std::int32_t>(row.windows_code_page);
    case EncodingMethod::is_single_byte:return (row.flags&1U)!=0;
    case EncodingMethod::is_browser_display:return (row.flags&2U)!=0;
    case EncodingMethod::is_browser_save:return (row.flags&4U)!=0;
    case EncodingMethod::is_mail_display:return (row.flags&8U)!=0;
    case EncodingMethod::is_mail_save:return (row.flags&16U)!=0;
    case EncodingMethod::is_read_only:return (row.flags&32U)!=0;
    case EncodingMethod::max_byte_count:case EncodingMethod::max_char_count:{
        if(count<0)throw runtime_error(RuntimeError::encoding_count,location);
        const auto input=static_cast<std::int64_t>(count);
        const auto value=method==EncodingMethod::max_byte_count?input*row.byte_multiplier+row.byte_extra
            :count==0?row.char_zero:input/row.char_divisor+(input%row.char_divisor+row.char_offset)/row.char_divisor+row.char_extra;
        if(value>std::numeric_limits<std::int32_t>::max())throw runtime_error(RuntimeError::encoding_count,location);
        return static_cast<std::int32_t>(value);
    }
    default:throw runtime_error(RuntimeError::invalid_bytecode,location);
    }
}
}

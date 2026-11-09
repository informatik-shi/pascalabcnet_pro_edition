#pragma once
#include "abc/core.hpp"
#include <string_view>
namespace abc {
std::string gb18030_encode(std::u16string_view,bool strict,SourceLocation);
// pending is the number of bytes retained by an earlier decoder call.
std::u16string gb18030_decode(std::string_view,std::size_t pending,bool final,bool strict,std::string& retained,SourceLocation);
}

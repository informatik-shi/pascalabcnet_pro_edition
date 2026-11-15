#pragma once
#include "abc/core.hpp"
#include <functional>

namespace abc {
std::u16string format_scalar(const Value&,SourceLocation);
void composite_format(const Program&,const Value& format,const Value& arguments,char16_t group_separator,
    SourceLocation,const std::function<void(std::u16string_view)>& append);
}

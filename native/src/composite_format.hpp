#pragma once
#include "abc/core.hpp"
#include <functional>

namespace abc {
struct NumberFormatSettings;
std::u16string format_scalar(const Value&,SourceLocation);
std::u16string format_number(const Value&,Type,std::u16string_view,const NumberFormatSettings&,SourceLocation);
std::u16string localize_number(std::u16string_view,const NumberFormatSettings&,SourceLocation);
void composite_format(const Program&,const Value& format,const Value& arguments,const NumberFormatSettings&,
    SourceLocation,const std::function<void(std::u16string_view)>& append);
}

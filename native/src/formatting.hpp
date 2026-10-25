#pragma once
#include "abc/core.hpp"

namespace abc {
std::u16string format_field(const Value& value, std::int32_t width, std::optional<std::int32_t> digits, SourceLocation location);
} // namespace abc

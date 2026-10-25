#pragma once
#include "abc/core.hpp"

namespace abc {
bool object_text_supported(const std::vector<TypeInfo>& types, Type type);
std::u16string object_text(const Program& program, const Value& value, Type type, SourceLocation location,
    const ExecutionOptions& options);
std::u16string pad_field(std::u16string text, std::int32_t width, SourceLocation location);
std::u16string format_field(const Value& value, std::int32_t width, std::optional<std::int32_t> digits, SourceLocation location);
} // namespace abc

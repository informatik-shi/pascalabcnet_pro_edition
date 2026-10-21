#pragma once
#include "abc/core.hpp"
namespace abc {
std::u16string read_text_file(std::u16string_view path, SourceLocation location);
void write_text_file(std::u16string_view path, std::u16string_view text, SourceLocation location);
bool file_exists(std::u16string_view path);
}

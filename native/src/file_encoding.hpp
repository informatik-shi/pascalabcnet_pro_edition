#pragma once
#include "abc/core.hpp"
#include <string_view>

namespace abc {
// File readers detect a Unicode preamble before applying the current UTF-8 profile.
std::u16string decode_file_bytes(std::string_view bytes,SourceLocation location);
}

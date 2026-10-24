#pragma once
#include "abc/core.hpp"

namespace abc {
class FileContext;
std::u16string file_operations_read(FileContext& files, const Value& name, SourceLocation location);
void file_operations_write(FileContext& files, const Value& name, const Value& text, SourceLocation location);
}

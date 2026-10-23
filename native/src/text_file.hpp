#pragma once
#include "abc/core.hpp"
#include <filesystem>
#include <fstream>

namespace abc {
class FileContext;
struct TextFile {
    std::filesystem::path filename;
    std::u16string content, pending;
    std::size_t cursor=0, position=0, size=0;
    bool reading=false;
    std::fstream writer;
    ~TextFile();
    bool is_open() const { return reading || writer.is_open(); }
    void flush(SourceLocation location);
    void close(SourceLocation location);
};
TextFile& text_file(const Value& value, SourceLocation location);
void text_write(TextFile& file, std::u16string_view value, SourceLocation location);
Value text_read(TextFile& file, Type type, SourceLocation location);
std::u16string text_word(TextFile& file, SourceLocation location);
void text_skip_line(TextFile& file, SourceLocation location);
bool text_eof(TextFile& file, bool line, bool skip_space, SourceLocation location);
void text_assign(FileContext& files, TextFile& file, const Value& name, SourceLocation location);
void text_open(FileContext& files, const std::shared_ptr<TextFile>& file, unsigned mode, SourceLocation location);
} // namespace abc

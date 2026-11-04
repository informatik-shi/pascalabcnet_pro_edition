#pragma once
#include "abc/core.hpp"
#include "file_encoding.hpp"
#include <filesystem>
#include <fstream>

namespace abc {
class FileContext;class LineReader;
struct TextFile {
    std::filesystem::path filename, opened_filename;
    std::u16string pending;
    std::size_t position=0, size=0;
    bool preamble_pending=false;
    std::shared_ptr<LineReader> reader;
    const ExecutionOptions* execution_options=nullptr;
    std::optional<TextEncoding> encoding;
    std::fstream writer;
    ~TextFile();
    bool is_open() const;
    const ExecutionOptions& options() const;
    void flush(SourceLocation location);
    void close(SourceLocation location);
};
TextFile& text_file(const Value& value, SourceLocation location);
void text_write(TextFile& file, std::u16string_view value, SourceLocation location);
Value text_read(TextFile& file, Type type, SourceLocation location);
std::u16string text_word(TextFile& file, SourceLocation location);
std::u16string text_name(const TextFile& file, bool full, SourceLocation location);
std::u16string text_read_end(TextFile& file, SourceLocation location);
void text_skip_line(TextFile& file, SourceLocation location);
bool text_eof(TextFile& file, bool line, bool skip_space, SourceLocation location);
void text_assign(FileContext& files, TextFile& file, const Value& name, SourceLocation location);
void text_open(FileContext& files, const std::shared_ptr<TextFile>& file, unsigned mode, SourceLocation location,const Value* encoding=nullptr);
void text_erase(const FileContext& files, const TextFile& file, SourceLocation location);
void text_rename(FileContext& files, const TextFile& file, const Value& name, SourceLocation location);
} // namespace abc

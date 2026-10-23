#pragma once
#include "abc/core.hpp"
#include <filesystem>
namespace abc {
// Each execution owns its current directory; the editor/host cwd never changes.
class FileContext {
public:
    FileContext();
    std::filesystem::path resolve(std::u16string_view path, SourceLocation location) const;
    std::u16string directory(SourceLocation location) const;
    void change_directory(std::u16string_view path, SourceLocation location);
    bool rename(std::u16string_view source, std::u16string_view destination, bool directory);
private:
    std::filesystem::path current_;
};
std::u16string read_text_file(const FileContext& files, std::u16string_view path, SourceLocation location);
void write_text_file(const FileContext& files, std::u16string_view path, std::u16string_view text, SourceLocation location);
bool file_exists(const FileContext& files, std::u16string_view path);
void create_directory(const FileContext& files, std::u16string_view path, SourceLocation location);
void remove_directory(const FileContext& files, std::u16string_view path, SourceLocation location);
bool delete_file(const FileContext& files, std::u16string_view path);
Value change_file_extension(const Value& path, const Value& extension, SourceLocation location);
}

#pragma once
#include "abc/core.hpp"
#include <filesystem>
#include <functional>
namespace abc {
struct TextEncoding;class LineReader;struct BinaryFile;
struct OpenFiles {
    std::vector<std::weak_ptr<TextFile>> texts;
    std::vector<std::weak_ptr<LineReader>> readers;
    std::vector<std::weak_ptr<BinaryFile>> binaries;
};
// Each execution owns its current directory; the editor/host cwd never changes.
class FileContext {
public:
    FileContext();
    const ExecutionOptions* execution_options=nullptr;
    std::function<const TextEncoding*(SourceLocation)> default_encoding;
    const TextEncoding* encoding(SourceLocation location) const { return default_encoding ? default_encoding(location) : nullptr; }
    std::filesystem::path resolve(std::u16string_view path, SourceLocation location) const;
    std::u16string full_name(std::u16string_view path,SourceLocation location) const;
    std::u16string directory(SourceLocation location) const;
    void change_directory(std::u16string_view path, SourceLocation location);
    bool rename(std::u16string_view source, std::u16string_view destination, bool directory);
    void prepare_text_open(const std::shared_ptr<TextFile>& file, SourceLocation location);
    void prepare_line_open(const std::shared_ptr<LineReader>& reader,SourceLocation location);
    void prepare_binary_open(const std::shared_ptr<BinaryFile>&,SourceLocation);
    void check_binary_conflict(const std::filesystem::path&,SourceLocation) const;
    bool text_file_locked(const std::filesystem::path& path, bool subtree=false) const;
private:
    std::filesystem::path current_;
    void initialize_drives();
    void remember_drive();
#ifdef _WIN32
    std::array<std::filesystem::path,26> drive_directories_;
#endif
    std::unique_ptr<OpenFiles> opened_;
};
std::u16string read_text_file(const FileContext& files, std::u16string_view path, SourceLocation location,const TextEncoding* encoding=nullptr);
void write_text_file(const FileContext& files, std::u16string_view path, std::u16string_view text, SourceLocation location,const TextEncoding* encoding=nullptr);
bool file_exists(const FileContext& files, std::u16string_view path);
void create_directory(const FileContext& files, std::u16string_view path, SourceLocation location);
void remove_directory(const FileContext& files, std::u16string_view path, SourceLocation location);
bool delete_file(const FileContext& files, std::u16string_view path);
Value change_file_extension(const Value& path, const Value& extension, SourceLocation location);
}

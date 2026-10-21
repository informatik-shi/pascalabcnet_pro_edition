#include "files.hpp"
#include "diagnostics.hpp"
#include <filesystem>
#include <fstream>

namespace abc {
namespace {
std::filesystem::path filename(std::u16string_view text, SourceLocation location) {
    if (text.empty() || text.find(char16_t{0}) != std::u16string_view::npos)
        throw runtime_error(RuntimeError::file_io,location);
    auto utf8 = to_utf8(text,true);
    return std::filesystem::path(std::u8string(utf8.begin(),utf8.end()));
}
}
std::u16string read_text_file(std::u16string_view path, SourceLocation location) {
    std::ifstream input(filename(path,location),std::ios::binary);
    if (!input) throw runtime_error(RuntimeError::file_io,location);
    std::string data; char buffer[8192];
    while (input.read(buffer,sizeof buffer) || input.gcount()) {
        data.append(buffer,static_cast<std::size_t>(input.gcount()));
        if (data.size() > 1'048'576) throw runtime_error(RuntimeError::file_limit,location);
    }
    if (input.bad()) throw runtime_error(RuntimeError::file_io,location);
    auto text = std::string_view(data); if (text.starts_with("\xef\xbb\xbf")) text.remove_prefix(3);
    try { return from_utf8(text); }
    catch (const std::invalid_argument&) { throw runtime_error(RuntimeError::file_encoding,location); }
}
void write_text_file(std::u16string_view path, std::u16string_view text, SourceLocation location) {
    const auto data = to_utf8(text,true);
    if (data.size() > 1'048'576) throw runtime_error(RuntimeError::file_limit,location);
    std::ofstream output(filename(path,location),std::ios::binary|std::ios::trunc);
    if (!output || !output.write(data.data(),static_cast<std::streamsize>(data.size())) || !output.flush())
        throw runtime_error(RuntimeError::file_io,location);
    output.close(); if (!output) throw runtime_error(RuntimeError::file_io,location);
}
bool file_exists(std::u16string_view path) {
    if (path.empty() || path.find(char16_t{0}) != std::u16string_view::npos) return false;
    std::error_code error;
    return std::filesystem::is_regular_file(filename(path,{}),error);
}
}

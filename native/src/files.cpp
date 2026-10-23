#include "files.hpp"
#include "diagnostics.hpp"
#include <filesystem>
#include <fstream>
#include <cstdio>
#ifndef _WIN32
#include <unistd.h>
#endif

namespace abc {
FileContext::FileContext() {
    std::error_code error;
    current_ = std::filesystem::current_path(error);
    if (error) current_.clear();
}
std::filesystem::path FileContext::resolve(std::u16string_view text, SourceLocation location) const {
    if (text.empty() || text.find(char16_t{0}) != std::u16string_view::npos)
        throw runtime_error(RuntimeError::file_io,location);
    auto utf8 = to_utf8(text,true);
    const std::filesystem::path input(std::u8string(utf8.begin(),utf8.end()));
    if (current_.empty() && !input.is_absolute()) throw runtime_error(RuntimeError::file_io,location);
    return (current_/input).lexically_normal();
}
std::u16string FileContext::directory(SourceLocation location) const {
    std::error_code error;
    if (current_.empty() || !std::filesystem::is_directory(current_,error) || error)
        throw runtime_error(RuntimeError::directory_io,location);
    try { return current_.u16string(); }
    catch (const std::filesystem::filesystem_error&) { throw runtime_error(RuntimeError::directory_io,location); }
}
namespace {
std::filesystem::path directory_path(const FileContext& files, std::u16string_view path, SourceLocation location) {
    try { return files.resolve(path,location); }
    catch (const Error& error) {
        if (error.code!="E_IO") throw;
        throw runtime_error(RuntimeError::directory_io,location);
    }
}
}
void FileContext::change_directory(std::u16string_view path, SourceLocation location) {
    const auto target=directory_path(*this,path,location);
    std::error_code error;
    if (!std::filesystem::is_directory(target,error) || error) throw runtime_error(RuntimeError::directory_io,location);
#ifndef _WIN32
    if (::access(target.c_str(),X_OK)!=0) throw runtime_error(RuntimeError::directory_io,location);
#endif
    auto absolute=std::filesystem::canonical(target,error);
    if (error) throw runtime_error(RuntimeError::directory_io,location);
    current_=std::move(absolute);
}
std::u16string read_text_file(const FileContext& files, std::u16string_view path, SourceLocation location) {
    std::ifstream input(files.resolve(path,location),std::ios::binary);
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
void write_text_file(const FileContext& files, std::u16string_view path, std::u16string_view text, SourceLocation location) {
    const auto data = to_utf8(text,true);
    if (data.size() > 1'048'576) throw runtime_error(RuntimeError::file_limit,location);
    std::ofstream output(files.resolve(path,location),std::ios::binary|std::ios::trunc);
    if (!output || !output.write(data.data(),static_cast<std::streamsize>(data.size())) || !output.flush())
        throw runtime_error(RuntimeError::file_io,location);
    output.close(); if (!output) throw runtime_error(RuntimeError::file_io,location);
}
bool file_exists(const FileContext& files, std::u16string_view path) {
    if (path.empty() || path.find(char16_t{0}) != std::u16string_view::npos) return false;
    std::error_code error;
    try { return std::filesystem::is_regular_file(files.resolve(path,{}),error); }
    catch (const Error&) { return false; }
}
void create_directory(const FileContext& files, std::u16string_view path, SourceLocation location) {
    const auto target=directory_path(files,path,location);
    std::error_code error;
    std::filesystem::create_directories(target,error);
    if (error) throw runtime_error(RuntimeError::directory_io,location);
}
void remove_directory(const FileContext& files, std::u16string_view path, SourceLocation location) {
    const auto target=directory_path(files,path,location);
    std::error_code error;
#ifdef _WIN32
    // Windows holds the process current directory open against deletion.
    // Preserve this rule for the execution-local directory without changing cwd.
    if (!std::filesystem::is_symlink(target,error)
        && std::filesystem::equivalent(target,files.resolve(u".",location),error) && !error)
        throw runtime_error(RuntimeError::directory_io,location);
    error.clear();
#endif
    if (!std::filesystem::is_directory(target,error) || error
        || !std::filesystem::remove(target,error) || error) throw runtime_error(RuntimeError::directory_io,location);
}
bool delete_file(const FileContext& files, std::u16string_view path) {
    if (!file_exists(files,path)) return false;
    const auto target=files.resolve(path,{});
#ifdef _WIN32
    return ::_wremove(target.c_str())==0;
#else
    return ::unlink(target.c_str())==0;
#endif
}
Value change_file_extension(const Value& path, const Value& extension, SourceLocation location) {
    if (std::holds_alternative<NilString>(path)) return NilString{};
    const auto& name=string_text(path);
    if (name.empty()) return name;
    auto length=name.size();
    for (auto i=name.size();i>0;--i) {
        const auto c=name[i-1];
        if (c==u'.') { length=i-1; break; }
        if (c==u'/'
#ifdef _WIN32
            || c==u'\\'
#endif
        ) break;
    }
    const auto& suffix=string_text(extension);
    const bool dot=!std::holds_alternative<NilString>(extension) && (suffix.empty() || suffix.front()!=u'.');
    constexpr std::size_t limit=1'048'576;
    if (suffix.size()>limit || length>limit-suffix.size() || (dot && length+suffix.size()==limit))
        throw runtime_error(RuntimeError::string_limit,location);
    auto result=name.substr(0,length);
    if (dot) result+=u'.';
    result+=suffix;
    return result;
}
}

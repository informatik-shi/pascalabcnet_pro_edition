#include "files.hpp"
#include "file_encoding.hpp"
#include "file_reader.hpp"
#include "file_writer.hpp"
#include "diagnostics.hpp"
#include <filesystem>
#include <cstdio>
#ifndef _WIN32
#include <unistd.h>
#endif

namespace abc {
FileContext::FileContext():opened_(std::make_unique<OpenFiles>()) {
    std::error_code error;
    current_ = std::filesystem::current_path(error);
    if (error) current_.clear();
    initialize_drives();
}
std::filesystem::path FileContext::resolve(std::u16string_view text, SourceLocation location) const {
    const auto normalized=full_name(text,location);
    auto utf8=to_utf8(normalized,true);return std::filesystem::path(std::u8string(utf8.begin(),utf8.end()));
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
    current_=std::move(absolute);remember_drive();
}
std::u16string read_text_file(const FileContext& files,std::u16string_view path,SourceLocation location,const TextEncoding* encoding) {
    const auto target=files.resolve(path,location);if(!encoding)encoding=files.encoding(location);
    files.check_binary_conflict(target,location);
    LineReader input(target,encoding ? *encoding : TextEncoding{TextEncodingKind::utf8,false,true},location);
    static const ExecutionOptions defaults;
    return input.read_to_end(location,files.execution_options ? *files.execution_options : defaults);
}
void write_text_file(const FileContext& files, std::u16string_view path, std::u16string_view text, SourceLocation location,const TextEncoding* encoding) {
    const auto target=files.resolve(path,location);if(!encoding)encoding=files.encoding(location);
    files.check_binary_conflict(target,location);
    auto data = encoding ? encode_text_bytes(text,*encoding,location) : to_utf8(text,true);
    if(encoding){const auto mark=text_encoding_preamble(*encoding);if(!mark.empty())data.insert(0,mark);}
    if (data.size() > 1'048'576) throw runtime_error(RuntimeError::file_limit,location);
    FileWriter output(target,false,location);output.write(data,location);output.flush(location);output.close();
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
    if (files.text_file_locked(target)) return false;
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

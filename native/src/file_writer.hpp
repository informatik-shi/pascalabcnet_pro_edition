#pragma once
#include "abc/core.hpp"
#include <filesystem>
#include <string>
#include <string_view>

namespace abc {
class FileWriter {
    std::filesystem::path path_;
    std::string pending_;
    std::size_t position_=0,append_start_=0;
#ifdef _WIN32
    void* handle_=nullptr;
#else
    int descriptor_=-1;
#endif
    void write_direct(std::string_view bytes,SourceLocation location);
public:
    FileWriter(std::filesystem::path path,bool append,SourceLocation location);
    ~FileWriter();
    FileWriter(const FileWriter&)=delete;
    FileWriter& operator=(const FileWriter&)=delete;
    bool is_open() const noexcept;
    const std::filesystem::path& path() const noexcept{return path_;}
    std::size_t position() const noexcept{return position_;}
    void write(std::string_view bytes,SourceLocation location);
    void flush(SourceLocation location);
    void seek_begin(SourceLocation location);
    void close() noexcept;
};
}

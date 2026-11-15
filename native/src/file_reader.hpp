#pragma once
#include "file_encoding.hpp"
#include <filesystem>

namespace abc {
class LineReader {
    std::filesystem::path path_;
    TextDecoder decoder_;
    std::u16string buffer_;
    std::size_t position_=0,received_=0;
    std::uint64_t work_=0;
    bool eof_=false;
#ifdef _WIN32
    void* handle_=nullptr;
#else
    int descriptor_=-1;
#endif
    void checkpoint(SourceLocation location,const ExecutionOptions& options);
    std::optional<char16_t> peek(SourceLocation location,const ExecutionOptions& options);
public:
    LineReader(std::filesystem::path path,TextEncoding encoding,SourceLocation location);
    ~LineReader();
    LineReader(const LineReader&)=delete;
    LineReader& operator=(const LineReader&)=delete;
    bool is_open() const noexcept;
    const std::filesystem::path& path() const noexcept{return path_;}
    void close() noexcept;
    void seek_begin_preserving_buffer(SourceLocation location);
    std::optional<char16_t> peek_character(SourceLocation location,const ExecutionOptions& options);
    std::optional<char16_t> read_character(SourceLocation location,const ExecutionOptions& options);
    std::u16string read_to_end(SourceLocation location,const ExecutionOptions& options);
    std::optional<std::u16string> read_line(SourceLocation location,const ExecutionOptions& options);
};
}

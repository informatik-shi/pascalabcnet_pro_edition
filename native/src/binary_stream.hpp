#pragma once
#include "abc/core.hpp"
#include <filesystem>
#include <span>
#include <string_view>
namespace abc {
class BinaryDevice {
public:
    virtual ~BinaryDevice()=default;
    virtual std::int64_t position(SourceLocation) const=0;
    virtual void seek(std::int64_t,SourceLocation)=0;
    virtual std::size_t read(std::span<char>,SourceLocation)=0;
    virtual void write(std::string_view,SourceLocation)=0;
    void read_exact(std::span<char>,SourceLocation);
};
// Seekable ReadWrite / ShareNone storage used by the source binary-file classes.
class BinaryStream final:public BinaryDevice {
    std::filesystem::path path_;
#ifdef _WIN32
    void* handle_=nullptr;
#else
    int descriptor_=-1;
    bool locked_=false;
#endif
    void require_open(SourceLocation) const;
public:
    BinaryStream(std::filesystem::path,bool create,SourceLocation);
    ~BinaryStream() override;
    BinaryStream(const BinaryStream&)=delete;
    BinaryStream& operator=(const BinaryStream&)=delete;
    bool is_open() const noexcept;
    const std::filesystem::path& path() const noexcept{return path_;}
    std::int64_t position(SourceLocation) const override;
    std::int64_t length(SourceLocation) const;
    void seek(std::int64_t,SourceLocation) override;
    void truncate(SourceLocation);
    std::size_t read(std::span<char>,SourceLocation) override;
    void write(std::string_view,SourceLocation) override;
    void close() noexcept;
};
}

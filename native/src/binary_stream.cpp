#include "binary_stream.hpp"
#include "diagnostics.hpp"
#include <algorithm>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace abc {
BinaryStream::BinaryStream(std::filesystem::path path,bool create,SourceLocation location):path_(std::move(path)){
#ifdef _WIN32
    auto opened=::CreateFileW(path_.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,create?CREATE_ALWAYS:OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(opened==INVALID_HANDLE_VALUE)throw runtime_error(RuntimeError::file_io,location);
    handle_=opened;
#else
    static_assert(sizeof(off_t)>=8,"Binary streams require 64-bit file offsets");
    descriptor_=::open(path_.c_str(),O_RDWR|O_CLOEXEC|(create?O_CREAT:0),0666);
    if(descriptor_<0)throw runtime_error(RuntimeError::file_io,location);
    struct stat status{};
    if(::fstat(descriptor_,&status)!=0||!S_ISREG(status.st_mode)){close();throw runtime_error(RuntimeError::file_io,location);}
    int result;do{result=::flock(descriptor_,LOCK_EX|LOCK_NB);}while(result<0&&errno==EINTR);
    if(result<0&&(errno==EWOULDBLOCK||errno==EAGAIN)){close();throw runtime_error(RuntimeError::file_io,location);}
    locked_=result==0;
    // Match FileStream: a refused exclusive open must not truncate existing data.
    if(create&&::ftruncate(descriptor_,0)!=0){close();throw runtime_error(RuntimeError::file_io,location);}
#endif
}
BinaryStream::~BinaryStream(){close();}
bool BinaryStream::is_open() const noexcept{
#ifdef _WIN32
    return handle_!=nullptr;
#else
    return descriptor_>=0;
#endif
}
void BinaryStream::require_open(SourceLocation location) const{if(!is_open())throw runtime_error(RuntimeError::binary_closed,location);}
void BinaryStream::close() noexcept{
#ifdef _WIN32
    if(handle_){::CloseHandle(static_cast<HANDLE>(handle_));handle_=nullptr;}
#else
    if(descriptor_>=0){if(locked_)::flock(descriptor_,LOCK_UN);::close(descriptor_);descriptor_=-1;locked_=false;}
#endif
}
std::int64_t BinaryStream::position(SourceLocation location) const{
    require_open(location);
#ifdef _WIN32
    LARGE_INTEGER offset{},value{};if(!::SetFilePointerEx(static_cast<HANDLE>(handle_),offset,&value,FILE_CURRENT))throw runtime_error(RuntimeError::file_io,location);
    return value.QuadPart;
#else
    const auto value=::lseek(descriptor_,0,SEEK_CUR);if(value<0)throw runtime_error(RuntimeError::file_io,location);return static_cast<std::int64_t>(value);
#endif
}
std::int64_t BinaryStream::length(SourceLocation location) const{
    require_open(location);
#ifdef _WIN32
    LARGE_INTEGER value{};if(!::GetFileSizeEx(static_cast<HANDLE>(handle_),&value))throw runtime_error(RuntimeError::file_io,location);return value.QuadPart;
#else
    struct stat value{};if(::fstat(descriptor_,&value)!=0||value.st_size<0)throw runtime_error(RuntimeError::file_io,location);return static_cast<std::int64_t>(value.st_size);
#endif
}
void BinaryStream::seek(std::int64_t position,SourceLocation location){
    require_open(location);if(position<0)throw runtime_error(RuntimeError::binary_position,location);
#ifdef _WIN32
    LARGE_INTEGER value{};value.QuadPart=position;if(!::SetFilePointerEx(static_cast<HANDLE>(handle_),value,nullptr,FILE_BEGIN))throw runtime_error(RuntimeError::file_io,location);
#else
    if(::lseek(descriptor_,static_cast<off_t>(position),SEEK_SET)<0)throw runtime_error(RuntimeError::file_io,location);
#endif
}
void BinaryStream::truncate(SourceLocation location){
    require_open(location);
#ifdef _WIN32
    if(!::SetEndOfFile(static_cast<HANDLE>(handle_)))throw runtime_error(RuntimeError::file_io,location);
#else
    if(::ftruncate(descriptor_,static_cast<off_t>(position(location)))!=0)throw runtime_error(RuntimeError::file_io,location);
#endif
}
std::size_t BinaryStream::read(std::span<char> bytes,SourceLocation location){
    require_open(location);if(bytes.empty())return 0;const auto size=std::min<std::size_t>(bytes.size(),0x7ffff000U);
#ifdef _WIN32
    DWORD received=0;if(!::ReadFile(static_cast<HANDLE>(handle_),bytes.data(),static_cast<DWORD>(size),&received,nullptr))throw runtime_error(RuntimeError::file_io,location);return received;
#else
    ssize_t received;do{received=::read(descriptor_,bytes.data(),size);}while(received<0&&errno==EINTR);
    if(received<0)throw runtime_error(RuntimeError::file_io,location);return static_cast<std::size_t>(received);
#endif
}
void BinaryDevice::read_exact(std::span<char> bytes,SourceLocation location){
    if(bytes.empty()){(void)read(bytes,location);return;}
    while(!bytes.empty()){const auto count=read(bytes,location);if(!count)throw runtime_error(RuntimeError::binary_end,location);bytes=bytes.subspan(count);}
}
void BinaryStream::write(std::string_view bytes,SourceLocation location){
    require_open(location);while(!bytes.empty()){const auto size=std::min<std::size_t>(bytes.size(),0x7ffff000U);
#ifdef _WIN32
        DWORD count=0;if(!::WriteFile(static_cast<HANDLE>(handle_),bytes.data(),static_cast<DWORD>(size),&count,nullptr)||!count)throw runtime_error(RuntimeError::file_io,location);
#else
        ssize_t count;do{count=::write(descriptor_,bytes.data(),size);}while(count<0&&errno==EINTR);if(count<=0)throw runtime_error(RuntimeError::file_io,location);
#endif
        bytes.remove_prefix(static_cast<std::size_t>(count));
    }
}
}

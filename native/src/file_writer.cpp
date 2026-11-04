#include "file_writer.hpp"
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
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace abc {
namespace {constexpr std::size_t file_limit=1'048'576,buffer_size=4096;}
FileWriter::FileWriter(std::filesystem::path path,bool append,SourceLocation location):path_(std::move(path)) {
#ifdef _WIN32
    auto opened=::CreateFileW(path_.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,append ? OPEN_ALWAYS : CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(opened==INVALID_HANDLE_VALUE)throw runtime_error(RuntimeError::file_io,location);
    handle_=opened;
    if(append) {
        LARGE_INTEGER offset{},position{};
        if(!::SetFilePointerEx(opened,offset,&position,FILE_END) || position.QuadPart<0){close();throw runtime_error(RuntimeError::file_io,location);}
        if(static_cast<unsigned long long>(position.QuadPart)>file_limit){close();throw runtime_error(RuntimeError::file_limit,location);}
        position_=static_cast<std::size_t>(position.QuadPart);
    }
#else
    descriptor_=::open(path_.c_str(),O_WRONLY|O_CREAT|O_CLOEXEC|(append ? 0 : O_TRUNC),0666);
    if(descriptor_<0)throw runtime_error(RuntimeError::file_io,location);
    struct stat status{};
    if(::fstat(descriptor_,&status)!=0 || !S_ISREG(status.st_mode)){close();throw runtime_error(RuntimeError::file_io,location);}
    if(append) {
        const auto position=::lseek(descriptor_,0,SEEK_END);
        if(position<0){close();throw runtime_error(RuntimeError::file_io,location);}
        if(static_cast<unsigned long long>(position)>file_limit){close();throw runtime_error(RuntimeError::file_limit,location);}
        position_=static_cast<std::size_t>(position);
    }
#endif
    append_start_=position_;
}
FileWriter::~FileWriter(){close();}
bool FileWriter::is_open() const noexcept {
#ifdef _WIN32
    return handle_!=nullptr;
#else
    return descriptor_>=0;
#endif
}
void FileWriter::close() noexcept {
#ifdef _WIN32
    if(handle_){::CloseHandle(static_cast<HANDLE>(handle_));handle_=nullptr;}
#else
    if(descriptor_>=0){::close(descriptor_);descriptor_=-1;}
#endif
    pending_.clear();
}
void FileWriter::write_direct(std::string_view bytes,SourceLocation location) {
    if(!is_open())throw runtime_error(RuntimeError::file_io,location);
    while(!bytes.empty()) {
#ifdef _WIN32
        DWORD count=0;
        if(!::WriteFile(static_cast<HANDLE>(handle_),bytes.data(),static_cast<DWORD>(bytes.size()),&count,nullptr) || !count)
            throw runtime_error(RuntimeError::file_io,location);
#else
        const auto count=::write(descriptor_,bytes.data(),bytes.size());
        if(count<0 && errno==EINTR)continue;
        if(count<=0)throw runtime_error(RuntimeError::file_io,location);
#endif
        bytes.remove_prefix(static_cast<std::size_t>(count));
    }
}
void FileWriter::flush(SourceLocation location) {
    write_direct(pending_,location);pending_.clear();
}
void FileWriter::write(std::string_view bytes,SourceLocation location) {
    if(!is_open())throw runtime_error(RuntimeError::file_io,location);
    if(position_>file_limit || bytes.size()>file_limit-position_)throw runtime_error(RuntimeError::file_limit,location);
    const auto count=bytes.size();
    if(!pending_.empty()) {
        const auto fit=std::min(bytes.size(),buffer_size-pending_.size());
        pending_.append(bytes.substr(0,fit));bytes.remove_prefix(fit);
        if(bytes.empty()){position_+=count;return;}
        flush(location);
    }
    if(bytes.size()>=buffer_size)write_direct(bytes,location);else pending_.assign(bytes);
    position_+=count;
}
void FileWriter::seek_begin(SourceLocation location) {
    flush(location);
    if(append_start_)throw runtime_error(RuntimeError::file_io,location);
#ifdef _WIN32
    LARGE_INTEGER offset{};
    if(!::SetFilePointerEx(static_cast<HANDLE>(handle_),offset,nullptr,FILE_BEGIN))throw runtime_error(RuntimeError::file_io,location);
#else
    if(::lseek(descriptor_,0,SEEK_SET)<0)throw runtime_error(RuntimeError::file_io,location);
#endif
    position_=0;
}
}

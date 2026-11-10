#include "file_reader.hpp"
#include "files.hpp"
#include "text_file.hpp"
#include "diagnostics.hpp"
#include "binary_file.hpp"
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
LineReader::LineReader(std::filesystem::path path,TextEncoding encoding,SourceLocation location):path_(std::move(path)),decoder_(encoding) {
#ifdef _WIN32
    auto opened=::CreateFileW(path_.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(opened==INVALID_HANDLE_VALUE)throw runtime_error(RuntimeError::file_io,location);
    handle_=opened;
#else
    descriptor_=::open(path_.c_str(),O_RDONLY|O_CLOEXEC);
    if(descriptor_<0)throw runtime_error(RuntimeError::file_io,location);
    struct stat status{};
    if(::fstat(descriptor_,&status)!=0 || S_ISDIR(status.st_mode)){close();throw runtime_error(RuntimeError::file_io,location);}
#endif
}
LineReader::~LineReader(){close();}
bool LineReader::is_open() const noexcept {
#ifdef _WIN32
    return handle_!=nullptr;
#else
    return descriptor_>=0;
#endif
}
void LineReader::close() noexcept {
#ifdef _WIN32
    if(handle_){::CloseHandle(static_cast<HANDLE>(handle_));handle_=nullptr;}
#else
    if(descriptor_>=0){::close(descriptor_);descriptor_=-1;}
#endif
}
void LineReader::seek_begin_preserving_buffer(SourceLocation location) {
    if(!is_open())throw runtime_error(RuntimeError::reader_closed,location);
#ifdef _WIN32
    LARGE_INTEGER offset{};
    if(!::SetFilePointerEx(static_cast<HANDLE>(handle_),offset,nullptr,FILE_BEGIN))throw runtime_error(RuntimeError::file_io,location);
#else
    if(::lseek(descriptor_,0,SEEK_SET)<0)throw runtime_error(RuntimeError::file_io,location);
#endif
    eof_=false;decoder_.resume_after_final();
}
void LineReader::checkpoint(SourceLocation location,const ExecutionOptions& options) {
    if(work_>=options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,location);
    if((work_&255U)==0 && options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,location);
    ++work_;
}
std::optional<char16_t> LineReader::peek(SourceLocation location,const ExecutionOptions& options) {
    if(!is_open())throw runtime_error(RuntimeError::reader_closed,location);
    while(position_>=buffer_.size()) {
        if(eof_)return std::nullopt;
        checkpoint(location,options);char bytes[1024];std::size_t count=0;
#ifdef _WIN32
        DWORD read=0;
        if(!::ReadFile(static_cast<HANDLE>(handle_),bytes,sizeof bytes,&read,nullptr))throw runtime_error(RuntimeError::file_io,location);
        count=read;
#else
        ssize_t read=0;
        do{read=::read(descriptor_,bytes,sizeof bytes);}while(read<0 && errno==EINTR);
        if(read<0)throw runtime_error(RuntimeError::file_io,location);
        count=static_cast<std::size_t>(read);
#endif
        if(count>1'048'576-received_)throw runtime_error(RuntimeError::file_limit,location);
        received_+=count;eof_=count==0;
        buffer_=decoder_.decode(std::string_view(bytes,count),eof_,location);position_=0;
    }
    return buffer_[position_];
}
std::optional<char16_t> LineReader::peek_character(SourceLocation location,const ExecutionOptions& options) {return peek(location,options);}
std::optional<char16_t> LineReader::read_character(SourceLocation location,const ExecutionOptions& options) {
    const auto next=peek(location,options);
    if(next){checkpoint(location,options);++position_;}
    return next;
}
std::u16string LineReader::read_to_end(SourceLocation location,const ExecutionOptions& options) {
    std::u16string result;
    while(const auto next=read_character(location,options)) {
        if(result.size()>=1'048'576)throw runtime_error(RuntimeError::file_limit,location);
        result.push_back(*next);
    }
    return result;
}
std::optional<std::u16string> LineReader::read_line(SourceLocation location,const ExecutionOptions& options) {
    std::u16string line;
    while(const auto next=peek(location,options)) {
        checkpoint(location,options);++position_;
        if(*next==u'\n')return line;
        if(*next==u'\r') {
            const auto following=peek(location,options);
            if(following && *following==u'\n')++position_;
            return line;
        }
        if(line.size()>=1'048'576)throw runtime_error(RuntimeError::file_limit,location);
        line.push_back(*next);
    }
    if(!line.empty())return line;
    return std::nullopt;
}
void FileContext::prepare_line_open(const std::shared_ptr<LineReader>& reader,SourceLocation location) {
    check_binary_conflict(reader->path(),location);
    std::erase_if(opened_->readers,[](const auto& weak){const auto reader=weak.lock();return !reader || !reader->is_open();});
    std::erase_if(opened_->texts,[](const auto& weak){const auto text=weak.lock();return !text || !text->is_open();});
    std::erase_if(opened_->binaries,[](const auto& p){const auto f=p.lock();return !f||!f->is_open();});
    if(opened_->readers.size()+opened_->texts.size()+opened_->binaries.size()>=64)throw runtime_error(RuntimeError::file_open_limit,location);
    opened_->readers.push_back(reader);
}
}

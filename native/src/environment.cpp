#include "environment.hpp"
#include "diagnostics.hpp"
#include "file_encoding.hpp"
#include <limits>
#include <new>
#include <filesystem>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#ifdef __APPLE__
#include <libproc.h>
#else
#include <sys/auxv.h>
#endif
#endif
namespace abc {
namespace {
constexpr std::size_t limit=1'048'576;
void check(std::size_t size,const ExecutionOptions& options,SourceLocation location){
    if(size>limit)throw runtime_error(RuntimeError::string_limit,location);
    if(size>options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,location);
    if(options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,location);
}
std::u16string image_name(){
#ifdef _WIN32
    for(std::size_t size=256;size<=limit;size*=2){
        std::wstring buffer(size,L'\0');const auto used=GetModuleFileNameW(nullptr,buffer.data(),static_cast<DWORD>(size));
        if(!used)throw runtime_error(RuntimeError::file_io,{});
        if(used<size)return std::u16string(buffer.begin(),buffer.begin()+used);
    }
#elif defined(__APPLE__)
    std::string buffer(PROC_PIDPATHINFO_MAXSIZE,'\0');const auto used=proc_pidpath(getpid(),buffer.data(),static_cast<std::uint32_t>(buffer.size()));
    if(used<=0)throw runtime_error(RuntimeError::file_io,{});
    const auto end=buffer.find('\0');buffer.resize(end==std::string::npos ? static_cast<std::size_t>(used) : end);
    return decode_text_bytes(buffer,{TextEncodingKind::utf8,false,false},false,{});
#else
    for(std::size_t size=256;size<=limit;size*=2){
        std::string buffer(size,'\0');const auto used=readlink("/proc/self/exe",buffer.data(),buffer.size());
        if(used<0){
            const auto address=getauxval(AT_EXECFN);
            if(address){
                const std::string executable(reinterpret_cast<const char*>(address));
                const auto path=std::filesystem::absolute(std::filesystem::path(std::u8string(executable.begin(),executable.end()))).lexically_normal();
                return decode_text_bytes(path.native(),{TextEncodingKind::utf8,false,false},false,{});
            }
            throw runtime_error(RuntimeError::file_io,{});
        }
        if(static_cast<std::size_t>(used)<size){buffer.resize(static_cast<std::size_t>(used));return decode_text_bytes(buffer,{TextEncodingKind::utf8,false,false},false,{});}
    }
#endif
    throw runtime_error(RuntimeError::string_limit,{});
}
}
const std::u16string& process_executable_name(){static const auto name=image_name();return name;}
bool ProgramEnvironment::redirected() const{
    return !options_.arguments.empty() && (options_.arguments[0]==u"[REDIRECTIOMODE]" || options_.arguments[0]==u"[RUNMODE]");
}
std::int32_t ProgramEnvironment::count(SourceLocation location) const{
    check(0,options_,location);const auto size=options_.arguments.size()-(redirected() ? 1U : 0U);
    if(size>static_cast<std::size_t>(INT32_MAX))throw runtime_error(RuntimeError::integer_overflow,location);
    return static_cast<std::int32_t>(size);
}
std::u16string ProgramEnvironment::executable(SourceLocation location) const try {
    const auto& name=options_.executable_name.empty() ? process_executable_name() : options_.executable_name;check(name.size(),options_,location);return name;
}catch(const std::bad_alloc&){throw runtime_error(RuntimeError::allocation_limit,location);}catch(const Error& error){throw Error(error.code,location,error.what(),error.russian);}
catch(const std::filesystem::filesystem_error&){throw runtime_error(RuntimeError::file_io,location); }
std::u16string ProgramEnvironment::parameter(std::int32_t index,SourceLocation location) const try {
    check(0,options_,location);const auto shifted=static_cast<std::int64_t>(index)+(redirected() ? 1 : 0);
    if(shifted<0 || static_cast<std::uint64_t>(shifted)>options_.arguments.size())throw runtime_error(RuntimeError::index_bounds,location);
    if(!shifted)return executable(location);
    const auto& value=options_.arguments[static_cast<std::size_t>(shifted-1)];check(value.size(),options_,location);return value;
}catch(const std::bad_alloc&){throw runtime_error(RuntimeError::allocation_limit,location);}
}

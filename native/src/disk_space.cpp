#include "disk_space.hpp"
#include "files.hpp"
#include "diagnostics.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <new>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <sys/mount.h>
#else
#include <sys/vfs.h>
#endif
namespace abc {
std::u16string disk_name_from_number(std::int32_t number,std::u16string_view argument_zero){
    if(!number){const auto colon=argument_zero.find(u':');return colon==std::u16string_view::npos ? std::u16string(u"C:") : std::u16string(argument_zero.substr(0,colon+1));}
    return {static_cast<char16_t>(u'A'+std::clamp(number,1,26)-1),u':'};
}
std::int64_t disk_space([[maybe_unused]] const FileContext& files,const Value& value,bool size,SourceLocation location,const ExecutionOptions& options) try {
    const auto& name=string_text(value);
    if(name.size()>1'048'576)throw runtime_error(RuntimeError::string_limit,location);
    if(name.size()>options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,location);
    if(options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,location);
    if(std::holds_alternative<NilString>(value) || name.empty())return -1;
#ifdef _WIN32
    // DriveInfo accepts a drive letter or extracts its root; it rejects UNC/device roots.
    if(!((name[0]>=u'A' && name[0]<=u'Z') || (name[0]>=u'a' && name[0]<=u'z')))return -1;
    if(name.size()>1 && name[1]!=u':')return -1;
    const std::wstring root{static_cast<wchar_t>(name[0]),L':',L'\\'};
    ULARGE_INTEGER available{},capacity{},free{};
    DWORD old_mode=0;const bool changed=SetThreadErrorMode(SEM_FAILCRITICALERRORS,&old_mode)!=0;
    const bool success=GetDiskFreeSpaceExW(root.c_str(),&available,&capacity,&free)!=0;
    if(changed)SetThreadErrorMode(old_mode,nullptr);
    if(!success)return -1;
    return std::bit_cast<std::int64_t>(static_cast<std::uint64_t>(size ? capacity.QuadPart : free.QuadPart));
#else
    if(name.find(char16_t{})!=std::u16string::npos)return -1;
    auto text=to_utf8(name,true);
    std::filesystem::path path(std::u8string(text.begin(),text.end()));
    if(!path.is_absolute()){
        const auto base=to_utf8(files.directory(location),true);path=std::filesystem::path(std::u8string(base.begin(),base.end()))/path;
    }
    struct statfs info{};if(statfs(path.c_str(),&info)!=0)return -1;
    const auto blocks=static_cast<std::uint64_t>(size ? info.f_blocks : info.f_bfree);
    const auto bytes=static_cast<std::uint64_t>(info.f_bsize)*blocks;
    return bytes>static_cast<std::uint64_t>(INT64_MAX) ? std::int64_t{-1} : static_cast<std::int64_t>(bytes);
#endif
}catch(const std::bad_alloc&){throw runtime_error(RuntimeError::allocation_limit,location);}
catch(const Error& error){if(error.code=="E_LIMIT" || error.code=="E_CANCELLED")throw;return -1;}
catch(const std::filesystem::filesystem_error&){return -1;}
}

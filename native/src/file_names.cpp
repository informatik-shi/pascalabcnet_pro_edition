#include "file_names.hpp"
#include "files.hpp"
#include "diagnostics.hpp"
#include <new>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace abc {
namespace {
constexpr std::size_t string_limit=1'048'576;
bool separator(char16_t c){
#ifdef _WIN32
    return c==u'/' || c==u'\\';
#else
    return c==u'/';
#endif
}
#ifdef _WIN32
bool drive_letter(char16_t c){return (c>=u'a' && c<=u'z') || (c>=u'A' && c<=u'Z');}
unsigned drive_index(char16_t c){return static_cast<unsigned>((c|32)-u'a');}
bool extended(std::u16string_view p){return p.size()>=4 && p[0]==u'\\' && (p[1]==u'\\' || p[1]==u'?') && p[2]==u'?' && p[3]==u'\\';}
bool device(std::u16string_view p){return extended(p) || (p.size()>=4 && separator(p[0]) && separator(p[1]) && (p[2]==u'.' || p[2]==u'?') && separator(p[3]));}
#endif
std::size_t root_length(std::u16string_view p){
#ifndef _WIN32
    return !p.empty() && p[0]==u'/' ? 1 : 0;
#else
    const bool special=device(p),unc=special && p.size()>=8 && p.substr(4,3)==u"UNC" && separator(p[7]);
    if((!special || unc) && !p.empty() && separator(p[0])){
        if(unc || (p.size()>1 && separator(p[1]))){
            auto i=unc ? std::size_t{8} : std::size_t{2};unsigned parts=2;
            while(i<p.size()){if(separator(p[i]) && !--parts)break;++i;}return i;
        }
        return 1;
    }
    if(special){std::size_t i=4;while(i<p.size() && !separator(p[i]))++i;if(i<p.size() && i>4)++i;return i;}
    if(p.size()>=2 && drive_letter(p[0]) && p[1]==u':')return p.size()>2 && separator(p[2]) ? 3 : 2;
    return 0;
#endif
}
std::u16string join(std::u16string_view base,std::u16string_view path){
    std::u16string result(base);if(!result.empty() && !separator(result.back()) && !path.empty() && !separator(path[0]))result+=static_cast<char16_t>(std::filesystem::path::preferred_separator);result+=path;return result;
}
#ifdef _WIN32
std::u16string expand_short_names(std::wstring full){
    if(full.find(L'~')==std::wstring::npos)return std::u16string(full.begin(),full.end());
    std::wstring probe=full;bool unc=false,dot_device=false;std::size_t added=0;
    const std::u16string original(full.begin(),full.end());
    if(device(original)){if(probe.size()>2 && probe[2]==L'.'){probe[2]=L'?';dot_device=true;}}
    else if(probe.size()>1 && separator(static_cast<char16_t>(probe[0])) && separator(static_cast<char16_t>(probe[1]))){probe=L"\\\\?\\UNC\\"+probe.substr(2);unc=true;added=6;}
    else {probe=L"\\\\?\\"+probe;added=4;}
    const auto qualified=probe;const auto root=root_length(std::u16string(probe.begin(),probe.end()));
    while(!probe.empty()){
        const auto needed=GetLongPathNameW(probe.c_str(),nullptr,0);
        if(needed){
            std::wstring expanded(needed,L'\0');const auto size=GetLongPathNameW(probe.c_str(),expanded.data(),needed);
            if(size && size<needed){
                expanded.resize(size);expanded+=qualified.substr(probe.size());if(dot_device)expanded[2]=L'.';
                if(unc)expanded=L"\\\\"+expanded.substr(8);else if(added)expanded.erase(0,added);
                return std::u16string(expanded.begin(),expanded.end());
            }
            break;
        }
        const auto error=GetLastError();if(error!=ERROR_FILE_NOT_FOUND && error!=ERROR_PATH_NOT_FOUND)break;
        const auto cut=probe.find_last_of(L"\\/");
        if(cut==std::wstring::npos || cut<=root)break;
        probe.resize(cut);
    }
    return std::u16string(full.begin(),full.end());
}
std::u16string normalize_windows(std::u16string_view path,SourceLocation location,bool expand=true){
    if(extended(path))return std::u16string(path);
    std::wstring input(path.begin(),path.end());
    const auto needed=GetFullPathNameW(input.c_str(),0,nullptr,nullptr);if(!needed)throw runtime_error(RuntimeError::file_io,location);
    if(needed>string_limit)throw runtime_error(RuntimeError::string_limit,location);
    std::wstring buffer(needed,L'\0');const auto size=GetFullPathNameW(input.c_str(),needed,buffer.data(),nullptr);
    if(!size || size>=needed)throw runtime_error(RuntimeError::file_io,location);
    buffer.resize(size);return expand ? expand_short_names(std::move(buffer)) : std::u16string(buffer.begin(),buffer.end());
}
#endif
std::u16string collapse_segments(std::u16string_view full){
    const auto root=root_length(full);std::u16string result(full.substr(0,root));std::vector<std::size_t> starts;
    for(std::size_t i=root;i<full.size();){
        while(i<full.size() && separator(full[i]))++i;auto end=i;while(end<full.size() && !separator(full[end]))++end;
        const auto part=full.substr(i,end-i);i=end;
        if(part.empty() || part==u".")continue;
        if(part==u".."){if(!starts.empty()){result.resize(starts.back());starts.pop_back();}continue;}
        starts.push_back(result.size());if(!result.empty() && !separator(result.back()))result+=static_cast<char16_t>(std::filesystem::path::preferred_separator);result+=part;
    }
    if(!full.empty() && separator(full.back()) && !result.empty() && !separator(result.back()))result+=static_cast<char16_t>(std::filesystem::path::preferred_separator);return result;
}
void check_work(std::u16string_view input,const ExecutionOptions& options,SourceLocation location){
    if(input.size()>string_limit)throw runtime_error(RuntimeError::string_limit,location);
    for(std::size_t i=0;i<input.size();++i){if(i>=options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,location);if((i&255U)==0 && options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,location);}
}
}
void FileContext::initialize_drives(){
#ifdef _WIN32
    for(unsigned i=0;i<26;++i){
        const std::u16string input{static_cast<char16_t>(u'A'+i),u':',u'.'};
        try{drive_directories_[i]=std::filesystem::path(normalize_windows(input,{},false));}catch(const Error&){}
    }
    remember_drive();
#endif
}
void FileContext::remember_drive(){
#ifdef _WIN32
    const auto root=current_.root_name().u16string();if(root.size()==2 && root[1]==u':' && drive_letter(root[0]))drive_directories_[drive_index(root[0])]=current_;
#endif
}
std::u16string FileContext::full_name(std::u16string_view input,SourceLocation location) const try {
    if(input.empty() || input.find(char16_t{})!=std::u16string_view::npos)throw runtime_error(RuntimeError::file_io,location);
    if(input.size()>string_limit)throw runtime_error(RuntimeError::string_limit,location);
#ifdef _WIN32
    if(input.find_first_not_of(u' ')==std::u16string_view::npos)throw runtime_error(RuntimeError::file_io,location);
    if(extended(input))return std::u16string(input);
    if(input.size()>1 && input[1]==u':' && !drive_letter(input[0]))return normalize_windows(input,location);
    // Bare DOS devices are rooted by Windows independently of the execution cwd.
    auto short_name=input;while(!short_name.empty() && (short_name.back()==u'.' || short_name.back()==u' '))short_name.remove_suffix(1);
    if(short_name.size()<=8 && input.find_first_of(u"\\/:")==std::u16string_view::npos){
        try{auto native=normalize_windows(input,location,false);if(device(native))return native;}
        catch(const Error& error){if(error.code!="E_IO")throw;}
    }
    std::u16string full;
    const bool fixed=(input.size()>1 && separator(input[0]) && (separator(input[1]) || input[1]==u'?')) || (input.size()>2 && drive_letter(input[0]) && input[1]==u':' && separator(input[2]));
    if(fixed)full=input;
    else if(current_.empty())throw runtime_error(RuntimeError::file_io,location);
    else if(separator(input[0])){const auto base=current_.u16string();full=join(std::u16string_view(base).substr(0,root_length(base)),input.substr(1));}
    else if(input.size()>1 && drive_letter(input[0]) && input[1]==u':'){
        const auto& cached=drive_directories_[drive_index(input[0])];
        if(cached.empty())full=input;
        else {auto base=cached.u16string();if(base.size()>1 && base[1]==u':')base[0]=input[0];full=join(base,input.substr(2));}
    }else full=join(current_.u16string(),input);
    return !fixed && extended(full) ? collapse_segments(full) : normalize_windows(full,location);
#else
    if(input[0]==u'/')return collapse_segments(input);
    if(current_.empty())throw runtime_error(RuntimeError::file_io,location);
    return collapse_segments(join(current_.u16string(),input));
#endif
}catch(const std::bad_alloc&){throw runtime_error(RuntimeError::allocation_limit,location);}catch(const std::filesystem::filesystem_error&){throw runtime_error(RuntimeError::file_io,location);}
Value file_name_info(const FileContext& files,const Value& value,FileNameMethod method,SourceLocation location,const ExecutionOptions& options) try {
    if(method<FileNameMethod::name || method>FileNameMethod::full)throw runtime_error(RuntimeError::invalid_bytecode,location);
    if(std::holds_alternative<NilString>(value))throw runtime_error(RuntimeError::nil_reference,location);
    const auto& original=std::get<std::u16string>(value);check_work(original,options,location);
    auto full=files.full_name(original,location);check_work(full,options,location);
    if(method==FileNameMethod::full)return full;
    if(method==FileNameMethod::name){
        const auto root=root_length(original);auto at=original.size();while(at>root && !separator(original[at-1]))--at;return original.substr(at);
    }
    if(method==FileNameMethod::extension){
        for(std::size_t i=full.size();i>0;--i){const auto c=full[i-1];if(c==u'.')return full.substr(i-1);if(separator(c)
#ifdef _WIN32
            || c==u':'
#endif
            )break;}return std::u16string{};
    }
    const auto root=root_length(full);if(full.size()<=root){if(method==FileNameMethod::directory)return NilString{};throw runtime_error(RuntimeError::nil_reference,location);}
    auto end=full.size();while(end>root && !separator(full[--end])){}while(end>root && separator(full[end-1]))--end;full.resize(end);
    if(method==FileNameMethod::directory)return full;
    if(method==FileNameMethod::drive){const auto colon=full.find(u':');return colon==std::u16string::npos ? std::u16string{} : full.substr(0,colon+1);}
    if(!full.empty() && full.back()!=u'\\' && full.back()!=u'/'){if(full.size()==string_limit)throw runtime_error(RuntimeError::string_limit,location);full+=u'\\';}return full;
}catch(const std::bad_alloc&){throw runtime_error(RuntimeError::allocation_limit,location);}catch(const std::filesystem::filesystem_error&){throw runtime_error(RuntimeError::file_io,location);}
}

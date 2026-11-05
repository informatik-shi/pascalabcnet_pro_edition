#include "directory_sequence.hpp"
#include "runtime_values.hpp"
#include "files.hpp"
#include "unicode.hpp"
#include "file_encoding.hpp"
#include <deque>
#include <filesystem>

namespace abc {
namespace {
namespace fs=std::filesystem;
void reserve(Values& values,std::size_t count){if(count>values.remaining)throw runtime_error(RuntimeError::allocation_limit,values.location);values.remaining-=count;}
void work(std::uint64_t& count,const ExecutionOptions& options,SourceLocation location){
    if(count>=options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,location);
    if((count++&255U)==0 && options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,location);
}
enum class PatternKind { literal, star, dos_star, dos_question, dos_dot, final_escape };
struct PatternToken {PatternKind kind;char16_t value=0;};
std::vector<PatternToken> pattern_tokens(std::u16string_view pattern){
    if(pattern.empty() || pattern==u"." || pattern==u"*.*")return {{PatternKind::star}};
    std::u16string expression;
#ifndef _WIN32
    const bool protect=pattern.find_first_of(u"\"<>")!=std::u16string_view::npos;
    for(auto c:pattern){if(protect && (c==u'\\' || c==u'"' || c==u'<' || c==u'>'))expression+=u'\\';expression+=c;}
#else
    expression=pattern;
#endif
    std::u16string translated;translated.reserve(expression.size());
    for(std::size_t i=0;i<expression.size();++i){
        const auto c=expression[i];
        if(c==u'.' && i+1==expression.size() && i && expression[i-1]==u'*')translated.back()=u'<';
        else if(c==u'.' && i+1<expression.size() && (expression[i+1]==u'*' || expression[i+1]==u'?'))translated+=u'"';
        else translated+=c==u'?' ? u'>' : c;
    }
    std::vector<PatternToken> tokens;tokens.reserve(translated.size());
    for(std::size_t i=0;i<translated.size();++i){
        const auto c=translated[i];
        if(c==u'\\'){if(i+1<translated.size())tokens.push_back({PatternKind::literal,translated[++i]});else tokens.push_back({PatternKind::final_escape});}
        else if(c==u'*')tokens.push_back({PatternKind::star});
        else if(c==u'<')tokens.push_back({PatternKind::dos_star});
        else if(c==u'>')tokens.push_back({PatternKind::dos_question});
        else if(c==u'"')tokens.push_back({PatternKind::dos_dot});
        else tokens.push_back({PatternKind::literal,c});
    }
    return tokens;
}
char32_t invariant_upper(char16_t c){return c==0x0131 ? static_cast<char32_t>(c) : unicode::to_upper(c);}
bool matches(const std::vector<PatternToken>& pattern,std::u16string_view name,bool ignore_case,std::uint64_t& used,const ExecutionOptions& options,SourceLocation location){
    if(name.empty())return false;
    std::vector<unsigned char> row(pattern.size()+1),next(row.size());row[0]=1;
    const auto last_dot=name.find_last_of(u'.');
    for(std::size_t n=0;n<=name.size();++n){
        std::fill(next.begin(),next.end(),0);const bool end=n==name.size(),last=n+1==name.size();bool terminal_zero=false;
        for(std::size_t p=0;p<pattern.size();++p){
            work(used,options,location);if(!row[p])continue;const auto token=pattern[p];
            switch(token.kind){
            case PatternKind::star:row[p+1]=1;if(last && p+1==pattern.size())terminal_zero=true;if(!end)next[p]=1;break;
            case PatternKind::dos_star:row[p+1]=1;if(last && p+1==pattern.size())terminal_zero=true;if(!end && n!=last_dot)next[p]=1;break;
            case PatternKind::dos_question:if(end || name[n]==u'.'){row[p+1]=1;if(last && p+1==pattern.size())terminal_zero=true;}else next[p+1]=1;break;
            case PatternKind::dos_dot:if(end)row[p+1]=1;else if(name[n]==u'.')next[p+1]=1;break;
            case PatternKind::final_escape:if(end)row[p+1]=1;else next[p+1]=1;break;
            case PatternKind::literal:if(!end && (token.value==name[n] || (ignore_case && invariant_upper(token.value)==invariant_upper(name[n]))))next[p+1]=1;break;
            }
        }
        if(end)return row.back()!=0;
        // A terminal DOS zero transition also accepts the final period itself.
        if(terminal_zero)return true;row.swap(next);
    }
    return false;
}
bool separator(char16_t c){
#ifdef _WIN32
    return c==u'/' || c==u'\\';
#else
    return c==u'/';
#endif
}
std::u16string join(std::u16string_view left,std::u16string_view right){
    std::u16string result(left);if(!result.empty() && !separator(result.back()))result+=static_cast<char16_t>(fs::path::preferred_separator);result+=right;return result;
}
fs::directory_iterator open(const fs::path& path,SourceLocation location,bool missing_ok=false){
    std::error_code ec;fs::directory_iterator it(path,fs::directory_options::none,ec);
    if(ec && !(missing_ok && (ec==std::errc::no_such_file_or_directory || ec==std::errc::not_a_directory)))throw runtime_error(RuntimeError::directory_io,location);
    return it;
}
struct DirectorySpec {fs::path root;std::u16string display;std::vector<PatternToken> pattern;DirectorySequenceMethod method;};
struct PendingDirectory {fs::path path;std::u16string display;fs::directory_iterator iterator;};
struct DirectoryCursor {
    PendingDirectory current;std::deque<PendingDirectory> pending;bool increment=false;
    explicit DirectoryCursor(const DirectorySpec& spec,SourceLocation location){
        // Open before copying descriptor fields; all cursor members are already constructed.
        auto iterator=open(spec.root,location);
        current.path=spec.root;current.display=spec.display;current.iterator=std::move(iterator);
    }
};
}
struct DirectorySequenceState {
    std::shared_ptr<const DirectorySpec> spec;std::unique_ptr<DirectoryCursor> cursor;Value current=NilString{};
    std::uint64_t next=0;bool ready=false,done=false;
};
bool directory_sequence_signature(const Program& program,Type type,DirectorySequenceMethod method){
    return method>=DirectorySequenceMethod::files && method<=DirectorySequenceMethod::all_directories && compound(type)
        && static_cast<unsigned>(type)-64U<program.types.size() && type_info(program,type).kind==CompoundKind::sequence && type_info(program,type).rank==1 && type_info(program,type).element==Type::string;
}
bool directory_pattern_matches(std::u16string_view pattern,std::u16string_view name,bool ignore_case,const ExecutionOptions& options,SourceLocation location){
    std::uint64_t used=0;return matches(pattern_tokens(pattern),name,ignore_case,used,options,location);
}
std::shared_ptr<Aggregate> directory_sequence_create(Values& values,FileContext& files,Type type,DirectorySequenceMethod method,const Value& path,const Value& search,const ExecutionOptions& options) try {
    if(!directory_sequence_signature(values.program,type,method))throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    if(std::holds_alternative<NilString>(path) || std::holds_alternative<NilString>(search))throw runtime_error(RuntimeError::nil_reference,values.location);
    auto display=string_text(path);auto pattern=string_text(search); // Both argument values precede OS access.
    if(display.empty() || display.find(char16_t{})!=std::u16string::npos || pattern.find(char16_t{})!=std::u16string::npos)throw runtime_error(RuntimeError::directory_io,values.location);
    if(!pattern.empty() && (separator(pattern[0])
#ifdef _WIN32
        || (pattern.size()>1 && pattern[1]==u':')
#endif
        ))throw runtime_error(RuntimeError::directory_io,values.location);
    for(std::size_t i=pattern.size();i>0;--i)if(separator(pattern[i-1])){display=join(display,std::u16string_view(pattern).substr(0,i-1));pattern.erase(0,i);break;}
    std::uint64_t used=0;for(std::size_t i=0;i<display.size()+pattern.size();++i)work(used,options,values.location);
    reserve(values,4+display.size()+pattern.size());
    auto spec=std::make_shared<DirectorySpec>();spec->root=files.resolve(display,values.location);spec->display=std::move(display);spec->method=method;spec->pattern=pattern_tokens(pattern);
    auto state=std::make_shared<DirectorySequenceState>();state->spec=std::move(spec);state->cursor=std::make_unique<DirectoryCursor>(*state->spec,values.location);
    auto output=std::make_shared<Aggregate>();output->type=type;output->sequence=std::make_unique<SequenceState>();output->sequence->provider=SequenceProvider::directory;output->sequence->directory=std::move(state);return output;
}
catch(const std::bad_alloc&){throw runtime_error(RuntimeError::allocation_limit,values.location);}
catch(const std::filesystem::filesystem_error&){throw runtime_error(RuntimeError::directory_io,values.location);}
std::shared_ptr<Aggregate> directory_sequence_begin(Values& values,const std::shared_ptr<Aggregate>& source) try {
    if(!source->sequence->directory || !source->sequence->directory->spec)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    if(!source->sequence->started){source->sequence->started=true;return source;}
    reserve(values,3);auto state=std::make_shared<DirectorySequenceState>();state->spec=source->sequence->directory->spec;state->cursor=std::make_unique<DirectoryCursor>(*state->spec,values.location);
    auto result=std::make_shared<Aggregate>();result->type=source->type;result->sequence=std::make_unique<SequenceState>();result->sequence->started=true;result->sequence->provider=SequenceProvider::directory;result->sequence->directory=std::move(state);return result;
}
catch(const std::bad_alloc&){throw runtime_error(RuntimeError::allocation_limit,values.location);}
catch(const std::filesystem::filesystem_error&){throw runtime_error(RuntimeError::directory_io,values.location);}
void directory_sequence_dispose(DirectorySequenceState& state) noexcept{state.cursor.reset();state.current=NilString{};state.ready=false;state.done=true;}
Value directory_sequence_current(const std::shared_ptr<Aggregate>& cursor){return cursor->sequence->directory->current;}
bool directory_sequence_resume(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options) try {
    auto& state=*sequence->sequence->directory;if(state.done)return false;
    if(state.ready && state.next && index==state.next-1)return true;
    if(index!=state.next || !state.cursor)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    const auto& spec=*state.spec;auto& cursor=*state.cursor;std::uint64_t used=0;
    const bool recursive=spec.method==DirectorySequenceMethod::all_files || spec.method==DirectorySequenceMethod::all_directories;
    const bool directories=spec.method==DirectorySequenceMethod::directories || spec.method==DirectorySequenceMethod::all_directories;
    for(;;){
        work(used,options,values.location);std::error_code ec;
        if(cursor.increment){cursor.current.iterator.increment(ec);cursor.increment=false;if(ec)throw runtime_error(RuntimeError::directory_io,values.location);}
        if(cursor.current.iterator==fs::directory_iterator{}){
            if(cursor.pending.empty()){directory_sequence_dispose(state);return false;}
            cursor.current=std::move(cursor.pending.front());cursor.pending.pop_front();
#ifndef _WIN32
            cursor.current.iterator=open(cursor.current.path,values.location,true);
#endif
            continue;
        }
        const auto entry=*cursor.current.iterator;cursor.increment=true;
        const bool is_directory=entry.is_directory(ec);if(ec && ec!=std::errc::no_such_file_or_directory)throw runtime_error(RuntimeError::directory_io,values.location);
#ifdef _WIN32
        const auto name=entry.path().filename().u16string();
#else
        const auto name=decode_text_bytes(entry.path().filename().native(),TextEncoding{TextEncodingKind::utf8,false,false},false,values.location);
#endif
        auto display=join(cursor.current.display,name);
        if(display.size()>1'048'576)throw runtime_error(RuntimeError::string_limit,values.location);
        if(recursive && is_directory){
            reserve(values,1+display.size());PendingDirectory pending{entry.path(),display,{}};
#ifdef _WIN32
            pending.iterator=open(pending.path,values.location,true);if(pending.iterator!=fs::directory_iterator{})cursor.pending.push_back(std::move(pending));
#else
            cursor.pending.push_back(std::move(pending));
#endif
        }
#if defined(_WIN32) || defined(__APPLE__)
        constexpr bool ignore_case=true;
#else
        constexpr bool ignore_case=false;
#endif
        if(is_directory!=directories || !matches(spec.pattern,name,ignore_case,used,options,values.location))continue;
        reserve(values,display.size());state.current=std::move(display);state.ready=true;++state.next;return true;
    }
}
catch(const std::bad_alloc&){throw runtime_error(RuntimeError::allocation_limit,values.location);}
catch(const std::filesystem::filesystem_error&){throw runtime_error(RuntimeError::directory_io,values.location);}

}

#include "regex.hpp"
#include "runtime_values.hpp"
#define PCRE2_CODE_UNIT_WIDTH 16
#define PCRE2_STATIC
#include <pcre2.h>
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <limits>

namespace abc {
namespace {
constexpr std::size_t text_limit=1'048'576;
struct Scratch {
    struct alignas(std::max_align_t) Header { std::size_t size; };
    std::size_t remaining;
    bool failed=false;
    static void* allocate(PCRE2_SIZE bytes,void* opaque) noexcept {
        auto& self=*static_cast<Scratch*>(opaque);
        if (bytes>self.remaining || bytes>std::numeric_limits<std::size_t>::max()-sizeof(Header)) { self.failed=true; return nullptr; }
        auto* header=static_cast<Header*>(std::malloc(sizeof(Header)+bytes));
        if (!header) { self.failed=true; return nullptr; }
        header->size=bytes; self.remaining-=bytes; return header+1;
    }
    static void release(void* pointer,void* opaque) noexcept {
        if (!pointer) return;
        auto& self=*static_cast<Scratch*>(opaque); auto* header=static_cast<Header*>(pointer)-1;
        self.remaining+=header->size; std::free(header);
    }
};
struct Work {
    const ExecutionOptions& options;
    std::uint64_t& count;
    SourceLocation location;
    std::exception_ptr failure;
    void check() {
        if (count>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,location);
        if ((count++&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,location);
    }
    static int callout(pcre2_callout_block*,void* opaque) noexcept {
        auto& self=*static_cast<Work*>(opaque);
        try { self.check(); return 0; }
        catch (...) { self.failure=std::current_exception(); return PCRE2_ERROR_CALLOUT; }
    }
};
void append(std::u16string& output,std::u16string_view text,SourceLocation location) {
    if (text.size()>text_limit-output.size()) throw runtime_error(RuntimeError::string_limit,location);
    output+=text;
}
enum class Action { swap, is_match, match_value };
Value search(Values& values,std::u16string_view text,std::u16string_view first,
    std::u16string_view second,Action action,bool nil_text,const ExecutionOptions& options,std::uint64_t& work) {
    try {
        const bool swap=action==Action::swap;
        if (first.size()>text_limit || (swap && (first.size()>text_limit-3 || second.size()>text_limit-3-first.size())))
            throw runtime_error(RuntimeError::string_limit,values.location);
        const auto pattern_size=first.size()+(swap ? second.size()+3 : 0);
        // Scratch is bounded by live bytes. Temporary C engine buffers do not consume
        // the cumulative budget for Pascal values on every match or repeated call.
        const auto capacity=std::min(values.remaining,text_limit)*sizeof(std::uint32_t);
        const auto input_bytes=(pattern_size+text.size()+2)*sizeof(PCRE2_UCHAR);
        if (input_bytes>capacity) throw runtime_error(RuntimeError::allocation_limit,values.location);
        Scratch scratch{capacity-input_bytes};
        std::vector<PCRE2_UCHAR> pattern; pattern.reserve(pattern_size+1);
        if (swap) pattern.push_back(u'(');
        pattern.insert(pattern.end(),first.begin(),first.end());
        if (swap) {
            pattern.push_back(u'|'); pattern.insert(pattern.end(),second.begin(),second.end()); pattern.push_back(u')');
        }
        pattern.push_back(0);
        std::vector<PCRE2_UCHAR> subject; subject.reserve(text.size()+1);
        subject.insert(subject.end(),text.begin(),text.end()); subject.push_back(0);
        Work budget{options,work,values.location,{}}; budget.check();
        const auto allocation=[&] { return runtime_error(RuntimeError::allocation_limit,values.location); };
        std::unique_ptr<pcre2_general_context,decltype(&pcre2_general_context_free)> general(
            pcre2_general_context_create(Scratch::allocate,Scratch::release,&scratch),pcre2_general_context_free);
        if (!general) throw allocation();
        std::unique_ptr<pcre2_compile_context,decltype(&pcre2_compile_context_free)> compiler(
            pcre2_compile_context_create(general.get()),pcre2_compile_context_free);
        if (!compiler) throw allocation();
        pcre2_set_newline(compiler.get(),PCRE2_NEWLINE_LF);
        pcre2_set_parens_nest_limit(compiler.get(),250);
        int compile_error=0; PCRE2_SIZE error_offset=0;
        std::unique_ptr<pcre2_code,decltype(&pcre2_code_free)> code(
            pcre2_compile(pattern.data(),pattern_size,PCRE2_AUTO_CALLOUT|PCRE2_UCP|PCRE2_NEVER_UTF,
                &compile_error,&error_offset,compiler.get()),pcre2_code_free);
        if (!code) {
            if (scratch.failed) throw allocation();
            throw runtime_error(RuntimeError::regex_syntax,values.location);
        }
        if (nil_text) throw runtime_error(RuntimeError::nil_reference,values.location);
        std::unique_ptr<pcre2_match_context,decltype(&pcre2_match_context_free)> context(
            pcre2_match_context_create(general.get()),pcre2_match_context_free);
        std::unique_ptr<pcre2_match_data,decltype(&pcre2_match_data_free)> match(
            pcre2_match_data_create_from_pattern(code.get(),general.get()),pcre2_match_data_free);
        if (!context || !match) throw allocation();
        pcre2_set_match_limit(context.get(),static_cast<std::uint32_t>(std::min<std::uint64_t>(options.instruction_limit,UINT32_MAX)));
        pcre2_set_depth_limit(context.get(),1000); pcre2_set_heap_limit(context.get(),4096);
        pcre2_set_callout(context.get(),Work::callout,&budget);
        std::u16string result; std::size_t previous=0,offset=0;
        while (offset<=text.size()) {
            budget.check();
            const auto status=pcre2_match(code.get(),subject.data(),text.size(),offset,0,match.get(),context.get());
            if (budget.failure) std::rethrow_exception(budget.failure);
            if (status==PCRE2_ERROR_NOMATCH) break;
            if (status==PCRE2_ERROR_MATCHLIMIT || status==PCRE2_ERROR_DEPTHLIMIT) throw runtime_error(RuntimeError::instruction_limit,values.location);
            if (scratch.failed || status==PCRE2_ERROR_HEAPLIMIT || status==PCRE2_ERROR_NOMEMORY) throw allocation();
            if (status<0) throw runtime_error(RuntimeError::regex_syntax,values.location);
            const auto* positions=pcre2_get_ovector_pointer(match.get()); const auto begin=positions[0],end=positions[1];
            if (begin<previous || end<begin || end>text.size()) throw runtime_error(RuntimeError::regex_syntax,values.location);
            if (action==Action::is_match) return true;
            if (action==Action::match_value) return std::u16string{text.substr(begin,end-begin)};
            append(result,text.substr(previous,begin-previous),values.location);
            const auto found=text.substr(begin,end-begin);
            if (found==first) append(result,second,values.location);
            else if (found==second) append(result,first,values.location);
            else throw runtime_error(RuntimeError::missing_swap_key,values.location);
            previous=end;
            // Regex.Replace advances a single UTF-16 unit after an empty match;
            // it does not try a nonempty alternative at the same position.
            if (begin==end && end==text.size()) break;
            offset=end+(begin==end ? 1 : 0);
        }
        if (action==Action::is_match) return false;
        if (action==Action::match_value) return std::u16string{};
        append(result,text.substr(previous),values.location); return result;
    } catch (const std::bad_alloc&) { throw runtime_error(RuntimeError::allocation_limit,values.location); }
}
}
std::u16string regex_swap(Values& values,std::u16string_view text,std::u16string_view first,
    std::u16string_view second,const ExecutionOptions& options,std::uint64_t& work) {
    return std::get<std::u16string>(search(values,text,first,second,Action::swap,false,options,work));
}
Value regex_search(Values& values,const Value& text,const Value& pattern,bool match_value,const ExecutionOptions& options) {
    if (std::holds_alternative<NilString>(pattern)) throw runtime_error(RuntimeError::nil_reference,values.location);
    std::uint64_t work=0;
    return search(values,string_text(text),string_text(pattern),{},match_value ? Action::match_value : Action::is_match,
        std::holds_alternative<NilString>(text),options,work);
}
}

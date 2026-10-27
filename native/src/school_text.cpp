#include "school.hpp"
#include "runtime_values.hpp"
#include <array>

namespace abc {
namespace {
struct TextWork {
    const ExecutionOptions& options;
    SourceLocation location;
    std::uint64_t work=0;
    void check() {
        if (work>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,location);
        if ((work++&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,location);
    }
    void append(std::u16string& target, std::u16string_view part) const {
        if (part.size()>1'048'576-target.size()) throw runtime_error(RuntimeError::string_limit,location);
        target+=part;
    }
};
template<class Found> void matches(std::u16string_view pattern, std::u16string_view text, bool overlap, TextWork& work, Found found) {
    std::vector<std::size_t> failure(pattern.size());
    for (std::size_t i=1,q=0;i<pattern.size();++i) {
        work.check();
        while (q && pattern[i]!=pattern[q]) { work.check(); q=failure[q-1]; }
        if (pattern[i]==pattern[q]) ++q;
        failure[i]=q;
    }
    for (std::size_t i=0,q=0;i<text.size();++i) {
        work.check();
        while (q && text[i]!=pattern[q]) { work.check(); q=failure[q-1]; }
        if (text[i]==pattern[q]) ++q;
        if (q==pattern.size()) { found(i+1-q); q=overlap ? failure[q-1] : 0; }
    }
}
std::u16string replace(std::u16string_view text, std::u16string_view source, std::u16string_view target, TextWork& work) {
    if (source.empty()) throw runtime_error(RuntimeError::empty_substring,work.location);
    std::u16string result; std::size_t previous=0;
    matches(source,text,false,work,[&](std::size_t index) {
        work.append(result,text.substr(previous,index-previous)); work.append(result,target); previous=index+source.size();
    });
    work.append(result,text.substr(previous)); return result;
}
}
Value school_replace_many(const Value& text, const std::shared_ptr<Aggregate>& source, const std::shared_ptr<Aggregate>& target,
    SourceLocation location, const ExecutionOptions& options) {
    TextWork work{options,location};
    // StringBuilder accepts a nil initial value; target.Count precedes source.Count.
    auto result=string_text(text);
    if (target->nil || source->nil) throw runtime_error(RuntimeError::nil_reference,location);
    for (std::size_t i=0;i<source->items.size();++i) {
        work.check(); const auto& pattern=source->items[i]->value;
        if (std::holds_alternative<NilString>(pattern)) throw runtime_error(RuntimeError::nil_reference,location);
        const auto replacement=i<target->items.size() ? std::u16string_view(string_text(target->items[i]->value)) : std::u16string_view{};
        result=replace(result,string_text(pattern),replacement,work);
    }
    return result;
}
void school_replace_last(Values& values, const std::shared_ptr<Cell>& receiver, const Value& source, const Value& target, const ExecutionOptions& options) {
    const auto& pattern=string_text(source);
    if (pattern.empty()) return;
    if (std::holds_alternative<NilString>(receiver->value)) throw runtime_error(RuntimeError::nil_reference,values.location);
    const auto& text=string_text(receiver->value); TextWork work{options,values.location};
    std::optional<std::size_t> last;
    matches(pattern,text,true,work,[&](std::size_t index) { last=index; });
    if (!last) return;
    auto remaining=text; remaining.erase(*last,pattern.size()); values.write(receiver,remaining);
    const std::array<Value,3> insert{target,receiver->value,static_cast<std::int32_t>(*last+1)};
    values.write(receiver,*stdlib::invoke_text(stdlib::Function::insert_string,insert,values.location));
}

void school_table_print(const std::shared_ptr<Aggregate>& table,std::int32_t filter,const Value& names,SourceLocation location,stdlib::Context& context) {
    if (table->nil || std::holds_alternative<NilString>(names)) throw runtime_error(RuntimeError::nil_reference,location);
    if (!table->columns) throw runtime_error(RuntimeError::string_length,location);
    const auto variables=table->columns-1;
    if (variables>1'048'576) throw runtime_error(RuntimeError::string_limit,location);
    TextWork work{context.options,location};
    auto header=string_text(names).substr(0,variables); header.resize(variables,u' ');
    const auto write=[&](std::u16string_view text) { context.write_text(text,location); };
    write(u" ");
    for (char16_t character:header) {
        work.check(); write(std::u16string_view(&character,1));
        const auto& delimiter=context.print_delimiter();
        write(std::holds_alternative<NilString>(delimiter) ? std::u16string_view{u"nil"} : std::u16string_view{string_text(delimiter)});
    }
    write(u"F\n");
    const auto dashes=2*table->columns-1;
    if (dashes>1'048'576) throw runtime_error(RuntimeError::string_limit,location);
    write(std::u16string{u" "}+std::u16string(dashes,u'-')); write(u"\n");
    for (std::size_t row=0;row<table->rows;++row) {
        work.check(); const auto start=row*table->columns;
        const auto value=std::get<bool>(table->items[start+variables]->value);
        if ((filter==0 && value) || (filter==1 && !value)) continue;
        for (std::size_t column=0;column<table->columns;++column) {
            work.check(); write(std::get<bool>(table->items[start+column]->value) ? u" 1" : u" 0");
        }
        write(u"\n");
    }
}

namespace stdlib {
Value invoke_school_text(Function function, std::span<const Value> arguments, SourceLocation location, const ExecutionOptions& options) {
    TextWork work{options,location};
    if (function==Function::school_replace_many) {
        if (std::holds_alternative<NilString>(arguments[2]) || std::holds_alternative<NilString>(arguments[1])) throw runtime_error(RuntimeError::nil_reference,location);
        auto result=string_text(arguments[0]); const auto& source=string_text(arguments[1]); const auto& target=string_text(arguments[2]);
        for (std::size_t i=0;i<source.size();++i) {
            work.check(); result=replace(result,std::u16string_view(source).substr(i,1),i<target.size() ? std::u16string_view(target).substr(i,1) : std::u16string_view{},work);
        }
        return result;
    }
    if (std::holds_alternative<NilString>(arguments[0])) throw runtime_error(RuntimeError::nil_reference,location);
    const auto& text=string_text(arguments[0]);
    if (text.empty()) return std::u16string{};
    if (std::holds_alternative<NilString>(arguments[1])) throw runtime_error(RuntimeError::nil_reference,location);
    const auto& source=string_text(arguments[1]);
    std::vector<std::int32_t> indices(65536,-1);
    for (std::size_t i=0;i<source.size();++i) { work.check(); if (indices[source[i]]<0) indices[source[i]]=static_cast<std::int32_t>(i); }
    std::u16string result; result.reserve(text.size());
    for (auto c:text) {
        work.check(); const auto index=indices[c];
        if (index<0) { result+=c; continue; }
        if (std::holds_alternative<NilString>(arguments[2])) throw runtime_error(RuntimeError::nil_reference,location);
        const auto& target=string_text(arguments[2]);
        if (static_cast<std::size_t>(index)<target.size()) result+=target[static_cast<std::size_t>(index)];
    }
    return result;
}
} // namespace stdlib
} // namespace abc

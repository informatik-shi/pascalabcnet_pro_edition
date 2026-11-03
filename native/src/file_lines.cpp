#include "file_lines.hpp"
#include "files.hpp"
#include "encoding.hpp"
#include "runtime_values.hpp"
#include "sequences.hpp"

namespace abc {
namespace {
void work(Values& values,std::uint64_t& used,const ExecutionOptions& options) {
    if(used>=options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,values.location);
    if((used&255U)==0 && options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,values.location);
    ++used;
}
const TextEncoding* selected(FileContext& files,const Value& path,const Value* encoding,SourceLocation location) {
    (void)files.resolve(string_text(path),location);
    return encoding ? &encoding_value(*encoding,location) : files.encoding(location);
}
}
Value file_read_lines(Values& values,FileContext& files,Type result,const Value& path,const Value* encoding,const ExecutionOptions& options) {
    const auto location=values.location;const auto data=selected(files,path,encoding,location);
    const auto text=read_text_file(files,string_text(path),location,data);
    auto output=values.array(result,0);std::size_t start=0;std::uint64_t used=0;
    for(std::size_t i=0;i<text.size();++i) {
        work(values,used,options);
        if(text[i]!=u'\r' && text[i]!=u'\n')continue;
        output->items.push_back(values.cell(Type::string,text.substr(start,i-start)));
        if(text[i]==u'\r' && i+1<text.size() && text[i+1]==u'\n')++i;
        start=i+1;
    }
    if(start<text.size())output->items.push_back(values.cell(Type::string,text.substr(start)));
    return output;
}
void file_write_lines(Values& values,FileContext& files,const Value& path,const std::shared_ptr<Aggregate>& source,const Value* encoding,const ExecutionOptions& options) {
    const auto location=values.location;(void)selected(files,path,encoding,location);
    if(!source || source->nil)throw runtime_error(RuntimeError::nil_reference,location);
    auto output=std::make_shared<TextFile>();text_assign(files,*output,path,location);
    text_open(files,output,1,location,encoding);
    std::uint64_t used=0;
    const auto write=[&](const Value& value) {
        work(values,used,options);const auto& text=string_text(value);
        for(std::size_t i=0;i<text.size();++i)work(values,used,options);
        text_write(*output,text,location);
#ifdef _WIN32
        text_write(*output,u"\r\n",location);
#else
        text_write(*output,u"\n",location);
#endif
    };
    if(type_info(values.program,source->type).kind==CompoundKind::sequence) {
        const auto cursor=sequence_begin(values,source);
        for(std::uint64_t i=0;sequence_resume(values,cursor,i,options);++i) {
            if(i>static_cast<std::uint64_t>(INT64_MAX))throw runtime_error(RuntimeError::integer_overflow,location);
            const auto line=sequence_at(values,cursor,static_cast<std::int64_t>(i),options);write(line);
        }
    } else for(const auto& line:source->items)write(line->value);
    output->close(location);
}
}

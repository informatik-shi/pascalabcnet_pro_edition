#include "file_lines.hpp"
#include "files.hpp"
#include "encoding.hpp"
#include "runtime_values.hpp"
#include "sequences.hpp"
#include "file_reader.hpp"

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
namespace {
void reserve(Values& values,std::size_t count) {
    if(count>values.remaining)throw runtime_error(RuntimeError::allocation_limit,values.location);
    values.remaining-=count;
}
std::shared_ptr<LineReader> open_reader(Values& values,FileContext& files,std::u16string_view path,TextEncoding encoding) {
    reserve(values,1);auto reader=std::make_shared<LineReader>(files.resolve(path,values.location),encoding,values.location);
    files.prepare_line_open(reader,values.location);return reader;
}
}
std::shared_ptr<Aggregate> file_sequence_begin(Values& values,const std::shared_ptr<Aggregate>& prototype) {
    const auto& source=prototype->sequence->file_line;
    if(!source || !source->files)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    if(!prototype->sequence->started){prototype->sequence->started=true;return prototype;}
    reserve(values,1);auto result=std::make_shared<Aggregate>();result->type=prototype->type;
    result->sequence=std::make_unique<SequenceState>();result->sequence->provider=SequenceProvider::file_lines;result->sequence->started=true;
    auto state=std::make_shared<FileLineState>();state->files=source->files;state->path=source->path;state->encoding=source->encoding;
    state->reader=source->reader ? source->reader : open_reader(values,*state->files,state->path,state->encoding);
    result->sequence->file_line=std::move(state);return result;
}
void file_sequence_dispose(FileLineState& state) noexcept {
    if(state.reader)state.reader->close();
    state.reader.reset();state.current=NilString{};state.ready=false;state.done=true;
}
bool file_sequence_resume(Values& values,const std::shared_ptr<Aggregate>& cursor,std::uint64_t index,const ExecutionOptions& options) {
    auto& state=*cursor->sequence->file_line;
    if(state.done)return false;
    if(state.ready && state.next && index==state.next-1)return true;
    if(index!=state.next || !state.reader)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    auto line=state.reader->read_line(values.location,options);
    if(!line){sequence_dispose(cursor);return false;}
    state.current=std::move(*line);state.ready=true;++state.next;cursor->sequence->count=state.next;return true;
}
Value file_read_lines(Values& values,FileContext& files,Type result,const Value& path,const Value* encoding,const ExecutionOptions& options) {
    const auto location=values.location;const auto data=selected(files,path,encoding,location);
    if(type_info(values.program,result).kind==CompoundKind::sequence) {
        reserve(values,2);auto output=std::make_shared<Aggregate>();output->type=result;output->sequence=std::make_unique<SequenceState>();
        auto state=std::make_shared<FileLineState>();state->files=&files;state->path=string_text(path);
        state->encoding=data ? *data : TextEncoding{TextEncodingKind::utf8,true,true};
        state->reader=open_reader(values,files,state->path,state->encoding);output->sequence->provider=SequenceProvider::file_lines;
        output->sequence->file_line=std::move(state);return output;
    }
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
Value text_lines_sequence(Values& values,Type result,const Value& file) {
    (void)text_file(file,values.location);reserve(values,1);
    auto output=std::make_shared<Aggregate>();output->type=result;output->sequence=std::make_unique<SequenceState>();
    output->sequence->provider=SequenceProvider::text_lines;output->sequence->text_input=std::get<std::shared_ptr<TextFile>>(file);return output;
}
bool text_lines_resume(Values& values,const std::shared_ptr<Aggregate>& cursor,std::uint64_t index) {
    auto& state=*cursor->sequence->text_line;const auto location=values.location;
    if(state.done)return false;
    if(state.ready && state.next && index==state.next-1)return true;
    if(index!=state.next || !state.file)throw runtime_error(RuntimeError::invalid_bytecode,location);
    if(!state.initialized) {
        if(!state.file->reader)throw runtime_error(RuntimeError::nil_reference,location);
        state.file->reader->seek_begin_preserving_buffer(location);state.initialized=true;
    }
    if(text_eof(*state.file,false,false,location)){state.done=true;state.ready=false;state.current=NilString{};return false;}
    state.current=text_read(*state.file,Type::string,location);text_skip_line(*state.file,location);
    state.ready=true;++state.next;cursor->sequence->count=state.next;return true;
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

#pragma once
#include "abc/core.hpp"
#include "file_encoding.hpp"
namespace abc {
struct Values;class FileContext;class LineReader;
struct FileLineState {
    FileContext* files=nullptr;
    std::u16string path;TextEncoding encoding;
    std::shared_ptr<LineReader> reader;
    Value current=NilString{};
    std::uint64_t next=0;bool ready=false,done=false;
};
std::shared_ptr<Aggregate> file_sequence_begin(Values& values,const std::shared_ptr<Aggregate>& prototype);
bool file_sequence_resume(Values& values,const std::shared_ptr<Aggregate>& cursor,std::uint64_t index,const ExecutionOptions& options);
void file_sequence_dispose(FileLineState& state) noexcept;
Value file_read_lines(Values& values,FileContext& files,Type result,const Value& path,const Value* encoding,const ExecutionOptions& options);
void file_write_lines(Values& values,FileContext& files,const Value& path,const std::shared_ptr<Aggregate>& source,const Value* encoding,const ExecutionOptions& options);
}

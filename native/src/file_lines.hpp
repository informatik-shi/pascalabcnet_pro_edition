#pragma once
#include "abc/core.hpp"
namespace abc {
struct Values;class FileContext;
Value file_read_lines(Values& values,FileContext& files,Type result,const Value& path,const Value* encoding,const ExecutionOptions& options);
void file_write_lines(Values& values,FileContext& files,const Value& path,const std::shared_ptr<Aggregate>& source,const Value* encoding,const ExecutionOptions& options);
}

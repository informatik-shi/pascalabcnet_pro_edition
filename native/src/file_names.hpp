#pragma once
#include "abc/core.hpp"
namespace abc {
class FileContext;
enum class FileNameMethod { name, extension, path, directory, drive, full };
Value file_name_info(const FileContext&,const Value&,FileNameMethod,SourceLocation,const ExecutionOptions&);
}

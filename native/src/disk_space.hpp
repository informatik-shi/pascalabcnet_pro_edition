#pragma once
#include "abc/core.hpp"
namespace abc {
class FileContext;
std::u16string disk_name_from_number(std::int32_t,std::u16string_view);
std::int64_t disk_space(const FileContext&,const Value&,bool,SourceLocation,const ExecutionOptions&);
}

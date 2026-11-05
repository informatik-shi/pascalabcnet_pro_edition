#pragma once
#include "abc/core.hpp"
namespace abc {
struct Values;class FileContext;
enum class DirectorySequenceMethod { files, all_files, directories, all_directories };
bool directory_sequence_signature(const Program&,Type,DirectorySequenceMethod);
std::shared_ptr<Aggregate> directory_sequence_create(Values&,FileContext&,Type,DirectorySequenceMethod,const Value&,const Value&,const ExecutionOptions&);
std::shared_ptr<Aggregate> directory_sequence_begin(Values&,const std::shared_ptr<Aggregate>&);
bool directory_sequence_resume(Values&,const std::shared_ptr<Aggregate>&,std::uint64_t,const ExecutionOptions&);
Value directory_sequence_current(const std::shared_ptr<Aggregate>&);
void directory_sequence_dispose(DirectorySequenceState&) noexcept;
// Used by the directory provider and independent filesystem compatibility corpus.
bool directory_pattern_matches(std::u16string_view,std::u16string_view,bool,const ExecutionOptions&,SourceLocation={});
}

#pragma once
#include "abc/core.hpp"

namespace abc {
enum class RuntimeError {
    integer_overflow, division_zero, invalid_bytecode, number_format,
    input_limit, input_stream, input_end, input_encoding, input_empty,
    input_boolean, input_real, input_range, output_limit, output_stream,
    instruction_limit, cancelled, string_limit, real_overflow, math_domain, round_digits, string_index,
    number_text, number_range, string_slice, decimal_separator, string_length, allocation_limit, index_bounds, array_length, call_limit, file_io, file_limit, file_encoding, nil_reference, stack_empty, queue_empty, linked_empty, directory_io,
    file_unassigned, file_closed, file_read_mode, file_write_mode, file_end, file_open_limit, file_erase, file_rename, collection_changed, character_digit, random_range, random_source, field_width
};
Error runtime_error(RuntimeError id, SourceLocation location, std::string_view context = {});
} // namespace abc

#pragma once
#include "abc/core.hpp"

namespace abc {
enum class RuntimeError {
    integer_overflow, division_zero, invalid_bytecode, number_format, big_integer_limit, power_exponent, school_table_variables, school_ip_address, school_ip_suffix, school_ip_mask, regex_syntax, duplicate_swap_key, missing_swap_key, sequence_empty, sequence_multiple, sequence_count_range,
    input_limit, input_stream, input_end, input_encoding, input_empty,
    input_boolean, input_real, input_range, output_limit, output_stream,
    instruction_limit, cancelled, string_limit, real_overflow, math_domain, round_digits, string_index,
    number_text, number_range, string_slice, decimal_separator, string_length, allocation_limit, index_bounds, array_length, call_limit, file_io, file_limit, file_encoding, nil_reference, stack_empty, queue_empty, linked_empty, directory_io,
    file_unassigned, file_closed, file_read_mode, file_write_mode, file_end, file_open_limit, file_erase, file_rename, collection_changed, character_digit, random_range, random_source, field_width, school_base, school_digit, school_expanded_digit, school_factorization, empty_substring, school_empty_arguments
};
Error runtime_error(RuntimeError id, SourceLocation location, std::string_view context = {});
} // namespace abc

#pragma once
#include "abc/core.hpp"

namespace abc {
enum class RuntimeError {
    integer_overflow, division_zero, invalid_bytecode, number_format,
    input_limit, input_stream, input_end, input_encoding, input_empty,
    input_boolean, input_real, input_range, output_limit, output_stream,
    instruction_limit, cancelled, string_limit, real_overflow, math_domain, round_digits, string_index
};
Error runtime_error(RuntimeError id, SourceLocation location, std::string_view context = {});
} // namespace abc

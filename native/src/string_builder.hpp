#pragma once
#include "abc/core.hpp"
#include <span>

namespace abc {
struct Values;
enum class StringBuilderMethod : std::uint8_t {
    create_empty, create_capacity, create_limited, create_text, create_text_capacity, create_slice,
    length, capacity, max_capacity, ensure_capacity, set_length, set_capacity,
    get_char, set_char, clear, text, text_range, append_text, append_char,
    append_repeated, append_slice, append_line, append_line_text, equals,
    append_scalar, append_array, append_array_range, append_builder, copy_to, replace_char, replace_char_range,
    insert_text, insert_repeated, insert_scalar, insert_array, insert_array_range, remove
};
bool string_builder_signature(const Program&,Type,std::size_t);
std::vector<Type> string_builder_parameters(Type,StringBuilderMethod,Type=Type::invalid);
Type string_builder_result(Type,StringBuilderMethod);
Value string_builder_invoke(Values&,Type,StringBuilderMethod,std::span<const Value>);
}

#pragma once
#include "abc/core.hpp"
#include <span>

namespace abc { class FileContext; }
namespace abc::stdlib {
enum class Function {
    abs, sqr, sign, minimum, maximum, sin, sinh, cos, cosh, tan, tanh,
    arcsin, arccos, arctan, exp, ln, log2, log10, logn, sqrt, cbrt, isqrt,
    power, round, trunc, int_part, frac, floor, ceil, deg_to_rad, rad_to_deg,
    length, copy, pos, last_pos, concat,
    trim, trim_left, trim_right, left_str, right_str, reverse_string, compare_str,
    int_to_str, str_to_int, str_to_int64, str_to_real, float_to_str,
    resize_string, insert_string, delete_string, read_all_text, write_all_text, file_exists,
    get_dir, change_dir, make_dir, remove_dir, create_dir, try_remove_dir, delete_file, set_current_dir, change_extension,
    rename_file, rename_directory, ordinal, character, to_word, successor, predecessor, string_of_char, repeat_string, to_digit, string_length,
    text_assign, text_reset, text_rewrite, text_append, text_close, text_flush,
    text_eof, text_eoln, text_seek_eof, text_seek_eoln, text_open_read, text_open_write, text_open_append,
    text_read, text_read_line, text_read_word, text_skip_line, text_name, text_full_name, text_read_end, text_erase, text_rename
};
struct Signature {
    std::string_view name;
    std::vector<Type> parameters;
    Type result;
    Function function;
};
// One catalog is shared by the type checker, bytecode verifier and VM.
std::span<const Signature> signatures();
std::optional<Value> constant(std::string_view name);
Value invoke(std::size_t id, std::span<const Value> arguments, SourceLocation location, FileContext& files);
std::optional<Value> invoke_text(Function function, std::span<const Value> arguments, SourceLocation location);
} // namespace abc::stdlib

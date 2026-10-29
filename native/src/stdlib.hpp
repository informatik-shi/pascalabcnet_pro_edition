#pragma once
#include "abc/core.hpp"
#include <span>

namespace abc { class FileContext; class RandomContext; class ConsoleInput; struct SchoolContext; struct Values; }
namespace abc::stdlib {
enum class Function {
    abs, sqr, sign, minimum, maximum, sin, sinh, cos, cosh, tan, tanh,
    arcsin, arccos, arctan, exp, ln, log2, log10, logn, sqrt, cbrt, isqrt,
    power, round, trunc, int_part, frac, floor, ceil, deg_to_rad, rad_to_deg, atan2, hypot, lerp, remap, gcd, lcm,
    length, copy, pos, last_pos, concat,
    trim, trim_left, trim_right, left_str, right_str, reverse_string, compare_str,
    int_to_str, str_to_int, str_to_int64, str_to_real, parse_real, is_integer, is_real, to_integer_default, to_real_default, float_to_str,
    resize_string, insert_string, delete_string, string_pad_left, string_pad_right, string_substring, string_insert_at, string_remove_at, read_all_text, write_all_text, file_exists,
    get_dir, change_dir, make_dir, remove_dir, create_dir, try_remove_dir, delete_file, set_current_dir, change_extension,
    rename_file, rename_directory, ordinal, character, integer_cast, successor, predecessor, string_of_char, repeat_string, to_digit, string_length,
    character_lower, character_upper, character_letter, character_digit, character_is_lower, character_is_upper, character_code, string_lower, string_upper,
    big_cast, big_parse, big_real_divide, big_zero, big_one, big_even,
    files_read_to_end, files_write_string,
    random_reset, random_integer, random_real, random_char, random_rounded,
    console_read, console_readln, console_line, console_token, format_value,
    school_bin, school_oct, school_hex, school_dec, school_dec_big, school_to_base, school_bin_format, school_perfect_square, school_is_prime, school_imp, school_replace_many, school_translate, school_gcd_alias, school_lcm_alias, school_sin_degrees, school_cos_degrees, school_tan_degrees,
    text_assign, text_reset, text_rewrite, text_append, text_close, text_flush,
    text_eof, text_eoln, text_seek_eof, text_seek_eoln, text_open_read, text_open_write, text_open_append,
    text_read, text_read_line, text_read_word, text_skip_line, text_name, text_full_name, text_read_end, text_erase, text_rename,
    regex_is_match, regex_match_value, assertion
};
struct Signature {
    std::string_view name;
    std::vector<Type> parameters;
    Type result;
    Function function;
    std::string_view unit = {}; // Empty means implicit PABCSystem; other units require uses.
};
struct Context {
    FileContext& files;
    RandomContext& random;
    ConsoleInput& console;
    std::function<void(std::u16string_view,SourceLocation)> write_text;
    std::function<const Value&()> print_delimiter;
    const ExecutionOptions& options;
    SchoolContext& school;
    Values& values;
};
// One catalog is shared by the type checker, bytecode verifier and VM.
std::span<const Signature> signatures();
std::optional<Value> constant(std::string_view name);
Value invoke(std::size_t id, std::span<const Value> arguments, SourceLocation location, Context& context);
std::optional<Value> invoke_text(Function function, std::span<const Value> arguments, SourceLocation location);
std::optional<Value> try_parse_number(Type type, const Value& source, SourceLocation location);
bool unicode_whitespace(char16_t character);
std::pair<std::u16string,std::int32_t> read_string_word(const Value& source, std::int32_t position,
    SourceLocation location, const ExecutionOptions& options, bool skip_only=false);
std::pair<std::int32_t,std::int32_t> read_string_integer(const Value& source, std::int32_t position,
    SourceLocation location, const ExecutionOptions& options);
} // namespace abc::stdlib

#pragma once
#include "abc/big_integer.hpp"

#include <cstdint>
#include <cstddef>
#include <compare>
#include <functional>
#include <memory>
#include <deque>
#include <istream>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <utility>
#include <vector>

namespace abc {

struct SourceLocation {
    std::size_t offset = 0;
    std::size_t line = 1;
    std::size_t column = 1;
};

class Error : public std::runtime_error {
public:
    Error(std::string code, SourceLocation location, std::string message, std::string russian = {})
        : std::runtime_error(std::move(message)), code(std::move(code)), location(location), russian(std::move(russian)) {}
    std::string code;
    SourceLocation location;
    std::string russian;
};

// English remains available through what(); runtime diagnostics also carry Russian.
std::string format_diagnostic(const Error& error, std::string_view filename);

enum class Type { integer, int64, real, boolean, string, invalid, unit, nil, text_file, character, word, byte, shortint, smallint, longword, uint64, big_integer, first_compound = 64 };
inline bool small_integer(Type t) { return t==Type::word || t==Type::byte || t==Type::shortint || t==Type::smallint; }
inline bool wide_unsigned(Type t) { return t==Type::longword || t==Type::uint64; }
inline bool integer_type(Type t) { return t==Type::integer || t==Type::int64 || small_integer(t) || wide_unsigned(t); }
inline bool ordinal(Type t) { return integer_type(t) || t==Type::character || t==Type::boolean; }
inline Type increment_step_type(Type t) { return t==Type::boolean ? Type::unit : t==Type::int64 || t==Type::uint64 ? Type::int64 : Type::integer; }
inline bool scalar(Type t) { return integer_type(t) || t==Type::big_integer || t==Type::real || t==Type::boolean || t==Type::string || t==Type::character; }
inline std::pair<std::int64_t,std::int64_t> small_integer_bounds(Type t) {
    switch (t) {
    case Type::byte: return {0,255};
    case Type::shortint: return {-128,127};
    case Type::smallint: return {-32768,32767};
    case Type::word: return {0,65535};
    default: throw std::logic_error("Small integer type required");
    }
}
inline bool compound(Type t) { return static_cast<unsigned>(t) >= 64 && static_cast<unsigned>(t) < 0x40000000U; }
inline Type reference_type(Type t) { return static_cast<Type>(static_cast<unsigned>(t) | 0x40000000U); }
std::string_view type_name(Type type);
std::optional<Type> find_type(std::string_view name);
bool numeric(Type type);
bool can_widen(Type from, Type to);

// Runtime representation is independent of the host's int, long and wchar_t.
struct Aggregate;
struct Cell;
struct TextFile;
struct LinkedLinks { std::weak_ptr<Aggregate> prev, next; };
// A null string keeps its static string type and differs from an empty string.
struct NilString { auto operator<=>(const NilString&) const = default; };
using Value = std::variant<std::int32_t, std::int64_t, double, bool, std::u16string,
    std::shared_ptr<Aggregate>, std::shared_ptr<Cell>, NilString, std::shared_ptr<TextFile>, char16_t, std::uint16_t,
    std::uint8_t, std::int8_t, std::int16_t, std::uint32_t, std::uint64_t, BigInteger>;
struct Cell { Type type; Value value; std::weak_ptr<Aggregate> list_owner{}; };
enum class SequenceProvider { school_ip_hosts, school_ip_binary, array, list };
inline bool collection_sequence(SequenceProvider provider) { return provider==SequenceProvider::array || provider==SequenceProvider::list; }
struct SequenceState {
    std::shared_ptr<Aggregate> source;
    SequenceProvider provider=SequenceProvider::school_ip_hosts;
    std::uint64_t first=0,count=0,version=0;
    bool started=false;
};
struct Aggregate {
    Type type;
    std::vector<std::shared_ptr<Cell>> items;
    bool nil = false;
    std::size_t count = 0; // Logical size of DynArray and SimpleSet; items holds capacity.
    std::size_t rows = 0, columns = 0; // Shape of a rectangular rank-two array.
    std::uint64_t version = 0; // List enumerators detect successful writes and structural changes.
    // Allocate deque storage only for constructed stacks and queues.
    std::unique_ptr<std::deque<std::shared_ptr<Cell>>> ends;
    std::unique_ptr<LinkedLinks> links;
    std::unique_ptr<SequenceState> sequence;
};
Type value_type(const Value& value);
// Use only where Pascal string operations explicitly treat nil as empty.
const std::u16string& string_text(const Value& value);
int compare_strings(const Value& left, const Value& right);
std::u16string from_utf8(std::string_view text);
std::string to_utf8(std::u16string_view text, bool replace_invalid = false);

enum class Op {
    push, load, store, convert, negate, logical_not,
    add, subtract, multiply, divide_real, divide_int, modulo,
    equal, not_equal, less, less_equal, greater, greater_equal,
    jump, jump_false, print, newline, read_line, halt, call_builtin,
    make_default, ref_local, load_global, store_global, ref_global, store_ref,
    get_index, ref_index, get_field, ref_field, array_new, array_literal, length, resize_array,
    call_user, return_user, drop, list_add, list_remove, list_clear, list_contains,
    read_lines, write_lines, array_copy, array_high, sequence_sort, sequence_fill, mutate_string, increment, read_value,
    endpoint_call, buffer_call, association_call, linked_call, file_print, file_newline, file_read, file_skip_line, file_output,
    iteration_version, iteration_check, set_character, random_array, fill_random_array, sequence_mutate, swap_values, write_array,
    bit_not, bit_and, bit_or, bit_xor, shift_left, shift_right, read_console, read_array, console_output, array_query, write_matrix, object_text, print_output, try_read, convert_string, try_parse, val, string_read, string_array, school_list, replace_last, replace_many, school_fold, tuple_literal, school_pair, school_minmax, school_table_print, school_table_new, school_ip, sequence_begin, sequence_count, sequence_array, sequence_wrap
};

enum class SchoolFold { gcd, lcm };

enum class SchoolList { digits, digits_big, digits_to_int64, digits_to_big, primes, primes_range, first_primes, factors, prime_divisors, factors_count, prime_divisors_count, divisors, divisors_count };
inline bool school_factor_method(SchoolList method) { return method>=SchoolList::factors; }
inline bool school_list_count(SchoolList method) {
    return method==SchoolList::factors_count || method==SchoolList::prime_divisors_count || method==SchoolList::divisors_count;
}
inline std::vector<Type> school_list_parameters(SchoolList method, Type list, Type element=Type::integer) {
    if (school_factor_method(method)) return {element};
    if (method==SchoolList::digits) return {Type::int64,Type::integer};
    if (method==SchoolList::digits_big) return {Type::big_integer,Type::integer};
    if (method==SchoolList::digits_to_int64 || method==SchoolList::digits_to_big) return {list,Type::integer};
    if (method==SchoolList::primes_range) return {Type::integer,Type::integer};
    return {Type::integer};
}
inline Type school_list_result(SchoolList method, Type list) { return school_list_count(method) ? Type::integer : method==SchoolList::digits_to_int64 ? Type::int64 : method==SchoolList::digits_to_big ? Type::big_integer : list; }

enum class StringArray { words, words_array, lines, integers, integers_count, reals, reals_separator, integers64 };
inline bool string_array_parameter(StringArray method) {
    return method==StringArray::words || method==StringArray::words_array || method==StringArray::integers_count || method==StringArray::reals_separator;
}
inline Type string_array_element(StringArray method) {
    if (method==StringArray::integers64) return Type::int64;
    return method==StringArray::words || method==StringArray::words_array || method==StringArray::lines ? Type::string
        : method==StringArray::integers || method==StringArray::integers_count ? Type::integer : Type::real;
}

enum class StringRead { integer, real, word, is_empty, try_integer, try_real, member_integer, member_real, member_word };
inline bool string_read_try(StringRead method) { return method==StringRead::try_integer || method==StringRead::try_real; }
inline bool string_read_member(StringRead method) { return method>=StringRead::member_integer; }
inline StringRead string_read_base(StringRead method) {
    return method==StringRead::member_integer ? StringRead::integer : method==StringRead::member_real ? StringRead::real : method==StringRead::member_word ? StringRead::word : method;
}
inline Type string_read_number(StringRead method) { return method==StringRead::try_real ? Type::real : Type::integer; }
inline Type string_read_result(StringRead method) {
    method=string_read_base(method);
    return method==StringRead::integer ? Type::integer : method==StringRead::real ? Type::real : method==StringRead::word ? Type::string : Type::boolean;
}

enum class ArrayQuery { length, upper_bound, lower_bound, rank, length_or_zero };

enum class SequenceMutation { reverse, reverse_range, shuffle, shuffle_result, swap_indices };
inline bool mutation_indices(SequenceMutation method) { return method==SequenceMutation::reverse_range || method==SequenceMutation::swap_indices; }
inline bool mutation_result(SequenceMutation method) { return method==SequenceMutation::shuffle_result || method==SequenceMutation::swap_indices; }

enum class RandomArrayMethod { integer, scaled_real, rounded_real, ranged_real };
inline Type random_array_element(RandomArrayMethod method) { return method==RandomArrayMethod::integer ? Type::integer : Type::real; }
inline std::vector<Type> random_array_parameters(RandomArrayMethod method) {
    if (method==RandomArrayMethod::integer) return {Type::integer,Type::integer};
    if (method==RandomArrayMethod::rounded_real) return {Type::real,Type::real,Type::integer};
    return {Type::real,Type::real};
}
enum class TextOutput { write, writeln, print, println };

struct Instruction {
    Op op = Op::halt;
    Type type = Type::invalid;
    std::size_t operand = 0;
    SourceLocation location;
};

struct Local {
    std::string name;
    Type type = Type::invalid;
    bool global = true;
};

enum class CompoundKind { array, record, list, stack, queue, dynarray, simple_set, association, linked_node, linked_list, single_node, tuple, school_address, school_ip, sequence };
inline bool node_kind(CompoundKind kind) { return kind == CompoundKind::linked_node || kind == CompoundKind::single_node; }
inline bool indexed_sequence(CompoundKind kind) { return kind == CompoundKind::array || kind == CompoundKind::list; }
inline bool endpoint_collection(CompoundKind kind) { return kind == CompoundKind::stack || kind == CompoundKind::queue; }
inline bool buffer_collection(CompoundKind kind) { return kind == CompoundKind::dynarray || kind == CompoundKind::simple_set; }
enum class BufferMethod { construct, add, insert, remove, find, contains, reserve, resize, count, capacity, get, set, to_string, print, println };
inline bool buffer_method_allowed(CompoundKind kind, BufferMethod method) {
    if (!buffer_collection(kind)) return false;
    if (kind == CompoundKind::dynarray) return method != BufferMethod::contains;
    return method == BufferMethod::add || method == BufferMethod::remove || method == BufferMethod::contains
        || method >= BufferMethod::to_string;
}
inline std::vector<Type> buffer_parameters(CompoundKind kind, Type element, BufferMethod method) {
    switch (method) {
    case BufferMethod::construct: case BufferMethod::reserve: case BufferMethod::resize: case BufferMethod::get: return {Type::integer};
    case BufferMethod::insert: case BufferMethod::set: return {Type::integer,element};
    case BufferMethod::remove: return {kind == CompoundKind::simple_set ? element : Type::integer};
    case BufferMethod::add: case BufferMethod::find: case BufferMethod::contains: return {element};
    default: return {};
    }
}
inline Type buffer_result(Type receiver, Type element, BufferMethod method) {
    switch (method) {
    case BufferMethod::construct: return receiver;
    case BufferMethod::get: return element;
    case BufferMethod::count: case BufferMethod::capacity: case BufferMethod::find: return Type::integer;
    case BufferMethod::contains: return Type::boolean;
    case BufferMethod::to_string: return Type::string;
    default: return Type::unit;
    }
}
enum class EndpointMethod { add, remove, top, is_empty, to_string, print, println };
inline Type endpoint_result(Type element, EndpointMethod method) {
    if (method == EndpointMethod::remove || method == EndpointMethod::top) return element;
    if (method == EndpointMethod::is_empty) return Type::boolean;
    if (method == EndpointMethod::to_string) return Type::string;
    return Type::unit;
}
struct TypeInfo {
    std::string name;
    CompoundKind kind = CompoundKind::array;
    Type element = Type::invalid;
    std::vector<Local> fields;
    Type key = Type::invalid;
    unsigned rank = 1;
};
enum class AssociationMethod { get, set, to_string, print, println };
inline std::vector<Type> association_parameters(const TypeInfo& info, AssociationMethod method) {
    if (method == AssociationMethod::get) return {info.key};
    if (method == AssociationMethod::set) return {info.key,info.element};
    return {};
}
inline Type association_result(const TypeInfo& info, AssociationMethod method) {
    if (method == AssociationMethod::get) return info.element;
    if (method == AssociationMethod::to_string) return Type::string;
    return Type::unit;
}
enum class LinkedMethod { construct, first, last, add_first, add_last, remove_first, remove_last,
    add_before, add_after, remove, to_string, print, println, value, set_value, prev, next, set_next };
inline bool linked_text_method(LinkedMethod method) {
    return method == LinkedMethod::to_string || method == LinkedMethod::print || method == LinkedMethod::println;
}
inline bool linked_method_allowed(CompoundKind kind, LinkedMethod method) {
    if (kind == CompoundKind::single_node) return method == LinkedMethod::construct || method == LinkedMethod::value
        || method == LinkedMethod::set_value || method == LinkedMethod::next || method == LinkedMethod::set_next;
    if (kind == CompoundKind::linked_node) return method == LinkedMethod::construct || (method >= LinkedMethod::value && method <= LinkedMethod::next);
    return kind == CompoundKind::linked_list && method >= LinkedMethod::first && method <= LinkedMethod::println;
}
inline std::vector<Type> linked_parameters(Type receiver, const TypeInfo& info, LinkedMethod method) {
    switch (method) {
    case LinkedMethod::construct:
        if (info.kind == CompoundKind::single_node) return {info.element,receiver};
        return {info.element,receiver,receiver};
    case LinkedMethod::set_next: return {receiver};
    case LinkedMethod::add_first: case LinkedMethod::add_last: case LinkedMethod::set_value: return {info.element};
    case LinkedMethod::add_before: case LinkedMethod::add_after: return {info.key,info.element};
    case LinkedMethod::remove: return {info.key};
    default: return {};
    }
}
inline Type linked_result(Type receiver, const TypeInfo& info, LinkedMethod method) {
    if (method == LinkedMethod::construct || method == LinkedMethod::prev || method == LinkedMethod::next) return receiver;
    if (method == LinkedMethod::first || method == LinkedMethod::last) return info.key;
    if (method == LinkedMethod::value) return info.element;
    if (method == LinkedMethod::to_string) return Type::string;
    return Type::unit;
}
struct Parameter { std::size_t local; Type type; bool by_reference = false; };
struct Routine {
    std::string name;
    Type result = Type::unit;
    std::size_t result_local = 0;
    std::vector<Parameter> parameters;
    std::vector<std::size_t> locals;
    std::vector<Instruction> code;
};

struct Program {
    std::vector<Value> constants;
    std::vector<Local> locals;
    std::vector<Instruction> code;
    std::vector<TypeInfo> types;
    std::vector<Routine> routines;
    std::optional<std::size_t> print_delimiter;
};

// compile never executes user code; no module is returned on any diagnostic.
Program compile(std::string_view source);
void verify(const Program& program);

struct ExecutionOptions {
    std::uint64_t instruction_limit = 10'000'000;
    std::size_t output_limit = 1'048'576;
    std::function<bool()> cancelled;
    std::size_t allocation_limit = 1'048'576;
    std::size_t call_limit = 128;
};

void execute(const Program& program, std::istream& input, std::ostream& output,
             const ExecutionOptions& options = {});

} // namespace abc

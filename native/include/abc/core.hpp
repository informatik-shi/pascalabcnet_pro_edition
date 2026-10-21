#pragma once

#include <cstdint>
#include <cstddef>
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

enum class Type { integer, int64, real, boolean, string, invalid, unit, nil, first_compound = 64 };
inline bool compound(Type t) { return static_cast<unsigned>(t) >= 64 && static_cast<unsigned>(t) < 0x40000000U; }
inline Type reference_type(Type t) { return static_cast<Type>(static_cast<unsigned>(t) | 0x40000000U); }
std::string_view type_name(Type type);
std::optional<Type> find_type(std::string_view name);
bool numeric(Type type);
bool can_widen(Type from, Type to);

// Runtime representation is independent of the host's int, long and wchar_t.
struct Aggregate;
struct Cell;
using Value = std::variant<std::int32_t, std::int64_t, double, bool, std::u16string,
    std::shared_ptr<Aggregate>, std::shared_ptr<Cell>>;
struct Cell { Type type; Value value; };
struct Aggregate {
    Type type;
    std::vector<std::shared_ptr<Cell>> items;
    bool nil = false;
    std::size_t count = 0; // Logical size of DynArray and SimpleSet; items holds capacity.
    // Allocate deque storage only for constructed stacks and queues.
    std::unique_ptr<std::deque<std::shared_ptr<Cell>>> ends;
};
Type value_type(const Value& value);
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
    endpoint_call, buffer_call, association_call
};

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

enum class CompoundKind { array, record, list, stack, queue, dynarray, simple_set, association };
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

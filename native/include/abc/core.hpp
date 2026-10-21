#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>
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

enum class Type { integer, int64, real, boolean, string, invalid };
std::string_view type_name(Type type);
std::optional<Type> find_type(std::string_view name);
bool numeric(Type type);
bool can_widen(Type from, Type to);

// Runtime representation is independent of the host's int, long and wchar_t.
using Value = std::variant<std::int32_t, std::int64_t, double, bool, std::u16string>;
Type value_type(const Value& value);
std::u16string from_utf8(std::string_view text);
std::string to_utf8(std::u16string_view text, bool replace_invalid = false);

enum class Op {
    push, load, store, convert, negate, logical_not,
    add, subtract, multiply, divide_real, divide_int, modulo,
    equal, not_equal, less, less_equal, greater, greater_equal,
    jump, jump_false, print, newline, read_line, halt, call_builtin
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
};

struct Program {
    std::vector<Value> constants;
    std::vector<Local> locals;
    std::vector<Instruction> code;
};

// compile never executes user code; no module is returned on any diagnostic.
Program compile(std::string_view source);
void verify(const Program& program);

struct ExecutionOptions {
    std::uint64_t instruction_limit = 10'000'000;
    std::size_t output_limit = 1'048'576;
    std::function<bool()> cancelled;
};

void execute(const Program& program, std::istream& input, std::ostream& output,
             const ExecutionOptions& options = {});

} // namespace abc

#include "abc/core.hpp"
#include "number.hpp"
#include "diagnostics.hpp"
#include "stdlib.hpp"

#include <charconv>
#include <cmath>
#include <limits>
#include <type_traits>

namespace abc {
namespace {

[[noreturn]] void trap(const Instruction& instruction, RuntimeError id, std::string_view context = {}) {
    throw runtime_error(id, instruction.location, context);
}

Value zero(Type type) {
    switch (type) {
    case Type::integer: return std::int32_t{0};
    case Type::int64: return std::int64_t{0};
    case Type::real: return 0.0;
    case Type::boolean: return false;
    case Type::string: return std::u16string{};
    default: throw runtime_error(RuntimeError::invalid_bytecode, {});
    }
}

template<class T> T integer_operation(T a, T b, const Instruction& instruction) {
    constexpr auto min = std::numeric_limits<T>::min();
    constexpr auto max = std::numeric_limits<T>::max();
    const auto overflow = [&]() -> T { trap(instruction, RuntimeError::integer_overflow); };
    switch (instruction.op) {
    case Op::add:
        if ((b > 0 && a > max - b) || (b < 0 && a < min - b)) return overflow();
        return static_cast<T>(a + b);
    case Op::subtract:
        if ((b > 0 && a < min + b) || (b < 0 && a > max + b)) return overflow();
        return static_cast<T>(a - b);
    case Op::multiply: {
        using U = std::make_unsigned_t<T>;
        const auto magnitude = [](T value) -> U {
            return value < 0 ? static_cast<U>(-(value + 1)) + U{1} : static_cast<U>(value);
        };
        const bool negative = (a < 0) != (b < 0);
        const U limit = negative ? magnitude(min) : static_cast<U>(max);
        const U left = magnitude(a), right = magnitude(b);
        if (right != 0 && left > limit / right) return overflow();
        const U product = left * right;
        if (negative && product == magnitude(min)) return min;
        return negative ? static_cast<T>(-static_cast<T>(product)) : static_cast<T>(product);
    }
    case Op::divide_int: case Op::modulo:
        if (b == 0) trap(instruction, RuntimeError::division_zero);
        if (a == min && b == -1) return instruction.op == Op::modulo ? T{0} : overflow();
        return instruction.op == Op::modulo ? static_cast<T>(a % b) : static_cast<T>(a / b);
    default: trap(instruction, RuntimeError::invalid_bytecode);
    }
}

std::string format(const Value& value, const Instruction& instruction) {
    if (const auto* text = std::get_if<std::u16string>(&value)) return to_utf8(*text, true);
    if (const auto* boolean = std::get_if<bool>(&value)) return *boolean ? "True" : "False";
    char buffer[128];
    std::to_chars_result result;
    if (const auto* integer = std::get_if<std::int32_t>(&value)) result = std::to_chars(buffer, buffer + sizeof buffer, *integer);
    else if (const auto* wide = std::get_if<std::int64_t>(&value)) result = std::to_chars(buffer, buffer + sizeof buffer, *wide);
    else result = std::to_chars(buffer, buffer + sizeof buffer, std::get<double>(value));
    if (result.ec != std::errc{}) trap(instruction, RuntimeError::number_format);
    return {buffer, result.ptr};
}

Value read_line(std::istream& input, const Instruction& instruction) {
    constexpr std::size_t maximum_line = 1'048'576;
    std::string line;
    bool received = false;
    char c;
    while (input.get(c)) {
        received = true;
        if (c == '\n') break;
        if (line.size() == maximum_line) trap(instruction, RuntimeError::input_limit);
        line += c;
    }
    if (input.bad()) trap(instruction, RuntimeError::input_stream);
    if (!received) trap(instruction, RuntimeError::input_end);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (instruction.type == Type::string) {
        try { return from_utf8(line); }
        catch (const std::invalid_argument&) { trap(instruction, RuntimeError::input_encoding); }
    }
    const auto first = line.find_first_not_of(" \t");
    const auto last = line.find_last_not_of(" \t");
    if (first == std::string::npos) trap(instruction, RuntimeError::input_empty);
    auto text = std::string_view(line).substr(first, last - first + 1);
    if (instruction.type == Type::boolean) {
        std::string normalized(text);
        for (auto& ch : normalized) if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch + 32);
        if (normalized == "true") return true;
        if (normalized == "false") return false;
        trap(instruction, RuntimeError::input_boolean);
    }
    if (text.starts_with('+')) text.remove_prefix(1);
    const auto parse = [&]<class T>() -> Value {
        if constexpr (std::is_floating_point_v<T>) {
            double value = 0;
            if (!parse_real(text, value)) trap(instruction, RuntimeError::input_real);
            return value;
        } else {
            T value{};
            const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
            if (result.ec != std::errc{} || result.ptr != text.data() + text.size())
                trap(instruction, RuntimeError::input_range, type_name(instruction.type));
            return value;
        }
    };
    switch (instruction.type) {
    case Type::integer: return parse.operator()<std::int32_t>();
    case Type::int64: return parse.operator()<std::int64_t>();
    case Type::real: return parse.operator()<double>();
    default: trap(instruction, RuntimeError::invalid_bytecode);
    }
}

} // namespace

void execute(const Program& program, std::istream& input, std::ostream& output, const ExecutionOptions& options) {
    try { verify(program); }
    catch (const Error& error) {
        throw Error(error.code, error.location, error.what(), "Некорректный байт-код: выполнение невозможно");
    }
    std::vector<Value> locals;
    for (const auto& local : program.locals) locals.push_back(zero(local.type));
    std::vector<Value> stack;
    std::uint64_t steps = 0;
    std::size_t output_bytes = 0;
    auto write = [&](std::string_view text, const Instruction& instruction) {
        if (text.size() > options.output_limit - output_bytes)
            trap(instruction, RuntimeError::output_limit);
        output.write(text.data(), static_cast<std::streamsize>(text.size()));
        output.flush();
        if (!output) trap(instruction, RuntimeError::output_stream);
        output_bytes += text.size();
    };
    auto pop = [&]() { auto value = std::move(stack.back()); stack.pop_back(); return value; };
    std::size_t pc = 0;
    while (pc < program.code.size()) {
        const auto& instruction = program.code[pc++];
        if (steps++ >= options.instruction_limit) trap(instruction, RuntimeError::instruction_limit);
        if (options.cancelled && options.cancelled()) trap(instruction, RuntimeError::cancelled);
        switch (instruction.op) {
        case Op::call_builtin: {
            const auto count = stdlib::signatures()[instruction.operand].parameters.size();
            const auto start = stack.size() - count;
            auto value = stdlib::invoke(instruction.operand, std::span<const Value>(stack).subspan(start), instruction.location);
            stack.resize(start);
            stack.push_back(std::move(value));
            break;
        }
        case Op::push: stack.push_back(program.constants[instruction.operand]); break;
        case Op::load: stack.push_back(locals[instruction.operand]); break;
        case Op::store: locals[instruction.operand] = pop(); break;
        case Op::convert: {
            const auto value = pop();
            if (value_type(value) == instruction.type) stack.push_back(value);
            else if (instruction.type == Type::int64) stack.emplace_back(static_cast<std::int64_t>(std::get<std::int32_t>(value)));
            else if (instruction.type == Type::real) {
                if (value_type(value) == Type::integer) stack.emplace_back(static_cast<double>(std::get<std::int32_t>(value)));
                else if (value_type(value) == Type::int64) stack.emplace_back(static_cast<double>(std::get<std::int64_t>(value)));
                else stack.push_back(value);
            } else stack.push_back(value);
            break;
        }
        case Op::negate: {
            auto value = pop();
            if (instruction.type == Type::integer) {
                const auto n = std::get<std::int32_t>(value);
                if (n == std::numeric_limits<std::int32_t>::min()) trap(instruction, RuntimeError::integer_overflow);
                stack.emplace_back(-n);
            } else if (instruction.type == Type::int64) {
                const auto n = std::get<std::int64_t>(value);
                if (n == std::numeric_limits<std::int64_t>::min()) trap(instruction, RuntimeError::integer_overflow);
                stack.emplace_back(-n);
            } else stack.emplace_back(-std::get<double>(value));
            break;
        }
        case Op::logical_not: stack.emplace_back(!std::get<bool>(pop())); break;
        case Op::add: case Op::subtract: case Op::multiply:
        case Op::divide_real: case Op::divide_int: case Op::modulo: {
            auto right = pop(), left = pop();
            if (instruction.type == Type::integer) stack.emplace_back(integer_operation(
                std::get<std::int32_t>(left), std::get<std::int32_t>(right), instruction));
            else if (instruction.type == Type::int64) stack.emplace_back(integer_operation(
                std::get<std::int64_t>(left), std::get<std::int64_t>(right), instruction));
            else if (instruction.type == Type::string) {
                constexpr std::size_t string_limit = 1'048'576;
                auto& a = std::get<std::u16string>(left);
                const auto& b = std::get<std::u16string>(right);
                if (b.size() > string_limit || a.size() > string_limit - b.size())
                    trap(instruction, RuntimeError::string_limit);
                a += b;
                stack.push_back(std::move(left));
            } else {
                const auto a = std::get<double>(left), b = std::get<double>(right);
                double result = 0;
                switch (instruction.op) {
                case Op::add: result = a + b; break;
                case Op::subtract: result = a - b; break;
                case Op::multiply: result = a * b; break;
                default:
                    if (b == 0) trap(instruction, RuntimeError::division_zero);
                    result = a / b;
                }
                if (!std::isfinite(result)) trap(instruction, RuntimeError::real_overflow);
                stack.emplace_back(result);
            }
            break;
        }
        case Op::equal: case Op::not_equal: case Op::less: case Op::less_equal: case Op::greater: case Op::greater_equal: {
            const auto right = pop(), left = pop();
            bool result = false;
            switch (instruction.op) {
            case Op::equal: result = left == right; break;
            case Op::not_equal: result = left != right; break;
            case Op::less: result = left < right; break;
            case Op::less_equal: result = left <= right; break;
            case Op::greater: result = left > right; break;
            default: result = left >= right;
            }
            stack.emplace_back(result);
            break;
        }
        case Op::jump: pc = instruction.operand; break;
        case Op::jump_false: if (!std::get<bool>(pop())) pc = instruction.operand; break;
        case Op::print: write(format(pop(), instruction), instruction); break;
        case Op::newline: write("\n", instruction); break;
        case Op::read_line: locals[instruction.operand] = read_line(input, instruction); break;
        case Op::halt: return;
        }
    }
}

} // namespace abc

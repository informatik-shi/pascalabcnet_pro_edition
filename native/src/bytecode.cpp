#include "hir.hpp"
#include "stdlib.hpp"

#include <deque>
#include <cmath>
#include <map>

namespace abc {
namespace {

class Generator {
public:
    Program generate(hir::Module module) {
        program_.locals = std::move(module.locals);
        statement(module.body);
        emit(Op::halt, Type::invalid, 0, module.body.location);
        return std::move(program_);
    }

private:
    std::size_t emit(Op op, Type type, std::size_t operand, SourceLocation location) {
        program_.code.push_back({op, type, operand, location});
        return program_.code.size() - 1;
    }
    void constant(Value value, SourceLocation location) {
        auto type = value_type(value);
        program_.constants.push_back(std::move(value));
        emit(Op::push, type, program_.constants.size() - 1, location);
    }
    void expression(const hir::Expr& node) {
        if (node.type == Type::invalid) throw Error("E_HIR", node.location, "Unresolved expression type");
        switch (node.kind) {
        case hir::ExprKind::literal: constant(node.value, node.location); break;
        case hir::ExprKind::local: emit(Op::load, node.type, node.local, node.location); break;
        case hir::ExprKind::convert:
            expression(*node.left);
            emit(Op::convert, node.type, 0, node.location);
            break;
        case hir::ExprKind::call:
            for (const auto& argument : node.arguments) expression(*argument);
            emit(Op::call_builtin, node.type, node.local, node.location);
            break;
        case hir::ExprKind::unary:
            expression(*node.left);
            if (node.operation != "+") emit(node.operation == "not" ? Op::logical_not : Op::negate,
                node.type, 0, node.location);
            break;
        case hir::ExprKind::binary: {
            expression(*node.left);
            if (node.operation == "and" || node.operation == "or") {
                const auto branch = emit(Op::jump_false, Type::invalid, 0, node.location);
                if (node.operation == "and") expression(*node.right);
                else constant(true, node.location);
                const auto finish = emit(Op::jump, Type::invalid, 0, node.location);
                program_.code[branch].operand = program_.code.size();
                if (node.operation == "and") constant(false, node.location);
                else expression(*node.right);
                program_.code[finish].operand = program_.code.size();
                break;
            }
            expression(*node.right);
            static const std::map<std::string, Op> operations{
                {"+", Op::add}, {"-", Op::subtract}, {"*", Op::multiply}, {"/", Op::divide_real},
                {"div", Op::divide_int}, {"mod", Op::modulo}, {"=", Op::equal}, {"<>", Op::not_equal},
                {"<", Op::less}, {"<=", Op::less_equal}, {">", Op::greater}, {">=", Op::greater_equal}};
            emit(operations.at(node.operation), node.left->type, 0, node.location);
            break;
        }
        }
    }
    void statement(const hir::Stmt& node) {
        switch (node.kind) {
        case hir::StmtKind::block:
            for (const auto& child : node.statements) statement(child);
            break;
        case hir::StmtKind::assign:
            expression(*node.expression);
            emit(Op::store, program_.locals.at(node.local).type, node.local, node.location);
            break;
        case hir::StmtKind::print:
            for (const auto& argument : node.arguments) {
                expression(*argument);
                emit(Op::print, argument->type, 0, argument->location);
            }
            if (node.newline) emit(Op::newline, Type::invalid, 0, node.location);
            break;
        case hir::StmtKind::read:
            emit(Op::read_line, program_.locals.at(node.local).type, node.local, node.location);
            break;
        case hir::StmtKind::conditional: {
            expression(*node.expression);
            const auto branch = emit(Op::jump_false, Type::invalid, 0, node.location);
            statement(*node.body);
            const auto finish = emit(Op::jump, Type::invalid, 0, node.location);
            program_.code[branch].operand = program_.code.size();
            if (node.alternative) statement(*node.alternative);
            program_.code[finish].operand = program_.code.size();
            break;
        }
        case hir::StmtKind::loop: {
            const auto start = program_.code.size();
            expression(*node.expression);
            const auto branch = emit(Op::jump_false, Type::invalid, 0, node.location);
            statement(*node.body);
            emit(Op::jump, Type::invalid, start, node.location);
            program_.code[branch].operand = program_.code.size();
            break;
        }
        }
    }
    Program program_;
};

[[noreturn]] void invalid(const Instruction& instruction, std::string message) {
    throw Error("E_BYTECODE", instruction.location, std::move(message));
}

bool valid_type(Type type) { return numeric(type) || type == Type::boolean || type == Type::string; }

} // namespace

Program hir::generate(Module module) { return Generator().generate(std::move(module)); }

void verify(const Program& program) {
    if (program.code.empty() || program.code.size() > 1'000'000)
        throw Error("E_BYTECODE", {}, "Invalid instruction count");
    for (const auto& local : program.locals)
        if (!valid_type(local.type)) throw Error("E_BYTECODE", {}, "Invalid local type");

    // Validate operands even in unreachable code before propagating stack types.
    for (const auto& instruction : program.code) {
        const auto type = instruction.type;
        switch (instruction.op) {
        case Op::call_builtin:
            if (instruction.operand >= stdlib::signatures().size()
                || type != stdlib::signatures()[instruction.operand].result) invalid(instruction, "Invalid standard function");
            break;
        case Op::push:
            if (instruction.operand >= program.constants.size()
                || type != value_type(program.constants[instruction.operand])) invalid(instruction, "Invalid constant");
            if (const auto* value = std::get_if<double>(&program.constants[instruction.operand]); value && !std::isfinite(*value))
                invalid(instruction, "Non-finite real constant");
            break;
        case Op::load: case Op::store: case Op::read_line:
            if (instruction.operand >= program.locals.size()
                || type != program.locals[instruction.operand].type) invalid(instruction, "Invalid local access");
            break;
        case Op::jump: case Op::jump_false:
            if (instruction.operand >= program.code.size()) invalid(instruction, "Invalid branch target");
            break;
        case Op::convert: case Op::negate:
            if (!numeric(type)) invalid(instruction, "Invalid numeric type");
            break;
        case Op::logical_not:
            if (type != Type::boolean) invalid(instruction, "Invalid logical type");
            break;
        case Op::add:
            if (!numeric(type) && type != Type::string) invalid(instruction, "Invalid addition type");
            break;
        case Op::subtract: case Op::multiply:
            if (!numeric(type)) invalid(instruction, "Invalid arithmetic type");
            break;
        case Op::divide_real:
            if (type != Type::real) invalid(instruction, "Real division requires real");
            break;
        case Op::divide_int: case Op::modulo:
            if (type != Type::integer && type != Type::int64) invalid(instruction, "Integer division requires integer");
            break;
        case Op::equal: case Op::not_equal: case Op::print:
            if (!valid_type(type)) invalid(instruction, "Invalid value type");
            break;
        case Op::less: case Op::less_equal: case Op::greater: case Op::greater_equal:
            if (!numeric(type) && type != Type::string) invalid(instruction, "Invalid ordered comparison");
            break;
        case Op::newline: case Op::halt: break;
        default: invalid(instruction, "Unknown instruction");
        }
    }

    using Stack = std::vector<Type>;
    std::vector<std::optional<Stack>> states(program.code.size());
    std::deque<std::size_t> pending{0};
    states[0] = Stack{};
    std::size_t state_items = 0;
    auto merge = [&](std::size_t target, const Stack& stack, const Instruction& instruction) {
        if (target >= states.size()) invalid(instruction, "Execution falls off module");
        if (!states[target]) {
            state_items += stack.size();
            if (state_items > 8'000'000) invalid(instruction, "Verifier state limit exceeded");
            states[target] = stack; pending.push_back(target);
        }
        else if (*states[target] != stack) invalid(instruction, "Incompatible stack types at branch merge");
    };
    while (!pending.empty()) {
        const auto pc = pending.front();
        pending.pop_front();
        const auto& instruction = program.code[pc];
        auto stack = *states[pc];
        auto pop = [&]() {
            if (stack.empty()) invalid(instruction, "Operand stack underflow");
            const auto type = stack.back(); stack.pop_back(); return type;
        };
        auto require = [&](Type expected) {
            if (pop() != expected) invalid(instruction, "Operand type mismatch");
        };
        switch (instruction.op) {
        case Op::call_builtin: {
            const auto& signature = stdlib::signatures()[instruction.operand];
            for (auto parameter = signature.parameters.rbegin(); parameter != signature.parameters.rend(); ++parameter)
                require(*parameter);
            stack.push_back(signature.result);
            break;
        }
        case Op::push: case Op::load: stack.push_back(instruction.type); break;
        case Op::store: case Op::print: require(instruction.type); break;
        case Op::convert:
            if (!can_widen(pop(), instruction.type)) invalid(instruction, "Invalid conversion");
            stack.push_back(instruction.type);
            break;
        case Op::negate: case Op::logical_not: require(instruction.type); stack.push_back(instruction.type); break;
        case Op::add: case Op::subtract: case Op::multiply:
        case Op::divide_real: case Op::divide_int: case Op::modulo:
            require(instruction.type); require(instruction.type); stack.push_back(instruction.type); break;
        case Op::equal: case Op::not_equal: case Op::less: case Op::less_equal: case Op::greater: case Op::greater_equal:
            require(instruction.type); require(instruction.type); stack.push_back(Type::boolean); break;
        case Op::jump_false: require(Type::boolean); break;
        case Op::halt:
            if (!stack.empty()) invalid(instruction, "Nonempty stack at halt");
            continue;
        case Op::read_line: case Op::newline: case Op::jump: break;
        }
        if (stack.size() > 4096) invalid(instruction, "Operand stack limit exceeded");
        if (instruction.op == Op::jump || instruction.op == Op::jump_false)
            merge(instruction.operand, stack, instruction);
        if (instruction.op != Op::jump) merge(pc + 1, stack, instruction);
    }
}

} // namespace abc

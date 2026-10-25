#include "hir.hpp"
#include "stdlib.hpp"
#include "runtime_values.hpp"
#include "formatting.hpp"

#include <deque>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace abc {
namespace {

class Generator {
public:
    Program generate(hir::Module module) {
        program_.locals = std::move(module.locals); program_.types = std::move(module.types);
        for (auto& routine : module.routines) program_.routines.push_back(std::move(routine.signature));
        code_ = &program_.code; statement(module.body); emit(Op::halt,Type::invalid,0,module.body.location);
        for (std::size_t i = 0; i < module.routines.size(); ++i) {
            routine_ = i; code_ = &program_.routines[i].code;
            statement(module.routines[i].body); emit(Op::return_user,program_.routines[i].result,0,{});
        }
        return std::move(program_);
    }

private:
    std::size_t emit(Op op, Type type, std::size_t operand, SourceLocation location) {
        code_->push_back({op, type, operand, location});
        return code_->size() - 1;
    }
    void constant(Value value, SourceLocation location) {
        auto type = value_type(value);
        program_.constants.push_back(std::move(value));
        emit(Op::push, type, program_.constants.size() - 1, location);
    }
    void local(Op op, std::size_t index, SourceLocation location) {
        if (routine_ && program_.locals[index].global) {
            if (op == Op::load) op = Op::load_global;
            if (op == Op::store) op = Op::store_global;
            if (op == Op::ref_local) op = Op::ref_global;
        }
        emit(op,program_.locals[index].type,index,location);
    }
    void reference(const hir::Expr& node) {
        if (node.kind == hir::ExprKind::local) local(Op::ref_local,node.local,node.location);
        else if (node.kind == hir::ExprKind::index) {
            expression(*node.left); expression(*node.right);
            for (const auto& argument:node.arguments) expression(*argument);
            emit(Op::ref_index,node.left->type,0,node.location);
        } else if (node.kind == hir::ExprKind::field) {
            reference(*node.left); emit(Op::ref_field,node.left->type,node.local,node.location);
        } else throw Error("E_HIR",node.location,"Expression is not assignable");
    }
    void expression(const hir::Expr& node) {
        if (node.type == Type::invalid) throw Error("E_HIR", node.location, "Unresolved expression type");
        switch (node.kind) {
        case hir::ExprKind::object_text:
            expression(*node.left); if (node.right) expression(*node.right);
            emit(Op::object_text,node.left->type,node.local,node.location); break;
        case hir::ExprKind::write_matrix:
            expression(*node.left); for (const auto& argument:node.arguments) expression(*argument);
            emit(Op::write_matrix,node.left->type,node.local,node.location); break;
        case hir::ExprKind::array_query:
            expression(*node.left); if (node.right) expression(*node.right);
            emit(Op::array_query,node.left->type,node.local,node.location); break;
        case hir::ExprKind::read_array:
            expression(*node.left); emit(Op::read_array,node.left->type,0,node.location); break;
        case hir::ExprKind::write_array:
            expression(*node.left); expression(*node.right); emit(Op::write_array,node.left->type,node.local,node.location); break;
        case hir::ExprKind::swap_values:
            reference(*node.arguments[0]); reference(*node.arguments[1]);
            emit(Op::swap_values,node.arguments[0]->type,0,node.location); break;
        case hir::ExprKind::sequence_mutate:
            if (node.left->type==Type::string) reference(*node.left); else expression(*node.left);
            for (const auto& argument:node.arguments) expression(*argument);
            emit(Op::sequence_mutate,node.left->type,node.local,node.location); break;
        case hir::ExprKind::literal: constant(node.value, node.location); break;
        case hir::ExprKind::local: local(Op::load,node.local,node.location); break;
        case hir::ExprKind::file_output:
            expression(*node.left);
            for (const auto& argument:node.arguments) expression(*argument);
            // Low two bits select Write/Writeln/Print/Println; remaining bits hold arity.
            emit(Op::file_output,Type::text_file,node.arguments.size()*4+node.local,node.location); break;
        case hir::ExprKind::linked_call:
            if (node.left) expression(*node.left);
            for (const auto& argument : node.arguments) expression(*argument);
            emit(Op::linked_call,node.left ? node.left->type : node.type,node.local,node.location); break;
        case hir::ExprKind::association_call:
            expression(*node.left);
            for (const auto& argument : node.arguments) expression(*argument);
            emit(Op::association_call,node.left->type,node.local,node.location); break;
        case hir::ExprKind::buffer_call:
            if (node.left) expression(*node.left);
            for (const auto& argument : node.arguments) expression(*argument);
            emit(Op::buffer_call,node.left ? node.left->type : node.type,node.local,node.location); break;
        case hir::ExprKind::endpoint_call:
            expression(*node.left);
            for (const auto& argument : node.arguments) expression(*argument);
            emit(Op::endpoint_call,node.left->type,node.local,node.location); break;
        case hir::ExprKind::read_lines:
            expression(*node.arguments[0]); emit(Op::read_lines,node.type,0,node.location); break;
        case hir::ExprKind::write_lines:
            expression(*node.arguments[0]); expression(*node.arguments[1]); emit(Op::write_lines,node.arguments[1]->type,0,node.location); break;
        case hir::ExprKind::array_copy: case hir::ExprKind::array_high: case hir::ExprKind::sequence_sort: case hir::ExprKind::sequence_fill:
            expression(*node.left);
            if (node.right) expression(*node.right);
            emit(node.kind==hir::ExprKind::array_copy ? Op::array_copy : node.kind==hir::ExprKind::array_high ? Op::array_high
                : node.kind==hir::ExprKind::sequence_sort ? Op::sequence_sort : Op::sequence_fill,node.left->type,node.local,node.location); break;
        case hir::ExprKind::null_literal: throw Error("E_HIR",node.location,"Unresolved nil type");
        case hir::ExprKind::random_array: case hir::ExprKind::fill_random_array:
            if (node.left) expression(*node.left);
            for (const auto& argument:node.arguments) expression(*argument);
            emit(node.kind==hir::ExprKind::random_array ? Op::random_array : Op::fill_random_array,
                node.left ? node.left->type : node.type,node.local,node.location); break;
        case hir::ExprKind::default_value: emit(Op::make_default,node.type,node.operation=="new" ? 1 : 0,node.location); break;
        case hir::ExprKind::index:
            expression(*node.left); expression(*node.right);
            for (const auto& argument:node.arguments) expression(*argument);
            emit(Op::get_index,node.left->type,0,node.location); break;
        case hir::ExprKind::field:
            expression(*node.left); emit(Op::get_field,node.left->type,node.local,node.location); break;
        case hir::ExprKind::length:
            expression(*node.left); emit(Op::length,node.left->type,node.local,node.location); break;
        case hir::ExprKind::array_new:
            for (const auto& argument:node.arguments) expression(*argument);
            emit(Op::array_new,node.type,0,node.location); break;
        case hir::ExprKind::array_literal:
            for (const auto& argument : node.arguments) expression(*argument);
            emit(Op::array_literal,node.type,node.arguments.size(),node.location); break;
        case hir::ExprKind::user_call: {
            const auto& signature = program_.routines[node.local];
            for (std::size_t n = 0; n < node.arguments.size(); ++n) {
                if (signature.parameters[n].by_reference) reference(*node.arguments[n]); else expression(*node.arguments[n]);
            }
            emit(Op::call_user,node.type,node.local,node.location); break;
        }
        case hir::ExprKind::list_add: case hir::ExprKind::list_remove: case hir::ExprKind::list_clear: case hir::ExprKind::list_contains:
            expression(*node.left);
            for (const auto& argument : node.arguments) expression(*argument);
            emit(node.kind == hir::ExprKind::list_add ? Op::list_add : node.kind == hir::ExprKind::list_remove ? Op::list_remove
                : node.kind == hir::ExprKind::list_clear ? Op::list_clear : Op::list_contains,node.left->type,0,node.location); break;
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
            if (node.operation != "+") emit(node.operation == "not" ? node.type==Type::boolean ? Op::logical_not : Op::bit_not : Op::negate,
                node.type, 0, node.location);
            break;
        case hir::ExprKind::binary: {
            expression(*node.left);
            if ((node.operation == "and" || node.operation == "or") && node.type==Type::boolean) {
                const auto branch = emit(Op::jump_false, Type::invalid, 0, node.location);
                if (node.operation == "and") expression(*node.right);
                else constant(true, node.location);
                const auto finish = emit(Op::jump, Type::invalid, 0, node.location);
                (*code_)[branch].operand = code_->size();
                if (node.operation == "and") constant(false, node.location);
                else expression(*node.right);
                (*code_)[finish].operand = code_->size();
                break;
            }
            expression(*node.right);
            static const std::map<std::string, Op> operations{
                {"+", Op::add}, {"-", Op::subtract}, {"*", Op::multiply}, {"/", Op::divide_real},
                {"div", Op::divide_int}, {"mod", Op::modulo}, {"=", Op::equal}, {"<>", Op::not_equal},
                {"<", Op::less}, {"<=", Op::less_equal}, {">", Op::greater}, {">=", Op::greater_equal},
                {"and",Op::bit_and}, {"or",Op::bit_or}, {"xor",Op::bit_xor}, {"shl",Op::shift_left}, {"shr",Op::shift_right}};
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
        case hir::StmtKind::break_stmt: case hir::StmtKind::continue_stmt: {
            if (loops_.empty()) throw Error("E_HIR",node.location,"Loop jump has no enclosing loop");
            auto& jumps=node.kind==hir::StmtKind::break_stmt ? loops_.back().breaks : loops_.back().continues;
            jumps.push_back(emit(Op::jump,Type::invalid,0,node.location)); break;
        }
        case hir::StmtKind::assign:
            if (node.target && node.target->kind==hir::ExprKind::index && node.target->left->type==Type::string) {
                reference(*node.target->left); expression(*node.target->right); expression(*node.expression);
                emit(Op::set_character,Type::string,0,node.location);
            }
            else if (node.target) { reference(*node.target); expression(*node.expression); emit(Op::store_ref,node.target->type,0,node.location); }
            else { expression(*node.expression); local(Op::store,node.local,node.location); }
            break;
        case hir::StmtKind::string_mutation: {
            const auto& signature=stdlib::signatures()[node.local];
            const std::size_t ref=signature.function==stdlib::Function::insert_string ? 1 : 0;
            for (std::size_t i=0;i<node.arguments.size();++i) {
                if (i==ref) reference(*node.arguments[i]); else expression(*node.arguments[i]);
            }
            emit(Op::mutate_string,Type::string,node.local,node.location); break;
        }
        case hir::StmtKind::resize_array:
            reference(*node.target);
            if (type_info(program_,node.target->type).rank==2) expression(*node.target);
            expression(*node.expression);
            for (const auto& argument:node.arguments) expression(*argument);
            emit(Op::resize_array,node.target->type,0,node.location); break;
        case hir::StmtKind::call:
            expression(*node.expression);
            if (node.expression->type != Type::unit) emit(Op::drop,node.expression->type,0,node.location);
            break;
        case hir::StmtKind::return_stmt:
            if (routine_) {
                const auto& routine = program_.routines[*routine_];
                if (node.expression) { expression(*node.expression); local(Op::store,routine.result_local,node.location); }
                emit(Op::return_user,routine.result,0,node.location);
            } else emit(Op::halt,Type::invalid,0,node.location);
            break;
        case hir::StmtKind::print:
            if (node.target) expression(*node.target);
            for (const auto& argument : node.arguments) expression(*argument);
            if (node.target) emit(Op::file_output,Type::text_file,node.arguments.size()*4+static_cast<std::size_t>(node.newline ? TextOutput::writeln : TextOutput::write),node.location);
            else emit(Op::console_output,Type::unit,node.arguments.size()*2+(node.newline ? 1U : 0U),node.location);
            break;
        case hir::StmtKind::read:
            if (node.target && node.target->type==Type::text_file) {
                expression(*node.target);
                for (const auto& target:node.arguments) {
                    reference(*target); emit(Op::file_read,target->type,0,target->location);
                }
                if (node.newline) emit(Op::file_skip_line,Type::text_file,0,node.location);
                emit(Op::drop,Type::text_file,0,node.location); break;
            }
            // The original converter lowers Read(a,b) into separate calls.
            for (const auto& target:node.arguments) { reference(*target); emit(Op::read_console,Type::unit,2,target->location); }
            if (node.newline || node.arguments.empty()) emit(Op::read_console,Type::unit,node.newline ? 1U : 0U,node.location);
            break;
        case hir::StmtKind::increment:
            reference(*node.target); if (node.expression) expression(*node.expression);
            emit(Op::increment,node.target->type,node.newline ? 1 : 0,node.location); break;
        case hir::StmtKind::conditional: {
            expression(*node.expression);
            const auto branch = emit(Op::jump_false, Type::invalid, 0, node.location);
            statement(*node.body);
            const auto finish = emit(Op::jump, Type::invalid, 0, node.location);
            (*code_)[branch].operand = code_->size();
            if (node.alternative) statement(*node.alternative);
            (*code_)[finish].operand = code_->size();
            break;
        }
        case hir::StmtKind::loop: {
            const auto start = code_->size();
            expression(*node.expression);
            const auto branch = emit(Op::jump_false, Type::invalid, 0, node.location);
            loops_.emplace_back();
            statement(*node.body);
            emit(Op::jump, Type::invalid, start, node.location);
            (*code_)[branch].operand = code_->size();
            finish_loop(start);
            break;
        }
        case hir::StmtKind::repeat_loop: {
            const auto start=code_->size(); loops_.emplace_back();
            for (const auto& child:node.statements) statement(child);
            const auto condition=code_->size();
            expression(*node.expression); emit(Op::jump_false,Type::invalid,start,node.location);
            finish_loop(condition);
            break;
        }
        case hir::StmtKind::foreach_loop: {
            const auto sequence=node.expression->type;
            const auto element=sequence==Type::string ? Type::character : type_info(program_,sequence).element;
            expression(*node.expression); local(Op::store,node.sequence_local,node.location);
            local(Op::load,node.sequence_local,node.location); emit(Op::iteration_version,sequence,0,node.location);
            local(Op::store,node.version_local,node.location);
            constant(std::int32_t{0},node.location); local(Op::store,node.index_local,node.location);
            const auto start=code_->size();
            local(Op::load,node.sequence_local,node.location); local(Op::load,node.version_local,node.location);
            emit(Op::iteration_check,sequence,0,node.location);
            local(Op::load,node.index_local,node.location); local(Op::load,node.sequence_local,node.location);
            emit(Op::length,sequence,0,node.location); emit(Op::less,Type::integer,0,node.location);
            const auto done=emit(Op::jump_false,Type::invalid,0,node.location);
            local(Op::load,node.sequence_local,node.location); local(Op::load,node.index_local,node.location);
            emit(Op::get_index,sequence,sequence==Type::string || type_info(program_,sequence).rank==2 ? 1 : 0,node.location);
            if (element!=node.target->type) emit(Op::convert,node.target->type,0,node.location);
            local(Op::store,node.target->local,node.location);
            loops_.emplace_back(); statement(*node.body);
            const auto advance=code_->size();
            local(Op::load,node.index_local,node.location); constant(std::int32_t{1},node.location);
            emit(Op::add,Type::integer,0,node.location); local(Op::store,node.index_local,node.location);
            emit(Op::jump,Type::invalid,start,node.location);
            (*code_)[done].operand=code_->size(); finish_loop(advance); break;
        }
        case hir::StmtKind::for_loop: {
            for (const auto& child:node.statements) statement(child);
            const auto type=node.target->type;
            const auto control=node.target->local;
            const auto compare=[&](Op op) {
                local(Op::load,control,node.location); local(Op::load,node.local,node.location);
                emit(op,type,0,node.location);
            };
            compare(node.newline ? Op::greater_equal : Op::less_equal);
            const auto empty=emit(Op::jump_false,Type::invalid,0,node.location);
            const auto start=code_->size(); loops_.emplace_back();
            statement(*node.body);
            const auto advance=code_->size();
            // Stop at the bound before incrementing, including MaxInt/MinInt64.
            compare(node.newline ? Op::greater : Op::less);
            const auto done=emit(Op::jump_false,Type::invalid,0,node.location);
            if (type==Type::character || type==Type::boolean || small_integer(type) || wide_unsigned(type)) {
                reference(*node.target);
                if (type!=Type::boolean) constant(increment_step_type(type)==Type::int64 ? Value{std::int64_t{1}} : Value{std::int32_t{1}},node.location);
                emit(Op::increment,type,node.newline ? 1 : 0,node.location);
            } else {
                local(Op::load,control,node.location);
                constant(type==Type::integer ? Value{std::int32_t{1}} : Value{std::int64_t{1}},node.location);
                emit(node.newline ? Op::subtract : Op::add,type,0,node.location); local(Op::store,control,node.location);
            }
            emit(Op::jump,Type::invalid,start,node.location);
            (*code_)[empty].operand=(*code_)[done].operand=code_->size();
            finish_loop(advance);
            break;
        }
        }
    }
    void finish_loop(std::size_t continuation) {
        for (auto jump:loops_.back().breaks) (*code_)[jump].operand=code_->size();
        for (auto jump:loops_.back().continues) (*code_)[jump].operand=continuation;
        loops_.pop_back();
    }
    struct LoopJumps { std::vector<std::size_t> breaks,continues; };
    std::vector<LoopJumps> loops_;
    Program program_;
    std::vector<Instruction>* code_ = nullptr;
    std::optional<std::size_t> routine_;
};

[[noreturn]] void invalid(const Instruction& instruction, std::string message) {
    throw Error("E_BYTECODE", instruction.location, std::move(message));
}

bool valid_type(Type type) { return scalar(type); }

} // namespace

Program hir::generate(Module module) { return Generator().generate(std::move(module)); }

static void verify_body(const Program& program, const std::vector<Instruction>& code, const Routine* routine) {
    if (code.empty() || code.size() > 1'000'000)
        throw Error("E_BYTECODE", {}, "Invalid instruction count");
    for (const auto& local : program.locals)
        if (!valid_value_type(program,local.type)) throw Error("E_BYTECODE", {}, "Invalid local type");

    auto allowed_local = [&](std::size_t index) {
        if (!routine) return program.locals[index].global;
        return std::find(routine->locals.begin(),routine->locals.end(),index) != routine->locals.end();
    };
    // Validate operands even in unreachable code before propagating stack types.
    for (const auto& instruction : code) {
        const auto type = instruction.type;
        switch (instruction.op) {
        case Op::object_text:
            if (!object_text_supported(program.types,type) || type==Type::nil || instruction.operand>1) invalid(instruction,"Invalid object formatting");
            break;
        case Op::write_matrix:
            if (!compound(type) || type_info(program,type).kind!=CompoundKind::array || type_info(program,type).rank!=2
                || !object_text_supported(program.types,type_info(program,type).element) || instruction.operand>1
                || (instruction.operand==1)!=(type_info(program,type).element==Type::real)) invalid(instruction,"Invalid matrix output");
            break;
        case Op::array_query:
            if (!compound(type) || type_info(program,type).kind!=CompoundKind::array || instruction.operand>static_cast<std::size_t>(ArrayQuery::length_or_zero))
                invalid(instruction,"Invalid array query");
            break;
        case Op::console_output:
            if (type!=Type::unit || instruction.operand/2>4096) invalid(instruction,"Invalid console output"); break;
        case Op::read_console:
            if (type!=Type::unit || instruction.operand/2>16) invalid(instruction,"Invalid console input"); break;
        case Op::read_array:
            if (!compound(type) || type_info(program,type).kind!=CompoundKind::array || type_info(program,type).rank!=1 || instruction.operand
                || (type_info(program,type).element!=Type::integer && type_info(program,type).element!=Type::real && type_info(program,type).element!=Type::string))
                invalid(instruction,"Invalid array input");
            break;
        case Op::write_array:
            if (!compound(type) || type_info(program,type).kind!=CompoundKind::array || (type_info(program,type).rank!=1 && !(instruction.operand&2U)) || !object_text_supported(program.types,type_info(program,type).element) || instruction.operand>3)
                invalid(instruction,"Invalid typed array output");
            break;
        case Op::swap_values:
            if (!valid_value_type(program,type) || instruction.operand) invalid(instruction,"Invalid typed swap"); break;
        case Op::sequence_mutate:
            if (instruction.operand>static_cast<std::size_t>(SequenceMutation::swap_indices)
                || (type==Type::string ? instruction.operand>static_cast<std::size_t>(SequenceMutation::reverse_range)
                    : !compound(type) || !indexed_sequence(type_info(program,type).kind) || type_info(program,type).rank!=1)
                || (instruction.operand==static_cast<std::size_t>(SequenceMutation::swap_indices) && type_info(program,type).kind!=CompoundKind::array))
                invalid(instruction,"Invalid sequence mutation");
            break;
        case Op::random_array: case Op::fill_random_array:
            if (!compound(type) || type_info(program,type).kind!=CompoundKind::array
                || instruction.operand>static_cast<std::size_t>(RandomArrayMethod::ranged_real)
                || type_info(program,type).element!=random_array_element(static_cast<RandomArrayMethod>(instruction.operand))
                || (instruction.op==Op::fill_random_array && instruction.operand==static_cast<std::size_t>(RandomArrayMethod::rounded_real)))
                invalid(instruction,"Invalid random array instruction");
            break;
        case Op::set_character:
            if (type!=Type::string || instruction.operand) invalid(instruction,"Invalid string character assignment"); break;
        case Op::iteration_version: case Op::iteration_check:
            if ((type!=Type::string && (!compound(type) || !indexed_sequence(type_info(program,type).kind))) || instruction.operand)
                invalid(instruction,"Invalid foreach sequence instruction");
            break;
        case Op::linked_call:
            if (!compound(type) || instruction.operand > static_cast<std::size_t>(LinkedMethod::set_next)
                || !linked_method_allowed(type_info(program,type).kind,static_cast<LinkedMethod>(instruction.operand))) invalid(instruction,"Invalid linked instruction");
            if (linked_text_method(static_cast<LinkedMethod>(instruction.operand)) && !scalar(type_info(program,type).element)) invalid(instruction,"Linked text output requires scalar elements");
            break;
        case Op::association_call:
            if (!compound(type) || type_info(program,type).kind != CompoundKind::association
                || instruction.operand > static_cast<std::size_t>(AssociationMethod::println)) invalid(instruction,"Invalid AssocArray instruction");
            if (instruction.operand >= static_cast<std::size_t>(AssociationMethod::to_string)
                && (!scalar(type_info(program,type).key) || !scalar(type_info(program,type).element))) invalid(instruction,"AssocArray text output requires scalar types");
            break;
        case Op::buffer_call:
            if (!compound(type) || instruction.operand > static_cast<std::size_t>(BufferMethod::println)
                || !buffer_method_allowed(type_info(program,type).kind,static_cast<BufferMethod>(instruction.operand)))
                invalid(instruction,"Invalid buffer collection instruction");
            if (instruction.operand >= static_cast<std::size_t>(BufferMethod::to_string) && !scalar(type_info(program,type).element))
                invalid(instruction,"Collection text output requires scalar elements");
            break;
        case Op::endpoint_call:
            if (!compound(type) || !endpoint_collection(type_info(program,type).kind)
                || instruction.operand > static_cast<std::size_t>(EndpointMethod::println)) invalid(instruction,"Invalid stack or queue instruction");
            if (instruction.operand >= static_cast<std::size_t>(EndpointMethod::to_string) && !scalar(type_info(program,type).element))
                invalid(instruction,"Collection text output requires scalar elements");
            break;
        case Op::read_value:
            if (!valid_type(type) || instruction.operand) invalid(instruction,"Invalid read type"); break;
        case Op::file_print: case Op::file_read:
            if (!scalar(type) || instruction.operand) invalid(instruction,"Invalid typed file IO"); break;
        case Op::file_newline: case Op::file_skip_line:
            if (type!=Type::text_file || instruction.operand) invalid(instruction,"Invalid text file instruction"); break;
        case Op::file_output:
            if (type!=Type::text_file || instruction.operand/4>4096) invalid(instruction,"Invalid Text output method"); break;
        case Op::increment:
            if (!ordinal(type) || instruction.operand>1) invalid(instruction,"Invalid increment"); break;
        case Op::mutate_string:
            if (type!=Type::string || instruction.operand>=stdlib::signatures().size()) invalid(instruction,"Invalid string mutation");
            {
                const auto function=stdlib::signatures()[instruction.operand].function;
                if (function!=stdlib::Function::resize_string && function!=stdlib::Function::insert_string && function!=stdlib::Function::delete_string) invalid(instruction,"Invalid mutation intrinsic");
            }
            break;
        case Op::read_lines: case Op::write_lines:
            if (!compound(type) || type_info(program,type).kind!=CompoundKind::array || type_info(program,type).rank!=1 || type_info(program,type).element!=Type::string) invalid(instruction,"File lines require string array"); break;
        case Op::array_copy:
            if (!compound(type) || type_info(program,type).kind!=CompoundKind::array) invalid(instruction,"Copy requires array"); break;
        case Op::array_high: case Op::sequence_sort: case Op::sequence_fill:
            if (!compound(type) || !indexed_sequence(type_info(program,type).kind)) invalid(instruction,"Indexed sequence required");
            if (instruction.op==Op::sequence_sort && (!scalar(type_info(program,type).element) || type_info(program,type).rank!=1 || instruction.operand>1)) invalid(instruction,"Invalid sort"); break;
        case Op::make_default: case Op::store_ref: case Op::drop:
            if (!valid_value_type(program,type) || (instruction.op==Op::make_default && instruction.operand>1)) invalid(instruction,"Invalid value type");
            if (instruction.op==Op::make_default && instruction.operand && compound(type) && node_kind(type_info(program,type).kind))
                invalid(instruction,"Linked nodes require the typed constructor");
            break;
        case Op::call_user:
            if (instruction.operand >= program.routines.size() || type != program.routines[instruction.operand].result) invalid(instruction,"Invalid user call"); break;
        case Op::return_user:
            if (!routine || type != routine->result) invalid(instruction,"Invalid return"); break;
        case Op::array_new: case Op::array_literal: case Op::resize_array:
            if (!compound(type) || type_info(program,type).kind != CompoundKind::array
                || (instruction.op==Op::array_literal ? instruction.operand>4096 || type_info(program,type).rank!=1 : instruction.operand!=0)) invalid(instruction,"Invalid array instruction");
            break;
        case Op::get_index: case Op::ref_index: case Op::length:
            if (type==Type::string) {
                if (instruction.op==Op::ref_index || instruction.operand>(instruction.op==Op::get_index ? 1U : 0U)) invalid(instruction,"Invalid string sequence instruction");
                break;
            }
            if (!compound(type) || !indexed_sequence(type_info(program,type).kind)) invalid(instruction,"Invalid sequence instruction");
            if (instruction.operand && !(instruction.operand==1 && (instruction.op==Op::length || (instruction.op==Op::get_index && type_info(program,type).rank==2))))
                invalid(instruction,"Invalid sequence access mode");
            break;
        case Op::get_field: case Op::ref_field:
            if (!compound(type) || type_info(program,type).kind != CompoundKind::record || instruction.operand >= type_info(program,type).fields.size()) invalid(instruction,"Invalid record access"); break;
        case Op::list_add: case Op::list_remove: case Op::list_clear: case Op::list_contains:
            if (!compound(type) || type_info(program,type).kind != CompoundKind::list) invalid(instruction,"Invalid list instruction"); break;
        case Op::call_builtin:
            if (instruction.operand >= stdlib::signatures().size()
                || type != stdlib::signatures()[instruction.operand].result) invalid(instruction, "Invalid standard function");
            break;
        case Op::push:
            if (!valid_type(type) || instruction.operand >= program.constants.size()
                || type != value_type(program.constants[instruction.operand])) invalid(instruction, "Invalid constant");
            if (const auto* value = std::get_if<double>(&program.constants[instruction.operand]); value && !std::isfinite(*value))
                invalid(instruction, "Non-finite real constant");
            break;
        case Op::load: case Op::store: case Op::read_line: case Op::ref_local: case Op::load_global: case Op::store_global: case Op::ref_global:
            if (instruction.operand >= program.locals.size()
                || type != program.locals[instruction.operand].type) invalid(instruction, "Invalid local access");
            if (instruction.op == Op::load_global || instruction.op == Op::store_global || instruction.op == Op::ref_global) {
                if (!program.locals[instruction.operand].global) invalid(instruction,"Invalid global access");
            } else if (!allowed_local(instruction.operand) && !(instruction.op == Op::read_line && program.locals[instruction.operand].global)) invalid(instruction,"Invalid routine local access");
            if (instruction.op == Op::read_line && !valid_type(type)) invalid(instruction,"Readln requires a scalar");
            break;
        case Op::jump: case Op::jump_false:
            if (instruction.operand >= code.size()) invalid(instruction, "Invalid branch target");
            break;
        case Op::convert:
            if (!numeric(type) && type!=Type::string) invalid(instruction,"Invalid conversion target");
            break;
        case Op::negate:
            if (!numeric(type) || small_integer(type) || type==Type::longword) invalid(instruction, "Invalid numeric type");
            break;
        case Op::logical_not:
            if (type != Type::boolean) invalid(instruction, "Invalid logical type");
            break;
        case Op::bit_not: case Op::bit_and: case Op::bit_or: case Op::bit_xor:
            if ((!integer_type(type) && !(instruction.op==Op::bit_xor && type==Type::boolean)) || instruction.operand)
                invalid(instruction,"Invalid bitwise type");
            break;
        case Op::shift_left: case Op::shift_right:
            if ((type!=Type::integer && type!=Type::int64 && type!=Type::longword && type!=Type::uint64) || instruction.operand)
                invalid(instruction,"Invalid shift type");
            break;
        case Op::add:
            if ((!numeric(type) && type != Type::string) || small_integer(type) || type==Type::longword) invalid(instruction, "Invalid addition type");
            break;
        case Op::subtract: case Op::multiply:
            if (!numeric(type) || small_integer(type) || type==Type::longword) invalid(instruction, "Invalid arithmetic type");
            break;
        case Op::divide_real:
            if (type != Type::real) invalid(instruction, "Real division requires real");
            break;
        case Op::divide_int: case Op::modulo:
            if (type != Type::integer && type != Type::int64 && type!=Type::uint64) invalid(instruction, "Integer division requires integer");
            break;
        case Op::equal: case Op::not_equal:
            if (!valid_value_type(program,type) || type==Type::text_file) invalid(instruction,"Invalid equality type"); break;
        case Op::print:
            if (!valid_type(type)) invalid(instruction, "Invalid value type"); break;
        case Op::less: case Op::less_equal: case Op::greater: case Op::greater_equal:
            if (!numeric(type) && type != Type::string && type!=Type::character && type!=Type::boolean) invalid(instruction, "Invalid ordered comparison");
            break;
        case Op::newline: break;
        case Op::halt: if (routine) invalid(instruction,"Routine cannot halt module"); break;
        default: invalid(instruction, "Unknown instruction");
        }
    }

    using Stack = std::vector<Type>;
    std::vector<std::optional<Stack>> states(code.size());
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
        const auto& instruction = code[pc];
        auto stack = *states[pc];
        auto pop = [&]() {
            if (stack.empty()) invalid(instruction, "Operand stack underflow");
            const auto type = stack.back(); stack.pop_back(); return type;
        };
        auto require = [&](Type expected) {
            if (pop() != expected) invalid(instruction, "Operand type mismatch");
        };
        switch (instruction.op) {
        case Op::iteration_version: require(instruction.type); stack.push_back(Type::int64); break;
        case Op::iteration_check: require(Type::int64); require(instruction.type); break;
        case Op::linked_call: {
            const auto method = static_cast<LinkedMethod>(instruction.operand);
            const auto& info = type_info(program,instruction.type);
            const auto parameters = linked_parameters(instruction.type,info,method);
            for (auto parameter=parameters.rbegin();parameter!=parameters.rend();++parameter) require(*parameter);
            if (method != LinkedMethod::construct) require(instruction.type);
            const auto result = linked_result(instruction.type,info,method);
            if (result != Type::unit) stack.push_back(result);
            break;
        }
        case Op::association_call: {
            const auto method = static_cast<AssociationMethod>(instruction.operand);
            const auto& info = type_info(program,instruction.type);
            const auto parameters = association_parameters(info,method);
            for (auto parameter=parameters.rbegin();parameter!=parameters.rend();++parameter) require(*parameter);
            require(instruction.type);
            const auto result = association_result(info,method);
            if (result != Type::unit) stack.push_back(result);
            break;
        }
        case Op::buffer_call: {
            const auto method = static_cast<BufferMethod>(instruction.operand);
            const auto& info = type_info(program,instruction.type);
            const auto parameters = buffer_parameters(info.kind,info.element,method);
            for (auto parameter = parameters.rbegin(); parameter != parameters.rend(); ++parameter) require(*parameter);
            if (method != BufferMethod::construct) require(instruction.type);
            const auto result = buffer_result(instruction.type,info.element,method);
            if (result != Type::unit) stack.push_back(result);
            break;
        }
        case Op::endpoint_call: {
            const auto method = static_cast<EndpointMethod>(instruction.operand);
            const auto element = type_info(program,instruction.type).element;
            if (method == EndpointMethod::add) require(element);
            require(instruction.type);
            const auto result = endpoint_result(element,method);
            if (result != Type::unit) stack.push_back(result);
            break;
        }
        case Op::console_output:
            for (std::size_t i=0;i<instruction.operand/2;++i) if (!object_text_supported(program.types,pop())) invalid(instruction,"Unsupported console output type");
            break;
        case Op::read_console:
            for (std::size_t i=0;i<instruction.operand/2;++i) {
                const auto type=static_cast<unsigned>(pop());
                if (!(type&0x40000000U) || !scalar(static_cast<Type>(type&~0x40000000U))) invalid(instruction,"Console input requires scalar references");
            }
            break;
        case Op::read_array: require(instruction.type); break;
        case Op::read_value: stack.push_back(instruction.type); break;
        case Op::file_print: require(instruction.type); require(Type::text_file); stack.push_back(Type::text_file); break;
        case Op::file_read: require(reference_type(instruction.type)); require(Type::text_file); stack.push_back(Type::text_file); break;
        case Op::file_newline: case Op::file_skip_line: require(Type::text_file); stack.push_back(Type::text_file); break;
        case Op::file_output:
            for (std::size_t n=0;n<instruction.operand/4;++n)
                if (!object_text_supported(program.types,pop())) invalid(instruction,"Unsupported Text output type");
            require(Type::text_file); break;
        case Op::set_character: require(Type::character); require(Type::integer); require(reference_type(Type::string)); break;
        case Op::increment:
            if (instruction.type!=Type::boolean) require(increment_step_type(instruction.type));
            require(reference_type(instruction.type)); break;
        case Op::mutate_string: {
            const auto& signature=stdlib::signatures()[instruction.operand];
            const std::size_t ref=signature.function==stdlib::Function::insert_string ? 1 : 0;
            for (std::size_t i=signature.parameters.size();i>0;--i)
                require(i-1==ref ? reference_type(Type::string) : signature.parameters[i-1]);
            break;
        }
        case Op::read_lines: require(Type::string); stack.push_back(instruction.type); break;
        case Op::write_lines: require(instruction.type); require(Type::string); break;
        case Op::array_copy: require(instruction.type); stack.push_back(instruction.type); break;
        case Op::array_high: require(instruction.type); stack.push_back(Type::integer); break;
        case Op::swap_values: require(reference_type(instruction.type)); require(reference_type(instruction.type)); break;
        case Op::sequence_mutate: {
            const auto method=static_cast<SequenceMutation>(instruction.operand);
            if (mutation_indices(method)) { require(Type::integer); require(Type::integer); }
            require(instruction.type==Type::string ? reference_type(Type::string) : instruction.type);
            if (mutation_result(method)) stack.push_back(instruction.type);
            break;
        }
        case Op::object_text:
            if (instruction.operand) require(Type::integer);
            require(instruction.type); stack.push_back(Type::string); break;
        case Op::write_matrix:
            if (instruction.operand) require(Type::integer);
            require(Type::integer); require(instruction.type); break;
        case Op::write_array: require(Type::string); require(instruction.type); break;
        case Op::sequence_sort: require(instruction.type); break;
        case Op::sequence_fill: require(type_info(program,instruction.type).element); require(instruction.type); break;
        case Op::make_default: stack.push_back(instruction.type); break;
        case Op::load_global: stack.push_back(instruction.type); break;
        case Op::ref_local: case Op::ref_global: stack.push_back(reference_type(instruction.type)); break;
        case Op::store_global: require(instruction.type); break;
        case Op::store_ref: require(instruction.type); require(reference_type(instruction.type)); break;
        case Op::drop: require(instruction.type); break;
        case Op::random_array: case Op::fill_random_array: {
            const auto parameters=random_array_parameters(static_cast<RandomArrayMethod>(instruction.operand));
            for (auto type=parameters.rbegin();type!=parameters.rend();++type) require(*type);
            if (instruction.op==Op::random_array) {
                for (unsigned i=0;i<type_info(program,instruction.type).rank;++i) require(Type::integer);
                stack.push_back(instruction.type);
            }
            else require(instruction.type);
            break;
        }
        case Op::array_query:
            if (instruction.operand!=static_cast<std::size_t>(ArrayQuery::rank)) require(Type::integer);
            require(instruction.type); stack.push_back(Type::integer); break;
        case Op::array_new:
            for (unsigned i=0;i<type_info(program,instruction.type).rank;++i) require(Type::integer);
            stack.push_back(instruction.type); break;
        case Op::array_literal:
            for (std::size_t n = 0; n < instruction.operand; ++n) require(type_info(program,instruction.type).element);
            stack.push_back(instruction.type); break;
        case Op::resize_array:
            for (unsigned i=0;i<type_info(program,instruction.type).rank;++i) require(Type::integer);
            if (type_info(program,instruction.type).rank==2) require(instruction.type);
            require(reference_type(instruction.type)); break;
        case Op::get_index: case Op::ref_index:
            require(Type::integer);
            if (instruction.type!=Type::string && type_info(program,instruction.type).rank==2 && !instruction.operand) require(Type::integer);
            require(instruction.type);
            if (instruction.type==Type::string) { stack.push_back(Type::character); break; }
            stack.push_back(instruction.op == Op::get_index ? type_info(program,instruction.type).element : reference_type(type_info(program,instruction.type).element)); break;
        case Op::get_field: case Op::ref_field:
            require(instruction.op == Op::get_field ? instruction.type : reference_type(instruction.type));
            stack.push_back(instruction.op == Op::get_field ? type_info(program,instruction.type).fields[instruction.operand].type : reference_type(type_info(program,instruction.type).fields[instruction.operand].type)); break;
        case Op::length: require(instruction.type); stack.push_back(Type::integer); break;
        case Op::call_user:
            for (auto parameter = program.routines[instruction.operand].parameters.rbegin(); parameter != program.routines[instruction.operand].parameters.rend(); ++parameter)
                require(parameter->by_reference ? reference_type(parameter->type) : parameter->type);
            if (instruction.type != Type::unit) stack.push_back(instruction.type);
            break;
        case Op::return_user:
            if (!stack.empty()) invalid(instruction,"Nonempty stack at return");
            continue;
        case Op::list_add: case Op::list_contains:
            require(type_info(program,instruction.type).element); require(instruction.type);
            if (instruction.op == Op::list_contains) stack.push_back(Type::boolean); break;
        case Op::list_remove: require(Type::integer); require(instruction.type); break;
        case Op::list_clear: require(instruction.type); break;
        case Op::call_builtin: {
            const auto& signature = stdlib::signatures()[instruction.operand];
            for (auto parameter = signature.parameters.rbegin(); parameter != signature.parameters.rend(); ++parameter)
                require(*parameter);
            if (signature.result != Type::unit) stack.push_back(signature.result);
            break;
        }
        case Op::push: case Op::load: stack.push_back(instruction.type); break;
        case Op::store: case Op::print: require(instruction.type); break;
        case Op::convert:
            if (!can_widen(pop(), instruction.type)) invalid(instruction, "Invalid conversion");
            stack.push_back(instruction.type);
            break;
        case Op::negate: case Op::logical_not: case Op::bit_not: require(instruction.type); stack.push_back(instruction.type); break;
        case Op::shift_left: case Op::shift_right:
            require(Type::integer); require(instruction.type); stack.push_back(instruction.type); break;
        case Op::bit_and: case Op::bit_or: case Op::bit_xor:
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

void verify(const Program& program) {
    if (program.types.size() > 1024 || program.locals.size() > 4096 || program.routines.size() > 256)
        throw Error("E_BYTECODE",{},"Module metadata limit exceeded");
    std::vector<unsigned> depth(program.types.size());
    for (std::size_t i = 0; i < program.types.size(); ++i) {
        const auto& type = program.types[i]; unsigned level = 1;
        if (type.rank<1 || type.rank>2 || (type.kind!=CompoundKind::array && type.rank!=1)) throw Error("E_BYTECODE",{},"Invalid array rank");
        auto prior = [&](Type t) {
            if (!valid_value_type(program,t) || (compound(t) && static_cast<unsigned>(t)-64 >= i)) throw Error("E_BYTECODE",{},"Invalid or cyclic type dependency");
            if (compound(t)) level = std::max(level,depth[static_cast<unsigned>(t)-64]+1);
        };
        if (type.kind == CompoundKind::record) {
            if (type.fields.size() > 256) throw Error("E_BYTECODE",{},"Too many record fields");
            for (const auto& field : type.fields) prior(field.type);
        } else if (type.kind == CompoundKind::association) { prior(type.key); prior(type.element); }
        else if (type.kind == CompoundKind::linked_list) {
            prior(type.element); prior(type.key);
            if (!compound(type.key) || type_info(program,type.key).kind != CompoundKind::linked_node || type_info(program,type.key).element != type.element)
                throw Error("E_BYTECODE",{},"Invalid linked list node type");
        }
        else if (indexed_sequence(type.kind) || endpoint_collection(type.kind) || buffer_collection(type.kind) || node_kind(type.kind)) prior(type.element);
        else throw Error("E_BYTECODE",{},"Invalid compound kind");
        if (level > 32) throw Error("E_BYTECODE",{},"Type nesting limit exceeded"); depth[i] = level;
    }
    std::size_t instructions=program.code.size();
    for (const auto& routine : program.routines) {
        instructions+=routine.code.size();
        if (instructions>1'000'000) throw Error("E_BYTECODE",{},"Module instruction limit exceeded");
        if (routine.parameters.size() > 16 || (routine.result != Type::unit && !valid_value_type(program,routine.result))) throw Error("E_BYTECODE",{},"Invalid routine signature");
        std::set<std::size_t> owned;
        for (auto local : routine.locals) if (local >= program.locals.size() || program.locals[local].global || !owned.insert(local).second) throw Error("E_BYTECODE",{},"Invalid routine locals");
        if (routine.result != Type::unit && (!owned.contains(routine.result_local) || program.locals[routine.result_local].type != routine.result)) throw Error("E_BYTECODE",{},"Invalid Result local");
        std::set<std::size_t> parameters;
        for (const auto& parameter : routine.parameters)
            if (!owned.contains(parameter.local) || program.locals[parameter.local].type != parameter.type
                || !parameters.insert(parameter.local).second || (routine.result!=Type::unit && parameter.local==routine.result_local))
                throw Error("E_BYTECODE",{},"Invalid parameter");
        verify_body(program,routine.code,&routine);
    }
    verify_body(program,program.code,nullptr);
}

} // namespace abc

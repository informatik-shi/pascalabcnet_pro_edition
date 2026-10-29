#include "hir.hpp"
#include "stdlib.hpp"
#include "runtime_values.hpp"
#include "formatting.hpp"
#include "school_ip.hpp"

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
        program_.print_delimiter=module.print_delimiter;
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
        case hir::ExprKind::sequence_wrap:
            expression(*node.left); emit(Op::sequence_wrap,node.left->type,static_cast<std::size_t>(node.type),node.location); break;
        case hir::ExprKind::sequence_snapshot:
            expression(*node.left); emit(Op::sequence_snapshot,node.left->type,static_cast<std::size_t>(node.type),node.location); break;
        case hir::ExprKind::sequence_repeat:
            expression(*node.left); expression(*node.right); emit(Op::sequence_repeat,node.type,0,node.location); break;
        case hir::ExprKind::sequence_range:
            expression(*node.left); expression(*node.right);
            for (const auto& argument:node.arguments) expression(*argument);
            emit(Op::sequence_range,node.type,node.arguments.size(),node.location); break;
        case hir::ExprKind::sequence_partition:
            for (const auto& argument:node.arguments) expression(*argument);
            emit(Op::sequence_partition,node.type,node.local,node.location); break;
        case hir::ExprKind::sequence_call:
            expression(*node.left); emit(node.local==2 ? Op::sequence_list : node.local ? Op::sequence_array : Op::sequence_count,node.left->type,node.local ? static_cast<std::size_t>(node.type) : 0,node.location); break;
        case hir::ExprKind::sequence_aggregate:
            expression(*node.left); emit(Op::sequence_aggregate,node.left->type,node.local,node.location); break;
        case hir::ExprKind::sequence_partial:
            expression(*node.left); emit(Op::sequence_partial,node.left->type,0,node.location); break;
        case hir::ExprKind::sequence_predicate:
            expression(*node.left); emit(Op::sequence_predicate,node.left->type,node.local,node.location); break;
        case hir::ExprKind::sequence_transform:
            expression(*node.left); emit(node.operation=="where" ? Op::sequence_where : node.operation=="selectmany" ? Op::sequence_flatmap : Op::sequence_select,node.left->type,(static_cast<std::size_t>(node.type)<<8)|node.local,node.location); break;
        case hir::ExprKind::sequence_concat: case hir::ExprKind::sequence_equal: case hir::ExprKind::sequence_default:
            expression(*node.left); expression(*node.right);
            emit(node.kind==hir::ExprKind::sequence_equal ? Op::sequence_equal : node.kind==hir::ExprKind::sequence_default ? Op::sequence_default : Op::sequence_concat,node.left->type,0,node.location); break;
        case hir::ExprKind::sequence_slice:
            expression(*node.left); expression(*node.right); emit(Op::sequence_slice,node.left->type,node.local,node.location); break;
        case hir::ExprKind::sequence_query:
            expression(*node.left); if (node.right) expression(*node.right);
            emit(Op::sequence_query,node.left->type,node.local,node.location); break;
        case hir::ExprKind::school_ip:
            for (const auto& argument:node.arguments) expression(*argument);
            emit(Op::school_ip,node.operation_type,node.local,node.location); break;
        case hir::ExprKind::school_minmax:
            expression(*node.left); emit(Op::school_minmax,node.left->type,static_cast<std::size_t>(node.type),node.location); break;
        case hir::ExprKind::school_table_print:
            expression(*node.left); for (const auto& argument:node.arguments) expression(*argument);
            emit(Op::school_table_print,node.left->type,0,node.location); break;
        case hir::ExprKind::school_table: {
            const auto matrix=node.temporaries[0],row=node.temporaries[1],count=node.temporaries[2];
            const auto load=[&](std::size_t index) { local(Op::load,index,node.location); };
            const auto store=[&](std::size_t index) { local(Op::store,index,node.location); };
            const auto number=[&](std::int32_t n) { constant(n,node.location); };
            const auto operation=[&](Op op,Type type=Type::integer) { emit(op,type,0,node.location); };
            const auto increment=[&](std::size_t index) { load(index); number(1); operation(Op::add); store(index); };
            expression(*node.left); store(count); load(count); operation(Op::school_table_new,node.type); store(matrix);
            number(0); store(row);
            const auto start=code_->size();
            load(row); load(matrix); number(0); emit(Op::array_query,node.type,static_cast<std::size_t>(ArrayQuery::length),node.location);
            operation(Op::less); const auto finish=emit(Op::jump_false,Type::invalid,0,node.location);
            if (node.operation_type!=Type::invalid) {
                const auto values=node.temporaries[3],column=node.temporaries[4];
                load(count); operation(Op::array_new,node.operation_type); store(values); number(0); store(column);
                const auto begin_column=code_->size(); load(column); load(count); operation(Op::less);
                const auto end_column=emit(Op::jump_false,Type::invalid,0,node.location);
                load(values); load(column); operation(Op::ref_index,node.operation_type);
                load(row); load(count); load(column); operation(Op::subtract); number(1); operation(Op::subtract);
                operation(Op::shift_right); number(1); operation(Op::bit_and); number(0); operation(Op::not_equal); operation(Op::store_ref,Type::boolean);
                load(matrix); load(row); load(column); operation(Op::ref_index,node.type);
                load(values); load(column); operation(Op::get_index,node.operation_type); operation(Op::store_ref,Type::boolean);
                increment(column); emit(Op::jump,Type::invalid,begin_column,node.location); (*code_)[end_column].operand=code_->size();
                load(matrix); load(row); load(count); operation(Op::ref_index,node.type); load(values);
            } else {
                const auto arity=program_.routines[node.local].parameters.size();
                for (std::size_t column=0;column<arity;++column) {
                    load(matrix); load(row); number(static_cast<std::int32_t>(column)); operation(Op::ref_index,node.type);
                    load(row); number(static_cast<std::int32_t>(arity-column-1)); operation(Op::shift_right);
                    number(1); operation(Op::bit_and); number(0); operation(Op::not_equal); operation(Op::store_ref,Type::boolean);
                }
                load(matrix); load(row); load(count); operation(Op::ref_index,node.type);
                for (std::size_t column=0;column<arity;++column) {
                    load(matrix); load(row); number(static_cast<std::int32_t>(column)); operation(Op::get_index,node.type);
                }
            }
            emit(Op::call_user,Type::boolean,node.local,node.location); operation(Op::store_ref,Type::boolean);
            increment(row); emit(Op::jump,Type::invalid,start,node.location); (*code_)[finish].operand=code_->size(); load(matrix); break;
        }
        case hir::ExprKind::tuple_literal: case hir::ExprKind::school_pair:
            for (const auto& argument:node.arguments) expression(*argument);
            emit(node.kind==hir::ExprKind::tuple_literal ? Op::tuple_literal : Op::school_pair,node.type,0,node.location); break;
        case hir::ExprKind::school_fold:
            expression(*node.left); emit(Op::school_fold,node.left->type,node.local,node.location); break;
        case hir::ExprKind::replace_last: case hir::ExprKind::swap_substrings:
            reference(*node.arguments[0]); expression(*node.arguments[1]); expression(*node.arguments[2]);
            emit(node.kind==hir::ExprKind::replace_last ? Op::replace_last : Op::swap_substrings,Type::string,0,node.location); break;
        case hir::ExprKind::replace_many:
            for (const auto& argument:node.arguments) expression(*argument);
            emit(Op::replace_many,node.arguments[1]->type,static_cast<std::size_t>(node.arguments[2]->type),node.location); break;
        case hir::ExprKind::school_list:
            for (const auto& argument:node.arguments) expression(*argument);
            emit(Op::school_list,node.operation_type,node.local,node.location); break;
        case hir::ExprKind::string_array:
            expression(*node.left); if (node.right) expression(*node.right);
            emit(Op::string_array,node.type,node.local,node.location); break;
        case hir::ExprKind::string_read:
            expression(*node.left); reference(*node.right);
            for (const auto& argument:node.arguments) reference(*argument);
            emit(Op::string_read,node.type,node.local,node.location); break;
        case hir::ExprKind::number_conversion:
            expression(*node.left); reference(*node.right);
            for (const auto& argument:node.arguments) reference(*argument);
            emit(node.local==0 ? Op::convert_string : node.local==1 ? Op::try_parse : Op::val,node.local==0 ? node.left->type : node.right->type,0,node.location); break;
        case hir::ExprKind::try_read:
            reference(*node.left); expression(*node.right); emit(Op::try_read,node.left->type,0,node.location); break;
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
            if (node.left) expression(*node.left);
            for (const auto& argument:node.arguments) expression(*argument);
            // Low two bits select Write/Writeln/Print/Println; remaining bits hold arity.
            emit(node.left ? Op::file_output : Op::print_output,node.left ? Type::text_file : Type::unit,node.arguments.size()*4+node.local,node.location); break;
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
        case hir::ExprKind::default_value: emit(Op::make_default,node.type,node.operation=="new" ? 1 : node.operation=="default" ? 2 : 0,node.location); break;
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
            const bool lazy=compound(sequence) && type_info(program_,sequence).kind==CompoundKind::sequence;
            const auto index_type=lazy ? Type::int64 : Type::integer;
            const auto element=sequence==Type::string ? Type::character : type_info(program_,sequence).element;
            expression(*node.expression); if (lazy) emit(Op::sequence_begin,sequence,0,node.location);
            local(Op::store,node.sequence_local,node.location);
            local(Op::load,node.sequence_local,node.location); emit(Op::iteration_version,sequence,0,node.location);
            local(Op::store,node.version_local,node.location);
            constant(lazy ? Value{std::int64_t{0}} : Value{std::int32_t{0}},node.location); local(Op::store,node.index_local,node.location);
            const auto start=code_->size();
            local(Op::load,node.sequence_local,node.location);
            if (lazy) {
                local(Op::load,node.index_local,node.location); emit(Op::sequence_resume,sequence,0,node.location);
            } else {
                local(Op::load,node.version_local,node.location); emit(Op::iteration_check,sequence,0,node.location);
                local(Op::load,node.index_local,node.location); local(Op::load,node.sequence_local,node.location);
                emit(Op::length,sequence,0,node.location); emit(Op::less,index_type,0,node.location);
            }
            const auto done=emit(Op::jump_false,Type::invalid,0,node.location);
            local(Op::load,node.sequence_local,node.location); local(Op::load,node.index_local,node.location);
            emit(Op::get_index,sequence,sequence==Type::string || type_info(program_,sequence).rank==2 ? 1 : 0,node.location);
            if (element!=node.target->type) {
                if (compound(node.target->type) && type_info(program_,node.target->type).kind==CompoundKind::sequence)
                    emit(Op::sequence_wrap,element,static_cast<std::size_t>(node.target->type),node.location);
                else emit(Op::convert,node.target->type,0,node.location);
            }
            local(Op::store,node.target->local,node.location);
            loops_.emplace_back(); statement(*node.body);
            const auto advance=code_->size();
            local(Op::load,node.index_local,node.location); constant(lazy ? Value{std::int64_t{1}} : Value{std::int32_t{1}},node.location);
            emit(Op::add,index_type,0,node.location); local(Op::store,node.index_local,node.location);
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
        case Op::sequence_snapshot: case Op::sequence_wrap: {
            const bool text=type==Type::string && instruction.op==Op::sequence_wrap;
            if ((!text && (!compound(type) || !indexed_sequence(type_info(program,type).kind) || type_info(program,type).rank!=1))
                || instruction.operand<64 || instruction.operand>=64+program.types.size()) invalid(instruction,"Invalid sequence view source");
            const auto& target=type_info(program,static_cast<Type>(instruction.operand));
            if (target.kind!=CompoundKind::sequence || target.element!=(text ? Type::character : type_info(program,type).element)) invalid(instruction,"Invalid sequence view element type");
            if (instruction.op==Op::sequence_snapshot && type_info(program,type).kind!=CompoundKind::array) invalid(instruction,"Seq snapshot requires an array");
            break;
        }
        case Op::sequence_repeat:
            if (!compound(type) || type_info(program,type).kind!=CompoundKind::sequence || instruction.operand) invalid(instruction,"Invalid SeqFill metadata");
            break;
        case Op::sequence_partition:
            if (!compound(type) || type_info(program,type).kind!=CompoundKind::sequence || type_info(program,type).element!=Type::real || instruction.operand>1) invalid(instruction,"Invalid partition generator metadata");
            break;
        case Op::sequence_range:
            if (!compound(type) || type_info(program,type).kind!=CompoundKind::sequence || instruction.operand>1
                || (type_info(program,type).element!=Type::integer && type_info(program,type).element!=Type::character && type_info(program,type).element!=Type::big_integer && type_info(program,type).element!=Type::real)
                || (type_info(program,type).element==Type::real && instruction.operand!=1)) invalid(instruction,"Invalid Range metadata");
            break;
        case Op::sequence_aggregate: case Op::sequence_partial:
            if (!compound(type) || type_info(program,type).kind!=CompoundKind::sequence || !numeric_sequence_element(type_info(program,type).element)) invalid(instruction,"Invalid numeric sequence metadata");
            if (instruction.op==Op::sequence_aggregate) {
                if (instruction.operand>static_cast<std::size_t>(SequenceAggregate::product)) invalid(instruction,"Invalid numeric aggregate operation");
            } else if (instruction.operand || !partial_sum_element(type_info(program,type).element)) invalid(instruction,"Invalid PartialSum element or flags");
            break;
        case Op::sequence_predicate: {
            const auto callback=instruction.operand&255U,method=instruction.operand>>8;
            if (!compound(type) || type_info(program,type).kind!=CompoundKind::sequence || callback>=program.routines.size()
                || method>static_cast<std::size_t>(PredicateQuery::single_default)) invalid(instruction,"Invalid predicate query metadata");
            const auto& signature=program.routines[callback];
            if (signature.parameters.size()!=1 || signature.parameters[0].by_reference || signature.parameters[0].type!=type_info(program,type).element
                || signature.result!=Type::boolean) invalid(instruction,"Invalid predicate query signature");
            break;
        }
        case Op::sequence_select: case Op::sequence_where: case Op::sequence_flatmap: {
            const auto callback=instruction.operand&255U,result=instruction.operand>>8;
            if (!compound(type) || type_info(program,type).kind!=CompoundKind::sequence || callback>=program.routines.size()
                || result<64 || result>=64+program.types.size()) invalid(instruction,"Invalid sequence callback metadata");
            const auto& signature=program.routines[callback]; const auto& output=type_info(program,static_cast<Type>(result));
            const auto element=type_info(program,type).element; const bool predicate=instruction.op==Op::sequence_where,flatten=instruction.op==Op::sequence_flatmap;
            auto returned=signature.result;
            if (flatten) {
                if (returned==Type::string) returned=Type::character;
                else {
                    if (!compound(returned)) invalid(instruction,"SelectMany callback must return an enumerable type");
                    const auto& collection=type_info(program,returned);
                    if ((collection.kind!=CompoundKind::sequence && !indexed_sequence(collection.kind)) || collection.rank!=1) invalid(instruction,"Invalid SelectMany callback result");
                    returned=collection.element;
                }
            }
            if ((signature.parameters.size()!=1 && signature.parameters.size()!=2) || signature.parameters[0].by_reference || signature.parameters[0].type!=element
                || (signature.parameters.size()==2 && (signature.parameters[1].by_reference || signature.parameters[1].type!=Type::integer))
                || signature.result==Type::unit || (predicate && signature.result!=Type::boolean) || output.kind!=CompoundKind::sequence
                || output.element!=(predicate ? element : returned)) invalid(instruction,"Invalid sequence callback signature");
            break;
        }
        case Op::sequence_concat: case Op::sequence_equal: case Op::sequence_default:
            if (!compound(type) || type_info(program,type).kind!=CompoundKind::sequence || instruction.operand) invalid(instruction,"Invalid Concat metadata");
            break;
        case Op::sequence_slice:
            if (!compound(type) || type_info(program,type).kind!=CompoundKind::sequence || instruction.operand>static_cast<std::size_t>(SequenceSlice::reverse)) invalid(instruction,"Invalid sequence slicing metadata");
            break;
        case Op::sequence_query:
            if (!compound(type) || type_info(program,type).kind!=CompoundKind::sequence
                || instruction.operand>static_cast<std::size_t>(SequenceQuery::single_default)) invalid(instruction,"Invalid sequence query metadata");
            break;
        case Op::sequence_resume: case Op::sequence_begin: case Op::sequence_count: case Op::sequence_array: case Op::sequence_list:
            if (!compound(type) || type_info(program,type).kind!=CompoundKind::sequence) invalid(instruction,"Sequence instruction requires a statically typed sequence");
            if (instruction.op==Op::sequence_array || instruction.op==Op::sequence_list) {
                if (instruction.operand<64 || instruction.operand>=64+program.types.size()) invalid(instruction,"Invalid sequence array result");
                const auto& result=type_info(program,static_cast<Type>(instruction.operand));
                const auto kind=instruction.op==Op::sequence_list ? CompoundKind::list : CompoundKind::array;
                if (result.kind!=kind || result.rank!=1 || result.element!=type_info(program,type).element) invalid(instruction,"Sequence collection element type mismatch");
            } else if (instruction.operand) invalid(instruction,"Invalid sequence flags");
            break;
        case Op::school_ip: {
            const auto method=static_cast<SchoolIPMethod>(instruction.operand&255U);
            if (!compound(type) || !school_ip_allowed(type_info(program,type).kind,method)) invalid(instruction,"Invalid IPv4 method owner or operation");
            if (school_ip_generator(method)) {
                const auto result=instruction.operand>>8;
                if (result<64 || result>=64+program.types.size()) invalid(instruction,"Invalid IPv4 generator result type");
                const auto& array=type_info(program,static_cast<Type>(result));
                if (array.kind!=CompoundKind::sequence || array.rank!=1 || array.element!=(method==SchoolIPMethod::ip_generate ? type_info(program,type).fields[0].type : Type::string))
                    invalid(instruction,"Invalid IPv4 generator element type");
            } else if (instruction.operand>>8) invalid(instruction,"Invalid IPv4 method flags");
            break;
        }
        case Op::school_minmax: {
            if (!compound(type) || (!indexed_sequence(type_info(program,type).kind) && type_info(program,type).kind!=CompoundKind::sequence) || type_info(program,type).rank!=1
                || instruction.operand<64 || instruction.operand>=64+program.types.size()) invalid(instruction,"Invalid School MinMax input");
            const auto element=type_info(program,type).element; const auto& tuple=type_info(program,static_cast<Type>(instruction.operand));
            if ((element!=Type::integer && element!=Type::int64 && element!=Type::real && element!=Type::big_integer) || tuple.kind!=CompoundKind::tuple || tuple.fields.size()!=2
                || tuple.fields[0].type!=element || tuple.fields[1].type!=element) invalid(instruction,"Invalid School MinMax result");
            break;
        }
        case Op::school_table_print: case Op::school_table_new:
            if (!compound(type) || type_info(program,type).kind!=CompoundKind::array || type_info(program,type).rank!=2
                || type_info(program,type).element!=Type::boolean || instruction.operand) invalid(instruction,"Invalid School truth-table output");
            break;
        case Op::tuple_literal: case Op::school_pair: {
            if (!compound(type) || type_info(program,type).kind!=CompoundKind::tuple || instruction.operand) invalid(instruction,"Invalid tuple construction");
            const auto& fields=type_info(program,type).fields;
            if (instruction.op==Op::school_pair && (fields.size()!=2 || fields[0].type!=Type::int64 || fields[1].type!=Type::int64)) invalid(instruction,"Invalid НОДНОК result type");
            break;
        }
        case Op::school_fold:
            if (instruction.operand>static_cast<std::size_t>(SchoolFold::lcm) || !compound(type) || type_info(program,type).kind!=CompoundKind::array
                || type_info(program,type).rank!=1 || (type_info(program,type).element!=Type::int64 && type_info(program,type).element!=Type::big_integer)) invalid(instruction,"Invalid School GCD/LCM fold");
            break;
        case Op::replace_last: case Op::swap_substrings:
            if (type!=Type::string || instruction.operand) invalid(instruction,"Invalid ReplaceLast");
            break;
        case Op::replace_many: {
            const auto valid=[&](Type container) {
                return compound(container) && indexed_sequence(type_info(program,container).kind) && type_info(program,container).rank==1 && type_info(program,container).element==Type::string;
            };
            if (instruction.operand<64 || instruction.operand>=64+program.types.size() || !valid(type) || !valid(static_cast<Type>(instruction.operand))) invalid(instruction,"Invalid ReplaceMany string collections");
            break;
        }
        case Op::school_list:
            if (instruction.operand>static_cast<std::size_t>(SchoolList::divisors_count) || !compound(type)
                || type_info(program,type).kind!=CompoundKind::list || type_info(program,type).rank!=1
                || (type_info(program,type).element!=Type::integer && !(school_factor_method(static_cast<SchoolList>(instruction.operand)) && type_info(program,type).element==Type::int64)))
                invalid(instruction,"Invalid School list operation");
            break;
        case Op::string_array:
            if (instruction.operand>static_cast<std::size_t>(StringArray::integers64) || !compound(type)
                || type_info(program,type).kind!=CompoundKind::array || type_info(program,type).rank!=1
                || type_info(program,type).element!=string_array_element(static_cast<StringArray>(instruction.operand))) invalid(instruction,"Invalid string array conversion");
            break;
        case Op::string_read:
            if (instruction.operand>static_cast<std::size_t>(StringRead::member_word)
                || type!=string_read_result(static_cast<StringRead>(instruction.operand))) invalid(instruction,"Invalid string reading");
            break;
        case Op::convert_string:
            if ((!numeric(type) && type!=Type::string) || instruction.operand) invalid(instruction,"Invalid Str conversion");
            break;
        case Op::try_parse: case Op::val:
            if (!numeric(type) || instruction.operand) invalid(instruction,"Invalid number parsing");
            break;
        case Op::try_read:
            if ((!numeric(type) && type!=Type::boolean) || instruction.operand) invalid(instruction,"Invalid typed TryRead");
            break;
        case Op::print_output:
            if (type!=Type::unit || instruction.operand/4>16 || instruction.operand%4<2) invalid(instruction,"Invalid Print output");
            break;
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
            if ((type!=Type::string && (!compound(type) || (!indexed_sequence(type_info(program,type).kind) && type_info(program,type).kind!=CompoundKind::sequence))) || instruction.operand)
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
            if (!valid_value_type(program,type) || (instruction.op==Op::make_default && instruction.operand>2)) invalid(instruction,"Invalid value type");
            if (instruction.op==Op::make_default && instruction.operand==1 && compound(type) && (node_kind(type_info(program,type).kind) || type_info(program,type).kind==CompoundKind::tuple || type_info(program,type).kind==CompoundKind::sequence || school_ip_kind(type_info(program,type).kind)))
                invalid(instruction,"Nodes and tuples require the typed constructor");
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
            if (compound(type) && type_info(program,type).kind==CompoundKind::sequence) {
                if (instruction.op==Op::ref_index || instruction.operand) invalid(instruction,"Invalid or writable lazy sequence access");
                break;
            }
            if (type==Type::string) {
                if (instruction.op==Op::ref_index || instruction.operand>(instruction.op==Op::get_index ? 1U : 0U)) invalid(instruction,"Invalid string sequence instruction");
                break;
            }
            if (!compound(type) || !indexed_sequence(type_info(program,type).kind)) invalid(instruction,"Invalid sequence instruction");
            if (instruction.operand && !(instruction.operand==1 && (instruction.op==Op::length || (instruction.op==Op::get_index && type_info(program,type).rank==2))))
                invalid(instruction,"Invalid sequence access mode");
            break;
        case Op::get_field: case Op::ref_field:
            if (!compound(type) || (type_info(program,type).kind!=CompoundKind::record && !(instruction.op==Op::get_field && type_info(program,type).kind==CompoundKind::tuple))
                || instruction.operand>=type_info(program,type).fields.size()) invalid(instruction,"Invalid or writable tuple/record access");
            break;
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
            if ((!integer_type(type) && type!=Type::big_integer && !(instruction.op==Op::bit_xor && type==Type::boolean)) || instruction.operand)
                invalid(instruction,"Invalid bitwise type");
            break;
        case Op::shift_left: case Op::shift_right:
            if ((type!=Type::integer && type!=Type::int64 && type!=Type::longword && type!=Type::uint64 && type!=Type::big_integer) || instruction.operand)
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
            if (type != Type::integer && type != Type::int64 && type!=Type::uint64 && type!=Type::big_integer) invalid(instruction, "Integer division requires integer");
            break;
        case Op::equal: case Op::not_equal:
            if (!valid_value_type(program,type)) invalid(instruction,"Invalid equality type"); break;
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
    std::vector<bool> entries(code.size()); entries[0]=true;
    for (const auto& instruction:code)
        if (instruction.op==Op::jump || instruction.op==Op::jump_false) entries[instruction.operand]=true;
    std::vector<std::optional<Stack>> states(code.size());
    std::deque<std::size_t> pending{0};
    states[0] = Stack{};
    std::optional<std::size_t> linear_pc;
    Stack linear_stack;
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
    while (linear_pc || !pending.empty()) {
        std::size_t pc; Stack stack;
        if (linear_pc) { pc=*linear_pc; linear_pc.reset(); stack=std::move(linear_stack); }
        else { pc=pending.front(); pending.pop_front(); stack=*states[pc]; }
        const auto& instruction = code[pc];
        auto pop = [&]() {
            if (stack.empty()) invalid(instruction, "Operand stack underflow");
            const auto type = stack.back(); stack.pop_back(); return type;
        };
        auto require = [&](Type expected) {
            if (pop() != expected) invalid(instruction, "Operand type mismatch");
        };
        switch (instruction.op) {
        case Op::iteration_version: require(instruction.type); stack.push_back(Type::int64); break;
        case Op::sequence_begin: require(instruction.type); stack.push_back(instruction.type); break;
        case Op::sequence_snapshot: case Op::sequence_wrap: require(instruction.type); stack.push_back(static_cast<Type>(instruction.operand)); break;
        case Op::sequence_repeat: require(type_info(program,instruction.type).element); require(Type::integer); stack.push_back(instruction.type); break;
        case Op::sequence_partition: require(Type::integer); require(Type::real); require(Type::real); stack.push_back(instruction.type); break;
        case Op::sequence_range:
            if (instruction.operand) require(type_info(program,instruction.type).element==Type::character ? Type::integer : type_info(program,instruction.type).element);
            require(type_info(program,instruction.type).element); require(type_info(program,instruction.type).element); stack.push_back(instruction.type); break;
        case Op::sequence_resume: require(Type::int64); require(instruction.type); stack.push_back(Type::boolean); break;
        case Op::sequence_count: require(instruction.type); stack.push_back(Type::integer); break;
        case Op::sequence_aggregate:
            require(instruction.type); stack.push_back(sequence_aggregate_result(type_info(program,instruction.type).element,static_cast<SequenceAggregate>(instruction.operand))); break;
        case Op::sequence_partial: require(instruction.type); stack.push_back(instruction.type); break;
        case Op::sequence_concat: require(instruction.type); require(instruction.type); stack.push_back(instruction.type); break;
        case Op::sequence_predicate: require(instruction.type); stack.push_back(predicate_query_result(type_info(program,instruction.type).element,static_cast<PredicateQuery>(instruction.operand>>8))); break;
        case Op::sequence_select: case Op::sequence_where: case Op::sequence_flatmap: require(instruction.type); stack.push_back(static_cast<Type>(instruction.operand>>8)); break;
        case Op::sequence_equal: require(instruction.type); require(instruction.type); stack.push_back(Type::boolean); break;
        case Op::sequence_default: require(type_info(program,instruction.type).element); require(instruction.type); stack.push_back(instruction.type); break;
        case Op::sequence_slice: require(Type::integer); require(instruction.type); stack.push_back(instruction.type); break;
        case Op::sequence_query: {
            const auto method=static_cast<SequenceQuery>(instruction.operand); const auto element=type_info(program,instruction.type).element;
            if (sequence_query_index(method)) require(Type::integer);
            else if (method==SequenceQuery::contains) require(element);
            require(instruction.type); stack.push_back(sequence_query_result(element,method)); break;
        }
        case Op::sequence_array: case Op::sequence_list: require(instruction.type); stack.push_back(static_cast<Type>(instruction.operand)); break;
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
        case Op::school_ip: {
            const auto method=static_cast<SchoolIPMethod>(instruction.operand&255U);
            const auto parameters=school_ip_parameters(instruction.type,method);
            for (auto parameter=parameters.rbegin();parameter!=parameters.rend();++parameter) require(*parameter);
            const auto result=school_ip_result(instruction.type,type_info(program,instruction.type),method,static_cast<Type>(instruction.operand>>8));
            if (result!=Type::unit) stack.push_back(result);
            break;
        }
        case Op::school_minmax: require(instruction.type); stack.push_back(static_cast<Type>(instruction.operand)); break;
        case Op::school_table_print: require(Type::string); require(Type::integer); require(instruction.type); break;
        case Op::school_table_new: require(Type::integer); stack.push_back(instruction.type); break;
        case Op::tuple_literal: case Op::school_pair:
            for (auto field=type_info(program,instruction.type).fields.rbegin();field!=type_info(program,instruction.type).fields.rend();++field) require(field->type);
            stack.push_back(instruction.type); break;
        case Op::school_fold: require(instruction.type); stack.push_back(type_info(program,instruction.type).element); break;
        case Op::replace_last: case Op::swap_substrings: require(Type::string); require(Type::string); require(reference_type(Type::string)); break;
        case Op::replace_many: require(static_cast<Type>(instruction.operand)); require(instruction.type); require(Type::string); stack.push_back(Type::string); break;
        case Op::school_list: {
            const auto method=static_cast<SchoolList>(instruction.operand);
            const auto parameters=school_list_parameters(method,instruction.type,type_info(program,instruction.type).element);
            for (auto parameter=parameters.rbegin();parameter!=parameters.rend();++parameter) require(*parameter);
            stack.push_back(school_list_result(method,instruction.type)); break;
        }
        case Op::string_array: {
            const auto method=static_cast<StringArray>(instruction.operand);
            if (method==StringArray::words_array) {
                const auto type=pop();
                if (!compound(type) || type_info(program,type).kind!=CompoundKind::array || type_info(program,type).rank!=1 || type_info(program,type).element!=Type::character)
                    invalid(instruction,"ToWords requires a statically typed char array");
            } else if (string_array_parameter(method)) require(method==StringArray::integers_count ? Type::integer : Type::string);
            require(Type::string); stack.push_back(instruction.type); break;
        }
        case Op::string_read: {
            const auto method=static_cast<StringRead>(instruction.operand);
            if (string_read_try(method)) require(reference_type(string_read_number(method)));
            require(reference_type(Type::integer)); require(Type::string); stack.push_back(instruction.type); break;
        }
        case Op::convert_string: require(reference_type(Type::string)); require(instruction.type); break;
        case Op::try_parse: case Op::val:
            if (instruction.op==Op::val) require(reference_type(Type::integer));
            require(reference_type(instruction.type)); require(Type::string);
            if (instruction.op==Op::try_parse) stack.push_back(Type::boolean);
            break;
        case Op::try_read: require(Type::string); require(reference_type(instruction.type)); stack.push_back(Type::boolean); break;
        case Op::read_array: require(instruction.type); break;
        case Op::read_value: stack.push_back(instruction.type); break;
        case Op::file_print: require(instruction.type); require(Type::text_file); stack.push_back(Type::text_file); break;
        case Op::file_read: require(reference_type(instruction.type)); require(Type::text_file); stack.push_back(Type::text_file); break;
        case Op::file_newline: case Op::file_skip_line: require(Type::text_file); stack.push_back(Type::text_file); break;
        case Op::print_output: case Op::file_output:
            for (std::size_t n=0;n<instruction.operand/4;++n)
                if (!object_text_supported(program.types,pop())) invalid(instruction,"Unsupported Text output type");
            if (instruction.op==Op::file_output) require(Type::text_file);
            break;
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
            require(compound(instruction.type) && type_info(program,instruction.type).kind==CompoundKind::sequence ? Type::int64 : Type::integer);
            if (instruction.type!=Type::string && type_info(program,instruction.type).rank==2 && !instruction.operand) require(Type::integer);
            require(instruction.type);
            if (instruction.type==Type::string) { stack.push_back(Type::character); break; }
            stack.push_back(instruction.op == Op::get_index ? type_info(program,instruction.type).element : reference_type(type_info(program,instruction.type).element)); break;
        case Op::get_field: case Op::ref_field:
            require(instruction.op == Op::get_field ? instruction.type : reference_type(instruction.type));
            stack.push_back(instruction.op == Op::get_field ? type_info(program,instruction.type).fields[instruction.operand].type : reference_type(type_info(program,instruction.type).fields[instruction.operand].type)); break;
        case Op::length: require(instruction.type); stack.push_back(compound(instruction.type) && type_info(program,instruction.type).kind==CompoundKind::sequence ? Type::int64 : Type::integer); break;
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
        if (instruction.op != Op::jump) {
            if (pc+1<code.size() && !entries[pc+1]) { linear_pc=pc+1; linear_stack=std::move(stack); }
            else merge(pc+1,stack,instruction);
        }
    }
}

void verify(const Program& program) {
    if (program.print_delimiter && (*program.print_delimiter>=program.locals.size() || !program.locals[*program.print_delimiter].global
        || program.locals[*program.print_delimiter].type!=Type::string)) throw Error("E_BYTECODE",{},"Invalid PrintDelimDefault global");
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
        if (type.kind==CompoundKind::sequence) {
            if (type.key!=Type::invalid || !type.fields.empty() || type.name!="sequence") throw Error("E_BYTECODE",{},"Invalid sequence type metadata");
            prior(type.element);
        } else if (school_ip_kind(type.kind)) {
            const bool address=type.kind==CompoundKind::school_address;
            const auto size=address ? school_address_fields.size() : school_ip_fields.size();
            if (type.fields.size()!=size || type.element!=Type::invalid || type.key!=Type::invalid || type.name!=(address ? "addr32" : "calcip"))
                throw Error("E_BYTECODE",{},"Invalid IPv4 class metadata");
            for (std::size_t field=0;field<size;++field) {
                const auto name=address ? school_address_fields[field] : school_ip_fields[field];
                const auto expected=address ? (field==2 ? Type::longword : Type::string) : field==2 ? Type::byte : field==3 ? Type::integer : type.fields[0].type;
                if (type.fields[field].global || type.fields[field].name!=name || type.fields[field].type!=expected) throw Error("E_BYTECODE",{},"Invalid private IPv4 field");
                prior(expected);
            }
            if (!address && (!compound(type.fields[0].type) || type_info(program,type.fields[0].type).kind!=CompoundKind::school_address))
                throw Error("E_BYTECODE",{},"CalcIP requires Addr32 fields");
        } else if (type.kind == CompoundKind::record || type.kind==CompoundKind::tuple) {
            if (type.fields.size() > 256) throw Error("E_BYTECODE",{},"Too many record fields");
            if (type.kind==CompoundKind::tuple) {
                if (type.fields.size()<2 || type.fields.size()>7 || type.element!=Type::invalid || type.key!=Type::invalid)
                    throw Error("E_BYTECODE",{},"Invalid tuple shape");
                for (std::size_t n=0;n<type.fields.size();++n) if (type.fields[n].global || type.fields[n].name!="item"+std::to_string(n+1))
                    throw Error("E_BYTECODE",{},"Invalid tuple field metadata");
            }
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
        for (std::size_t n=0;n<routine.parameters.size();++n) {
            const auto& parameter=routine.parameters[n];
            if (parameter.variadic && (n+1!=routine.parameters.size() || parameter.by_reference || !compound(parameter.type)
                || type_info(program,parameter.type).kind!=CompoundKind::array || type_info(program,parameter.type).rank!=1))
                throw Error("E_BYTECODE",{},"Invalid params signature");
        }
        std::set<std::size_t> parameters;
        for (const auto& parameter : routine.parameters)
            if (!owned.contains(parameter.local) || program.locals[parameter.local].type != parameter.type
                || !parameters.insert(parameter.local).second || (routine.result!=Type::unit && parameter.local==routine.result_local))
                throw Error("E_BYTECODE",{},"Invalid parameter");
        verify_body(program,routine.code,&routine);
    }
    verify_body(program,program.code,nullptr);
    std::optional<Type> school_cache_type;
    const auto check_school_cache=[&](const std::vector<Instruction>& code) {
        for (const auto& instruction:code) if (instruction.op==Op::school_list) {
            const auto method=static_cast<SchoolList>(instruction.operand);
            if (type_info(program,instruction.type).element!=Type::integer
                || (method!=SchoolList::first_primes && method!=SchoolList::primes_range && !school_factor_method(method))) continue;
            if (school_cache_type && *school_cache_type!=instruction.type) invalid(instruction,"Conflicting types for the shared School prime cache");
            school_cache_type=instruction.type;
        }
    };
    check_school_cache(program.code);
    for (const auto& routine:program.routines) check_school_cache(routine.code);
}

} // namespace abc

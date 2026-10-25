#pragma once

#include "abc/core.hpp"
#include <memory>

namespace abc::hir {

enum class ExprKind { literal, local, convert, unary, binary, call, default_value,
    index, field, array_new, array_literal, length, user_call, list_add, list_remove, list_clear, list_contains,
    read_lines, write_lines, array_copy, array_high, sequence_sort, sequence_fill, null_literal, endpoint_call, buffer_call, association_call, linked_call, file_output, random_array, fill_random_array, sequence_mutate, swap_values, write_array, read_array, array_query, write_matrix, object_text, try_read, number_conversion };
struct Expr {
    ExprKind kind;
    Type type;
    SourceLocation location;
    Value value = std::int32_t{0};
    std::size_t local = 0;
    std::string operation;
    std::unique_ptr<Expr> left, right;
    std::vector<std::unique_ptr<Expr>> arguments{};
};

enum class StmtKind { block, assign, print, read, conditional, loop, call, return_stmt, resize_array, increment, string_mutation,
    for_loop, repeat_loop, break_stmt, continue_stmt, foreach_loop };
struct Stmt {
    StmtKind kind = StmtKind::block;
    SourceLocation location;
    std::size_t local = 0;
    bool newline = false;
    std::size_t sequence_local = 0, index_local = 0, version_local = 0;
    std::unique_ptr<Expr> expression;
    std::vector<std::unique_ptr<Expr>> arguments;
    std::vector<Stmt> statements;
    std::unique_ptr<Stmt> body, alternative;
    std::unique_ptr<Expr> target{};
};

struct RoutineBody { Routine signature; Stmt body; };

struct Module {
    std::vector<Local> locals;
    Stmt body;
    std::vector<TypeInfo> types;
    std::vector<RoutineBody> routines;
    std::optional<std::size_t> print_delimiter;
};

Module analyze_pascal(std::string_view source);
Program generate(Module module);

} // namespace abc::hir

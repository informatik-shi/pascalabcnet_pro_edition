#pragma once

#include "abc/core.hpp"
#include <memory>

namespace abc::hir {

enum class ExprKind { literal, local, convert, unary, binary, call, default_value, conditional,
    index, field, property_access, class_construct, encoding_call, array_new, array_literal, array_initialized, length, user_call, routine_value, value_call, list_add, list_remove, list_clear, list_contains,
    read_lines, write_lines, text_lines, enum_call, comparable_call, ini_call, hash_set_call, new_set_call, new_set_hash, empty_call, print_call, cache_value, compound_call, array_copy, array_high, sequence_sort, sequence_fill, null_literal, endpoint_call, buffer_call, association_call, linked_call, file_output, random_array, fill_random_array, sequence_mutate, swap_values, write_array, read_array, array_query, write_matrix, object_text, try_read, number_conversion, string_read, string_array, school_list, replace_last, replace_many, school_fold, tuple_literal, school_pair, school_minmax, school_table_print, school_table, school_ip, sequence_call, sequence_wrap, swap_substrings, sequence_query, sequence_snapshot, sequence_repeat, sequence_range, sequence_partition, sequence_aggregate, sequence_partial, sequence_slice, sequence_concat, sequence_equal, sequence_default, sequence_transform, sequence_predicate, sequence_zip, generator, cartesian_power, combinatorics, next_permutation, array_view, matrix_operation, array_operation, collection_operation, matrix_build, list_operation, list_bulk, list_capacity, list_copy, sequence_join, ordinal_range_call, real_interval_call, evaluate_then, random_sequence, tuple_construct, directory_sequence, type_operation, dictionary_call, enumerator_call, interface_cast, interface_call, comparer_create, sort_call, sequence_set };
struct Expr {
    ExprKind kind;
    Type type;
    SourceLocation location;
    Value value = std::int32_t{0};
    std::size_t local = 0;
    std::string operation;
    std::unique_ptr<Expr> left, right;
    std::vector<std::unique_ptr<Expr>> arguments{};
    Type operation_type=Type::invalid;
    std::vector<std::size_t> temporaries{};
    std::optional<std::size_t> getter{},setter{};
};

enum class StmtKind { block, assign, print, read, conditional, loop, call, return_stmt, resize_array, increment, string_mutation,
    for_loop, repeat_loop, break_stmt, continue_stmt, foreach_loop, for_step_check };
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
    bool declaration=false;
    bool deferred_write=false;
};

struct RoutineBody { Routine signature; Stmt body; };

struct Module {
    std::vector<Local> locals;
    Stmt body;
    std::vector<TypeInfo> types;
    std::vector<RoutineBody> routines;
    std::optional<std::size_t> print_delimiter;
    std::optional<std::size_t> print_matrix_format;
    std::optional<std::size_t> default_encoding;
    std::vector<FixedArrayShape> fixed_arrays;
    std::vector<StaticClass> static_classes;
    std::vector<StaticMethod> static_methods;
};

Module analyze_pascal(std::string_view source);
Program generate(Module module);

} // namespace abc::hir

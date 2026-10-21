#pragma once

#include "abc/core.hpp"
#include <memory>

namespace abc::hir {

enum class ExprKind { literal, local, convert, unary, binary, call };
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

enum class StmtKind { block, assign, print, read, conditional, loop };
struct Stmt {
    StmtKind kind = StmtKind::block;
    SourceLocation location;
    std::size_t local = 0;
    bool newline = false;
    std::unique_ptr<Expr> expression;
    std::vector<std::unique_ptr<Expr>> arguments;
    std::vector<Stmt> statements;
    std::unique_ptr<Stmt> body, alternative;
};

struct Module {
    std::vector<Local> locals;
    Stmt body;
};

Module analyze_pascal(std::string_view source);
Program generate(Module module);

} // namespace abc::hir

#include "constant.hpp"
#include "stdlib.hpp"

namespace abc::hir {
namespace {
std::unique_ptr<Expr> clone_constant(const Expr& source) {
    if (compound(source.type) || source.type==Type::unit || source.type==Type::text_file) return {};
    switch (source.kind) {
    case ExprKind::literal: case ExprKind::convert: case ExprKind::unary:
    case ExprKind::binary: case ExprKind::default_value: break;
    case ExprKind::call: {
        const auto function=stdlib::signatures()[source.local].function;
        if (function!=stdlib::Function::integer_cast && function!=stdlib::Function::ordinal
            && function!=stdlib::Function::character && function!=stdlib::Function::successor
            && function!=stdlib::Function::predecessor) return {};
        break;
    }
    default: return {};
    }
    auto result=std::make_unique<Expr>();
    result->kind=source.kind; result->type=source.type; result->location=source.location;
    result->value=source.value; result->local=source.local; result->operation=source.operation;
    result->operation_type=source.operation_type;
    if (source.left) {
        result->left=clone_constant(*source.left);
        if (!result->left) return {};
    }
    if (source.right) {
        result->right=clone_constant(*source.right);
        if (!result->right) return {};
    }
    for (const auto& argument:source.arguments) {
        auto item=clone_constant(*argument);
        if (!item) return {};
        result->arguments.push_back(std::move(item));
    }
    return result;
}
}
std::optional<Value> constant_value(const Expr& expression) {
    auto copied=clone_constant(expression);
    if (!copied) return {};
    if (copied->kind==ExprKind::literal) return copied->value;
    Module module;
    module.locals.push_back({"$constant",copied->type,true});
    Stmt assignment; assignment.kind=StmtKind::assign; assignment.location=copied->location;
    assignment.expression=std::move(copied);
    module.body.statements.push_back(std::move(assignment));
    return execute_scalar_constant(generate(std::move(module)));
}
}

#include "hir.hpp"
#include "number.hpp"
#include "stdlib.hpp"


#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <map>
#include <set>

namespace abc {
namespace {

enum class TokenKind { name, number, text, symbol, eof };
struct Token {
    TokenKind kind;
    std::string text;
    SourceLocation location;
};

bool digit(char c) { return c >= '0' && c <= '9'; }
bool letter(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
char lower(char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c + ('a' - 'A')) : c; }

class Lexer {
public:
    explicit Lexer(std::string_view source) : source_(source) {
        try { (void)from_utf8(source); }
        catch (const std::invalid_argument&) { throw Error("E_ENCODING", {}, "Source must be valid UTF-8"); }
        if (source_.starts_with("\xef\xbb\xbf")) position_.offset = 3;
    }

    Token next() {
        skip();
        const auto start = position_;
        if (peek() == '\0' && position_.offset == source_.size()) return {TokenKind::eof, "", start};
        if (letter(peek())) {
            std::string name;
            do { name += lower(take()); } while (letter(peek()) || digit(peek()));
            return {TokenKind::name, std::move(name), start};
        }
        if (digit(peek())) {
            std::string number;
            while (digit(peek())) number += take();
            if (peek() == '.' && digit(peek(1))) {
                number += take();
                while (digit(peek())) number += take();
            }
            if (peek() == 'e' || peek() == 'E') {
                number += take();
                if (peek() == '+' || peek() == '-') number += take();
                if (!digit(peek())) throw Error("E_SYNTAX", start, "Expected digits in exponent");
                while (digit(peek())) number += take();
            }
            return {TokenKind::number, std::move(number), start};
        }
        if (peek() == '\'') {
            take();
            std::string text;
            for (;;) {
                if (at_end() || peek() == '\n' || peek() == '\r')
                    throw Error("E_SYNTAX", start, "Unterminated string literal");
                const char c = take();
                if (c == '\'') {
                    if (peek() == '\'') { take(); text += '\''; }
                    else break;
                } else text += c;
            }
            return {TokenKind::text, std::move(text), start};
        }
        std::string symbol(1, take());
        if ((symbol == ":" && peek() == '=') || (symbol == "<" && (peek() == '=' || peek() == '>'))
            || (symbol == ">" && peek() == '=')) symbol += take();
        if (std::string_view(";:,.()+-*/=<> ").find(symbol.front()) == std::string_view::npos)
            throw Error("E_SYNTAX", start, "Unsupported character (identifiers currently use ASCII letters)");
        return {TokenKind::symbol, std::move(symbol), start};
    }

private:
    bool at_end() const { return position_.offset >= source_.size(); }
    char peek(std::size_t ahead = 0) const {
        return position_.offset + ahead < source_.size() ? source_[position_.offset + ahead] : '\0';
    }
    char take() {
        const auto c = source_[position_.offset++];
        if (c == '\n') { ++position_.line; position_.column = 1; }
        else ++position_.column;
        return c;
    }
    void skip() {
        for (;;) {
            while (!at_end() && (peek() == ' ' || peek() == '\t' || peek() == '\r' || peek() == '\n')) take();
            if (peek() == '/' && peek(1) == '/') {
                while (!at_end() && peek() != '\n') take();
            } else if (peek() == '{' || (peek() == '(' && peek(1) == '*')) {
                const auto start = position_;
                // Mixed nested comments have explicitly matched delimiters.
                std::vector<char> delimiters;
                do {
                    if (peek() == '{') {
                        take();
                        if (peek() == '$') throw Error("E_UNSUPPORTED", start, "Compiler directives are not supported yet");
                        delimiters.push_back('}');
                    } else if (peek() == '(' && peek(1) == '*') {
                        take(); take(); delimiters.push_back(')');
                    } else if (peek() == '}' && delimiters.back() == '}') {
                        take(); delimiters.pop_back();
                    } else if (peek() == '*' && peek(1) == ')' && delimiters.back() == ')') {
                        take(); take(); delimiters.pop_back();
                    } else if (at_end()) throw Error("E_SYNTAX", start, "Unterminated comment");
                    else take();
                    if (delimiters.size() > 256) throw Error("E_LIMIT", start, "Comment nesting limit exceeded");
                } while (!delimiters.empty());
            } else break;
        }
    }
    std::string_view source_;
    SourceLocation position_;
};

enum class SyntaxExprKind { literal, name, unary, binary, call };
struct SyntaxExpr {
    SyntaxExprKind kind;
    Token token;
    std::unique_ptr<SyntaxExpr> left, right;
    unsigned depth = 1;
    std::vector<std::unique_ptr<SyntaxExpr>> arguments{};
};

std::unique_ptr<SyntaxExpr> make_expression(SyntaxExprKind kind, Token token,
    std::unique_ptr<SyntaxExpr> left = {}, std::unique_ptr<SyntaxExpr> right = {}) {
    const auto depth = 1 + std::max(left ? left->depth : 0, right ? right->depth : 0);
    if (depth > 256) throw Error("E_LIMIT", token.location, "Expression tree nesting limit exceeded");
    return std::make_unique<SyntaxExpr>(SyntaxExpr{kind, std::move(token), std::move(left), std::move(right), depth});
}

enum class SyntaxStmtKind { block, declaration, assignment, print, read, conditional, loop, increment };
struct SyntaxStmt {
    SyntaxStmtKind kind = SyntaxStmtKind::block;
    Token token{TokenKind::eof, "", {}};
    std::vector<Token> names;
    std::optional<Token> annotation;
    std::unique_ptr<SyntaxExpr> expression;
    std::vector<std::unique_ptr<SyntaxExpr>> arguments;
    std::vector<SyntaxStmt> statements;
    std::unique_ptr<SyntaxStmt> body, alternative;
};

bool reserved(std::string_view text) {
    static const std::set<std::string_view> words{
        "program", "begin", "end", "var", "if", "then", "else", "while", "do", "and", "or", "not",
        "div", "mod", "true", "false", "uses", "unit", "type", "const", "function", "procedure",
        "for", "to", "downto", "repeat", "until", "case", "of", "array", "record", "class", "nil",
        "set", "try", "except", "finally", "raise", "break", "continue", "exit", "writeln", "write", "readln"};
    return words.contains(text);
}

// Bound recursive parsing separately from executable loop iteration limits.
struct DepthGuard {
    unsigned& depth;
    explicit DepthGuard(unsigned& depth, SourceLocation location) : depth(depth) {
        if (++depth > 256) throw Error("E_LIMIT", location, "Syntax nesting limit exceeded");
    }
    ~DepthGuard() { --depth; }
};

class Parser {
public:
    explicit Parser(std::string_view source) : lexer_(source), current_(lexer_.next()) {}
    SyntaxStmt parse() {
        SyntaxStmt root;
        if (accept("program")) { name(); expect(";"); }
        if (accept("uses")) {
            do {
                const auto module = name();
                if (module.text != "pabcsystem")
                    throw Error("E_UNSUPPORTED", module.location, "Unit is not ported yet: '" + module.text + "'");
            } while (accept(","));
            expect(";");
        }
        while (accept("var")) {
            do { root.statements.push_back(declaration()); expect(";"); }
            while (current_.kind == TokenKind::name && !reserved(current_.text));
        }
        root.statements.push_back(block());
        expect(".");
        if (current_.kind != TokenKind::eof) fail("Unexpected text after end of program");
        return root;
    }

private:
    [[noreturn]] void fail(std::string message) const { throw Error("E_SYNTAX", current_.location, std::move(message)); }
    Token consume() { auto old = current_; current_ = lexer_.next(); return old; }
    bool is(std::string_view text) const { return current_.kind != TokenKind::text && current_.text == text; }
    bool accept(std::string_view text) { if (!is(text)) return false; consume(); return true; }
    Token expect(std::string_view text) {
        if (!is(text)) fail("Expected '" + std::string(text) + "', found '" + current_.text + "'");
        return consume();
    }
    Token name() {
        if (current_.kind != TokenKind::name || reserved(current_.text)) fail("Expected an identifier");
        return consume();
    }
    SyntaxStmt declaration() {
        SyntaxStmt result;
        result.kind = SyntaxStmtKind::declaration;
        result.token = current_;
        result.names.push_back(name());
        while (accept(",")) result.names.push_back(name());
        if (accept(":")) result.annotation = name();
        if (accept(":=")) result.expression = expression();
        if (!result.annotation && !result.expression) fail("Variable needs a type or an initializer");
        if (result.names.size() > 1 && result.expression) fail("Multiple initialized names are not supported yet");
        return result;
    }
    SyntaxStmt block() {
        DepthGuard guard(depth_, current_.location);
        SyntaxStmt result;
        result.token = expect("begin");
        while (!is("end")) {
            if (current_.kind == TokenKind::eof) fail("Expected 'end'");
            if (accept(";")) continue;
            result.statements.push_back(statement());
            if (!accept(";") && !is("end")) fail("Expected ';' between statements");
        }
        expect("end");
        return result;
    }
    SyntaxStmt statement() {
        DepthGuard guard(depth_, current_.location);
        if (is("begin")) return block();
        if (accept("var")) return declaration();
        SyntaxStmt result;
        result.token = current_;
        if (accept("if")) {
            result.kind = SyntaxStmtKind::conditional;
            result.expression = expression();
            expect("then");
            result.body = std::make_unique<SyntaxStmt>(statement());
            if (accept("else")) result.alternative = std::make_unique<SyntaxStmt>(statement());
        } else if (accept("while")) {
            result.kind = SyntaxStmtKind::loop;
            result.expression = expression();
            expect("do");
            result.body = std::make_unique<SyntaxStmt>(statement());
        } else if (is("write") || is("writeln")) {
            result.kind = SyntaxStmtKind::print;
            consume();
            if (accept("(")) {
                if (!is(")")) {
                    do { result.arguments.push_back(expression()); } while (accept(","));
                }
                expect(")");
            }
        } else if (accept("readln")) {
            result.kind = SyntaxStmtKind::read;
            expect("(");
            result.names.push_back(name());
            expect(")");
        } else if (current_.kind == TokenKind::name && !reserved(current_.text)) {
            const auto target = name();
            result.names.push_back(target);
            if ((target.text == "inc" || target.text == "dec") && accept("(")) {
                result.kind = SyntaxStmtKind::increment;
                result.names.push_back(name());
                if (accept(",")) result.expression = expression();
                expect(")");
            } else {
                result.kind = SyntaxStmtKind::assignment;
                expect(":=");
                result.expression = expression();
            }
        } else {
            throw Error("E_UNSUPPORTED", current_.location, "Statement is not supported: '" + current_.text + "'");
        }
        return result;
    }
    static int precedence(std::string_view op) {
        if (op == "=" || op == "<>" || op == "<" || op == "<=" || op == ">" || op == ">=") return 1;
        if (op == "+" || op == "-" || op == "or") return 2;
        if (op == "*" || op == "/" || op == "div" || op == "mod" || op == "and") return 3;
        return 0;
    }
    std::unique_ptr<SyntaxExpr> expression(int minimum = 1) {
        DepthGuard guard(depth_, current_.location);
        auto left = prefix();
        while (current_.kind != TokenKind::text && precedence(current_.text) >= minimum) {
            const auto op = consume();
            auto right = expression(precedence(op.text) + 1);
            left = make_expression(SyntaxExprKind::binary, op, std::move(left), std::move(right));
        }
        return left;
    }
    std::unique_ptr<SyntaxExpr> prefix() {
        DepthGuard guard(depth_, current_.location);
        if (is("-") || is("+") || is("not")) {
            auto op = consume();
            // Parse the signed literal together so -2147483648 has type integer.
            if ((op.text == "-" || op.text == "+") && current_.kind == TokenKind::number) {
                auto token = consume();
                if (op.text == "-") token.text.insert(token.text.begin(), '-');
                token.location = op.location;
                return make_expression(SyntaxExprKind::literal, token);
            }
            return make_expression(SyntaxExprKind::unary, op, prefix());
        }
        if (accept("(")) { auto result = expression(); expect(")"); return result; }
        if (current_.kind == TokenKind::number || current_.kind == TokenKind::text || is("true") || is("false"))
            return make_expression(SyntaxExprKind::literal, consume());
        if (current_.kind == TokenKind::name && !reserved(current_.text)) {
            auto result = make_expression(SyntaxExprKind::name, consume());
            if (accept("(")) {
                result->kind = SyntaxExprKind::call;
                if (!is(")")) do {
                    auto argument = expression();
                    result->depth = std::max(result->depth, argument->depth + 1);
                    if (result->depth > 256 || result->arguments.size() >= 16)
                        throw Error("E_LIMIT", result->token.location, "Function call nesting or argument limit exceeded");
                    result->arguments.push_back(std::move(argument));
                } while (accept(","));
                expect(")");
            }
            return result;
        }
        fail("Expected an expression");
    }
    Lexer lexer_;
    Token current_;
    unsigned depth_ = 0;
};

Value literal(const Token& token) {
    if (token.kind == TokenKind::text) return from_utf8(token.text);
    if (token.text == "true" || token.text == "false") return token.text == "true";
    if (token.text.find_first_of(".eE") != std::string::npos) {
        double value = 0;
        if (!parse_real(token.text, value))
            throw Error("E_RANGE", token.location, "Real literal is out of range");
        return value;
    }
    std::int64_t value = 0;
    const auto parsed = std::from_chars(token.text.data(), token.text.data() + token.text.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != token.text.data() + token.text.size())
        throw Error("E_RANGE", token.location, "Integer literal is out of int64 range");
    if (value >= std::numeric_limits<std::int32_t>::min() && value <= std::numeric_limits<std::int32_t>::max())
        return static_cast<std::int32_t>(value);
    return value;
}

class Analyzer {
public:
    hir::Module analyze(const SyntaxStmt& source) {
        module_.body = statement(source);
        return std::move(module_);
    }

private:
    std::optional<std::size_t> lookup(const Token& token) const {
        for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
            if (const auto found = scope->find(token.text); found != scope->end()) return found->second;
        }
        return std::nullopt;
    }
    std::size_t resolve(const Token& token) const {
        if (const auto local = lookup(token)) return *local;
        throw Error("E_NAME", token.location, "Unknown name '" + token.text + "'");
    }
    static std::unique_ptr<hir::Expr> convert(std::unique_ptr<hir::Expr> expression, Type type) {
        if (expression->type == type) return expression;
        if (!can_widen(expression->type, type))
            throw Error("E_TYPE", expression->location, "Cannot assign " + std::string(type_name(expression->type))
                + " to " + std::string(type_name(type)));
        const auto location = expression->location;
        return std::make_unique<hir::Expr>(hir::Expr{
            hir::ExprKind::convert, type, location, std::int32_t{0}, 0, "", std::move(expression), {}});
    }
    std::unique_ptr<hir::Expr> expression(const SyntaxExpr& source) {
        auto result = std::make_unique<hir::Expr>();
        result->location = source.token.location;
        result->operation = source.token.text;
        switch (source.kind) {
        case SyntaxExprKind::literal:
            result->kind = hir::ExprKind::literal;
            result->value = literal(source.token);
            result->type = value_type(result->value);
            break;
        case SyntaxExprKind::name:
            if (const auto local = lookup(source.token)) {
                result->kind = hir::ExprKind::local;
                result->local = *local;
                result->type = module_.locals[*local].type;
            } else if (const auto value = stdlib::constant(source.token.text)) {
                result->kind = hir::ExprKind::literal;
                result->value = *value;
                result->type = value_type(*value);
            } else {
                (void)resolve(source.token);
            }
            break;
        case SyntaxExprKind::call: {
            if (lookup(source.token)) throw Error("E_TYPE", result->location, "Variable is not callable: '" + source.token.text + "'");
            result->kind = hir::ExprKind::call;
            for (const auto& argument : source.arguments) result->arguments.push_back(expression(*argument));
            const auto catalog = stdlib::signatures();
            std::vector<std::size_t> candidates;
            bool known = false;
            for (std::size_t i = 0; i < catalog.size(); ++i) {
                const auto& signature = catalog[i];
                if (signature.name != source.token.text) continue;
                known = true;
                if (signature.parameters.size() != result->arguments.size()) continue;
                bool compatible = true;
                for (std::size_t n = 0; n < signature.parameters.size(); ++n)
                    if (!can_widen(result->arguments[n]->type, signature.parameters[n])) compatible = false;
                if (compatible) candidates.push_back(i);
            }
            if (!known) throw Error("E_NAME", result->location, "Unknown function '" + source.token.text + "'");
            // Partial order: an overload wins only if no parameter needs a wider type.
            const auto dominates = [&](std::size_t a, std::size_t b) {
                bool better = false;
                for (std::size_t n = 0; n < catalog[a].parameters.size(); ++n) {
                    const auto left = catalog[a].parameters[n], right = catalog[b].parameters[n];
                    if (!can_widen(left, right)) return false;
                    if (left != right) better = true;
                }
                return better;
            };
            std::optional<std::size_t> chosen;
            for (auto candidate : candidates) {
                bool dominated = false;
                for (auto other : candidates) if (dominates(other, candidate)) dominated = true;
                if (!dominated) {
                    if (chosen) throw Error("E_TYPE", result->location, "Ambiguous overload for '" + source.token.text + "'");
                    chosen = candidate;
                }
            }
            if (!chosen) throw Error("E_TYPE", result->location, "No matching overload for '" + source.token.text + "'");
            const auto& signature = catalog[*chosen];
            result->local = *chosen;
            result->type = signature.result;
            for (std::size_t n = 0; n < result->arguments.size(); ++n)
                result->arguments[n] = convert(std::move(result->arguments[n]), signature.parameters[n]);
            break;
        }
        case SyntaxExprKind::unary:
            result->kind = hir::ExprKind::unary;
            result->left = expression(*source.left);
            result->type = result->left->type;
            if (source.token.text == "not" ? result->type != Type::boolean : !numeric(result->type))
                throw Error("E_TYPE", source.token.location, "Invalid operand for '" + source.token.text + "'");
            break;
        case SyntaxExprKind::binary: {
            result->kind = hir::ExprKind::binary;
            result->left = expression(*source.left);
            result->right = expression(*source.right);
            auto left = result->left->type;
            auto right = result->right->type;
            const auto& op = result->operation;
            if (op == "and" || op == "or") {
                if (left != Type::boolean || right != Type::boolean)
                    throw Error("E_TYPE", result->location, "Logical operands must be boolean");
                result->type = Type::boolean;
                break;
            }
            const bool comparison = op == "=" || op == "<>" || op == "<" || op == "<=" || op == ">" || op == ">=";
            if (numeric(left) && numeric(right)) {
                if ((op == "div" || op == "mod") && (left == Type::real || right == Type::real))
                    throw Error("E_TYPE", result->location, "div and mod require integer operands");
                const auto common = op == "/" || left == Type::real || right == Type::real ? Type::real
                    : left == Type::int64 || right == Type::int64 ? Type::int64 : Type::integer;
                result->left = convert(std::move(result->left), common);
                result->right = convert(std::move(result->right), common);
                result->type = comparison ? Type::boolean : common;
            } else if (left == Type::string && right == Type::string && (op == "+" || comparison)) {
                result->type = comparison ? Type::boolean : Type::string;
            } else if (left == Type::boolean && right == Type::boolean && (op == "=" || op == "<>")) {
                result->type = Type::boolean;
            } else throw Error("E_TYPE", result->location, "Incompatible operands for '" + op + "': "
                + std::string(type_name(left)) + " and " + std::string(type_name(right)));
            break;
        }
        }
        return result;
    }
    hir::Stmt scoped_statement(const SyntaxStmt& source) {
        scopes_.emplace_back();
        auto result = statement(source);
        scopes_.pop_back();
        return result;
    }
    hir::Stmt statement(const SyntaxStmt& source) {
        hir::Stmt result;
        result.location = source.token.location;
        switch (source.kind) {
        case SyntaxStmtKind::block:
            scopes_.emplace_back();
            for (const auto& child : source.statements) result.statements.push_back(statement(child));
            scopes_.pop_back();
            break;
        case SyntaxStmtKind::declaration: {
            auto initial = source.expression ? expression(*source.expression) : nullptr;
            auto type = initial ? initial->type : Type::invalid;
            if (source.annotation) {
                const auto found = find_type(source.annotation->text);
                if (!found) throw Error("E_UNSUPPORTED", source.annotation->location,
                    "Type is not supported in this prototype: '" + source.annotation->text + "'");
                type = *found;
            }
            if (initial) initial = convert(std::move(initial), type);
            for (const auto& token : source.names) {
                if (scopes_.back().contains(token.text))
                    throw Error("E_DUPLICATE", token.location, "Duplicate name '" + token.text + "'");
                const auto local = module_.locals.size();
                module_.locals.push_back({token.text, type});
                scopes_.back().emplace(token.text, local);
                hir::Stmt assignment;
                assignment.kind = hir::StmtKind::assign;
                assignment.location = token.location;
                assignment.local = local;
                // Initialize when the declaration is executed, including inside loops.
                if (initial) assignment.expression = std::move(initial);
                else {
                    Value zero;
                    switch (type) {
                    case Type::integer: zero = std::int32_t{0}; break;
                    case Type::int64: zero = std::int64_t{0}; break;
                    case Type::real: zero = 0.0; break;
                    case Type::boolean: zero = false; break;
                    case Type::string: zero = std::u16string{}; break;
                    default: throw Error("E_TYPE", token.location, "Unresolved variable type");
                    }
                    assignment.expression = std::make_unique<hir::Expr>(hir::Expr{
                        hir::ExprKind::literal, type, token.location, std::move(zero), 0, "", {}, {}});
                }
                result.statements.push_back(std::move(assignment));
            }
            break;
        }
        case SyntaxStmtKind::increment: {
            if (lookup(source.names.front())) throw Error("E_TYPE", result.location, "Variable is not callable");
            result.kind = hir::StmtKind::assign;
            result.local = resolve(source.names[1]);
            const auto type = module_.locals[result.local].type;
            if (type != Type::integer && type != Type::int64)
                throw Error("E_TYPE", result.location, "Inc/Dec requires an integer variable");
            auto operation = std::make_unique<hir::Expr>();
            operation->kind = hir::ExprKind::binary;
            operation->location = result.location;
            operation->type = type;
            operation->operation = source.names.front().text == "inc" ? "+" : "-";
            operation->left = std::make_unique<hir::Expr>();
            operation->left->kind = hir::ExprKind::local;
            operation->left->type = type;
            operation->left->location = result.location;
            operation->left->local = result.local;
            if (source.expression) operation->right = convert(expression(*source.expression), type);
            else {
                operation->right = std::make_unique<hir::Expr>();
                operation->right->kind = hir::ExprKind::literal;
                operation->right->type = type;
                operation->right->location = result.location;
                operation->right->value = type == Type::integer ? Value(std::int32_t{1}) : Value(std::int64_t{1});
            }
            result.expression = std::move(operation);
            break;
        }
        case SyntaxStmtKind::assignment:
            result.kind = hir::StmtKind::assign;
            result.local = resolve(source.names.front());
            result.expression = convert(expression(*source.expression), module_.locals[result.local].type);
            break;
        case SyntaxStmtKind::print:
            result.kind = hir::StmtKind::print;
            result.newline = source.token.text == "writeln";
            for (const auto& argument : source.arguments) result.arguments.push_back(expression(*argument));
            break;
        case SyntaxStmtKind::read:
            result.kind = hir::StmtKind::read;
            result.local = resolve(source.names.front());
            break;
        case SyntaxStmtKind::conditional:
        case SyntaxStmtKind::loop:
            result.kind = source.kind == SyntaxStmtKind::conditional ? hir::StmtKind::conditional : hir::StmtKind::loop;
            result.expression = expression(*source.expression);
            if (result.expression->type != Type::boolean)
                throw Error("E_TYPE", result.expression->location, "Condition must be boolean");
            result.body = std::make_unique<hir::Stmt>(scoped_statement(*source.body));
            if (source.alternative) result.alternative = std::make_unique<hir::Stmt>(scoped_statement(*source.alternative));
            break;
        }
        return result;
    }
    hir::Module module_;
    std::vector<std::map<std::string, std::size_t>> scopes_;
};

} // namespace

hir::Module hir::analyze_pascal(std::string_view source) {
    if (source.size() > 1'048'576) throw Error("E_LIMIT", {}, "Source exceeds 1 MiB prototype limit");
    const auto syntax = Parser(source).parse();
    return Analyzer().analyze(syntax);
}

Program compile(std::string_view source) {
    auto program = hir::generate(hir::analyze_pascal(source));
    verify(program);
    return program;
}

} // namespace abc

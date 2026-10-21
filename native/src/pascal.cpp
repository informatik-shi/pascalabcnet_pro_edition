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
        if (std::string_view(";:,.()[]+-*/=<> ").find(symbol.front()) == std::string_view::npos)
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

enum class SyntaxExprKind { literal, name, unary, binary, call, index, field, member_call, construction };
struct SyntaxType {
    Token token;
    std::string kind;
    std::shared_ptr<SyntaxType> element;
    std::vector<std::pair<Token, std::shared_ptr<SyntaxType>>> fields;
};
struct SyntaxExpr {
    SyntaxExprKind kind;
    Token token;
    std::unique_ptr<SyntaxExpr> left, right;
    unsigned depth = 1;
    std::shared_ptr<SyntaxType> constructed{};
    std::vector<std::unique_ptr<SyntaxExpr>> arguments{};
    bool array_construction = false;
};

std::unique_ptr<SyntaxExpr> make_expression(SyntaxExprKind kind, Token token,
    std::unique_ptr<SyntaxExpr> left = {}, std::unique_ptr<SyntaxExpr> right = {}) {
    const auto depth = 1 + std::max(left ? left->depth : 0, right ? right->depth : 0);
    if (depth > 256) throw Error("E_LIMIT", token.location, "Expression tree nesting limit exceeded");
    return std::make_unique<SyntaxExpr>(SyntaxExpr{kind, std::move(token), std::move(left), std::move(right), depth, {}});
}

enum class SyntaxStmtKind { block, declaration, assignment, print, read, conditional, loop, increment, string_mutation, call, return_stmt };
struct SyntaxRoutine;
struct SyntaxStmt {
    SyntaxStmtKind kind = SyntaxStmtKind::block;
    Token token{TokenKind::eof, "", {}};
    std::vector<Token> names;
    std::shared_ptr<SyntaxType> annotation;
    std::unique_ptr<SyntaxExpr> expression;
    std::vector<std::unique_ptr<SyntaxExpr>> arguments;
    std::vector<SyntaxStmt> statements;
    std::unique_ptr<SyntaxStmt> body, alternative;
    std::unique_ptr<SyntaxExpr> target;
    std::vector<std::pair<Token,std::shared_ptr<SyntaxType>>> type_declarations;
    std::vector<std::shared_ptr<SyntaxRoutine>> routines;
    bool collections_imported = false;
};

struct SyntaxParameter { Token name; std::shared_ptr<SyntaxType> type; bool by_reference; };
struct SyntaxRoutine {
    Token name;
    std::shared_ptr<SyntaxType> result;
    std::vector<SyntaxParameter> parameters;
    SyntaxStmt body;
};

bool reserved(std::string_view text) {
    static const std::set<std::string_view> words{
        "program", "begin", "end", "var", "if", "then", "else", "while", "do", "and", "or", "not",
        "div", "mod", "true", "false", "uses", "unit", "type", "const", "function", "procedure",
        "for", "to", "downto", "repeat", "until", "case", "of", "array", "record", "class", "nil",
        "new", "set", "try", "except", "finally", "raise", "break", "continue", "exit", "writeln", "write", "readln"};
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
                if (module.text != "pabcsystem" && module.text != "arrays" && module.text != "collections")
                    throw Error("E_UNSUPPORTED", module.location, "Unit is not ported yet: '" + module.text + "'");
                if (module.text == "collections") root.collections_imported = true;
            } while (accept(","));
            expect(";");
        }
        while (!is("begin")) {
            if (accept("type")) {
                do {
                    const auto type_name = name(); expect("=");
                    auto type = type_syntax(); type->token = type_name;
                    root.type_declarations.emplace_back(type_name, std::move(type)); expect(";");
                } while (current_.kind == TokenKind::name && !reserved(current_.text));
            } else if (accept("var")) {
                do { root.statements.push_back(declaration()); expect(";"); }
                while (current_.kind == TokenKind::name && !reserved(current_.text));
            } else if (is("function") || is("procedure")) root.routines.push_back(routine());
            else fail("Expected type, var, function, procedure or begin");
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
    std::shared_ptr<SyntaxType> type_syntax() {
        DepthGuard guard(depth_, current_.location);
        auto result = std::make_shared<SyntaxType>(); result->token = current_;
        if (accept("array")) { result->kind = "array"; expect("of"); result->element = type_syntax(); }
        else if (accept("record")) {
            result->kind = "record";
            while (!is("end")) {
                std::vector<Token> fields{name()}; while (accept(",")) fields.push_back(name());
                expect(":"); auto type = type_syntax();
                for (const auto& field : fields) result->fields.emplace_back(field,type);
                if (!accept(";") && !is("end")) fail("Expected ';' after record field");
            }
            expect("end");
        } else {
            result->token = name(); result->kind = result->token.text;
            if ((result->kind == "list" || result->kind == "stack" || result->kind == "queue" || result->kind == "dynarray" || result->kind == "simpleset") && accept("<")) {
                result->element = type_syntax(); expect(">");
            }
        }
        return result;
    }
    std::shared_ptr<SyntaxRoutine> routine() {
        const bool function = accept("function"); if (!function) expect("procedure");
        auto result = std::make_shared<SyntaxRoutine>(); result->name = name();
        if (accept("(")) {
            if (!is(")")) do {
                const bool by_reference = accept("var");
                std::vector<Token> names{name()}; while (accept(",")) names.push_back(name());
                expect(":"); auto type = type_syntax();
                for (const auto& parameter : names) result->parameters.push_back({parameter,type,by_reference});
                if (result->parameters.size() > 16) fail("Too many routine parameters");
            } while (accept(";"));
            expect(")");
        }
        if (function) { expect(":"); result->result = type_syntax(); }
        expect(";");
        while (accept("var")) {
            do { result->body.statements.push_back(declaration()); expect(";"); }
            while (current_.kind == TokenKind::name && !reserved(current_.text));
        }
        result->body.statements.push_back(block()); expect(";");
        return result;
    }
    SyntaxStmt declaration() {
        SyntaxStmt result;
        result.kind = SyntaxStmtKind::declaration;
        result.token = current_;
        result.names.push_back(name());
        while (accept(",")) result.names.push_back(name());
        if (accept(":")) result.annotation = type_syntax();
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
        if (accept("exit")) {
            result.kind = SyntaxStmtKind::return_stmt;
            if (accept("(")) { if (!is(")")) result.expression = expression(); expect(")"); }
        } else if (accept("if")) {
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
            result.target = expression();
            expect(")");
        } else if (current_.kind == TokenKind::name && !reserved(current_.text)) {
            const auto target = name();
            result.names.push_back(target);
            if ((target.text == "inc" || target.text == "dec") && accept("(")) {
                result.kind = SyntaxStmtKind::increment;
                result.target = expression();
                if (accept(",")) result.expression = expression();
                expect(")");
            } else if ((target.text == "setlength" || target.text == "insert" || target.text == "delete") && accept("(")) {
                result.kind = SyntaxStmtKind::string_mutation;
                if (!is(")")) do {
                    if (result.arguments.size() >= 3) fail("Too many arguments for string procedure");
                    result.arguments.push_back(expression());
                } while (accept(","));
                expect(")");
            } else {
                auto target_expression = postfix(make_expression(SyntaxExprKind::name, target));
                if (accept(":=")) {
                    result.kind = SyntaxStmtKind::assignment;
                    result.target = std::move(target_expression);
                    result.expression = expression();
                } else {
                    result.kind = SyntaxStmtKind::call;
                    if (target_expression->kind == SyntaxExprKind::name) target_expression->kind = SyntaxExprKind::call;
                    result.expression = std::move(target_expression);
                }
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
        if (current_.kind == TokenKind::number || current_.kind == TokenKind::text || is("true") || is("false") || is("nil"))
            return make_expression(SyntaxExprKind::literal, consume());
        if (accept("new")) {
            const auto token = current_; auto type = type_syntax();
            auto result = make_expression(SyntaxExprKind::construction, token); result->constructed = type;
            if (accept("[")) { result->array_construction = true; result->arguments.push_back(expression()); expect("]"); }
            else if (accept("(")) {
                if (!is(")")) result->arguments.push_back(expression());
                expect(")");
            }
            return postfix(std::move(result));
        }
        if (current_.kind == TokenKind::name && !reserved(current_.text))
            return postfix(make_expression(SyntaxExprKind::name, consume()));
        fail("Expected an expression");
    }
    std::unique_ptr<SyntaxExpr> postfix(std::unique_ptr<SyntaxExpr> result) {
        for (;;) {
            if (accept("[")) {
                auto index = expression(); expect("]");
                const auto token = result->token;
                result = make_expression(SyntaxExprKind::index, token, std::move(result), std::move(index));
            } else if (accept(".")) {
                const auto field = name();
                result = make_expression(SyntaxExprKind::field, field, std::move(result));
            } else if (accept("(")) {
                if (result->kind != SyntaxExprKind::name && result->kind != SyntaxExprKind::field) fail("Expression is not callable");
                result->kind = result->kind == SyntaxExprKind::name ? SyntaxExprKind::call : SyntaxExprKind::member_call;
                if (!is(")")) do {
                    auto argument = expression(); result->depth = std::max(result->depth, argument->depth + 1);
                    if (result->depth > 256 || result->arguments.size() >= 16) fail("Function call limit exceeded");
                    result->arguments.push_back(std::move(argument));
                } while (accept(","));
                expect(")");
            } else break;
        }
        return result;
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
        collections_imported_ = source.collections_imported;
        for (const auto& [name, syntax] : source.type_declarations) {
            if (aliases_.contains(name.text) || find_type(name.text)) throw Error("E_DUPLICATE",name.location,"Duplicate type");
            aliases_[name.text] = resolve_type(*syntax);
        }
        scopes_.emplace_back();
        for (std::size_t i = 0; i + 1 < source.statements.size(); ++i)
            module_.body.statements.push_back(statement(source.statements[i]));
        for (const auto& routine : source.routines) {
            if (routine_names_.contains(routine->name.text) || lookup(routine->name)) throw Error("E_DUPLICATE",routine->name.location,"Duplicate routine");
            hir::RoutineBody body;
            body.signature.name = routine->name.text;
            body.signature.result = routine->result ? resolve_type(*routine->result) : Type::unit;
            for (const auto& parameter : routine->parameters)
                body.signature.parameters.push_back({0,resolve_type(*parameter.type),parameter.by_reference});
            routine_names_[routine->name.text] = module_.routines.size(); module_.routines.push_back(std::move(body));
        }
        for (std::size_t i = 0; i < source.routines.size(); ++i) {
            const auto& syntax = *source.routines[i]; auto& routine = module_.routines[i];
            in_routine_ = true; current_result_ = routine.signature.result; current_name_ = syntax.name.text; scopes_.emplace_back();
            const auto first = module_.locals.size();
            for (std::size_t n = 0; n < syntax.parameters.size(); ++n) {
                const auto& name = syntax.parameters[n].name;
                if (scopes_.back().contains(name.text)) throw Error("E_DUPLICATE", name.location,"Duplicate parameter");
                auto& parameter = routine.signature.parameters[n]; parameter.local = module_.locals.size();
                scopes_.back()[name.text] = parameter.local; module_.locals.push_back({name.text,parameter.type,false});
            }
            if (current_result_ != Type::unit) {
                if (scopes_.back().contains("result")) throw Error("E_DUPLICATE",syntax.name.location,"Result is reserved inside functions");
                routine.signature.result_local = module_.locals.size(); scopes_.back()["result"] = module_.locals.size();
                module_.locals.push_back({"result",current_result_,false});
                if (scopes_.back().contains(current_name_)) throw Error("E_DUPLICATE",syntax.name.location,"Parameter conflicts with function name");
                scopes_.back()[current_name_] = routine.signature.result_local;
            }
            routine.body = statement(syntax.body);
            for (auto n = first; n < module_.locals.size(); ++n) routine.signature.locals.push_back(n);
            scopes_.pop_back(); in_routine_ = false; current_name_.clear();
        }
        module_.body.statements.push_back(statement(source.statements.back()));
        scopes_.pop_back();
        if (module_.locals.size() > 4096) throw Error("E_LIMIT",{},"Local variable limit exceeded");
        return std::move(module_);
    }

private:
    Type sequence_type(CompoundKind kind, Type element) {
        for (std::size_t i = 0; i < module_.types.size(); ++i)
            if (module_.types[i].kind == kind && module_.types[i].element == element) return static_cast<Type>(64+i);
        if (module_.types.size() >= 1024) throw Error("E_LIMIT",{},"Type limit exceeded");
        const auto name = kind == CompoundKind::array ? "array" : kind == CompoundKind::list ? "list"
            : kind == CompoundKind::stack ? "stack" : kind == CompoundKind::queue ? "queue"
            : kind == CompoundKind::dynarray ? "dynarray" : "simpleset";
        module_.types.push_back({name,kind,element,{}});
        return static_cast<Type>(63+module_.types.size());
    }
    const TypeInfo& info(Type type, SourceLocation location) const {
        if (!compound(type)) throw Error("E_TYPE",location,"Compound type required");
        return module_.types.at(static_cast<unsigned>(type)-64);
    }
    Type resolve_type(const SyntaxType& syntax) {
        if (auto found=resolved_types_.find(&syntax); found!=resolved_types_.end()) return found->second;
        if (syntax.kind == "array" || (syntax.kind == "list" && syntax.element))
            return sequence_type(syntax.kind == "array" ? CompoundKind::array : CompoundKind::list,resolve_type(*syntax.element));
        if ((syntax.kind == "stack" || syntax.kind == "queue" || syntax.kind == "dynarray" || syntax.kind == "simpleset") && syntax.element) {
            if (!collections_imported_) throw Error("E_UNSUPPORTED",syntax.token.location,"This collection type requires uses Collections");
            const auto kind = syntax.kind == "stack" ? CompoundKind::stack : syntax.kind == "queue" ? CompoundKind::queue
                : syntax.kind == "dynarray" ? CompoundKind::dynarray : CompoundKind::simple_set;
            return sequence_type(kind,resolve_type(*syntax.element));
        }
        if (syntax.kind == "record") {
            TypeInfo record{syntax.token.text,CompoundKind::record,Type::invalid,{}};
            for (const auto& [name,type] : syntax.fields) {
                for (const auto& field : record.fields) if (field.name == name.text) throw Error("E_DUPLICATE",name.location,"Duplicate record field");
                record.fields.push_back({name.text,resolve_type(*type)});
            }
            if (record.fields.size() > 256 || module_.types.size() >= 1024) throw Error("E_LIMIT",syntax.token.location,"Record/type limit exceeded");
            module_.types.push_back(std::move(record));
            const auto type=static_cast<Type>(63+module_.types.size()); resolved_types_[&syntax]=type; return type;
        }
        if (auto found = find_type(syntax.kind)) return *found;
        if (auto found = aliases_.find(syntax.kind); found != aliases_.end()) return found->second;
        throw Error("E_UNSUPPORTED",syntax.token.location,"Unknown or unsupported type '"+syntax.kind+"'");
    }
    static bool lvalue(const hir::Expr& expression) {
        return expression.kind == hir::ExprKind::local
            || ((expression.kind == hir::ExprKind::index || expression.kind == hir::ExprKind::field) && lvalue(*expression.left));
    }
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
    std::unique_ptr<hir::Expr> convert(std::unique_ptr<hir::Expr> expression, Type type) {
        if (expression->type == Type::nil && compound(type) && info(type,expression->location).kind!=CompoundKind::record) {
            expression->type=type; expression->kind=hir::ExprKind::default_value; return expression;
        }
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
        case SyntaxExprKind::construction: {
            auto type = resolve_type(*source.constructed);
            if (source.array_construction) {
                result->kind = hir::ExprKind::array_new; result->type = sequence_type(CompoundKind::array,type);
                result->arguments.push_back(convert(expression(*source.arguments[0]),Type::integer));
            } else {
                if (!compound(type)) throw Error("E_TYPE",result->location,"new requires an array length, record or collection type");
                if (!source.arguments.empty()) {
                    if (info(type,result->location).kind != CompoundKind::dynarray)
                        throw Error("E_TYPE",result->location,"Only DynArray supports a constructor size argument");
                    result->kind = hir::ExprKind::buffer_call; result->type = type;
                    result->local = static_cast<std::size_t>(BufferMethod::construct);
                    result->arguments.push_back(convert(expression(*source.arguments[0]),Type::integer)); break;
                }
                result->kind = hir::ExprKind::default_value; result->type = type; result->operation="new";
            }
            break;
        }
        case SyntaxExprKind::index: {
            if (source.left->kind == SyntaxExprKind::field && source.left->token.text == "elem") {
                auto receiver = expression(*source.left->left);
                if (compound(receiver->type) && info(receiver->type,result->location).kind == CompoundKind::dynarray) result->left = std::move(receiver);
            }
            if (!result->left) result->left = expression(*source.left);
            result->right = convert(expression(*source.right),Type::integer);
            const auto& type = info(result->left->type,result->location);
            if (type.kind == CompoundKind::dynarray) {
                result->kind = hir::ExprKind::buffer_call; result->type = type.element;
                result->local = static_cast<std::size_t>(BufferMethod::get);
                result->arguments.push_back(std::move(result->right)); break;
            }
            if (!indexed_sequence(type.kind)) throw Error("E_TYPE",result->location,"Only arrays and lists are indexable");
            result->type = type.element; result->kind = hir::ExprKind::index; break;
        }
        case SyntaxExprKind::field:
        case SyntaxExprKind::member_call: {
            result->left = expression(*source.left); const auto receiver = result->left->type;
            const auto& type = info(receiver,result->location);
            if (buffer_collection(type.kind)) {
                const auto& name = source.token.text;
                const auto kind = type.kind; const auto element = type.element;
                BufferMethod method;
                if (name == "add") method = BufferMethod::add;
                else if (name == "remove") method = BufferMethod::remove;
                else if (name == "contains" && kind == CompoundKind::simple_set) method = BufferMethod::contains;
                else if (name == "insert" && kind == CompoundKind::dynarray) method = BufferMethod::insert;
                else if (name == "find" && kind == CompoundKind::dynarray) method = BufferMethod::find;
                else if (name == "reserve" && kind == CompoundKind::dynarray) method = BufferMethod::reserve;
                else if (name == "resize" && kind == CompoundKind::dynarray) method = BufferMethod::resize;
                else if (name == "count" && kind == CompoundKind::dynarray) method = BufferMethod::count;
                else if (name == "capacity" && kind == CompoundKind::dynarray) method = BufferMethod::capacity;
                else if (name == "tostring") method = BufferMethod::to_string;
                else if (name == "print") method = BufferMethod::print;
                else if (name == "println") method = BufferMethod::println;
                else throw Error("E_NAME",result->location,"Unknown collection member '"+name+"'");
                if (source.kind == SyntaxExprKind::member_call && (method == BufferMethod::count || method == BufferMethod::capacity))
                    throw Error("E_TYPE",result->location,"Collection property is not callable");
                const auto parameters = buffer_parameters(kind,element,method);
                if (parameters.size() != source.arguments.size()) throw Error("E_TYPE",result->location,"Wrong collection argument count");
                if (method >= BufferMethod::to_string && compound(element)) throw Error("E_UNSUPPORTED",result->location,"Collection text output currently requires scalar elements");
                result->kind = hir::ExprKind::buffer_call; result->local = static_cast<std::size_t>(method);
                result->type = buffer_result(receiver,element,method);
                for (std::size_t i=0;i<parameters.size();++i) result->arguments.push_back(convert(expression(*source.arguments[i]),parameters[i]));
                break;
            }
            if (endpoint_collection(type.kind)) {
                const auto& name = source.token.text;
                const auto element = type.element;
                EndpointMethod method;
                if ((name == "push" && type.kind == CompoundKind::stack) || (name == "enqueue" && type.kind == CompoundKind::queue)) method = EndpointMethod::add;
                else if ((name == "pop" && type.kind == CompoundKind::stack) || (name == "dequeue" && type.kind == CompoundKind::queue)) method = EndpointMethod::remove;
                else if (name == "top") method = EndpointMethod::top;
                else if (name == "isempty") method = EndpointMethod::is_empty;
                else if (name == "tostring") method = EndpointMethod::to_string;
                else if (name == "print") method = EndpointMethod::print;
                else if (name == "println") method = EndpointMethod::println;
                else throw Error("E_NAME",result->location,"Unknown stack or queue method '"+name+"'");
                const auto count = method == EndpointMethod::add ? 1U : 0U;
                if (source.arguments.size() != count) throw Error("E_TYPE",result->location,"Wrong collection method argument count");
                if (method >= EndpointMethod::to_string && compound(element))
                    throw Error("E_UNSUPPORTED",result->location,"Collection text output currently requires scalar elements");
                result->kind = hir::ExprKind::endpoint_call; result->local = static_cast<std::size_t>(method);
                result->type = endpoint_result(element,method);
                if (count) result->arguments.push_back(convert(expression(*source.arguments[0]),element));
                break;
            }
            if (type.kind != CompoundKind::record) {
                if ((source.token.text == "length" && type.kind == CompoundKind::array) || (source.token.text == "count" && type.kind == CompoundKind::list)) {
                    if (!source.arguments.empty()) throw Error("E_TYPE",result->location,"Property takes no arguments");
                    result->kind = hir::ExprKind::length; result->type = Type::integer; break;
                }
                if (type.kind != CompoundKind::list) throw Error("E_NAME",result->location,"Unknown array member");
                const auto element = type.element;
                const auto& name = source.token.text;
                const auto count = name == "clear" ? 0U : 1U;
                if (source.arguments.size() != count) throw Error("E_TYPE",result->location,"Wrong list argument count");
                result->type = Type::unit;
                if (name == "add") result->kind = hir::ExprKind::list_add;
                else if (name == "removeat") result->kind = hir::ExprKind::list_remove;
                else if (name == "clear") result->kind = hir::ExprKind::list_clear;
                else if (name == "contains") { result->kind = hir::ExprKind::list_contains; result->type = Type::boolean; }
                else throw Error("E_NAME",result->location,"Unknown list method");
                if (count) result->arguments.push_back(convert(expression(*source.arguments[0]),name == "removeat" ? Type::integer : element));
                break;
            }
            if (source.kind == SyntaxExprKind::member_call) throw Error("E_TYPE",result->location,"Record field is not callable");
            result->kind = hir::ExprKind::field;
            for (std::size_t i = 0; i < type.fields.size(); ++i) if (type.fields[i].name == source.token.text) {
                result->local = i; result->type = type.fields[i].type; return result;
            }
            throw Error("E_NAME",result->location,"Unknown record field '"+source.token.text+"'");
        }
        case SyntaxExprKind::literal:
            if (source.token.kind==TokenKind::name && source.token.text=="nil") { result->kind=hir::ExprKind::null_literal; result->type=Type::nil; break; }
            result->kind = hir::ExprKind::literal;
            result->value = literal(source.token);
            result->type = value_type(result->value);
            break;
        case SyntaxExprKind::name:
            if (const auto local = lookup(source.token)) {
                result->kind = hir::ExprKind::local;
                result->local = *local;
                result->type = module_.locals[*local].type;
            } else if (collections_imported_ && (source.token.text == "min_cap" || source.token.text == "inc_cap_factor")) {
                result->kind = hir::ExprKind::literal; result->type = Type::integer;
                result->value = std::int32_t{source.token.text == "min_cap" ? 4 : 2};
            } else if (const auto value = stdlib::constant(source.token.text)) {
                result->kind = hir::ExprKind::literal;
                result->value = *value;
                result->type = value_type(*value);
            } else if (auto routine = routine_names_.find(source.token.text); routine != routine_names_.end() && module_.routines[routine->second].signature.parameters.empty()) {
                SyntaxExpr call{SyntaxExprKind::call,source.token,{},{},1,{},{}};
                return expression(call);
            } else {
                (void)resolve(source.token);
            }
            break;
        case SyntaxExprKind::call: {
            if (lookup(source.token) && source.token.text != current_name_) throw Error("E_TYPE", result->location, "Variable is not callable: '" + source.token.text + "'");
            result->kind = hir::ExprKind::call;
            for (const auto& argument : source.arguments) result->arguments.push_back(expression(*argument));
            if (auto routine = routine_names_.find(source.token.text); routine != routine_names_.end()) {
                const auto& signature = module_.routines[routine->second].signature;
                if (signature.parameters.size() != result->arguments.size()) throw Error("E_TYPE",result->location,"Wrong routine argument count");
                result->kind = hir::ExprKind::user_call; result->local = routine->second; result->type = signature.result;
                for (std::size_t n = 0; n < result->arguments.size(); ++n) {
                    const auto& parameter = signature.parameters[n];
                    if (parameter.by_reference) {
                        if (!lvalue(*result->arguments[n]) || result->arguments[n]->type != parameter.type)
                            throw Error("E_TYPE",result->location,"var argument requires an assignable value of exactly the parameter type");
                    } else result->arguments[n] = convert(std::move(result->arguments[n]),parameter.type);
                }
                break;
            }
            if (source.token.text == "arr") {
                if (result->arguments.empty()) throw Error("E_TYPE",result->location,"Cannot infer empty Arr element type");
                auto element = result->arguments.front()->type;
                if (element == Type::unit || element == Type::nil) throw Error("E_TYPE",result->location,"Procedure has no value");
                for (const auto& argument : result->arguments) {
                    if (can_widen(element,argument->type)) element = argument->type;
                    else if (!can_widen(argument->type,element)) throw Error("E_TYPE",result->location,"Incompatible array elements");
                }
                for (auto& argument : result->arguments) argument = convert(std::move(argument),element);
                result->kind = hir::ExprKind::array_literal; result->type = sequence_type(CompoundKind::array,element); break;
            }
            if (source.token.text == "length" && result->arguments.size() == 1 && compound(result->arguments[0]->type)) {
                if (!indexed_sequence(info(result->arguments[0]->type,result->location).kind)) throw Error("E_TYPE",result->location,"Length requires an array or list");
                result->kind = hir::ExprKind::length; result->left = std::move(result->arguments[0]); result->arguments.clear(); result->type = Type::integer; break;
            }
            if (source.token.text == "readalllines") {
                if (result->arguments.size()!=1) throw Error("E_TYPE",result->location,"ReadAllLines requires one path");
                result->arguments[0] = convert(std::move(result->arguments[0]),Type::string);
                result->kind=hir::ExprKind::read_lines; result->type=sequence_type(CompoundKind::array,Type::string); break;
            }
            if (source.token.text == "writealllines") {
                if (result->arguments.size()!=2) throw Error("E_TYPE",result->location,"WriteAllLines requires a path and string array");
                result->arguments[0]=convert(std::move(result->arguments[0]),Type::string);
                const auto& type=info(result->arguments[1]->type,result->location);
                if (type.kind!=CompoundKind::array || type.element!=Type::string) throw Error("E_TYPE",result->location,"WriteAllLines requires a string array");
                result->kind=hir::ExprKind::write_lines; result->type=Type::unit; break;
            }
            if (!result->arguments.empty() && compound(result->arguments[0]->type)) {
                const auto sequence=result->arguments[0]->type; const auto& type=info(sequence,result->location); const auto& name=source.token.text;
                if (endpoint_collection(type.kind) && (name=="copy" || name=="high" || name=="sort" || name=="sortdescending" || name=="fillbyvalue"))
                    throw Error("E_TYPE",result->location,"Operation requires an array or list");
                if (indexed_sequence(type.kind) && ((name=="copy" && type.kind==CompoundKind::array) || name=="high" || name=="sort" || name=="sortdescending" || name=="fillbyvalue")) {
                    const auto count=name=="fillbyvalue" ? 2U : 1U;
                    if (result->arguments.size()!=count) throw Error("E_TYPE",result->location,"Wrong sequence argument count");
                    result->left=std::move(result->arguments[0]);
                    if (name=="copy") { result->kind=hir::ExprKind::array_copy; result->type=sequence; }
                    else if (name=="high") { result->kind=hir::ExprKind::array_high; result->type=Type::integer; }
                    else if (name=="fillbyvalue") {
                        result->kind=hir::ExprKind::sequence_fill; result->type=Type::unit;
                        result->right=convert(std::move(result->arguments[1]),type.element);
                    } else {
                        if (compound(type.element)) throw Error("E_TYPE",result->location,"Sort requires scalar elements");
                        result->kind=hir::ExprKind::sequence_sort; result->type=Type::unit; result->local=name=="sortdescending" ? 1 : 0;
                    }
                    result->arguments.clear(); break;
                }
            }
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
            if ((op=="=" || op=="<>") && (compound(left) || compound(right))) {
                if (left==Type::nil) { result->left=convert(std::move(result->left),right); left=right; }
                if (right==Type::nil) { result->right=convert(std::move(result->right),left); right=left; }
                if (left!=right) throw Error("E_TYPE",result->location,"Incompatible aggregate comparison");
                result->type=Type::boolean;
            } else if (numeric(left) && numeric(right)) {
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
            if (source.annotation) type = resolve_type(*source.annotation);
            if (type == Type::unit || type == Type::nil) throw Error("E_TYPE",source.token.location,"Procedure has no value");
            if (initial) initial = convert(std::move(initial), type);
            for (const auto& token : source.names) {
                if (scopes_.back().contains(token.text))
                    throw Error("E_DUPLICATE", token.location, "Duplicate name '" + token.text + "'");
                const auto local = module_.locals.size();
                module_.locals.push_back({token.text, type, !in_routine_});
                scopes_.back().emplace(token.text, local);
                hir::Stmt assignment;
                assignment.kind = hir::StmtKind::assign;
                assignment.location = token.location;
                assignment.local = local;
                // Initialize when the declaration is executed, including inside loops.
                if (initial) assignment.expression = std::move(initial);
                else {
                    assignment.expression = std::make_unique<hir::Expr>();
                    assignment.expression->kind = hir::ExprKind::default_value;
                    assignment.expression->type = type; assignment.expression->location = token.location;
                }
                result.statements.push_back(std::move(assignment));
            }
            break;
        }
        case SyntaxStmtKind::string_mutation: {
            if (lookup(source.names.front())) throw Error("E_TYPE", result.location, "Variable is not callable");
            const auto& name = source.names.front().text;
            const bool resize = name == "setlength", insert = name == "insert";
            const std::size_t count = resize ? 2 : 3, reference = insert ? 1 : 0;
            if (source.arguments.size() != count)
                throw Error("E_TYPE", result.location, "Wrong argument count for '" + name + "'");
            auto target = expression(*source.arguments[reference]);
            if (!lvalue(*target)) throw Error("E_TYPE",target->location,"var argument must be assignable");
            if (resize && compound(target->type) && info(target->type,result.location).kind == CompoundKind::array) {
                result.kind = hir::StmtKind::resize_array; result.target = std::move(target);
                result.expression = convert(expression(*source.arguments[1]),Type::integer); break;
            }
            if (target->type != Type::string) throw Error("E_TYPE",target->location,"String var argument must have type string");
            result.kind = hir::StmtKind::string_mutation;
            const auto function = resize ? stdlib::Function::resize_string
                : insert ? stdlib::Function::insert_string : stdlib::Function::delete_string;
            const auto catalog = stdlib::signatures();
            std::size_t intrinsic = 0;
            while (intrinsic < catalog.size() && catalog[intrinsic].function != function) ++intrinsic;
            if (intrinsic == catalog.size()) throw std::logic_error("Missing string mutation intrinsic");
            result.local=intrinsic;
            for (std::size_t i=0;i<count;++i) {
                if (i==reference) result.arguments.push_back(std::move(target));
                else result.arguments.push_back(convert(expression(*source.arguments[i]),catalog[intrinsic].parameters[i]));
            }
            break;
        }
        case SyntaxStmtKind::increment: {
            if (lookup(source.names.front())) throw Error("E_TYPE",result.location,"Variable is not callable");
            result.kind=hir::StmtKind::increment; result.target=expression(*source.target);
            const auto type=result.target->type;
            if (!lvalue(*result.target) || (type!=Type::integer && type!=Type::int64)) throw Error("E_TYPE",result.location,"Inc/Dec requires an integer variable");
            result.newline=source.names.front().text=="dec";
            if (source.expression) result.expression=convert(expression(*source.expression),type);
            else {
                result.expression=std::make_unique<hir::Expr>(); result.expression->kind=hir::ExprKind::literal;
                result.expression->type=type; result.expression->location=result.location;
                result.expression->value=type==Type::integer ? Value(std::int32_t{1}) : Value(std::int64_t{1});
            }
            break;
        }
        case SyntaxStmtKind::assignment:
            result.kind = hir::StmtKind::assign;
            result.target = expression(*source.target);
            if (result.target->kind == hir::ExprKind::buffer_call) {
                const auto method = static_cast<BufferMethod>(result.target->local);
                if (method == BufferMethod::get || method == BufferMethod::count || method == BufferMethod::capacity) {
                    auto setter = std::move(result.target);
                    setter->arguments.push_back(convert(expression(*source.expression),setter->type));
                    setter->local = static_cast<std::size_t>(method == BufferMethod::get ? BufferMethod::set
                        : method == BufferMethod::count ? BufferMethod::resize : BufferMethod::reserve);
                    setter->type = Type::unit; result.kind = hir::StmtKind::call; result.expression = std::move(setter); break;
                }
            }
            if (!lvalue(*result.target)) throw Error("E_TYPE",result.location,"Assignment target must be assignable");
            result.expression = convert(expression(*source.expression), result.target->type);
            break;
        case SyntaxStmtKind::call:
            result.kind = hir::StmtKind::call; result.expression = expression(*source.expression); break;
        case SyntaxStmtKind::return_stmt:
            result.kind = hir::StmtKind::return_stmt;
            if (source.expression) {
                if (!in_routine_ || current_result_ == Type::unit) throw Error("E_TYPE",result.location,"Procedure Exit takes no value");
                result.expression = convert(expression(*source.expression),current_result_);
            }
            break;
        case SyntaxStmtKind::print:
            result.kind = hir::StmtKind::print;
            result.newline = source.token.text == "writeln";
            for (const auto& argument : source.arguments) {
                auto value = expression(*argument);
                if (compound(value->type) || value->type == Type::unit || value->type == Type::nil) throw Error("E_TYPE",value->location,"Write requires scalar values");
                result.arguments.push_back(std::move(value));
            }
            break;
        case SyntaxStmtKind::read:
            result.kind = hir::StmtKind::read; result.target=expression(*source.target);
            if (!lvalue(*result.target) || compound(result.target->type) || result.target->type==Type::unit)
                throw Error("E_TYPE",result.location,"Readln requires a scalar variable");
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
    std::map<std::string,Type> aliases_;
    std::map<const SyntaxType*,Type> resolved_types_;
    std::map<std::string,std::size_t> routine_names_;
    bool in_routine_ = false;
    bool collections_imported_ = false;
    Type current_result_ = Type::unit;
    std::string current_name_;
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

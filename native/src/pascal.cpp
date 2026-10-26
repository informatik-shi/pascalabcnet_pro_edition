#include "hir.hpp"
#include "number.hpp"
#include "stdlib.hpp"
#include "integer_values.hpp"
#include "formatting.hpp"


#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <map>
#include <set>

namespace abc {
namespace {

enum class TokenKind { name, number, text, character_code, symbol, eof };
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
        if (peek() == '#') {
            take(); std::string number;
            while (digit(peek())) number += take();
            if (number.empty()) throw Error("E_SYNTAX",start,"Expected a character code after #");
            return {TokenKind::character_code,std::move(number),start};
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

enum class SyntaxExprKind { literal, literal_group, name, unary, binary, call, index, field, member_call, construction, formatted };
struct SyntaxType {
    Token token;
    std::string kind;
    std::shared_ptr<SyntaxType> element;
    std::vector<std::pair<Token, std::shared_ptr<SyntaxType>>> fields;
    std::shared_ptr<SyntaxType> key;
    unsigned rank = 1;
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

enum class SyntaxStmtKind { block, declaration, assignment, print, read, conditional, loop, increment, string_mutation, call, return_stmt,
    for_loop, repeat_loop, break_stmt, continue_stmt, foreach_loop };
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
    std::set<std::string> imported_units;
    bool collections_imported = false;
    bool declares_control = false, descending = false;
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
        "div", "mod", "xor", "shl", "shr", "true", "false", "uses", "unit", "type", "const", "function", "procedure",
        "for", "to", "downto", "repeat", "until", "foreach", "in", "case", "of", "array", "record", "class", "nil",
        "new", "set", "try", "except", "finally", "raise", "break", "continue", "exit", "writeln", "write", "readln", "read"};
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
                if (module.text != "pabcsystem" && module.text != "arrays" && module.text != "collections"
                    && module.text != "filesoperations" && module.text != "events" && module.text != "pointrect" && module.text != "school")
                    throw Error("E_UNSUPPORTED", module.location, "Unit is not ported yet: '" + module.text + "'");
                if (module.text == "collections") root.collections_imported = true;
                root.imported_units.insert(module.text);
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
        if (accept("array")) {
            result->kind = "array";
            if (accept("[")) {
                if (!accept(",")) throw Error("E_UNSUPPORTED",current_.location,"Fixed-bound arrays are not implemented");
                result->rank=2;
                if (is(",")) throw Error("E_UNSUPPORTED",current_.location,"Array ranks above two are not implemented");
                expect("]");
            }
            expect("of"); result->element = type_syntax();
        }
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
            if (result->kind == "assocarray" && accept("<")) {
                result->key = type_syntax(); expect(","); result->element = type_syntax(); expect(">");
            } else if ((result->kind == "list" || result->kind == "stack" || result->kind == "queue" || result->kind == "dynarray" || result->kind == "simpleset" || result->kind == "linkedlist" || result->kind == "linkedlistnode" || result->kind == "singlenode") && accept("<")) {
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
        if (is(";")) return SyntaxStmt{};
        if (is("begin")) return block();
        if (accept("var")) return declaration();
        SyntaxStmt result;
        result.token = current_;
        if (accept("exit")) {
            result.kind = SyntaxStmtKind::return_stmt;
            if (accept("(")) { if (!is(")")) result.expression = expression(); expect(")"); }
        } else if (accept("break")) {
            result.kind=SyntaxStmtKind::break_stmt;
        } else if (accept("continue")) {
            result.kind=SyntaxStmtKind::continue_stmt;
        } else if (accept("foreach")) {
            result.kind=SyntaxStmtKind::foreach_loop; result.declares_control=accept("var");
            if (is("(")) throw Error("E_UNSUPPORTED",current_.location,"Foreach tuple deconstruction is not ported yet");
            result.names.push_back(name());
            if (accept(":")) { result.annotation=type_syntax(); result.declares_control=true; }
            expect("in"); result.expression=expression();
            if (is("index")) throw Error("E_UNSUPPORTED",current_.location,"Foreach index is not ported yet");
            expect("do"); result.body=std::make_unique<SyntaxStmt>(statement());
        } else if (accept("for")) {
            result.kind=SyntaxStmtKind::for_loop; result.declares_control=accept("var");
            result.names.push_back(name());
            if (accept(":")) { result.annotation=type_syntax(); result.declares_control=true; }
            expect(":="); result.expression=expression();
            if (!is("to") && !is("downto")) fail("Expected to or downto in for loop");
            result.descending=accept("downto"); if (!result.descending) expect("to");
            result.arguments.push_back(expression());
            if (is("step")) throw Error("E_UNSUPPORTED",current_.location,"For step is not ported yet");
            expect("do"); result.body=std::make_unique<SyntaxStmt>(statement());
        } else if (accept("repeat")) {
            result.kind=SyntaxStmtKind::repeat_loop;
            while (!is("until")) {
                if (current_.kind==TokenKind::eof || is("end")) fail("Expected until after repeat body");
                if (accept(";")) continue;
                result.statements.push_back(statement());
                if (!accept(";") && !is("until")) fail("Expected ';' between repeat statements");
            }
            expect("until"); result.expression=expression();
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
                    do { result.arguments.push_back(output_argument()); } while (accept(","));
                }
                expect(")");
            }
        } else if (is("readln") || is("read")) {
            result.kind = SyntaxStmtKind::read;
            consume();
            if (accept("(")) {
                if (!is(")")) do {
                    if (result.arguments.size()>=17) fail("Too many Read arguments");
                    result.arguments.push_back(expression());
                } while (accept(","));
                expect(")");
            }
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
        if (op == "+" || op == "-" || op == "or" || op == "xor") return 2;
        if (op == "*" || op == "/" || op == "div" || op == "mod" || op == "and" || op=="shl" || op=="shr") return 3;
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
        if (accept("(")) { auto result = expression(); expect(")"); return postfix(std::move(result)); }
        if (current_.kind==TokenKind::text || current_.kind==TokenKind::character_code) {
            auto result=make_expression(SyntaxExprKind::literal,consume());
            if (current_.kind==TokenKind::text || current_.kind==TokenKind::character_code) {
                auto group=make_expression(SyntaxExprKind::literal_group,result->token);
                group->arguments.push_back(std::move(result));
                do { group->arguments.push_back(make_expression(SyntaxExprKind::literal,consume())); }
                while (current_.kind==TokenKind::text || current_.kind==TokenKind::character_code);
                result=std::move(group);
            }
            return postfix(std::move(result));
        }
        if (current_.kind == TokenKind::number || is("true") || is("false") || is("nil"))
            return postfix(make_expression(SyntaxExprKind::literal, consume()));
        if (accept("new")) {
            const auto token = current_; auto type = type_syntax();
            auto result = make_expression(SyntaxExprKind::construction, token); result->constructed = type;
            if (accept("[")) {
                result->array_construction=true;
                do {
                    if (result->arguments.size()>=2) throw Error("E_UNSUPPORTED",current_.location,"Array ranks above two are not implemented");
                    result->arguments.push_back(expression());
                } while (accept(","));
                expect("]");
            }
            else if (accept("(")) {
                if (!is(")")) do {
                    if (result->arguments.size() >= 3) fail("Constructor argument limit exceeded");
                    auto argument = expression(); result->depth = std::max(result->depth,argument->depth+1);
                    if (result->depth > 256) fail("Constructor nesting limit exceeded");
                    result->arguments.push_back(std::move(argument));
                } while (accept(","));
                expect(")");
            }
            return postfix(std::move(result));
        }
        if (current_.kind == TokenKind::name && !reserved(current_.text))
            return postfix(make_expression(SyntaxExprKind::name, consume()));
        fail("Expected an expression");
    }
    std::unique_ptr<SyntaxExpr> output_argument() {
        auto value=expression();
        if (!accept(":")) return value;
        const auto token=value->token;
        auto result=make_expression(SyntaxExprKind::formatted,token);
        result->arguments.push_back(std::move(value));
        result->arguments.push_back(expression());
        if (accept(":")) result->arguments.push_back(expression());
        for (const auto& argument:result->arguments) result->depth=std::max(result->depth,argument->depth+1);
        if (result->depth>256) fail("Formatted expression nesting limit exceeded");
        return result;
    }
    std::unique_ptr<SyntaxExpr> postfix(std::unique_ptr<SyntaxExpr> result) {
        for (;;) {
            if (accept("[")) {
                auto index = expression();
                const auto token = result->token;
                result = make_expression(SyntaxExprKind::index, token, std::move(result), std::move(index));
                while (accept(",")) {
                    if (!result->arguments.empty()) throw Error("E_UNSUPPORTED",current_.location,"Array ranks above two are not implemented");
                    result->arguments.push_back(expression());
                }
                expect("]");
            } else if (accept(".")) {
                // These standard Text methods also have statement keyword spellings.
                const auto field = is("write") || is("writeln") || is("readln") ? consume() : name();
                result = make_expression(SyntaxExprKind::field, field, std::move(result));
            } else if (accept("(")) {
                if (result->kind != SyntaxExprKind::name && result->kind != SyntaxExprKind::field) fail("Expression is not callable");
                result->kind = result->kind == SyntaxExprKind::name ? SyntaxExprKind::call : SyntaxExprKind::member_call;
                if (!is(")")) do {
                    const auto& name=result->token.text;
                    auto argument = (name=="print" || name=="println" || name=="str" || (result->kind==SyntaxExprKind::member_call && (name=="write" || name=="writeln")))
                        ? output_argument() : expression();
                    result->depth = std::max(result->depth, argument->depth + 1);
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
    if (token.kind == TokenKind::text) {
        auto text=from_utf8(token.text);
        return text.size()==1 ? Value{text.front()} : Value{std::move(text)};
    }
    if (token.kind == TokenKind::character_code) {
        std::uint32_t code=0;
        const auto parsed=std::from_chars(token.text.data(),token.text.data()+token.text.size(),code);
        if (parsed.ec!=std::errc{} || code>65535) throw Error("E_RANGE",token.location,"Character code must be in 0..65535");
        return static_cast<char16_t>(code);
    }
    if (token.text == "true" || token.text == "false") return token.text == "true";
    if (token.text.find_first_of(".eE") != std::string::npos) {
        double value = 0;
        if (!parse_real(token.text, value))
            throw Error("E_RANGE", token.location, "Real literal is out of range");
        return value;
    }
    std::int64_t value = 0;
    const auto parsed = std::from_chars(token.text.data(), token.text.data() + token.text.size(), value);
    if (parsed.ec==std::errc::result_out_of_range && !token.text.starts_with('-')) {
        std::uint64_t wide=0;
        const auto converted=std::from_chars(token.text.data(),token.text.data()+token.text.size(),wide);
        if (converted.ec==std::errc{} && converted.ptr==token.text.data()+token.text.size()) return wide;
    }
    if (parsed.ec != std::errc{} || parsed.ptr != token.text.data() + token.text.size())
        throw Error("E_RANGE", token.location, "Integer literal is outside the supported integer range");
    if (value >= std::numeric_limits<std::int32_t>::min() && value <= std::numeric_limits<std::int32_t>::max())
        return static_cast<std::int32_t>(value);
    return value;
}

class Analyzer {
public:
    hir::Module analyze(const SyntaxStmt& source) {
        collections_imported_ = source.collections_imported;
        imported_units_=source.imported_units; imported_units_.insert("pabcsystem");
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
            for (auto n = first; n < module_.locals.size(); ++n) if (!module_.locals[n].global) routine.signature.locals.push_back(n);
            scopes_.pop_back(); in_routine_ = false; current_name_.clear();
        }
        module_.body.statements.push_back(statement(source.statements.back()));
        if (module_.print_delimiter) {
            hir::Stmt initial; initial.kind=hir::StmtKind::assign; initial.local=*module_.print_delimiter;
            initial.expression=std::make_unique<hir::Expr>(); initial.expression->kind=hir::ExprKind::literal;
            initial.expression->type=Type::string; initial.expression->value=std::u16string{u" "};
            module_.body.statements.insert(module_.body.statements.begin(),std::move(initial));
        }
        scopes_.pop_back();
        if (module_.locals.size() > 4096) throw Error("E_LIMIT",{},"Local variable limit exceeded");
        return std::move(module_);
    }

private:
    std::size_t print_delimiter() {
        if (!module_.print_delimiter) {
            module_.print_delimiter=module_.locals.size();
            module_.locals.push_back({"$printdelimdefault",Type::string,true});
        }
        return *module_.print_delimiter;
    }
    Type sequence_type(CompoundKind kind, Type element, unsigned rank = 1) {
        for (std::size_t i = 0; i < module_.types.size(); ++i)
            if (module_.types[i].kind == kind && module_.types[i].element == element && module_.types[i].rank==rank) return static_cast<Type>(64+i);
        if (module_.types.size() >= 1024) throw Error("E_LIMIT",{},"Type limit exceeded");
        const auto name = kind == CompoundKind::array ? "array" : kind == CompoundKind::list ? "list"
            : kind == CompoundKind::stack ? "stack" : kind == CompoundKind::queue ? "queue"
            : kind == CompoundKind::dynarray ? "dynarray" : kind == CompoundKind::linked_node ? "linkedlistnode"
            : kind == CompoundKind::single_node ? "singlenode" : "simpleset";
        module_.types.push_back({name,kind,element,{},Type::invalid,rank});
        return static_cast<Type>(63+module_.types.size());
    }
    const TypeInfo& info(Type type, SourceLocation location) const {
        if (!compound(type)) throw Error("E_TYPE",location,"Compound type required");
        return module_.types.at(static_cast<unsigned>(type)-64);
    }
    Type association_type(Type key, Type element) {
        for (std::size_t i=0;i<module_.types.size();++i) {
            const auto& type = module_.types[i];
            if (type.kind == CompoundKind::association && type.key == key && type.element == element) return static_cast<Type>(64+i);
        }
        if (module_.types.size() >= 1024) throw Error("E_LIMIT",{},"Type limit exceeded");
        module_.types.push_back({"assocarray",CompoundKind::association,element,{},key});
        return static_cast<Type>(63+module_.types.size());
    }
    Type resolve_type(const SyntaxType& syntax) {
        if (auto found=resolved_types_.find(&syntax); found!=resolved_types_.end()) return found->second;
        if (syntax.kind == "array" || (syntax.kind == "list" && syntax.element))
            return sequence_type(syntax.kind == "array" ? CompoundKind::array : CompoundKind::list,resolve_type(*syntax.element),syntax.rank);
        if (syntax.kind == "assocarray" && syntax.key && syntax.element) {
            if (!collections_imported_) throw Error("E_UNSUPPORTED",syntax.token.location,"AssocArray requires uses Collections");
            const auto key = resolve_type(*syntax.key), element = resolve_type(*syntax.element);
            return association_type(key,element);
        }
        if (syntax.kind == "singlenode" && syntax.element) {
            if (!collections_imported_) throw Error("E_UNSUPPORTED",syntax.token.location,"SingleNode requires uses Collections");
            return sequence_type(CompoundKind::single_node,resolve_type(*syntax.element));
        }
        if ((syntax.kind == "linkedlist" || syntax.kind == "linkedlistnode") && syntax.element) {
            if (!collections_imported_) throw Error("E_UNSUPPORTED",syntax.token.location,"LinkedList requires uses Collections");
            const auto element = resolve_type(*syntax.element);
            const auto node = sequence_type(CompoundKind::linked_node,element);
            if (syntax.kind == "linkedlistnode") return node;
            for (std::size_t i=0;i<module_.types.size();++i)
                if (module_.types[i].kind == CompoundKind::linked_list && module_.types[i].element == element) return static_cast<Type>(64+i);
            if (module_.types.size() >= 1024) throw Error("E_LIMIT",{},"Type limit exceeded");
            module_.types.push_back({"linkedlist",CompoundKind::linked_list,element,{},node});
            return static_cast<Type>(63+module_.types.size());
        }
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
    bool lvalue(const hir::Expr& expression) const {
        if (expression.kind==hir::ExprKind::index && expression.left->type==Type::string) return false;
        if (expression.kind==hir::ExprKind::index && expression.left->kind==hir::ExprKind::local
            && for_controls_.contains(expression.left->local) && compound(expression.left->type)
            && indexed_sequence(info(expression.left->type,expression.location).kind)) return true;
        return (expression.kind == hir::ExprKind::local && !for_controls_.contains(expression.local))
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
        if (integer_type(type) && integer_type(expression->type) && expression->kind==hir::ExprKind::literal && !can_widen(expression->type,type)) {
            try {
                if (small_integer(type)) expression->value=checked_small_integer(type,integral_number(expression->value,expression->location,type_name(type)),expression->location);
                else if (wide_unsigned(type)) expression->value=checked_unsigned_integer(type,expression->value,expression->location);
                else expression->value=checked_signed_integer(type,expression->value,expression->location);
            } catch (const Error& error) {
                if (error.code!="E_OVERFLOW") throw;
                throw Error("E_RANGE",expression->location,"Constant is outside the range of "+std::string(type_name(type)));
            }
            expression->type=type;
            return expression;
        }
        if (expression->type == Type::nil && type == Type::string) {
            expression->type=type; expression->kind=hir::ExprKind::literal; expression->value=NilString{}; return expression;
        }
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
                result->kind = hir::ExprKind::array_new; result->type = sequence_type(CompoundKind::array,type,static_cast<unsigned>(source.arguments.size()));
                for (const auto& argument:source.arguments) result->arguments.push_back(convert(expression(*argument),Type::integer));
            } else {
                if (type==Type::text_file) {
                    if (!source.arguments.empty()) throw Error("E_TYPE",result->location,"Text constructor takes no arguments");
                    result->kind=hir::ExprKind::default_value; result->type=type; result->operation="new"; break;
                }
                if (!compound(type)) throw Error("E_TYPE",result->location,"new requires an array length, record or collection type");
                if (node_kind(info(type,result->location).kind)) {
                    const auto parameters = linked_parameters(type,info(type,result->location),LinkedMethod::construct);
                    if (source.arguments.size() != parameters.size()) throw Error("E_TYPE",result->location,"Wrong node constructor argument count");
                    result->kind = hir::ExprKind::linked_call; result->type = type;
                    result->local = static_cast<std::size_t>(LinkedMethod::construct);
                    for (std::size_t i=0;i<parameters.size();++i) result->arguments.push_back(convert(expression(*source.arguments[i]),parameters[i]));
                    break;
                }
                if (!source.arguments.empty()) {
                    if (info(type,result->location).kind != CompoundKind::dynarray || source.arguments.size() != 1)
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
                if (compound(receiver->type)) {
                    const auto kind = info(receiver->type,result->location).kind;
                    if (kind == CompoundKind::dynarray || kind == CompoundKind::association) result->left = std::move(receiver);
                }
            }
            if (!result->left) result->left = expression(*source.left);
            const auto receiver_type = result->left->type;
            const auto rank=compound(receiver_type) && info(receiver_type,result->location).kind==CompoundKind::array ? info(receiver_type,result->location).rank : 1U;
            if (source.arguments.size()+1!=rank) throw Error("E_TYPE",result->location,"Wrong number of array indices");
            if (receiver_type==Type::string) {
                result->right=convert(expression(*source.right),Type::integer);
                result->type=Type::character; result->kind=hir::ExprKind::index; break;
            }
            const auto kind = info(receiver_type,result->location).kind;
            if (kind == CompoundKind::association) {
                const auto key = info(receiver_type,result->location).key, element = info(receiver_type,result->location).element;
                result->kind = hir::ExprKind::association_call; result->type = element;
                result->local = static_cast<std::size_t>(AssociationMethod::get);
                result->arguments.push_back(convert(expression(*source.right),key)); break;
            }
            result->right = convert(expression(*source.right),Type::integer);
            for (const auto& argument:source.arguments) result->arguments.push_back(convert(expression(*argument),Type::integer));
            const auto& type = info(receiver_type,result->location);
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
            if (source.left->kind==SyntaxExprKind::name && !lookup(source.left->token) && imported_units_.contains(source.left->token.text)) {
                if (source.kind==SyntaxExprKind::field && source.left->token.text=="pabcsystem") {
                    if (source.token.text=="printdelimdefault") {
                        result->kind=hir::ExprKind::local; result->type=Type::string; result->local=print_delimiter(); break;
                    }
                    if (auto value=stdlib::constant(source.token.text)) {
                        result->kind=hir::ExprKind::literal; result->type=value_type(*value); result->value=std::move(*value); break;
                    }
                }
                result->kind=hir::ExprKind::call;
                for (const auto& argument:source.arguments) result->arguments.push_back(expression(*argument));
                if (bind_print(*result,source.token.text,source.left->token.text)) break;
                if (bind_try_read(*result,source.token.text,source.left->token.text)) break;
                if (bind_number_conversion(*result,source.token.text,source.left->token.text)) break;
                if (bind_string_read(*result,source.token.text,source.left->token.text)) break;
                if (bind_string_array(*result,source.token.text,source.left->token.text)) break;
                if (bind_school_list(*result,source.token.text,source.left->token.text)) break;
                if (bind_random_array(*result,source.token.text,source.left->token.text)) break;
                if (bind_sequence_mutation(*result,source.token.text,source.left->token.text)) break;
                if (bind_array_output(*result,source.token.text,source.left->token.text)) break;
                if (bind_array_length(*result,source.token.text,source.left->token.text)) break;
                if (bind_sequence_library(*result,source.token.text,source.left->token.text)) break;
                if (bind_object_text(*result,source.token.text,source.left->token.text)) break;
                bind_builtin(*result,source.token.text,source.left->token.text); break;
            }
            result->left = expression(*source.left); const auto receiver = result->left->type;
            if (imported_units_.contains("school") && (source.token.text=="digits" || source.token.text=="digitstoint64")) {
                result->arguments.push_back(std::move(result->left));
                for (const auto& argument:source.arguments) result->arguments.push_back(expression(*argument));
                (void)bind_school_list(*result,source.token.text,"school"); break;
            }
            if (imported_units_.contains("school") && integer_type(receiver) && (source.token.text=="tobase" || source.token.text=="perfectsquare" || source.token.text=="isprime")) {
                result->arguments.push_back(std::move(result->left));
                for (const auto& argument:source.arguments) result->arguments.push_back(expression(*argument));
                result->kind=hir::ExprKind::call; bind_builtin(*result,source.token.text,"school"); break;
            }
            if (receiver==Type::string && (source.token.text=="towords" || source.token.text=="tolines" || source.token.text=="tointegers" || source.token.text=="toreals")) {
                result->arguments.push_back(std::move(result->left));
                for (const auto& argument:source.arguments) result->arguments.push_back(expression(*argument));
                (void)bind_string_array(*result,source.token.text); break;
            }
            if (receiver==Type::string && (source.token.text=="readinteger" || source.token.text=="readreal" || source.token.text=="readword"
                || source.token.text=="tointeger" || source.token.text=="toreal" || source.token.text=="isinteger" || source.token.text=="isreal")) {
                result->arguments.push_back(std::move(result->left));
                for (const auto& argument:source.arguments) result->arguments.push_back(expression(*argument));
                if (!bind_string_read(*result,source.token.text,{},true)) {
                    result->kind=hir::ExprKind::call; bind_builtin(*result,source.token.text,"pabcsystem");
                }
                break;
            }
            if (receiver==Type::string && (source.token.text=="trytointeger" || source.token.text=="trytoreal")) {
                result->arguments.push_back(std::move(result->left));
                for (const auto& argument:source.arguments) result->arguments.push_back(expression(*argument));
                (void)bind_number_conversion(*result,source.token.text=="trytointeger" ? "trystrtoint" : "trystrtoreal"); break;
            }
            if (compound(receiver) && info(receiver,result->location).kind==CompoundKind::array
                && (source.token.text=="rowcount" || source.token.text=="colcount")) {
                if (info(receiver,result->location).rank!=2 || !source.arguments.empty()) throw Error("E_TYPE",result->location,"RowCount/ColCount require a matrix and no arguments");
                result->kind=hir::ExprKind::array_query; result->type=Type::integer; result->local=static_cast<std::size_t>(ArrayQuery::length);
                result->right=std::make_unique<hir::Expr>(); result->right->kind=hir::ExprKind::literal; result->right->type=Type::integer;
                result->right->value=std::int32_t{source.token.text=="rowcount" ? 0 : 1}; result->right->location=result->location; break;
            }
            if (compound(receiver) && info(receiver,result->location).kind==CompoundKind::array
                && (source.token.text=="copy" || source.token.text=="high" || source.token.text=="sort" || source.token.text=="sortdescending")) {
                const bool sort=source.token.text=="sort" || source.token.text=="sortdescending";
                const auto& type=info(receiver,result->location);
                if (!source.arguments.empty() || (sort && (type.rank!=1 || !scalar(type.element)))) throw Error("E_TYPE",result->location,"Wrong array operation arguments");
                result->kind=source.token.text=="copy" ? hir::ExprKind::array_copy : source.token.text=="high" ? hir::ExprKind::array_high : hir::ExprKind::sequence_sort;
                result->type=source.token.text=="copy" ? receiver : source.token.text=="high" ? Type::integer : Type::unit;
                result->local=source.token.text=="sortdescending" ? 1U : 0U; break;
            }
            if (compound(receiver) && info(receiver,result->location).kind==CompoundKind::array
                && (source.token.text=="getlength" || source.token.text=="getupperbound" || source.token.text=="getlowerbound" || source.token.text=="rank")) {
                const bool rank=source.token.text=="rank";
                if (source.arguments.size()!=(rank ? 0U : 1U) || (rank && source.kind==SyntaxExprKind::member_call))
                    throw Error("E_TYPE",result->location,"Wrong array query arguments");
                result->kind=hir::ExprKind::array_query; result->type=Type::integer;
                result->local=static_cast<std::size_t>(rank ? ArrayQuery::rank : source.token.text=="getupperbound" ? ArrayQuery::upper_bound
                    : source.token.text=="getlowerbound" ? ArrayQuery::lower_bound : ArrayQuery::length);
                if (!rank) result->right=convert(expression(*source.arguments[0]),Type::integer);
                break;
            }
            if (compound(receiver) && info(receiver,result->location).kind==CompoundKind::array && imported_units_.contains("arrays")
                && (source.token.text=="write" || source.token.text=="writeln")) {
                result->arguments.push_back(std::move(result->left));
                for (const auto& argument:source.arguments) result->arguments.push_back(expression(*argument));
                (void)bind_array_output(*result,source.token.text,"arrays",true); break;
            }
            if (compound(receiver) && indexed_sequence(info(receiver,result->location).kind)
                && (source.token.text=="shuffle" || (source.token.text=="reverse" && info(receiver,result->location).kind==CompoundKind::list)
                    || (source.token.text=="swap" && info(receiver,result->location).kind==CompoundKind::array))) {
                result->arguments.push_back(std::move(result->left));
                for (const auto& argument:source.arguments) result->arguments.push_back(expression(*argument));
                (void)bind_sequence_mutation(*result,source.token.text,{},true); break;
            }
            if (compound(receiver) && info(receiver,result->location).kind==CompoundKind::array && source.token.text=="fillrandom") {
                result->arguments.push_back(std::move(result->left));
                for (const auto& argument:source.arguments) result->arguments.push_back(expression(*argument));
                (void)bind_random_array(*result,"fillrandom"); break;
            }
            if ((receiver==Type::string && source.token.text=="length") || (receiver==Type::character && source.token.text=="todigit")) {
                if (!source.arguments.empty() || (receiver==Type::string && source.kind==SyntaxExprKind::member_call))
                    throw Error("E_TYPE",result->location,"Wrong scalar member arguments");
                const auto name=receiver==Type::string ? "$string_length" : "todigit";
                const auto catalog=stdlib::signatures();
                const auto found=std::find_if(catalog.begin(),catalog.end(),[&](const auto& signature){return signature.name==name;});
                if (found==catalog.end()) throw Error("E_HIR",result->location,"Missing scalar member signature");
                result->kind=hir::ExprKind::call; result->type=found->result;
                result->local=static_cast<std::size_t>(found-catalog.begin());
                result->arguments.push_back(std::move(result->left)); break;
            }
            if (receiver==Type::text_file) {
                const auto& name=source.token.text;
                if (name=="write" || name=="writeln" || name=="print" || name=="println") {
                    if (name=="print" || name=="println") (void)print_delimiter();
                    result->kind=hir::ExprKind::file_output; result->type=Type::unit;
                    result->local=static_cast<std::size_t>(name=="write" ? TextOutput::write : name=="writeln" ? TextOutput::writeln
                        : name=="print" ? TextOutput::print : TextOutput::println);
                    for (const auto& argument:source.arguments) {
                        auto value=expression(*argument);
                        if (value->type==Type::nil) value=convert(std::move(value),Type::string);
                        if (!object_text_supported(module_.types,value->type)) throw Error("E_TYPE",value->location,"Text output for this static type is not implemented");
                        result->arguments.push_back(std::move(value));
                    }
                    break;
                }
                static const std::set<std::string> methods={"readinteger","readreal","readstring","readboolean","readword","readchar",
                    "readlninteger","readlnreal","readlnstring","readlnboolean","readlnchar","readln","reset","rewrite","append",
                    "eof","eoln","seekeof","seekeoln","close","flush","name","fullname","readtoend","erase","rename"};
                if (!methods.contains(name)) throw Error("E_NAME",result->location,"Text member is not ported: '"+name+"'");
                const auto count=name=="rename" ? 1U : 0U;
                if (source.arguments.size()!=count) throw Error("E_TYPE",result->location,"Wrong Text method argument count");
                const std::string_view target=name=="readword" ? "readlexem" : name=="readln" ? "$text_skip_line"
                    : name=="name" ? "$text_name" : name=="fullname" ? "$text_full_name" : name=="readtoend" ? "$text_read_end" : std::string_view(name);
                const auto catalog=stdlib::signatures();
                const auto found=std::find_if(catalog.begin(),catalog.end(),[&](const auto& signature) {
                    return signature.name==target && signature.parameters.size()==count+1 && signature.parameters[0]==Type::text_file;
                });
                if (found==catalog.end()) throw Error("E_HIR",result->location,"Missing Text method signature");
                result->kind=hir::ExprKind::call; result->type=found->result;
                result->local=static_cast<std::size_t>(found-catalog.begin());
                result->arguments.push_back(std::move(result->left));
                for (std::size_t n=0;n<source.arguments.size();++n)
                    result->arguments.push_back(convert(expression(*source.arguments[n]),found->parameters[n+1]));
                break;
            }
            const auto& type = info(receiver,result->location);
            if (type.kind == CompoundKind::linked_list || node_kind(type.kind)) {
                const auto& name = source.token.text; const auto kind = type.kind;
                LinkedMethod method; bool property = false;
                if (node_kind(kind)) {
                    property = true;
                    if ((name == "value" && kind == CompoundKind::linked_node) || (name == "data" && kind == CompoundKind::single_node)) method = LinkedMethod::value;
                    else if (name == "prev" && kind == CompoundKind::linked_node) method = LinkedMethod::prev;
                    else if (name == "next") method = LinkedMethod::next;
                    else throw Error("E_NAME",result->location,"Unknown linked node member '"+name+"'");
                } else {
                    if (name == "first") { method = LinkedMethod::first; property = true; }
                    else if (name == "last") { method = LinkedMethod::last; property = true; }
                    else if (name == "addfirst") method = LinkedMethod::add_first;
                    else if (name == "addlast") method = LinkedMethod::add_last;
                    else if (name == "removefirst") method = LinkedMethod::remove_first;
                    else if (name == "removelast") method = LinkedMethod::remove_last;
                    else if (name == "addbefore") method = LinkedMethod::add_before;
                    else if (name == "addafter") method = LinkedMethod::add_after;
                    else if (name == "remove") method = LinkedMethod::remove;
                    else if (name == "tostring") method = LinkedMethod::to_string;
                    else if (name == "print") method = LinkedMethod::print;
                    else if (name == "println") method = LinkedMethod::println;
                    else throw Error("E_NAME",result->location,"Unknown linked list member '"+name+"'");
                }
                if (property && source.kind == SyntaxExprKind::member_call) throw Error("E_TYPE",result->location,"Linked property is not callable");
                if (linked_text_method(method) && !scalar(type.element)) throw Error("E_UNSUPPORTED",result->location,"Linked list text output currently requires scalar elements");
                const auto parameters = linked_parameters(receiver,type,method);
                result->kind = hir::ExprKind::linked_call; result->local = static_cast<std::size_t>(method);
                result->type = linked_result(receiver,type,method);
                if (source.arguments.size() != parameters.size()) throw Error("E_TYPE",result->location,"Wrong linked method argument count");
                for (std::size_t i=0;i<parameters.size();++i) result->arguments.push_back(convert(expression(*source.arguments[i]),parameters[i]));
                break;
            }
            if (type.kind == CompoundKind::association) {
                const auto& name = source.token.text;
                AssociationMethod method;
                if (name == "tostring") method = AssociationMethod::to_string;
                else if (name == "print") method = AssociationMethod::print;
                else if (name == "println") method = AssociationMethod::println;
                else throw Error("E_NAME",result->location,"Unknown AssocArray member '"+name+"'");
                if (!source.arguments.empty()) throw Error("E_TYPE",result->location,"AssocArray text output takes no arguments");
                if (!scalar(type.key) || !scalar(type.element)) throw Error("E_UNSUPPORTED",result->location,"AssocArray text output currently requires scalar keys and values");
                result->kind = hir::ExprKind::association_call; result->local = static_cast<std::size_t>(method);
                result->type = association_result(type,method); break;
            }
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
                if (method >= BufferMethod::to_string && !scalar(element)) throw Error("E_UNSUPPORTED",result->location,"Collection text output currently requires scalar elements");
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
                if (method >= EndpointMethod::to_string && !scalar(element))
                    throw Error("E_UNSUPPORTED",result->location,"Collection text output currently requires scalar elements");
                result->kind = hir::ExprKind::endpoint_call; result->local = static_cast<std::size_t>(method);
                result->type = endpoint_result(element,method);
                if (count) result->arguments.push_back(convert(expression(*source.arguments[0]),element));
                break;
            }
            if (type.kind != CompoundKind::record) {
                if ((source.token.text == "length" && type.kind == CompoundKind::array) || (source.token.text == "count" && type.kind == CompoundKind::list)) {
                    if (!source.arguments.empty() || (type.kind==CompoundKind::array && source.kind==SyntaxExprKind::member_call)) throw Error("E_TYPE",result->location,"Property takes no arguments");
                    result->kind = hir::ExprKind::length; result->type = Type::integer; result->local=1; break;
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
        case SyntaxExprKind::literal_group: {
            std::u16string text;
            for (const auto& part:source.arguments) {
                auto value=literal(part->token);
                if (const auto* ch=std::get_if<char16_t>(&value)) text+=*ch;
                else text+=std::get<std::u16string>(value);
            }
            result->kind=hir::ExprKind::literal; result->type=Type::string; result->value=std::move(text); break;
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
            } else if (auto routine = routine_names_.find(source.token.text); routine != routine_names_.end()) {
                if (!module_.routines[routine->second].signature.parameters.empty())
                    throw Error("E_TYPE",result->location,"Routine requires arguments; routine values are not supported yet");
                SyntaxExpr call{SyntaxExprKind::call,source.token,{},{},1,{},{}};
                return expression(call);
            } else if (source.token.text=="printdelimdefault") {
                result->kind=hir::ExprKind::local; result->type=Type::string; result->local=print_delimiter();
            } else if (source.token.text=="print" || source.token.text=="println") {
                SyntaxExpr call{SyntaxExprKind::call,source.token,{},{},1,{},{}};
                return expression(call);
            } else if (collections_imported_ && (source.token.text == "min_cap" || source.token.text == "inc_cap_factor")) {
                result->kind = hir::ExprKind::literal; result->type = Type::integer;
                result->value = std::int32_t{source.token.text == "min_cap" ? 4 : 2};
            } else if (const auto value = stdlib::constant(source.token.text)) {
                result->kind = hir::ExprKind::literal;
                result->value = *value;
                result->type = value_type(*value);
            } else if (source.token.text=="arrrandom" || source.token.text=="arrrandominteger" || source.token.text=="arrrandomreal" || source.token.text=="matrrandom" || source.token.text=="matrrandominteger" || source.token.text=="matrrandomreal") {
                SyntaxExpr call{SyntaxExprKind::call,source.token,{},{},1,{},{}};
                return expression(call);
            } else if (std::any_of(stdlib::signatures().begin(),stdlib::signatures().end(),[&](const stdlib::Signature& signature) {
                return signature.name==source.token.text && signature.parameters.empty();
            })) {
                SyntaxExpr call{SyntaxExprKind::call,source.token,{},{},1,{},{}};
                return expression(call);
            } else {
                (void)resolve(source.token);
            }
            break;
        case SyntaxExprKind::formatted: {
            result->kind=hir::ExprKind::call;
            for (const auto& argument:source.arguments) result->arguments.push_back(expression(*argument));
            if (result->arguments.size()==3 && result->arguments[0]->type!=Type::real)
                throw Error("E_TYPE",result->location,"Precision in a formatted expression requires a real value");
            if (!bind_object_text(*result,"formatvalue")) bind_builtin(*result,"formatvalue","pabcsystem");
            break;
        }
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
            if (bind_print(*result,source.token.text)) break;
            if (bind_try_read(*result,source.token.text)) break;
            if (bind_number_conversion(*result,source.token.text)) break;
            if (bind_string_read(*result,source.token.text)) break;
            if (bind_string_array(*result,source.token.text)) break;
            if (bind_school_list(*result,source.token.text)) break;
            if (bind_random_array(*result,source.token.text)) break;
            if (bind_sequence_mutation(*result,source.token.text)) break;
            if (bind_array_output(*result,source.token.text)) break;
            if (bind_array_length(*result,source.token.text)) break;
            if (bind_object_text(*result,source.token.text)) break;
            if (source.token.text == "arr") {
                if (result->arguments.empty()) throw Error("E_TYPE",result->location,"Cannot infer empty Arr element type");
                auto element = Type::nil;
                for (const auto& argument : result->arguments) if (argument->type != Type::nil) { element=argument->type; break; }
                if (element == Type::unit || element == Type::nil) throw Error("E_TYPE",result->location,"Procedure has no value");
                for (const auto& argument : result->arguments) {
                    if (argument->type == Type::nil) continue;
                    if ((element==Type::uint64 && integer_type(argument->type)) || (argument->type==Type::uint64 && integer_type(element))) {
                        element=Type::uint64; continue;
                    }
                    if (can_widen(element,argument->type)) element = argument->type;
                    else if (!can_widen(argument->type,element)) {
                        bool joined=false;
                        if (numeric(element) && numeric(argument->type))
                            for (auto type:{Type::byte,Type::shortint,Type::smallint,Type::word,Type::integer,Type::int64,Type::longword,Type::uint64,Type::real})
                                if (can_widen(element,type) && can_widen(argument->type,type)) { element=type; joined=true; break; }
                        if (!joined) throw Error("E_TYPE",result->location,"Incompatible array elements");
                    }
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
                if (type.kind!=CompoundKind::array || type.rank!=1 || type.element!=Type::string) throw Error("E_TYPE",result->location,"WriteAllLines requires a string array");
                result->kind=hir::ExprKind::write_lines; result->type=Type::unit; break;
            }
            if (bind_sequence_library(*result,source.token.text)) break;
            bind_builtin(*result,source.token.text);
            break;
        }
        case SyntaxExprKind::unary:
            result->kind = hir::ExprKind::unary;
            result->left = expression(*source.left);
            if (source.token.text=="-" && small_integer(result->left->type)) result->left=convert(std::move(result->left),Type::integer);
            if (source.token.text=="-" && result->left->type==Type::longword) result->left=convert(std::move(result->left),Type::int64);
            result->type = result->left->type;
            if (source.token.text == "not" ? result->type != Type::boolean && !integer_type(result->type) : !numeric(result->type))
                throw Error("E_TYPE", source.token.location, "Invalid operand for '" + source.token.text + "'");
            break;
        case SyntaxExprKind::binary: {
            result->kind = hir::ExprKind::binary;
            result->left = expression(*source.left);
            result->right = expression(*source.right);
            auto left = result->left->type;
            auto right = result->right->type;
            const auto& op = result->operation;
            if ((op=="and" || op=="or" || op=="xor") && left==Type::boolean && right==Type::boolean) {
                result->type = Type::boolean;
                break;
            }
            if (op=="shl" || op=="shr") {
                if (!integer_type(left) || !integer_type(right)) throw Error("E_TYPE",result->location,"Shift operands must be integers");
                const auto type=small_integer(left) ? Type::integer : left;
                result->left=convert(std::move(result->left),type); result->right=convert(std::move(result->right),Type::integer);
                result->type=type; break;
            }
            if (op=="and" || op=="or" || op=="xor") {
                if (!integer_type(left) || !integer_type(right)) throw Error("E_TYPE",result->location,"Bitwise operands must be integers of compatible types");
                if (left==Type::uint64 && result->right->kind==hir::ExprKind::literal) { result->right=convert(std::move(result->right),left); right=left; }
                if (right==Type::uint64 && result->left->kind==hir::ExprKind::literal) { result->left=convert(std::move(result->left),right); left=right; }
                auto type=Type::invalid;
                for (auto candidate:{Type::byte,Type::shortint,Type::smallint,Type::word,Type::integer,Type::longword,Type::int64,Type::uint64})
                    if (can_widen(left,candidate) && can_widen(right,candidate)) { type=candidate; break; }
                if (type==Type::invalid) throw Error("E_TYPE",result->location,"No common integer type for bitwise operation");
                result->left=convert(std::move(result->left),type); result->right=convert(std::move(result->right),type);
                result->type=type; break;
            }
            const bool comparison = op == "=" || op == "<>" || op == "<" || op == "<=" || op == ">" || op == ">=";
            if (op=="*" && (((left==Type::string || left==Type::character) && (right==Type::integer || small_integer(right)))
                || ((right==Type::string || right==Type::character) && (left==Type::integer || small_integer(left))))) {
                const auto left_type=left==Type::integer || small_integer(left) ? Type::integer : Type::string;
                const auto right_type=left_type==Type::integer ? Type::string : Type::integer;
                result->arguments.push_back(convert(std::move(result->left),left_type));
                result->arguments.push_back(convert(std::move(result->right),right_type));
                const auto catalog=stdlib::signatures();
                const auto found=std::find_if(catalog.begin(),catalog.end(),[&](const auto& signature) {
                    return signature.name=="$repeat" && signature.parameters[0]==left_type;
                });
                if (found==catalog.end()) throw Error("E_HIR",result->location,"Missing string repetition signature");
                result->kind=hir::ExprKind::call; result->type=Type::string;
                result->local=static_cast<std::size_t>(found-catalog.begin()); break;
            }
            if ((op == "+" || comparison) && (left == Type::string || right == Type::string || (op=="+" && (left==Type::character || right==Type::character)))) {
                if (left == Type::nil) { result->left=convert(std::move(result->left),Type::string); left=Type::string; }
                if (right == Type::nil) { result->right=convert(std::move(result->right),Type::string); right=Type::string; }
                if (left==Type::character) { result->left=convert(std::move(result->left),Type::string); left=Type::string; }
                if (right==Type::character) { result->right=convert(std::move(result->right),Type::string); right=Type::string; }
            }
            if ((op=="=" || op=="<>") && (compound(left) || compound(right))) {
                if (left==Type::nil) { result->left=convert(std::move(result->left),right); left=right; }
                if (right==Type::nil) { result->right=convert(std::move(result->right),left); right=left; }
                if (left!=right) throw Error("E_TYPE",result->location,"Incompatible aggregate comparison");
                result->type=Type::boolean;
            } else if (numeric(left) && numeric(right)) {
                if ((op == "div" || op == "mod") && (left == Type::real || right == Type::real))
                    throw Error("E_TYPE", result->location, "div and mod require integer operands");
                const auto common = op == "/" || left == Type::real || right == Type::real ? Type::real
                    : left==Type::uint64 || right==Type::uint64 ? Type::uint64
                    : left == Type::int64 || right == Type::int64 || left==Type::longword || right==Type::longword ? Type::int64 : Type::integer;
                result->left = convert(std::move(result->left), common);
                result->right = convert(std::move(result->right), common);
                result->type = comparison ? Type::boolean : common;
            } else if (left == Type::string && right == Type::string && (op == "+" || comparison)) {
                result->type = comparison ? Type::boolean : Type::string;
            } else if (left==Type::character && right==Type::character && comparison) {
                result->type=Type::boolean;
            } else if (left == Type::boolean && right == Type::boolean && comparison) {
                result->type = Type::boolean;
            } else throw Error("E_TYPE", result->location, "Incompatible operands for '" + op + "': "
                + std::string(type_name(left)) + " and " + std::string(type_name(right)));
            break;
        }
        }
        return result;
    }
    bool bind_sequence_library(hir::Expr& result, std::string_view name, std::string_view unit = {}) {
        const bool sort=name=="sort" || name=="sortdescending", fill=name=="fillbyvalue";
        if ((!sort && !fill && name!="copy" && name!="high") || result.arguments.empty() || !compound(result.arguments[0]->type)) return false;
        if (!unit.empty() && !(unit=="pabcsystem" && !fill) && !(unit=="arrays" && (sort || fill))) return false;
        const auto sequence=result.arguments[0]->type; const auto& type=info(sequence,result.location);
        if (!indexed_sequence(type.kind) || (name=="copy" && type.kind!=CompoundKind::array) || (unit=="arrays" && type.kind!=CompoundKind::array))
            throw Error("E_TYPE",result.location,"Operation requires an array or list");
        if (result.arguments.size()!=(fill ? 2U : 1U) || (sort && (type.rank!=1 || !scalar(type.element))))
            throw Error("E_TYPE",result.location,"Wrong sequence arguments");
        result.left=std::move(result.arguments[0]);
        if (name=="copy") { result.kind=hir::ExprKind::array_copy; result.type=sequence; }
        else if (name=="high") { result.kind=hir::ExprKind::array_high; result.type=Type::integer; }
        else if (fill) {
            result.kind=hir::ExprKind::sequence_fill; result.type=Type::unit; result.right=convert(std::move(result.arguments[1]),type.element);
        } else { result.kind=hir::ExprKind::sequence_sort; result.type=Type::unit; result.local=name=="sortdescending" ? 1U : 0U; }
        result.arguments.clear(); return true;
    }
    bool bind_array_length(hir::Expr& result, std::string_view name, std::string_view unit = {}) {
        const bool rows=name=="rowcount" || name=="colcount";
        if ((name!="length" && !rows) || (!unit.empty() && unit!="pabcsystem") || result.arguments.empty() || !compound(result.arguments[0]->type)) return false;
        const auto& type=info(result.arguments[0]->type,result.location);
        if (rows) {
            if (result.arguments.size()!=1 || type.kind!=CompoundKind::array || type.rank!=2) throw Error("E_TYPE",result.location,"RowCount/ColCount require a matrix");
            result.left=std::move(result.arguments[0]); result.kind=hir::ExprKind::array_query; result.type=Type::integer; result.local=static_cast<std::size_t>(ArrayQuery::length);
            result.right=std::make_unique<hir::Expr>(); result.right->kind=hir::ExprKind::literal; result.right->type=Type::integer;
            result.right->value=std::int32_t{name=="rowcount" ? 0 : 1}; result.right->location=result.location;
            result.arguments.clear(); return true;
        }
        if (!indexed_sequence(type.kind) || (result.arguments.size()!=1 && (result.arguments.size()!=2 || type.kind!=CompoundKind::array)))
            throw Error("E_TYPE",result.location,"Length requires an array/list and an optional array dimension");
        result.left=std::move(result.arguments[0]); result.type=Type::integer;
        if (result.arguments.size()==1) { result.kind=hir::ExprKind::length; result.local=0; }
        else {
            result.kind=hir::ExprKind::array_query; result.local=static_cast<std::size_t>(ArrayQuery::length_or_zero);
            result.right=convert(std::move(result.arguments[1]),Type::integer);
        }
        result.arguments.clear(); return true;
    }
    bool bind_school_list(hir::Expr& result, std::string_view name, std::string_view unit = {}) {
        if (name!="digits" && name!="digitstoint64" && name!="primes" && name!="firstprimes") return false;
        if ((!unit.empty() && unit!="school") || !imported_units_.contains("school")) return false;
        const bool digits=name=="digits" || name=="digitstoint64";
        if (result.arguments.empty() || result.arguments.size()>2 || (name=="firstprimes" && result.arguments.size()!=1))
            throw Error("E_TYPE",result.location,"Wrong School list argument count");
        if (digits && result.arguments.size()==1) {
            auto base=std::make_unique<hir::Expr>(); base->kind=hir::ExprKind::literal; base->type=Type::integer;
            base->value=std::int32_t{10}; base->location=result.location; result.arguments.push_back(std::move(base));
        }
        const auto method=name=="digits" ? SchoolList::digits : name=="digitstoint64" ? SchoolList::digits_to_int64
            : name=="firstprimes" ? SchoolList::first_primes : result.arguments.size()==2 ? SchoolList::primes_range : SchoolList::primes;
        const auto list=sequence_type(CompoundKind::list,Type::integer);
        const auto parameters=school_list_parameters(method,list);
        for (std::size_t i=0;i<parameters.size();++i) result.arguments[i]=convert(std::move(result.arguments[i]),parameters[i]);
        result.kind=hir::ExprKind::school_list; result.local=static_cast<std::size_t>(method); result.type=school_list_result(method,list); return true;
    }
    bool bind_string_array(hir::Expr& result, std::string_view name, std::string_view unit = {}) {
        if ((name!="towords" && name!="tolines" && name!="tointegers" && name!="toreals") || (!unit.empty() && unit!="pabcsystem")) return false;
        if (result.arguments.empty()) throw Error("E_TYPE",result.location,"String array conversion requires a string");
        auto args=std::move(result.arguments); result.arguments.clear(); result.left=convert(std::move(args[0]),Type::string);
        StringArray method;
        if (name=="towords") {
            method=StringArray::words;
            if (args.size()==1) {
                result.right=std::make_unique<hir::Expr>(); result.right->kind=hir::ExprKind::literal;
                result.right->type=Type::string; result.right->value=std::u16string{u" "}; result.right->location=result.location;
            } else if (args.size()==2 && args[1]->type==Type::string) result.right=std::move(args[1]);
            else if (args.size()==2 && compound(args[1]->type)) {
                const auto& type=info(args[1]->type,result.location);
                if (type.kind!=CompoundKind::array || type.rank!=1 || type.element!=Type::character) throw Error("E_TYPE",result.location,"ToWords delimiter requires a string or char array");
                result.right=std::move(args[1]); method=StringArray::words_array;
            } else {
                if (args.size()==2 && args[1]->type==Type::nil) throw Error("E_TYPE",result.location,"ToWords nil delimiter requires an explicit string or char-array type");
                result.right=std::make_unique<hir::Expr>(); result.right->kind=hir::ExprKind::array_literal;
                result.right->type=sequence_type(CompoundKind::array,Type::character); result.right->location=result.location;
                for (std::size_t i=1;i<args.size();++i) result.right->arguments.push_back(convert(std::move(args[i]),Type::character));
                method=StringArray::words_array;
            }
        } else {
            if (args.size()>2 || (name=="tolines" && args.size()!=1)) throw Error("E_TYPE",result.location,"Wrong string array conversion argument count");
            method=name=="tolines" ? StringArray::lines : name=="tointegers" ? StringArray::integers : StringArray::reals;
            if (args.size()==2) {
                const bool count=name=="tointegers";
                result.right=convert(std::move(args[1]),count ? Type::integer : Type::string);
                method=count ? StringArray::integers_count : StringArray::reals_separator;
            }
        }
        result.local=static_cast<std::size_t>(method); result.type=sequence_type(CompoundKind::array,string_array_element(method));
        result.kind=hir::ExprKind::string_array; return true;
    }
    bool bind_string_read(hir::Expr& result, std::string_view name, std::string_view unit = {}, bool member = false) {
        if (!unit.empty() && unit!="pabcsystem") return false;
        std::optional<StringRead> method;
        if (member || result.arguments.size()==2) {
            if (name=="readinteger") method=StringRead::member_integer;
            else if (name=="readreal") method=StringRead::member_real;
            else if (name=="readword") method=StringRead::member_word;
        }
        for (const auto& [key,value]:{std::pair<std::string_view,StringRead>{"readintegerfromstring",StringRead::integer},
            {"readrealfromstring",StringRead::real},{"readwordfromstring",StringRead::word},{"stringisempty",StringRead::is_empty},
            {"tryreadintegerfromstring",StringRead::try_integer},{"tryreadrealfromstring",StringRead::try_real}}) if (key==name) method=value;
        if (!method) return false;
        if (result.arguments.size()!=(string_read_try(*method) ? 3U : 2U)) throw Error("E_TYPE",result.location,"Wrong string reading argument count");
        auto source=convert(std::move(result.arguments[0]),Type::string), cursor=std::move(result.arguments[1]);
        if (!lvalue(*cursor) || cursor->type!=Type::integer) throw Error("E_TYPE",result.location,"String cursor must be a writable integer");
        if (string_read_try(*method)) {
            auto output=std::move(result.arguments[2]);
            if (!lvalue(*output) || output->type!=string_read_number(*method)) throw Error("E_TYPE",result.location,"Wrong string reading result type");
            result.arguments.clear(); result.arguments.push_back(std::move(output));
        } else result.arguments.clear();
        result.left=std::move(source); result.right=std::move(cursor); result.local=static_cast<std::size_t>(*method);
        result.kind=hir::ExprKind::string_read; result.type=string_read_result(*method); return true;
    }
    bool bind_number_conversion(hir::Expr& result, std::string_view name, std::string_view unit = {}) {
        const bool str=name=="str", val=name=="val";
        if ((!str && !val && name!="trystrtoint" && name!="trystrtoint64" && name!="trystrtoreal" && name!="trystrtofloat")
            || (!unit.empty() && unit!="pabcsystem")) return false;
        if (result.arguments.size()!=(val ? 3U : 2U)) throw Error("E_TYPE",result.location,"Wrong string conversion argument count");
        auto source=std::move(result.arguments[0]), target=std::move(result.arguments[1]);
        if (!lvalue(*target)) throw Error("E_TYPE",result.location,"String conversion requires a writable destination");
        if (str) {
            if (target->type!=Type::string) throw Error("E_TYPE",result.location,"Str destination must have string type");
            if (source->type==Type::character || source->type==Type::nil) source=convert(std::move(source),Type::string);
            if (small_integer(source->type)) source=convert(std::move(source),Type::integer);
            if (!numeric(source->type) && source->type!=Type::string) throw Error("E_TYPE",result.location,"Str requires a number or string");
        } else {
            source=convert(std::move(source),Type::string);
            const auto expected=name=="trystrtoint" ? Type::integer : name=="trystrtoint64" ? Type::int64 : Type::real;
            if (!numeric(target->type) || (!val && target->type!=expected)) throw Error("E_TYPE",result.location,"Wrong numeric conversion destination type");
        }
        if (val) {
            auto& code=result.arguments[2];
            if (!lvalue(*code) || code->type!=Type::integer) throw Error("E_TYPE",result.location,"Val error destination must be a writable integer");
            auto error=std::move(code); result.arguments.clear(); result.arguments.push_back(std::move(error));
        } else result.arguments.clear();
        result.kind=hir::ExprKind::number_conversion; result.local=str ? 0U : val ? 2U : 1U;
        result.left=std::move(source); result.right=std::move(target); result.type=str || val ? Type::unit : Type::boolean; return true;
    }
    bool bind_try_read(hir::Expr& result, std::string_view name, std::string_view unit = {}) {
        if (name!="tryread" || (!unit.empty() && unit!="pabcsystem")) return false;
        if (result.arguments.empty() || result.arguments.size()>2) throw Error("E_TYPE",result.location,"TryRead requires a writable number/boolean and optional message");
        auto& target=result.arguments[0];
        if (!lvalue(*target) || (!numeric(target->type) && target->type!=Type::boolean)) throw Error("E_TYPE",result.location,"TryRead requires a writable number/boolean");
        result.left=std::move(target);
        if (result.arguments.size()==2) result.right=convert(std::move(result.arguments[1]),Type::string);
        else {
            result.right=std::make_unique<hir::Expr>(); result.right->kind=hir::ExprKind::literal;
            result.right->type=Type::string; result.right->value=std::u16string{}; result.right->location=result.location;
        }
        result.arguments.clear(); result.kind=hir::ExprKind::try_read; result.type=Type::boolean; return true;
    }
    bool bind_print(hir::Expr& result, std::string_view name, std::string_view unit = {}) {
        if ((name!="print" && name!="println") || (!unit.empty() && unit!="pabcsystem")) return false;
        (void)print_delimiter();
        if (!result.arguments.empty() && result.arguments[0]->type==Type::text_file) {
            result.left=std::move(result.arguments[0]); result.arguments.erase(result.arguments.begin());
        }
        for (auto& value:result.arguments) {
            if (value->type==Type::nil) value=convert(std::move(value),Type::string);
            if (!object_text_supported(module_.types,value->type)) throw Error("E_TYPE",value->location,"Print for this static type is not implemented");
        }
        result.kind=hir::ExprKind::file_output; result.type=Type::unit;
        result.local=static_cast<std::size_t>(name=="print" ? TextOutput::print : TextOutput::println); return true;
    }
    bool bind_object_text(hir::Expr& result, std::string_view name, std::string_view unit = {}) {
        if ((name!="objecttostring" && name!="formatvalue") || (!unit.empty() && unit!="pabcsystem")) return false;
        const bool width=name=="formatvalue";
        if (width && !result.arguments.empty() && scalar(result.arguments[0]->type)) return false;
        if (result.arguments.size()!=(width ? 2U : 1U)) throw Error("E_TYPE",result.location,"Wrong object formatting argument count");
        auto value=std::move(result.arguments[0]);
        if (value->type==Type::nil) value=convert(std::move(value),Type::string);
        if (!object_text_supported(module_.types,value->type)) throw Error("E_UNSUPPORTED",result.location,"Formatting for this static type is not implemented");
        result.left=std::move(value);
        if (width) result.right=convert(std::move(result.arguments[1]),Type::integer);
        result.arguments.clear(); result.kind=hir::ExprKind::object_text; result.type=Type::string; result.local=width ? 1U : 0U;
        return true;
    }
    bool bind_array_output(hir::Expr& result, std::string_view name, std::string_view unit = {}, bool member = false) {
        if ((member ? name!="write" && name!="writeln" : name!="writearray" && name!="writelnarray" && name!="readarray" && name!="writematrix")
            || (unit.empty() ? !imported_units_.contains("arrays") : unit!="arrays")) return false;
        if (name=="writematrix") {
            if (result.arguments.empty() || result.arguments.size()>3) throw Error("E_TYPE",result.location,"WriteMatrix requires a matrix, optional width and real precision");
            auto args=std::move(result.arguments); result.arguments.clear(); result.left=std::move(args[0]);
            const auto& type=info(result.left->type,result.location);
            if (type.kind!=CompoundKind::array || type.rank!=2 || (type.element!=Type::real && args.size()>2)) throw Error("E_TYPE",result.location,"WriteMatrix requires a rectangular matrix");
            if (!object_text_supported(module_.types,type.element)) throw Error("E_UNSUPPORTED",result.location,"Matrix element formatting is not implemented");
            const auto literal=[&](std::int32_t value) {
                auto node=std::make_unique<hir::Expr>(); node->kind=hir::ExprKind::literal; node->type=Type::integer; node->value=value; node->location=result.location; return node;
            };
            result.arguments.push_back(args.size()>1 ? convert(std::move(args[1]),Type::integer) : literal(0));
            if (type.element==Type::real) result.arguments.push_back(args.size()>2 ? convert(std::move(args[2]),Type::integer) : literal(2));
            result.kind=hir::ExprKind::write_matrix; result.type=Type::unit; result.local=type.element==Type::real ? 1U : 0U; return true;
        }
        if (result.arguments.size()!=1 && result.arguments.size()!=2) throw Error("E_TYPE",result.location,"Array output requires an array and optional delimiter");
        result.left=std::move(result.arguments[0]);
        const auto& type=info(result.left->type,result.location);
        if (type.kind!=CompoundKind::array || (type.rank!=1 && !member)) throw Error("E_TYPE",result.location,"Array IO requires a one-dimensional array");
        if (name=="readarray") {
            if (result.arguments.size()!=1 || (type.element!=Type::integer && type.element!=Type::real && type.element!=Type::string))
                throw Error("E_TYPE",result.location,"ReadArray requires an integer, real or string array");
            result.arguments.clear(); result.kind=hir::ExprKind::read_array; result.type=Type::unit; return true;
        }
        if (!object_text_supported(module_.types,type.element)) throw Error("E_UNSUPPORTED",result.location,"Array element formatting is not implemented");
        if (result.arguments.size()==2) result.right=convert(std::move(result.arguments[1]),Type::string);
        else {
            result.right=std::make_unique<hir::Expr>(); result.right->kind=hir::ExprKind::literal;
            result.right->type=Type::string; result.right->value=std::u16string{u" "}; result.right->location=result.location;
        }
        result.arguments.clear(); result.kind=hir::ExprKind::write_array; result.type=Type::unit;
        result.local=(name=="writelnarray" || name=="writeln" ? 1U : 0U)+(member ? 2U : 0U); return true;
    }
    bool bind_sequence_mutation(hir::Expr& result, std::string_view name, std::string_view unit = {}, bool member = false) {
        if ((name!="reverse" && name!="shuffle" && name!="swap") || (!unit.empty() && unit!="pabcsystem")) return false;
        const auto count=result.arguments.size();
        if (name=="swap" && !member) {
            if (count!=2 || result.arguments[0]->type!=result.arguments[1]->type
                || !lvalue(*result.arguments[0]) || !lvalue(*result.arguments[1]))
                throw Error("E_TYPE",result.location,"Swap requires two assignable values of exactly the same type");
            result.kind=hir::ExprKind::swap_values; result.type=Type::unit; return true;
        }
        if ((name=="reverse" && count!=1 && count!=3) || (name=="shuffle" && count!=1) || (name=="swap" && count!=3))
            throw Error("E_TYPE",result.location,"Wrong sequence mutation argument count");
        const auto receiver=result.arguments[0]->type;
        if (receiver==Type::string) {
            if (name!="reverse" || !lvalue(*result.arguments[0])) throw Error("E_TYPE",result.location,"Reverse requires a writable string");
        } else if (!compound(receiver) || !indexed_sequence(info(receiver,result.location).kind))
            throw Error("E_TYPE",result.location,"Reverse/Shuffle requires an array or List<T>");
        else if (info(receiver,result.location).rank!=1) throw Error("E_TYPE",result.location,"Reverse/Shuffle requires a one-dimensional array");
        result.local=static_cast<std::size_t>(name=="reverse" ? count==1 ? SequenceMutation::reverse : SequenceMutation::reverse_range
            : name=="swap" ? SequenceMutation::swap_indices : member ? SequenceMutation::shuffle_result : SequenceMutation::shuffle);
        for (std::size_t i=1;i<count;++i) result.arguments[i]=convert(std::move(result.arguments[i]),Type::integer);
        result.left=std::move(result.arguments[0]); result.arguments.erase(result.arguments.begin());
        result.kind=hir::ExprKind::sequence_mutate;
        result.type=mutation_result(static_cast<SequenceMutation>(result.local)) ? receiver : Type::unit;
        return true;
    }
    bool bind_random_array(hir::Expr& result, std::string_view name, std::string_view unit = {}) {
        const bool matrix_factory=name=="createrandomintegermatrix" || name=="createrandomrealmatrix" || name=="matrrandom" || name=="matrrandominteger" || name=="matrrandomreal";
        const bool arrays=name=="fillbyrandom" || name=="createrandomintegerarray" || name=="createrandomrealarray" || name=="createrandomintegermatrix" || name=="createrandomrealmatrix";
        const bool system=name=="arrrandom" || name=="arrrandominteger" || name=="arrrandomreal" || name=="fillrandom" || name=="matrrandom" || name=="matrrandominteger" || name=="matrrandomreal";
        if ((!arrays && !system) || (!unit.empty() && unit!=(arrays ? "arrays" : "pabcsystem"))
            || (arrays && unit.empty() && !imported_units_.contains("arrays"))) return false;
        auto args=std::move(result.arguments); result.arguments.clear(); const auto count=args.size();
        const bool fill=name=="fillbyrandom" || name=="fillrandom";
        const bool rounded=name=="arrrandomreal" || name=="matrrandomreal";
        unsigned rank=matrix_factory ? 2U : 1U;
        const auto fail=[&](){throw Error("E_TYPE",result.location,"Wrong random array/matrix arguments for '"+std::string(name)+"'");};
        const auto literal=[&](Value value) {
            auto node=std::make_unique<hir::Expr>(); node->kind=hir::ExprKind::literal;
            node->type=value_type(value); node->location=result.location; node->value=std::move(value); return node;
        };
        Type element;
        if (fill) {
            if ((name=="fillbyrandom" && count!=1 && count!=3) || (name=="fillrandom" && count!=3)) fail();
            result.left=std::move(args[0]); const auto& type=info(result.left->type,result.location);
            if (type.kind!=CompoundKind::array || (type.element!=Type::integer && type.element!=Type::real)) fail();
            element=type.element; rank=type.rank;
        } else element=rounded || name=="createrandomrealarray" || name=="createrandomrealmatrix" ? Type::real : Type::integer;
        const auto method=element==Type::integer ? RandomArrayMethod::integer
            : rounded ? RandomArrayMethod::rounded_real : arrays ? RandomArrayMethod::scaled_real : RandomArrayMethod::ranged_real;
        if (!fill) {
            if (rank==2) {
                if ((arrays && count!=2 && count!=4) || (!arrays && count>(rounded ? 5U : 4U))) fail();
                for (unsigned i=0;i<2;++i) result.arguments.push_back(count>i ? convert(std::move(args[i]),Type::integer) : literal(std::int32_t{5}));
            } else {
                if ((arrays && count!=1 && count!=3) || (name=="arrrandom" && count>3)
                    || (name=="arrrandominteger" && count!=0 && count!=1 && count!=3) || (rounded && count>4)) fail();
                result.arguments.push_back(count ? convert(std::move(args[0]),Type::integer) : literal(std::int32_t{10}));
            }
        }
        const auto bounds=fill ? 1U : rank;
        const bool low_given=fill ? count==3 : rank==2 ? count>2 : rounded ? count>=3 : count==3 || (name=="arrrandom" && count==2);
        const bool high_given=fill ? count==3 : rank==2 ? count>3 : low_given && count>=3;
        result.arguments.push_back(low_given ? convert(std::move(args[bounds]),element) : literal(element==Type::integer ? Value{std::int32_t{0}} : Value{0.0}));
        result.arguments.push_back(high_given ? convert(std::move(args[bounds+1]),element)
            : literal(element==Type::integer ? Value{std::int32_t{arrays && rank==1 ? 99 : 100}} : Value{10.0}));
        if (rounded) result.arguments.push_back(rank==2 ? count==5 ? convert(std::move(args[4]),Type::integer) : literal(std::int32_t{2})
            : count==4 ? convert(std::move(args[3]),Type::integer) : count==2 ? convert(std::move(args[1]),Type::integer) : literal(std::int32_t{2}));
        result.local=static_cast<std::size_t>(method);
        result.kind=fill ? hir::ExprKind::fill_random_array : hir::ExprKind::random_array;
        result.type=fill ? Type::unit : sequence_type(CompoundKind::array,element,rank);
        return true;
    }
    void bind_builtin(hir::Expr& result, std::string_view name, std::string_view unit = {}) {
        if ((unit.empty() || unit=="pabcsystem") && (name=="min" || name=="max") && result.arguments.size()>=2 && result.arguments.size()<=4
            && std::all_of(result.arguments.begin(),result.arguments.end(),[](const auto& arg){return integer_type(arg->type);})) {
            Type context=Type::invalid;
            for (const auto& arg:result.arguments) {
                if (arg->type==Type::uint64) { context=Type::uint64; break; }
                if (arg->type==Type::longword) context=Type::longword;
            }
            bool compatible=context!=Type::invalid;
            if (context==Type::longword) for (const auto& arg:result.arguments) {
                if (can_widen(arg->type,context)) continue;
                if (arg->kind!=hir::ExprKind::literal) { compatible=false; break; }
                try { if (unsigned_number(arg->value,arg->location)>UINT32_MAX) compatible=false; }
                catch (const Error& error) { if (error.code!="E_OVERFLOW") throw; compatible=false; }
            }
            if (compatible) for (auto& arg:result.arguments) arg=convert(std::move(arg),context);
        }
        const auto catalog=stdlib::signatures(); std::vector<std::size_t> candidates; bool known=false;
        for (std::size_t i=0;i<catalog.size();++i) {
            const auto& signature=catalog[i];
            if (signature.name!=name) continue;
            if (unit.empty()) {
                if (!signature.unit.empty() && !imported_units_.contains(std::string(signature.unit))) continue;
            } else if (signature.unit != (unit=="pabcsystem" ? std::string_view{} : unit)) continue;
            known=true;
            if (signature.parameters.size()!=result.arguments.size()) continue;
            bool compatible=true;
            for (std::size_t n=0;n<signature.parameters.size();++n)
                if (!can_widen(result.arguments[n]->type,signature.parameters[n])) compatible=false;
            if (compatible) candidates.push_back(i);
        }
        if (!known) throw Error("E_NAME",result.location,"Unknown function '"+std::string(name)+"'");
        const auto dominates=[&](std::size_t a,std::size_t b) {
            bool better=false;
            for (std::size_t n=0;n<catalog[a].parameters.size();++n) {
                const auto left=catalog[a].parameters[n],right=catalog[b].parameters[n];
                if (!can_widen(left,right)) return false;
                if (left!=right) better=true;
            }
            return better;
        };
        std::optional<std::size_t> chosen;
        for (auto candidate:candidates) {
            bool dominated=false;
            for (auto other:candidates) if (dominates(other,candidate)) dominated=true;
            if (!dominated) {
                if (chosen) throw Error("E_TYPE",result.location,"Ambiguous overload for '"+std::string(name)+"'");
                chosen=candidate;
            }
        }
        if (!chosen) throw Error("E_TYPE",result.location,"No matching overload for '"+std::string(name)+"'");
        const auto& signature=catalog[*chosen]; result.local=*chosen; result.type=signature.result;
        for (std::size_t n=0;n<result.arguments.size();++n)
            result.arguments[n]=convert(std::move(result.arguments[n]),signature.parameters[n]);
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
            if ((!resize && source.arguments.size()!=count) || (resize && (source.arguments.size()<2 || source.arguments.size()>3)))
                throw Error("E_TYPE", result.location, "Wrong argument count for '" + name + "'");
            auto target = expression(*source.arguments[reference]);
            if (!lvalue(*target)) throw Error("E_TYPE",target->location,"var argument must be assignable");
            if (resize && compound(target->type) && info(target->type,result.location).kind == CompoundKind::array) {
                if (source.arguments.size()!=info(target->type,result.location).rank+1) throw Error("E_TYPE",result.location,"SetLength dimensions must match the array rank");
                result.kind = hir::StmtKind::resize_array; result.target = std::move(target);
                result.expression = convert(expression(*source.arguments[1]),Type::integer);
                for (std::size_t i=2;i<source.arguments.size();++i) result.arguments.push_back(convert(expression(*source.arguments[i]),Type::integer));
                break;
            }
            if (source.arguments.size()!=count) throw Error("E_TYPE",result.location,"Wrong string mutation argument count");
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
            if (source.names.front().text=="dec" && imported_units_.contains("school") && source.expression
                && (type==Type::string || type==Type::nil || (type==Type::character && result.target->kind==hir::ExprKind::literal))) {
                auto call=std::make_unique<hir::Expr>(); call->kind=hir::ExprKind::call; call->location=result.location;
                call->arguments.push_back(std::move(result.target)); call->arguments.push_back(expression(*source.expression));
                bind_builtin(*call,"dec","school"); result.kind=hir::StmtKind::call; result.expression=std::move(call); break;
            }
            if (!lvalue(*result.target) || !ordinal(type)) throw Error("E_TYPE",result.location,"Inc/Dec requires an ordinal variable");
            result.newline=source.names.front().text=="dec";
            if (type==Type::boolean) {
                if (source.expression) throw Error("E_TYPE",result.location,"Boolean Inc/Dec takes no step argument");
                break;
            }
            const auto step_type=increment_step_type(type);
            if (source.expression) result.expression=convert(expression(*source.expression),step_type);
            else {
                result.expression=std::make_unique<hir::Expr>(); result.expression->kind=hir::ExprKind::literal;
                result.expression->type=step_type; result.expression->location=result.location;
                result.expression->value=step_type==Type::integer ? Value(std::int32_t{1}) : Value(std::int64_t{1});
            }
            break;
        }
        case SyntaxStmtKind::assignment:
            result.kind = hir::StmtKind::assign;
            result.target = expression(*source.target);
            if (result.target->kind==hir::ExprKind::index && result.target->left->type==Type::string) {
                if (!lvalue(*result.target->left)) throw Error("E_TYPE",result.location,"String character assignment requires a writable string");
                result.expression=convert(expression(*source.expression),Type::character); break;
            }
            if (result.target->kind == hir::ExprKind::linked_call && result.target->local == static_cast<std::size_t>(LinkedMethod::next)
                && info(result.target->left->type,result.location).kind == CompoundKind::single_node) {
                auto setter = std::move(result.target);
                setter->arguments.push_back(convert(expression(*source.expression),setter->type));
                setter->local = static_cast<std::size_t>(LinkedMethod::set_next); setter->type = Type::unit;
                result.kind = hir::StmtKind::call; result.expression = std::move(setter); break;
            }
            if (result.target->kind == hir::ExprKind::linked_call && result.target->local == static_cast<std::size_t>(LinkedMethod::value)) {
                auto setter = std::move(result.target);
                setter->arguments.push_back(convert(expression(*source.expression),setter->type));
                setter->local = static_cast<std::size_t>(LinkedMethod::set_value); setter->type = Type::unit;
                result.kind = hir::StmtKind::call; result.expression = std::move(setter); break;
            }
            if (result.target->kind == hir::ExprKind::association_call && result.target->local == static_cast<std::size_t>(AssociationMethod::get)) {
                auto setter = std::move(result.target);
                setter->arguments.push_back(convert(expression(*source.expression),setter->type));
                setter->local = static_cast<std::size_t>(AssociationMethod::set); setter->type = Type::unit;
                result.kind = hir::StmtKind::call; result.expression = std::move(setter); break;
            }
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
        case SyntaxStmtKind::break_stmt: case SyntaxStmtKind::continue_stmt:
            if (!loop_depth_) throw Error("E_SYNTAX",result.location,"Break/Continue requires an enclosing loop");
            result.kind=source.kind==SyntaxStmtKind::break_stmt ? hir::StmtKind::break_stmt : hir::StmtKind::continue_stmt;
            break;
        case SyntaxStmtKind::foreach_loop: {
            result.kind=hir::StmtKind::foreach_loop; result.expression=expression(*source.expression);
            const auto sequence=result.expression->type;
            if (sequence!=Type::string && (!compound(sequence) || !indexed_sequence(info(sequence,result.location).kind)))
                throw Error("E_UNSUPPORTED",result.location,"Foreach requires a string, array or List<T>");
            const auto element=sequence==Type::string ? Type::character : info(sequence,result.location).element;
            scopes_.emplace_back(); const auto& name=source.names.front();
            std::size_t control;
            if (source.declares_control) {
                const auto type=source.annotation ? resolve_type(*source.annotation) : element;
                control=module_.locals.size(); module_.locals.push_back({name.text,type,!in_routine_});
                scopes_.back()[name.text]=control;
            } else {
                control=resolve(name);
                if (for_controls_.contains(control)) throw Error("E_TYPE",name.location,"Cannot reuse an active iteration variable");
                if (in_routine_ && module_.locals[control].global) throw Error("E_TYPE",name.location,"Foreach variable must be local to the routine");
                if (in_routine_ && current_result_!=Type::unit && module_.locals[control].name=="result")
                    throw Error("E_UNSUPPORTED",name.location,"Function Result as foreach variable is not supported");
            }
            const auto type=module_.locals[control].type;
            if (!can_widen(element,type)) throw Error("E_TYPE",name.location,"Foreach element cannot be converted to the iteration variable type");
            auto temporary=[&](Type type) {
                const auto local=module_.locals.size(); module_.locals.push_back({"$foreach_"+std::to_string(local),type,!in_routine_}); return local;
            };
            result.sequence_local=temporary(sequence); result.index_local=temporary(Type::integer); result.version_local=temporary(Type::int64);
            result.target=std::make_unique<hir::Expr>(); result.target->kind=hir::ExprKind::local;
            result.target->local=control; result.target->type=type; result.target->location=name.location;
            for_controls_.insert(control); ++loop_depth_;
            result.body=std::make_unique<hir::Stmt>(scoped_statement(*source.body));
            --loop_depth_; for_controls_.erase(control); scopes_.pop_back(); break;
        }
        case SyntaxStmtKind::for_loop: {
            auto initial=expression(*source.expression);
            scopes_.emplace_back();
            const auto& name=source.names.front();
            std::size_t control;
            if (source.declares_control) {
                const auto type=source.annotation ? resolve_type(*source.annotation) : initial->type;
                control=module_.locals.size(); module_.locals.push_back({name.text,type,!in_routine_});
                scopes_.back()[name.text]=control;
            } else {
                control=resolve(name);
                if (for_controls_.contains(control)) throw Error("E_TYPE",name.location,"Cannot reuse an active for control variable");
                if (in_routine_ && module_.locals[control].global) throw Error("E_TYPE",name.location,"For control must be local to the routine");
                if (in_routine_ && current_result_!=Type::unit && module_.locals[control].name=="result") throw Error("E_UNSUPPORTED",name.location,"Function Result as for control is not supported");
            }
            const auto type=module_.locals[control].type;
            if (!ordinal(type)) throw Error("E_TYPE",name.location,"For control must be an ordinal type");
            initial=convert(std::move(initial),type);
            auto finish=convert(expression(*source.arguments[0]),type);
            const auto bound=module_.locals.size(); module_.locals.push_back({"$for_bound_"+std::to_string(bound),type,!in_routine_});
            auto assign=[&](std::size_t local,std::unique_ptr<hir::Expr> value) {
                hir::Stmt node; node.kind=hir::StmtKind::assign; node.local=local; node.location=value->location;
                node.expression=std::move(value); result.statements.push_back(std::move(node));
            };
            // Match the source converter: declared counters initialize before the bound;
            // pre-existing counters evaluate the bound before their initializer.
            if (source.declares_control) { assign(control,std::move(initial)); assign(bound,std::move(finish)); }
            else { assign(bound,std::move(finish)); assign(control,std::move(initial)); }
            result.kind=hir::StmtKind::for_loop; result.local=bound; result.newline=source.descending;
            result.target=std::make_unique<hir::Expr>(); result.target->kind=hir::ExprKind::local;
            result.target->local=control; result.target->type=type; result.target->location=name.location;
            for_controls_.insert(control); ++loop_depth_;
            result.body=std::make_unique<hir::Stmt>(scoped_statement(*source.body));
            --loop_depth_; for_controls_.erase(control); scopes_.pop_back(); break;
        }
        case SyntaxStmtKind::repeat_loop:
            result.kind=hir::StmtKind::repeat_loop; scopes_.emplace_back(); ++loop_depth_;
            for (const auto& child:source.statements) result.statements.push_back(statement(child));
            result.expression=expression(*source.expression);
            if (result.expression->type!=Type::boolean) throw Error("E_TYPE",result.expression->location,"Until condition must be boolean");
            --loop_depth_; scopes_.pop_back(); break;
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
                if (argument==source.arguments.front() && value->type==Type::text_file) { result.target=std::move(value); continue; }
                if (value->type==Type::nil) value=convert(std::move(value),Type::string);
                if (!object_text_supported(module_.types,value->type)) throw Error("E_TYPE",value->location,"Write for this static type is not implemented");
                result.arguments.push_back(std::move(value));
            }
            break;
        case SyntaxStmtKind::read: {
            result.kind = hir::StmtKind::read;
            result.newline=source.token.text=="readln";
            if (source.arguments.empty()) {
                break;
            }
            auto first=expression(*source.arguments[0]);
            if (first->type==Type::text_file) {
                result.target=std::move(first);
                for (std::size_t i=1;i<source.arguments.size();++i) {
                    auto target=expression(*source.arguments[i]);
                    if (!lvalue(*target) || !scalar(target->type)) throw Error("E_TYPE",target->location,"File Read requires scalar variables");
                    result.arguments.push_back(std::move(target));
                }
                break;
            }
            if (source.arguments.size()>16) throw Error("E_TYPE",result.location,"Console Read accepts at most sixteen variables");
            for (std::size_t i=0;i<source.arguments.size();++i) {
                auto target=i==0 ? std::move(first) : expression(*source.arguments[i]);
                if (!lvalue(*target) || !scalar(target->type)) throw Error("E_TYPE",target->location,"Console Read requires writable scalar variables");
                result.arguments.push_back(std::move(target));
            }
            break;
        }
        case SyntaxStmtKind::conditional:
        case SyntaxStmtKind::loop:
            result.kind = source.kind == SyntaxStmtKind::conditional ? hir::StmtKind::conditional : hir::StmtKind::loop;
            result.expression = expression(*source.expression);
            if (result.expression->type != Type::boolean)
                throw Error("E_TYPE", result.expression->location, "Condition must be boolean");
            if (source.kind==SyntaxStmtKind::loop) ++loop_depth_;
            result.body = std::make_unique<hir::Stmt>(scoped_statement(*source.body));
            if (source.kind==SyntaxStmtKind::loop) --loop_depth_;
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
    std::set<std::string> imported_units_;
    Type current_result_ = Type::unit;
    std::string current_name_;
    std::vector<std::map<std::string, std::size_t>> scopes_;
    std::set<std::size_t> for_controls_;
    unsigned loop_depth_=0;
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

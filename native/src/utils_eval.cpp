#include "utils_eval.hpp"
#include "big_integer_runtime.hpp"
#include "comparable.hpp"
#include "composite_format.hpp"
#include "decimal.hpp"
#include "integer_values.hpp"
#include "number_format.hpp"
#include "runtime_values.hpp"
#include "stdlib.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>

namespace abc {
namespace {
enum class Symbol {
    end, literal, number, open, close, comma, plus, minus, multiply, divide, modulus,
    equal, unequal, less, less_equal, greater, greater_equal, conjunction,
    disjunction, negate, is, null_value, true_value, false_value, is_null, is_not_null,
    length, trim, substring, choose, replace_null, absolute
};

struct Token {
    Symbol kind;
    std::u16string text;
};

struct Node {
    Symbol kind;
    Value value=NilString{};
    std::size_t left=0, right=0;
    bool literal=false;
    std::size_t third=0;
};

bool digit(char16_t ch) { return ch>=u'0' && ch<=u'9'; }
bool letter(char16_t ch) { return (ch>=u'a' && ch<=u'z') || (ch>=u'A' && ch<=u'Z') || ch==u'_'; }
char16_t lower_ascii(char16_t ch) { return ch>=u'A' && ch<=u'Z' ? static_cast<char16_t>(ch+32) : ch; }
std::u16string_view trim(std::u16string_view text) {
    while(!text.empty() && stdlib::unicode_whitespace(text.front()))text.remove_prefix(1);
    while(!text.empty() && stdlib::unicode_whitespace(text.back()))text.remove_suffix(1);
    return text;
}
std::u16string_view comparison_text(std::u16string_view text) {
    while(!text.empty() && (text.back()==u' ' || text.back()==u'\u3000'))text.remove_suffix(1);
    return text;
}
bool null(const Value& value) { return std::holds_alternative<NilString>(value); }
std::optional<bool> boolean_text(std::u16string_view text) {
    auto lower=std::u16string(trim(text));
    for(auto& ch:lower)ch=lower_ascii(ch);
    if(lower==u"true")return true;
    if(lower==u"false")return false;
    return {};
}
unsigned argument_count(Symbol kind) {
    switch(kind) {
    case Symbol::length:case Symbol::trim:case Symbol::absolute:return 1;
    case Symbol::replace_null:return 2;
    case Symbol::substring:case Symbol::choose:return 3;
    default:return 0;
    }
}
unsigned precedence(Symbol kind) {
    switch(kind) {
    case Symbol::disjunction:return 1;
    case Symbol::conjunction:return 2;
    case Symbol::equal:case Symbol::unequal:case Symbol::less:case Symbol::less_equal:
    case Symbol::greater:case Symbol::greater_equal:case Symbol::is:return 3;
    case Symbol::plus:case Symbol::minus:return 4;
    case Symbol::multiply:case Symbol::divide:case Symbol::modulus:return 5;
    default:return 0;
    }
}

class Evaluation {
    Values& values_;
    const ExecutionOptions& options_;
    SourceLocation location_;
    Type decimal_;
    std::u16string_view source_;
    std::size_t cursor_=0, work_=0;
    std::vector<Node> nodes_;
    Token token_{Symbol::end,{}};

    [[noreturn]] void syntax() const {
        throw Error("E_FORMAT",location_,"Invalid Utils evaluation expression","Некорректное выражение Utils");
    }
    [[noreturn]] void operands() const {
        throw Error("E_TYPE",location_,"Unsupported operand types in Utils expression","Несовместимые типы операндов в выражении Utils");
    }
    void step(unsigned depth=0) {
        if(depth>128 || work_>=options_.instruction_limit || nodes_.size()>4096)
            throw runtime_error(RuntimeError::instruction_limit,location_);
        if((work_++&255U)==0 && options_.cancelled && options_.cancelled())
            throw runtime_error(RuntimeError::cancelled,location_);
    }
    Token scan() {
        while(cursor_<source_.size() && stdlib::unicode_whitespace(source_[cursor_]))++cursor_;
        if(cursor_==source_.size())return {Symbol::end,{}};
        const auto begin=cursor_;
        const auto ch=source_[cursor_++];
        auto take=[&](char16_t next) {
            if(cursor_==source_.size() || source_[cursor_]!=next)return false;
            ++cursor_;
            return true;
        };
        switch(ch) {
        case u'(':return {Symbol::open,{}};
        case u')':return {Symbol::close,{}};
        case u',':return {Symbol::comma,{}};
        case u'+':return {Symbol::plus,{}};
        case u'-':return {Symbol::minus,{}};
        case u'*':return {Symbol::multiply,{}};
        case u'/':return {Symbol::divide,{}};
        case u'%':return {Symbol::modulus,{}};
        case u'=':return {Symbol::equal,{}};
        case u'<':return {take(u'=') ? Symbol::less_equal : take(u'>') ? Symbol::unequal : Symbol::less,{}};
        case u'>':return {take(u'=') ? Symbol::greater_equal : Symbol::greater,{}};
        case u'\'': {
            std::u16string text;
            while(cursor_<source_.size()){
                const auto unit=source_[cursor_++];
                if(unit==u'\'' && !take(u'\''))return {Symbol::literal,std::move(text)};
                text.push_back(unit);
            }
            syntax();
        }
        default:break;
        }
        if(digit(ch) || (ch==u'.' && cursor_<source_.size() && digit(source_[cursor_]))) {
            while(cursor_<source_.size() && digit(source_[cursor_]))++cursor_;
            if(ch!=u'.' && take(u'.'))
                while(cursor_<source_.size() && digit(source_[cursor_]))++cursor_;
            if(take(u'e') || take(u'E')){
                if(!take(u'+'))(void)take(u'-');
                const auto exponent=cursor_;
                while(cursor_<source_.size() && digit(source_[cursor_]))++cursor_;
                if(exponent==cursor_)syntax();
            }
            return {Symbol::number,std::u16string(source_.substr(begin,cursor_-begin))};
        }
        if(letter(ch)){
            std::u16string word(1,lower_ascii(ch));
            while(cursor_<source_.size() && (letter(source_[cursor_]) || digit(source_[cursor_])))
                word.push_back(lower_ascii(source_[cursor_++]));
            if(word==u"and")return {Symbol::conjunction,{}};
            if(word==u"or")return {Symbol::disjunction,{}};
            if(word==u"not")return {Symbol::negate,{}};
            if(word==u"is")return {Symbol::is,{}};
            if(word==u"null")return {Symbol::null_value,{}};
            if(word==u"true")return {Symbol::true_value,{}};
            if(word==u"false")return {Symbol::false_value,{}};
            if(word==u"len")return {Symbol::length,{}};
            if(word==u"trim")return {Symbol::trim,{}};
            if(word==u"substring")return {Symbol::substring,{}};
            if(word==u"iif")return {Symbol::choose,{}};
            if(word==u"isnull")return {Symbol::replace_null,{}};
            if(word==u"abs")return {Symbol::absolute,{}};
            throw Error("E_UNSUPPORTED",location_,"This name or function is unavailable in Utils evaluation",
                "Это имя или функция недоступны в вычислениях Utils");
        }
        syntax();
    }
    Symbol advance() {
        step();
        return std::exchange(token_,scan()).kind;
    }
    std::size_t add(Node node) {
        nodes_.push_back(std::move(node));
        return nodes_.size()-1;
    }
    template<std::size_t n>
    Value decimal_call(DecimalMethod method,const std::array<Value,n>& arguments) {
        if(decimal_==Type::invalid)throw runtime_error(RuntimeError::invalid_bytecode,location_);
        return decimal_invoke(values_,decimal_,static_cast<std::size_t>(method),arguments);
    }
    Value decimal_call(DecimalMethod method,Value value) {
        return decimal_call(method,std::array<Value,1>{std::move(value)});
    }
    double real_text(const Value& value) {
        NumberFormatSettings format;
        format.positive_infinity=u"∞";
        format.negative_infinity=u"-∞";
        return stdlib::parse_provider_real(value,format,location_);
    }
    Value number(std::u16string text) {
        if(text.find_first_of(u"eE")==std::u16string::npos){
            if(text.find(u'.')!=std::u16string::npos) {
                try {return decimal_call(DecimalMethod::parse,Value{text});}
                catch(const Error& error) {if(error.code!="E_OVERFLOW")throw;}
            } else {
                const std::string ascii(text.begin(),text.end());
                std::int64_t value=0;
                if(const auto result=std::from_chars(ascii.data(),ascii.data()+ascii.size(),value);
                   result.ec==std::errc{} && result.ptr==ascii.data()+ascii.size()){
                    if(value<=INT32_MAX)return static_cast<std::int32_t>(value);
                    return value;
                }
            }
        }
        return real_text(Value{std::move(text)});
    }
    std::size_t parse_function(Symbol kind,unsigned depth) {
        if(advance()!=Symbol::open)syntax();
        std::array<std::size_t,3> arguments{};
        const auto count=argument_count(kind);
        for(unsigned n=0;n<count;++n){
            if(n!=0 && advance()!=Symbol::comma)syntax();
            arguments[n]=parse(1,depth+1);
        }
        if(advance()!=Symbol::close)syntax();
        return add({kind,NilString{},arguments[0],arguments[1],false,arguments[2]});
    }
    std::size_t parse(unsigned minimum,unsigned depth) {
        step(depth);
        auto initial=std::move(token_);
        advance();
        std::size_t left;
        switch(initial.kind){
        case Symbol::number:left=add({initial.kind,number(std::move(initial.text)),0,0,true});break;
        case Symbol::literal:left=add({initial.kind,std::move(initial.text),0,0,true});break;
        case Symbol::null_value:left=add({initial.kind,NilString{},0,0,true});break;
        case Symbol::true_value:case Symbol::false_value:
            left=add({initial.kind,initial.kind==Symbol::true_value,0,0,true});break;
        case Symbol::open:
            left=parse(1,depth+1);
            if(advance()!=Symbol::close)syntax();
            break;
        case Symbol::plus:case Symbol::minus:case Symbol::negate:
            left=add({initial.kind,NilString{},parse(initial.kind==Symbol::negate ? 3U : 6U,depth+1),0,false});
            break;
        case Symbol::length:case Symbol::trim:case Symbol::substring:
        case Symbol::choose:case Symbol::replace_null:case Symbol::absolute:
            left=parse_function(initial.kind,depth);
            break;
        default:syntax();
        }
        while(precedence(token_.kind)>=minimum) {
            const auto kind=advance();
            if(kind==Symbol::is){
                const bool negate=token_.kind==Symbol::negate;
                if(negate)advance();
                if(advance()!=Symbol::null_value)syntax();
                left=add({negate ? Symbol::is_not_null : Symbol::is_null,NilString{},left,0,false});
            } else {
                const auto right=parse(precedence(kind)+1,depth+1);
                left=add({kind,NilString{},left,right,false});
            }
        }
        return left;
    }
    Type kind(const Value& value) const {
        if(const auto* object=std::get_if<std::shared_ptr<Aggregate>>(&value))return (*object)->type;
        return value_type(value);
    }
    double real(const Value& value) {
        const auto type=kind(value);
        if(type==Type::real)return std::get<double>(value);
        if(type==Type::boolean || type==Type::integer || type==Type::int64)return static_cast<double>(integral_number(value,location_));
        if(type==decimal_)return std::get<double>(decimal_call(DecimalMethod::to_real,value));
        if(type==Type::string)return real_text(value);
        operands();
    }
    std::u16string text(const Value& value) {
        if(null(value))return {};
        const auto type=kind(value);
        if(type==Type::string)return std::get<std::u16string>(value);
        NumberFormatSettings format;
        format.positive_infinity=u"∞";
        format.negative_infinity=u"-∞";
        if(type==decimal_)return format_decimal(value,{},format,location_);
        if(type==Type::real || type==Type::integer || type==Type::int64)
            return format_number(value,type,{},format,location_);
        return format_scalar(value,location_);
    }
    Value as_decimal(const Value& value) {
        if(kind(value)==decimal_)return value;
        if(kind(value)==Type::string)return decimal_call(DecimalMethod::parse,value);
        return decimal_call(DecimalMethod::from_signed,Value{integral_number(value,location_)});
    }
    unsigned numeric_rank(Type type) const {
        if(type==Type::integer)return 1;
        if(type==Type::int64)return 2;
        if(type==decimal_)return 3;
        if(type==Type::real)return 4;
        return 0;
    }
    Value arithmetic(Symbol op,const Value& left,const Value& right) {
        const auto a=kind(left), b=kind(right);
        if(op==Symbol::plus && (a==Type::string || b==Type::string)){
            auto result=text(left);
            const auto suffix=text(right);
            if(result.size()+suffix.size()>1'048'576)throw runtime_error(RuntimeError::string_limit,location_);
            result+=suffix;
            return result;
        }
        const auto rank=std::max(numeric_rank(a),numeric_rank(b));
        if(!numeric_rank(a) || !numeric_rank(b) || (op==Symbol::modulus && rank>2))operands();
        if(rank==4 || (op==Symbol::divide && rank<3)){
            const auto x=real(left),y=real(right);
            switch(op) {
            case Symbol::plus:return x+y;
            case Symbol::minus:return x-y;
            case Symbol::multiply:return x*y;
            default:return x/y;
            }
        }
        if(rank==3){
            using M=DecimalMethod;
            const auto method=op==Symbol::plus ? M::add : op==Symbol::minus ? M::subtract
                : op==Symbol::multiply ? M::multiply : M::divide;
            return decimal_call(method,std::array<Value,2>{as_decimal(left),as_decimal(right)});
        }
        const auto x=integral_number(left,location_),y=integral_number(right,location_);
        Op operation=Op::modulo;
        if(op==Symbol::plus)operation=Op::add;
        if(op==Symbol::minus)operation=Op::subtract;
        if(op==Symbol::multiply)operation=Op::multiply;
        const auto exact=big_binary(operation,BigInteger::from_signed(x),BigInteger::from_signed(y),location_,options_);
        const auto result=checked_big(location_,[&]{return exact.to_signed();});
        return checked_signed_integer(rank==1 ? Type::integer : Type::int64,Value{result},location_);
    }
    int compare(const Value& left,const Value& right) {
        const auto a=kind(left),b=kind(right);
        if(a==Type::string && b==Type::string)
            return compare_culture_strings(comparison_text(std::get<std::u16string>(left)),
                comparison_text(std::get<std::u16string>(right)),location_,options_,true);
        const auto rank=std::max(numeric_rank(a),numeric_rank(b));
        if(rank==4){
            const auto x=real(left),y=real(right);
            if(std::isnan(x))return std::isnan(y) ? 0 : -1;
            if(std::isnan(y))return 1;
            return (x>y)-(x<y);
        }
        if(rank==3)return compare_decimal_values(as_decimal(left),as_decimal(right),location_);
        if(rank!=0 || (a==Type::boolean && b==Type::boolean)){
            const auto integer=[&](const Value& operand) {
                if(kind(operand)==Type::string){
                    const auto parsed=stdlib::try_parse_number(rank==1 ? Type::integer : Type::int64,operand,location_);
                    if(!parsed)operands();
                    return integral_number(*parsed,location_);
                }
                const auto type=kind(operand);
                if(type!=Type::integer && type!=Type::int64 && type!=Type::boolean)operands();
                return integral_number(operand,location_);
            };
            const auto x=integer(left),y=integer(right);
            return (x>y)-(x<y);
        }
        operands();
    }
    bool boolean(const Value& value) {
        if(const auto* logical=std::get_if<bool>(&value))return *logical;
        operands();
    }
    Value function(const Node& node,unsigned depth) {
        const auto count=argument_count(node.kind);
        const auto first=evaluate(node.left,depth);
        const auto second=count>=2 ? evaluate(node.right,depth) : Value{NilString{}};
        const auto third=count==3 ? evaluate(node.third,depth) : Value{NilString{}};
        if(node.kind==Symbol::absolute)return NilString{};
        if(node.kind==Symbol::replace_null)return null(first) ? second : first;
        if(node.kind==Symbol::choose){
            if(null(first))return third;
            if(const auto* text=std::get_if<std::u16string>(&first)){
                if(const auto condition=boolean_text(*text))return *condition ? second : third;
                operands();
            }
            return boolean(first) ? second : third;
        }
        if(null(first))return first;
        const auto* text=std::get_if<std::u16string>(&first);
        if(!text)operands();
        if(node.kind==Symbol::length)return static_cast<std::int32_t>(text->size());
        if(node.kind==Symbol::trim)return std::u16string(trim(*text));
        if(null(second) || null(third))return NilString{};
        const auto index=[&](const Value& argument){
            if(kind(argument)!=Type::integer && kind(argument)!=Type::int64)operands();
            return std::get<std::int32_t>(checked_signed_integer(Type::integer,argument,location_));
        };
        const auto start=index(second),length=index(third);
        if(start<1 || length<0)
            throw Error("E_RANGE",location_,"SUBSTRING requires a positive start and a nonnegative length",
                "SUBSTRING требует положительную начальную позицию и неотрицательную длину");
        if(length==0)return std::u16string{};
        const auto offset=static_cast<std::size_t>(start-1);
        if(offset>text->size())return NilString{};
        return text->substr(offset,static_cast<std::size_t>(length));
    }
    Value evaluate(std::size_t index,unsigned depth) {
        step(depth);
        const auto& node=nodes_[index];
        if(node.literal)return node.value;
        if(argument_count(node.kind))return function(node,depth+1);
        auto left=evaluate(node.left,depth+1);
        if(node.kind==Symbol::is_null)return null(left);
        if(node.kind==Symbol::is_not_null)return !null(left);
        if(node.kind==Symbol::negate)return null(left) ? left : Value{!boolean(left)};
        if(node.right==0){
            if(null(left))return left;
            if(!numeric_rank(kind(left)))operands();
            if(node.kind!=Symbol::minus)return left;
            if(kind(left)==Type::real)return -std::get<double>(left);
            if(kind(left)==decimal_)return decimal_call(DecimalMethod::negate,left);
            return arithmetic(Symbol::minus,Value{std::int32_t{0}},left);
        }
        auto right=evaluate(node.right,depth+1);
        if(node.kind==Symbol::conjunction){
            if(null(left) || !boolean(left))return left;
            return null(right) ? right : Value{boolean(right)};
        }
        if(node.kind==Symbol::disjunction){
            if(null(left))return right;
            if(boolean(left) || null(right))return left;
            return boolean(right);
        }
        if(null(left) || null(right))return NilString{};
        switch(node.kind){
        case Symbol::equal:return compare(left,right)==0;
        case Symbol::unequal:return compare(left,right)!=0;
        case Symbol::less:return compare(left,right)<0;
        case Symbol::less_equal:return compare(left,right)<=0;
        case Symbol::greater:return compare(left,right)>0;
        case Symbol::greater_equal:return compare(left,right)>=0;
        default:return arithmetic(node.kind,left,right);
        }
    }
public:
    Evaluation(Values& values,const ExecutionOptions& options,Type decimal,std::u16string_view source)
        :values_(values),options_(options),location_(values.location),decimal_(decimal),source_(source){}
    Value run(Type result) {
        advance();
        Value value=NilString{};
        if(token_.kind!=Symbol::end){
            const auto root=parse(1,0);
            if(token_.kind!=Symbol::end)syntax();
            value=evaluate(root,0);
        }
        if(result==Type::string)return text(value);
        if(null(value))throw Error("E_TYPE",location_,"A null evaluation result cannot be converted to the requested type",
            "Пустой результат выражения нельзя преобразовать к запрошенному типу");
        if(result==Type::real)return real(value);
        const auto type=kind(value);
        if(result==Type::boolean){
            if(type==Type::string){
                if(const auto converted=boolean_text(std::get<std::u16string>(value)))return *converted;
                throw runtime_error(RuntimeError::number_text,location_,"boolean");
            }
            if(type==Type::boolean)return value;
            if(type==decimal_)return compare_decimal_values(value,decimal_call(DecimalMethod::zero,std::array<Value,0>{}),location_)!=0;
            return real(value)!=0;
        }
        if(type==Type::boolean || type==Type::integer || type==Type::int64)
            return checked_signed_integer(Type::integer,value,location_);
        if(type==decimal_)
            return decimal_call(DecimalMethod::to_int32,decimal_call(DecimalMethod::round,value));
        if(type==Type::string) {
            const auto answer=stdlib::try_parse_number(Type::integer,value,location_);
            if(!answer)throw runtime_error(RuntimeError::number_text,location_,"integer");
            return *answer;
        }
        const auto input=std::get<double>(value);
        const auto floor=std::floor(input);
        const auto fraction=input-floor;
        const auto rounded=fraction>0.5 || (fraction==0.5 && std::fmod(floor,2)!=0) ? floor+1 : floor;
        if(!std::isfinite(rounded) || rounded<INT32_MIN || rounded>INT32_MAX)
            throw runtime_error(RuntimeError::integer_overflow,location_);
        return static_cast<std::int32_t>(rounded);
    }
};
}

Value utils_eval(Values& values,const Value& expression,Type result,const ExecutionOptions& options) {
    Type decimal=Type::invalid;
    for(std::size_t i=0;i<values.program.types.size();++i)
        if(decimal_type(values.program.types[i])){decimal=static_cast<Type>(64+i);break;}
    return Evaluation(values,options,decimal,string_text(expression)).run(result);
}
}

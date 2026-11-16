#include "utils_eval.hpp"
#include "big_integer_runtime.hpp"
#include "calendar.hpp"
#include "comparable.hpp"
#include "composite_format.hpp"
#include "decimal.hpp"
#include "guid.hpp"
#include "sql_scalar.hpp"
#include "integer_values.hpp"
#include "number_format.hpp"
#include "runtime_values.hpp"
#include "stdlib.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <limits>

namespace abc {
namespace {
enum class Symbol {
    end, literal, number, date_literal, name, parent, child, dot, open, close, comma, plus, minus, multiply, divide, modulus,
    equal, unequal, less, less_equal, greater, greater_equal, conjunction,
    disjunction, negate, is, null_value, true_value, false_value, is_null, is_not_null,
    length, trim, substring, choose, replace_null, absolute, membership, membership_arguments, like, conversion,
    grouped, invalid_is, between, bit_and, bit_or, bit_xor, bit_not
};

struct Token {
    Symbol kind;
    std::u16string text;
};

enum class MissingBinding { column, parent_relation, default_parent, default_child };

struct BindingError {
    MissingBinding kind;
    std::u16string name;
};

struct Node {
    Symbol kind;
    Value value=NilString{};
    std::size_t left=0, right=0;
    bool literal=false;
    std::size_t third=0;
    std::vector<std::size_t> members{};
};

struct PendingOperator {
    Symbol kind;
    unsigned priority;
    unsigned arity;
};

bool digit(char16_t ch) { return ch>=u'0' && ch<=u'9'; }
bool letter(char16_t ch) { return (ch>=u'a' && ch<=u'z') || (ch>=u'A' && ch<=u'Z') || ch==u'_'; }
bool name_character(char16_t ch) {
    return letter(ch) || ch==u'$' || (ch>=0x80 && !stdlib::unicode_whitespace(ch));
}
char16_t lower_ascii(char16_t ch) { return ch>=u'A' && ch<=u'Z' ? static_cast<char16_t>(ch+32) : ch; }
std::u16string lowercase(std::u16string_view text) {
    std::u16string result(text);
    for(auto& ch:result)ch=lower_ascii(ch);
    return result;
}
std::u16string_view trim(std::u16string_view text) {
    while(!text.empty() && stdlib::unicode_whitespace(text.front()))text.remove_prefix(1);
    while(!text.empty() && stdlib::unicode_whitespace(text.back()))text.remove_suffix(1);
    return text;
}
std::optional<std::u16string_view> framework_type_name(std::u16string_view text) {
    std::size_t brackets=0;
    for(std::size_t i=0;i<text.size();++i){
        if(text[i]==u'[')++brackets;
        if(text[i]==u']'){
            if(!brackets)return {};
            --brackets;
        }
        if(text[i]!=u',' || brackets)continue;
        const auto suffix=text.substr(i+1);
        const auto assembly=lowercase(trim(suffix.substr(0,suffix.find(u','))));
        if(assembly!=u"mscorlib" && assembly!=u"mscorlib.dll")return {};
        auto name=text.substr(0,i);
        while(!name.empty() && stdlib::unicode_whitespace(name.front()))name.remove_prefix(1);
        return name;
    }
    if(brackets)return {};
    return text;
}
std::optional<std::u16string_view> conversion_base_name(std::u16string_view name) {
    bool allow_reference=true;
    while(!name.empty()){
        if(name.back()==u'&'){
            if(!allow_reference)return {};
            name.remove_suffix(1);
        }else if(name.back()==u'*')name.remove_suffix(1);
        else if(name.back()==u']'){
            const auto open=name.rfind(u'[');
            if(open==name.npos)return {};
            const auto shape=trim(name.substr(open+1,name.size()-open-2));
            if(shape!=u"*"){
                unsigned rank=1;
                for(const auto unit:shape){
                    if(unit==u',')++rank;
                    else if(!stdlib::unicode_whitespace(unit))return {};
                }
                if(rank>32)return {};
            }
            name=name.substr(0,open);
        }else break;
        allow_reference=false;
    }
    return name;
}
std::u16string_view comparison_text(std::u16string_view text) {
    while(!text.empty() && (text.back()==u' ' || text.back()==u'\u3000'))text.remove_suffix(1);
    return text;
}
bool null(const Value& value) { return std::holds_alternative<NilString>(value); }
bool unsigned_integer_type(Type type) {
    return type==Type::byte || type==Type::word || wide_unsigned(type);
}
double rounded_integral(double input) {
    const auto floor=std::floor(input);
    const auto fraction=input-floor;
    return fraction>0.5 || (fraction==0.5 && std::fmod(floor,2)!=0) ? floor+1 : floor;
}
int floating_order(double left,double right) {
    if(std::isnan(left))return std::isnan(right) ? 0 : -1;
    if(std::isnan(right))return 1;
    return (left>right)-(left<right);
}
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
    case Symbol::bit_xor:return 1;
    case Symbol::disjunction:case Symbol::bit_or:return 2;
    case Symbol::conjunction:case Symbol::bit_and:return 3;
    case Symbol::bit_not:return 4;
    case Symbol::is:return 5;
    case Symbol::membership:case Symbol::between:case Symbol::like:return 6;
    case Symbol::equal:case Symbol::unequal:case Symbol::less:case Symbol::less_equal:
    case Symbol::greater:case Symbol::greater_equal:return 7;
    case Symbol::plus:case Symbol::minus:return 8;
    case Symbol::modulus:return 9;
    case Symbol::multiply:case Symbol::divide:return 10;
    default:return 0;
    }
}
std::string_view unsupported_operator(Symbol kind) {
    switch(kind){
    case Symbol::between:return "BETWEEN";
    case Symbol::bit_and:return "&";
    case Symbol::bit_or:return "|";
    case Symbol::bit_xor:return "^";
    case Symbol::bit_not:return "~";
    default:return {};
    }
}

class Evaluation {
    Values& values_;
    const ExecutionOptions& options_;
    SourceLocation location_;
    Type decimal_;
    Type date_;
    Type interval_;
    Type guid_;
    Type offset_date_;
    std::u16string_view source_;
    std::size_t cursor_=0, work_=0;
    std::vector<Node> nodes_;
    Token token_{Symbol::end,{}};
    std::optional<BindingError> binding_error_;

    [[noreturn]] void syntax() const {
        throw Error("E_FORMAT",location_,"Invalid Utils evaluation expression","Некорректное выражение Utils");
    }
    [[noreturn]] void operands() const {
        throw Error("E_TYPE",location_,"Unsupported operand types in Utils expression","Несовместимые типы операндов в выражении Utils");
    }
    [[noreturn]] void aggregate_syntax() const {
        throw Error("E_FORMAT",location_,"A Utils aggregate requires one column, optionally qualified by Child",
            "Агрегат Utils требует один столбец, возможно с квалификатором Child");
    }
    [[noreturn]] void parent_syntax() const {
        throw Error("E_FORMAT",location_,"Expected Parent[(relation)].column in Utils expression",
            "В выражении Utils ожидается Parent[(отношение)].столбец");
    }
    [[noreturn]] void wrong_argument_count(std::u16string_view function) const {
        const auto name=to_utf8(function)+"()";
        throw Error("E_TYPE",location_,"Wrong number of arguments to "+name,
            "Недопустимое число аргументов функции "+name);
    }
    std::size_t unresolved(MissingBinding kind, std::u16string name={}) {
        if(!binding_error_)binding_error_=BindingError{kind,std::move(name)};
        return add({Symbol::name,NilString{},0,0,true});
    }
    void check_binding() const {
        if(!binding_error_)return;
        const auto name=to_utf8(binding_error_->name);
        switch(binding_error_->kind){
        case MissingBinding::column:
            throw Error("E_NAME",location_,"Cannot find column ["+name+"] in Utils expression",
                "В выражении Utils не найден столбец ["+name+"]");
        case MissingBinding::parent_relation:
            throw Error("E_NAME",location_,"Cannot find parent relation '"+name+"' in Utils expression",
                "В выражении Utils не найдено родительское отношение '"+name+"'");
        case MissingBinding::default_parent:
            throw Error("E_NAME",location_,"Cannot find the default parent relation in Utils expression",
                "В выражении Utils не найдено родительское отношение по умолчанию");
        case MissingBinding::default_child:
            throw Error("E_NAME",location_,"Cannot bind an aggregate to a child relation in Utils expression",
                "Не удалось связать агрегат с дочерним отношением в выражении Utils");
        }
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
        case u'&':return {Symbol::bit_and,{}};
        case u'|':return {Symbol::bit_or,{}};
        case u'^':return {Symbol::bit_xor,{}};
        case u'~':return {Symbol::bit_not,{}};
        case u'=':return {Symbol::equal,{}};
        case u'[':case u'`':{
            const auto close=ch==u'[' ? u']' : ch;
            std::u16string name;
            while(cursor_<source_.size()){
                auto unit=source_[cursor_++];
                if(unit==close){
                    if(name.empty())syntax();
                    return {Symbol::name,std::move(name)};
                }
                if(ch==u'[' && unit==u'\\' && cursor_<source_.size()
                    && (source_[cursor_]==close || source_[cursor_]==u'\\'))unit=source_[cursor_++];
                name.push_back(unit);
            }
            syntax();
        }
        case u'#':{
            const auto end=source_.find(u'#',cursor_);
            if(end==source_.npos || end==cursor_)syntax();
            auto text=std::u16string(source_.substr(cursor_,end-cursor_));
            cursor_=end+1;
            return {Symbol::date_literal,std::move(text)};
        }
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
        if(ch==u'.')return {Symbol::dot,{}};
        if(name_character(ch)){
            while(cursor_<source_.size() && (name_character(source_[cursor_]) || digit(source_[cursor_])))++cursor_;
            const auto spelling=source_.substr(begin,cursor_-begin);
            const auto word=lowercase(spelling);
            if(word==u"and")return {Symbol::conjunction,{}};
            if(word==u"or")return {Symbol::disjunction,{}};
            if(word==u"not")return {Symbol::negate,{}};
            if(word==u"is")return {Symbol::is,{}};
            if(word==u"null")return {Symbol::null_value,{}};
            if(word==u"true")return {Symbol::true_value,{}};
            if(word==u"false")return {Symbol::false_value,{}};
            if(word==u"in")return {Symbol::membership,{}};
            if(word==u"like")return {Symbol::like,{}};
            if(word==u"between")return {Symbol::between,{}};
            if(word==u"parent")return {Symbol::parent,{}};
            if(word==u"child")return {Symbol::child,{}};
            return {Symbol::name,std::u16string(spelling)};
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
    Value calendar_call(CalendarMethod method,std::span<const Value> arguments={}) {
        if(date_==Type::invalid)throw runtime_error(RuntimeError::invalid_bytecode,location_);
        return calendar_invoke(values_,date_,static_cast<std::size_t>(method),arguments);
    }
    unsigned month_name(std::u16string_view token) const {
        constexpr std::array<std::u16string_view,12> names{
            u"january",u"february",u"march",u"april",u"may",u"june",
            u"july",u"august",u"september",u"october",u"november",u"december"};
        if(!token.empty() && token.back()==u'.')token.remove_suffix(1);
        std::u16string name(token);
        for(auto& ch:name)ch=lower_ascii(ch);
        for(std::size_t i=0;i<names.size();++i)
            if(names[i]==name || (name.size()==3 && names[i].starts_with(name)))return static_cast<unsigned>(i+1);
        return 0;
    }
    std::u16string normalized_date(std::u16string_view value) {
        const auto text=trim(value);
        const auto numeric=[](std::u16string_view part){return !part.empty() && std::all_of(part.begin(),part.end(),digit);};
        const auto colon=text.find(u':');
        if(colon!=text.npos && numeric(text.substr(0,colon))){
            return calendar_text(calendar_call(CalendarMethod::today),u"yyyy-MM-dd",location_)+u" "+std::u16string(text);
        }
        const auto date_end=text.find_first_of(u" Tt");
        const auto prefix=text.substr(0,date_end);
        const auto slash=prefix.find(u'/');
        if(slash!=prefix.npos && numeric(prefix.substr(0,slash)) && numeric(prefix.substr(slash+1))){
            auto result=std::u16string(prefix)+u"/"+calendar_text(calendar_call(CalendarMethod::today),u"yyyy",location_);
            if(date_end!=text.npos)result.append(text.substr(date_end));
            return result;
        }
        if(std::any_of(text.begin(),text.end(),letter)){
            std::array<std::u16string_view,3> words{};
            auto remaining=text;
            for(auto& word:words){
                const auto end=remaining.find_first_of(u" ,");
                word=remaining.substr(0,end);
                remaining.remove_prefix(word.size());
                while(!remaining.empty() && (remaining.front()==u' ' || remaining.front()==u','))remaining.remove_prefix(1);
            }
            auto month=month_name(words[0]);
            auto day=words[1],year=words[2];
            if(!month){
                month=month_name(words[1]);
                day=words[0];
                if(day.size()==4)std::swap(day,year);
            }
            if(month && numeric(day) && numeric(year)){
                auto result=from_utf8(std::to_string(month))+u"/"+std::u16string(day)+u"/"+std::u16string(year);
                if(!remaining.empty()){result.push_back(u' ');result.append(remaining);}
                return result;
            }
        }
        return std::u16string(text);
    }
    Value date_value(const Value& value) {
        if(kind(value)==date_)return value;
        const auto* text=std::get_if<std::u16string>(&value);
        if(!text)operands();
        const std::array<Value,1> input{normalized_date(*text)};
        return calendar_call(CalendarMethod::parse,input);
    }
    Value duration_value(const Value& value) {
        const auto type=kind(value);
        if(type==interval_)return value;
        std::int64_t ticks;
        if(const auto* text=std::get_if<std::u16string>(&value))ticks=parse_xml_duration(trim(*text),location_);
        else if(type==Type::integer || type==Type::int64)ticks=integral_number(value,location_);
        else operands();
        const std::array<Value,1> arguments{ticks};
        return calendar_invoke(values_,interval_,static_cast<std::size_t>(CalendarMethod::from_ticks),arguments);
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
    std::size_t parse_function(Symbol kind,std::u16string_view name,unsigned depth) {
        if(advance()!=Symbol::open)syntax();
        std::array<std::size_t,3> arguments{};
        const auto count=argument_count(kind);
        for(unsigned n=0;n<count;++n){
            if(token_.kind==Symbol::close)wrong_argument_count(name);
            if(n!=0 && advance()!=Symbol::comma)syntax();
            arguments[n]=parse(depth+1);
        }
        if(token_.kind==Symbol::comma)wrong_argument_count(name);
        if(advance()!=Symbol::close)syntax();
        return add({kind,NilString{},arguments[0],arguments[1],false,arguments[2]});
    }
    std::u16string dotted_name(std::u16string name) {
        while(token_.kind==Symbol::dot){
            advance();
            if(token_.kind!=Symbol::name)syntax();
            name.push_back(u'.');
            name+=token_.text;
            advance();
        }
        return name;
    }
    std::pair<std::u16string,std::u16string> relation_column(bool parent) {
        const auto bad_syntax=[&]{if(parent)parent_syntax();aggregate_syntax();};
        std::u16string relation;
        if(token_.kind==Symbol::open){
            advance();
            if(token_.kind!=Symbol::name)bad_syntax();
            relation=std::move(token_.text);
            advance();
            if(advance()!=Symbol::close)bad_syntax();
        }
        if(advance()!=Symbol::dot || token_.kind!=Symbol::name)bad_syntax();
        auto column=std::move(token_.text);
        advance();
        return {std::move(relation),std::move(column)};
    }
    std::size_t parse_aggregate() {
        advance();
        std::u16string relation;
        std::u16string column;
        const bool child=token_.kind==Symbol::child;
        if(child){
            advance();
            auto names=relation_column(false);
            relation=std::move(names.first);
            column=std::move(names.second);
        }else{
            if(token_.kind!=Symbol::name)aggregate_syntax();
            column=std::move(token_.text);
            advance();
        }
        if(advance()!=Symbol::close)aggregate_syntax();
        if(child && relation.empty())return unresolved(MissingBinding::default_child);
        return unresolved(MissingBinding::column,std::move(column));
    }
    std::size_t parse_name(std::u16string name,unsigned depth) {
        if(token_.kind!=Symbol::open)return unresolved(MissingBinding::column,dotted_name(std::move(name)));
        const auto word=lowercase(name);
        const std::pair<std::u16string_view,Symbol> functions[]{
            {u"len",Symbol::length},{u"trim",Symbol::trim},{u"substring",Symbol::substring},
            {u"iif",Symbol::choose},{u"isnull",Symbol::replace_null},{u"abs",Symbol::absolute}};
        for(const auto& [spelling,kind]:functions)
            if(word==spelling)return parse_function(kind,name,depth);
        if(word==u"convert")return parse_conversion(depth);
        constexpr std::u16string_view aggregates[]{u"sum",u"avg",u"min",u"max",u"count",u"stdev",u"var"};
        if(std::find(std::begin(aggregates),std::end(aggregates),word)!=std::end(aggregates))return parse_aggregate();
        const auto called=to_utf8(name)+"()";
        throw Error("E_NAME",location_,"Unknown function "+called+" in Utils expression",
            "Неизвестная функция "+called+" в выражении Utils");
    }
    std::size_t parse_membership_arguments(unsigned depth) {
        if(advance()!=Symbol::open)syntax();
        Node test{Symbol::membership_arguments};
        test.members.push_back(parse(depth+1));
        while(token_.kind==Symbol::comma){
            advance();
            test.members.push_back(parse(depth+1));
        }
        if(advance()!=Symbol::close)syntax();
        return add(std::move(test));
    }
    std::optional<Type> simple_conversion_type(std::u16string_view name) {
        if(name==u"System.Object")return Type::invalid; // Object preserves the operand's scalar value.
        constexpr std::u16string_view null_only[]{u"System.DBNull",u"System.IntPtr",u"System.UIntPtr",u"System.Type",
            u"System.Version",u"System.Exception",u"System.Collections.ArrayList",u"System.ValueType",u"System.Void"};
        if(std::find(std::begin(null_only),std::end(null_only),name)!=std::end(null_only))return Type::nil;
        if(name==u"System.Boolean")return Type::boolean;
        if(name==u"System.Char")return Type::character;
        if(name==u"System.SByte")return Type::shortint;
        if(name==u"System.Byte")return Type::byte;
        if(name==u"System.Int16")return Type::smallint;
        if(name==u"System.UInt16")return Type::word;
        if(name==u"System.Int32")return Type::integer;
        if(name==u"System.UInt32")return Type::longword;
        if(name==u"System.Int64")return Type::int64;
        if(name==u"System.UInt64")return Type::uint64;
        if(name==u"System.Single")return Type::single;
        if(name==u"System.Double")return Type::real;
        if(name==u"System.Decimal")return decimal_;
        if(name==u"System.DateTime")return date_;
        if(name==u"System.TimeSpan")return interval_;
        if(name==u"System.Guid")return guid_;
        if(name==u"System.DateTimeOffset")return offset_date_;
        if(name==u"System.String")return Type::string;
        return {};
    }
    bool sql_value(Type type) const {
        return compound(type) && sql_scalar_record(type_info(values_.program,type));
    }
    Type sql_base(Type type) const {
        return sql_value(type) ? type_info(values_.program,type).element : type;
    }
    Type sql_storage(Type primitive) const {
        for(std::size_t i=0;i<values_.program.types.size();++i){
            const auto& shape=values_.program.types[i];
            if(sql_scalar_record(shape) && shape.element==primitive)return static_cast<Type>(64+i);
        }
        throw runtime_error(RuntimeError::invalid_bytecode,location_);
    }
    Value sql_construct(Type owner,Value value) {
        const std::array<Value,1> arguments{std::move(value)};
        return sql_scalar_invoke(values_,owner,SqlScalarMethod::create,arguments);
    }
    Value sql_payload(Type owner,const Value& value) {
        const std::array<Value,1> arguments{sql_scalar_convert(values_,owner,kind(value),value)};
        return sql_scalar_invoke(values_,owner,SqlScalarMethod::value,arguments);
    }
    bool sql_flag(Type type) const {return sql_value(type) && sql_base(type)==Type::boolean;}
    Type conversion_type(std::u16string_view source) {
        constexpr std::u16string_view prefix=u"System.Data.SqlTypes.";
        if(source.starts_with(prefix)){
            const auto name=source.substr(prefix.size());
            const auto types=sql_scalar_types();
            const auto sql=std::ranges::find(types,name,&SqlScalarType::framework_name);
            if(sql!=types.end())
                for(std::size_t i=0;i<values_.program.types.size();++i){
                    const auto& shape=values_.program.types[i];
                    if(sql_scalar_record(shape) && shape.name==sql->name)return static_cast<Type>(64+i);
                }
        }
        if(const auto name=framework_type_name(source)){
            if(const auto base=conversion_base_name(*name)){
                if(const auto type=simple_conversion_type(*base))
                    return base->size()==name->size() ? *type : Type::nil;
            }
        }
        throw Error("E_UNSUPPORTED",location_,"Unknown or unsupported CONVERT target type",
            "Неизвестный или неподдерживаемый тип результата CONVERT");
    }
    std::size_t parse_conversion(unsigned depth) {
        if(advance()!=Symbol::open)syntax();
        const auto value=parse(depth+1);
        if(advance()!=Symbol::comma)syntax();
        if(token_.kind!=Symbol::literal && token_.kind!=Symbol::name)syntax();
        const bool quoted=token_.kind==Symbol::literal;
        auto name=std::move(token_.text);
        advance();
        if(!quoted)name=dotted_name(std::move(name));
        if(advance()!=Symbol::close)syntax();
        return add({Symbol::conversion,std::move(name),value});
    }
    std::size_t parse_atom(unsigned depth) {
        auto initial=std::move(token_);
        advance();
        switch(initial.kind){
        case Symbol::number:return add({initial.kind,number(std::move(initial.text)),0,0,true});
        case Symbol::date_literal:return add({initial.kind,date_value(Value{std::move(initial.text)}),0,0,true});
        case Symbol::literal:return add({initial.kind,std::move(initial.text),0,0,true});
        case Symbol::open:{
            const auto value=parse(depth+1);
            if(advance()!=Symbol::close)syntax();
            return add({Symbol::grouped,NilString{},value});
        }
        case Symbol::name:return parse_name(std::move(initial.text),depth);
        case Symbol::parent:{
            auto relation=relation_column(true).first;
            return relation.empty() ? unresolved(MissingBinding::default_parent)
                : unresolved(MissingBinding::parent_relation,std::move(relation));
        }
        default:syntax();
        }
    }
    std::size_t binary_node(Symbol kind,std::size_t left,std::size_t right) {
        if(const auto name=unsupported_operator(kind);!name.empty())
            throw Error("E_UNSUPPORTED",location_,"Unsupported Utils operator "+std::string(name),
                "В выражении Utils не поддерживается оператор "+std::string(name));
        if(kind==Symbol::is){
            const bool negative=nodes_[right].kind==Symbol::negate;
            const auto tested=negative ? nodes_[right].left : right;
            if(nodes_[tested].kind!=Symbol::null_value)return add({Symbol::invalid_is,NilString{},left});
            return add({negative ? Symbol::is_not_null : Symbol::is_null,NilString{},left});
        }
        Node node{kind,NilString{},left,right};
        if(kind==Symbol::membership && nodes_[right].kind==Symbol::membership_arguments)
            node.members=std::move(nodes_[right].members);
        return add(std::move(node));
    }
    void reduce(std::vector<PendingOperator>& operators,std::vector<std::size_t>& operands,unsigned priority) {
        while(!operators.empty() && operators.back().priority>=priority){
            step();
            const auto operation=operators.back();
            operators.pop_back();
            if(operands.size()<operation.arity)
                throw Error("E_RANGE",location_,"Utils operator has no value to apply to",
                    "В выражении Utils оператору недостаёт значения");
            if(operation.arity==0){
                const auto value=operation.kind==Symbol::null_value ? Value{NilString{}}
                    : Value{operation.kind==Symbol::true_value};
                operands.push_back(add({operation.kind,value,0,0,true}));
            }else if(operation.arity==1){
                operands.back()=add({operation.kind,NilString{},operands.back()});
            }else{
                const auto right=operands.back();
                operands.pop_back();
                operands.back()=binary_node(operation.kind,operands.back(),right);
            }
        }
    }
    std::size_t parse(unsigned depth) {
        step(depth);
        std::vector<PendingOperator> operators;
        std::vector<std::size_t> operands;
        bool needs_operand=true;
        while(token_.kind!=Symbol::end && token_.kind!=Symbol::close && token_.kind!=Symbol::comma){
            const auto kind=token_.kind;
            const bool sign=kind==Symbol::plus || kind==Symbol::minus;
            if(kind==Symbol::negate || (needs_operand && sign)){
                operators.push_back({advance(),kind==Symbol::negate ? 4U : 11U,1});
                continue;
            }
            if(const auto priority=precedence(kind);priority){
                if(needs_operand)syntax();
                reduce(operators,operands,priority);
                operators.push_back({advance(),priority,2});
                needs_operand=true;
                if(kind==Symbol::membership && token_.kind!=Symbol::parent){
                    operands.push_back(parse_membership_arguments(depth));
                    needs_operand=false;
                }
                continue;
            }
            if(!needs_operand)syntax();
            // These three constants are emitted when operators are reduced, including a following NOT.
            if(kind==Symbol::null_value || kind==Symbol::true_value || kind==Symbol::false_value)
                operators.push_back({advance(),12,0});
            else operands.push_back(parse_atom(depth));
            needs_operand=false;
        }
        if(needs_operand)syntax();
        reduce(operators,operands,0);
        if(operands.size()!=1)syntax();
        return operands.front();
    }
    Type kind(const Value& value) const {
        if(const auto* object=std::get_if<std::shared_ptr<Aggregate>>(&value))return (*object)->type;
        return value_type(value);
    }
    double real(const Value& value) {
        const auto type=kind(value);
        if(type==Type::real)return std::get<double>(value);
        if(type==Type::single)return std::get<float>(value);
        if(wide_unsigned(type))return static_cast<double>(unsigned_number(value,location_));
        if(type==Type::boolean || integer_type(type))return static_cast<double>(integral_number(value,location_));
        if(type==decimal_)return std::get<double>(decimal_call(DecimalMethod::to_real,value));
        if(type==Type::string)return real_text(value);
        operands();
    }
    float single(const Value& value) {
        const auto type=kind(value);
        if(type==Type::single)return std::get<float>(value);
        if(type==decimal_)return std::get<float>(decimal_call(DecimalMethod::to_single,value));
        if(wide_unsigned(type))return static_cast<float>(unsigned_number(value,location_));
        if(type==Type::boolean || integer_type(type))return static_cast<float>(integral_number(value,location_));
        const auto number=real(value);
        if(std::isnan(number))return std::numeric_limits<float>::quiet_NaN();
        if(std::abs(number)>=0x1.ffffffp127){
            if(type==Type::string && std::isfinite(number))throw runtime_error(RuntimeError::real_overflow,location_);
            return std::signbit(number) ? -std::numeric_limits<float>::infinity() : std::numeric_limits<float>::infinity();
        }
        const auto maximum=std::numeric_limits<float>::max();
        if(number>maximum)return maximum;
        if(number< -maximum)return -maximum;
        return static_cast<float>(number);
    }
    std::u16string text(const Value& value) {
        if(null(value))return {};
        const auto type=kind(value);
        if(type==Type::string)return std::get<std::u16string>(value);
        NumberFormatSettings format;
        format.positive_infinity=u"∞";
        format.negative_infinity=u"-∞";
        if(type==decimal_)return format_decimal(value,{},format,location_);
        if(type==date_ || type==interval_)return calendar_text(value,{},location_);
        if(type==guid_)return guid_text(value,{},location_);
        if(type==offset_date_)return date_offset_text(value,{},location_);
        if(sql_value(type))return sql_scalar_text(value,location_);
        if(type==Type::real || type==Type::single || integer_type(type))
            return format_number(value,type,{},format,location_);
        return format_scalar(value,location_);
    }
    Value as_decimal(const Value& value) {
        const auto type=kind(value);
        if(type==decimal_)return value;
        if(type==Type::string)return decimal_call(DecimalMethod::parse,value);
        if(type==Type::real)return decimal_call(DecimalMethod::from_real,value);
        if(type==Type::single)return decimal_call(DecimalMethod::from_single,value);
        if(wide_unsigned(type))
            return decimal_call(DecimalMethod::from_unsigned,Value{unsigned_number(value,location_)});
        if(type==Type::character)operands();
        return decimal_call(DecimalMethod::from_signed,Value{integral_number(value,location_)});
    }
    Value signed_integer(const Value& value,Type target) {
        const auto type=kind(value);
        if(type==Type::boolean || integer_type(type) || type==Type::character)
            return checked_signed_integer(target,value,location_);
        if(type==decimal_)
            return decimal_call(target==Type::integer ? DecimalMethod::to_int32 : DecimalMethod::to_int64,
                decimal_call(DecimalMethod::round,value));
        if(type==Type::string) {
            const auto answer=stdlib::try_parse_number(target,value,location_);
            if(!answer)throw runtime_error(RuntimeError::number_text,location_,type_name(target));
            return *answer;
        }
        if(type!=Type::real && type!=Type::single)operands();
        const auto rounded=rounded_integral(real(value));
        constexpr double int64_limit=0x1p63;
        if(!std::isfinite(rounded) || rounded< -int64_limit || rounded>=int64_limit)
            throw runtime_error(RuntimeError::integer_overflow,location_);
        return checked_signed_integer(target,Value{static_cast<std::int64_t>(rounded)},location_);
    }
    Value unsigned_integer(const Value& value,Type target) {
        const auto type=kind(value);
        if(type==Type::string){
            const auto answer=stdlib::try_parse_number(target,value,location_);
            if(!answer)throw runtime_error(RuntimeError::number_text,location_,type_name(target));
            return *answer;
        }
        if(type==decimal_)
            return checked_unsigned_integer(target,decimal_call(DecimalMethod::to_uint64,
                decimal_call(DecimalMethod::round,value)),location_);
        if(type==Type::single || type==Type::real){
            const auto rounded=rounded_integral(real(value));
            if(!std::isfinite(rounded) || rounded<0 || rounded>=0x1p64)
                throw runtime_error(RuntimeError::integer_overflow,location_);
            return checked_unsigned_integer(target,Value{static_cast<std::uint64_t>(rounded)},location_);
        }
        return checked_unsigned_integer(target,value,location_);
    }
    unsigned numeric_rank(Type type) const {
        if(type==decimal_)return 9;
        switch(type){
        case Type::shortint:return 1;
        case Type::byte:return 2;
        case Type::smallint:return 3;
        case Type::word:return 4;
        case Type::integer:return 5;
        case Type::longword:return 6;
        case Type::int64:return 7;
        case Type::uint64:return 8;
        case Type::single:return 10;
        case Type::real:return 11;
        default:return 0;
        }
    }
    Type numeric_type(Type a,Type b) {
        const auto higher=numeric_rank(a)>=numeric_rank(b) ? a : b;
        if(!integer_type(a) || !integer_type(b)
            || unsigned_integer_type(a)==unsigned_integer_type(b) || !unsigned_integer_type(higher))return higher;
        switch(higher){
        case Type::byte:return Type::smallint;
        case Type::word:return Type::integer;
        case Type::longword:return Type::int64;
        default:operands();
        }
    }
    Type sql_result_base(Type left,Type right) {
        const auto a=sql_base(left),b=sql_base(right);
        const auto number_a=numeric_rank(a),number_b=numeric_rank(b);
        if(!number_a || !number_b){
            if(a==Type::boolean && b==Type::boolean)return Type::boolean;
            if((a==Type::string || a==Type::character) && (b==Type::string || b==Type::character))
                return Type::string;
            operands();
        }
        const auto selected=number_a>=number_b ? a : b;
        if(selected==decimal_ || selected==Type::uint64)return decimal_;
        if(a==Type::byte && b==Type::byte && sql_value(left)!=sql_value(right))return Type::smallint;
        switch(selected){
        case Type::shortint:return Type::byte;
        case Type::word:return Type::integer;
        case Type::longword:return Type::int64;
        case Type::int64:case Type::integer:case Type::smallint:case Type::byte:
        case Type::single:case Type::real:return selected;
        default:operands();
        }
    }
    BigInteger wide_integer(const Value& value) {
        if(wide_unsigned(kind(value)))return BigInteger::from_unsigned(unsigned_number(value,location_));
        return BigInteger::from_signed(integral_number(value,location_));
    }
    Value arithmetic(Symbol op,const Value& left,const Value& right) {
        const auto a=kind(left), b=kind(right);
        if(a==guid_ || b==guid_ || a==offset_date_ || b==offset_date_)operands();
        const bool sql=sql_value(a) || sql_value(b);
        const bool sql_string=(sql_value(a) && sql_base(a)==Type::string) || (sql_value(b) && sql_base(b)==Type::string);
        if(sql_string && op==Symbol::plus){
            const auto owner=sql_storage(Type::string);
            return sql_construct(owner,arithmetic(op,sql_payload(owner,left),sql_payload(owner,right)));
        }
        if(op==Symbol::plus && (a==Type::string || b==Type::string || (a==Type::character && b==Type::character))){
            auto result=text(left);
            const auto suffix=text(right);
            if(result.size()+suffix.size()>1'048'576)throw runtime_error(RuntimeError::string_limit,location_);
            result+=suffix;
            return result;
        }
        if(sql)return sql_arithmetic(op,left,right);
        if(!numeric_rank(a) || !numeric_rank(b) || (op==Symbol::modulus && (!integer_type(a) || !integer_type(b))))operands();
        const auto type=op==Symbol::divide && integer_type(a) && integer_type(b) ? Type::real : numeric_type(a,b);
        if(type==Type::real || type==Type::single){
            const auto calculate=[op](auto x,auto y)->Value{
            switch(op) {
            case Symbol::plus:return x+y;
            case Symbol::minus:return x-y;
            case Symbol::multiply:return x*y;
            default:return x/y;
            }
            };
            return type==Type::single ? calculate(single(left),single(right)) : calculate(real(left),real(right));
        }
        if(type==decimal_){
            using M=DecimalMethod;
            const auto method=op==Symbol::plus ? M::add : op==Symbol::minus ? M::subtract
                : op==Symbol::multiply ? M::multiply : M::divide;
            return decimal_call(method,std::array<Value,2>{as_decimal(left),as_decimal(right)});
        }
        Op operation=Op::modulo;
        if(op==Symbol::plus)operation=Op::add;
        if(op==Symbol::minus)operation=Op::subtract;
        if(op==Symbol::multiply)operation=Op::multiply;
        const auto exact=big_binary(operation,wide_integer(left),wide_integer(right),location_,options_);
        if(wide_unsigned(type))
            return checked_unsigned_integer(type,Value{checked_big(location_,[&]{return exact.to_unsigned();})},location_);
        const auto result=checked_big(location_,[&]{return exact.to_signed();});
        if(small_integer(type))return checked_small_integer(type,result,location_);
        return checked_signed_integer(type,Value{result},location_);
    }
    Value sql_arithmetic(Symbol op,const Value& left,const Value& right) {
        auto primitive=sql_result_base(kind(left),kind(right));
        if(!numeric_rank(primitive) || (op==Symbol::modulus && !integer_type(primitive)))operands();
        if(primitive==decimal_)
            throw Error("E_UNSUPPORTED",location_,"This operation requires SqlDecimal",
                "Для этой операции требуется SqlDecimal");
        if(op==Symbol::divide && integer_type(primitive))primitive=Type::real;
        const auto owner=sql_storage(primitive);
        if(op==Symbol::modulus){
            const auto int64=sql_storage(Type::int64);
            const auto x=std::get<std::int64_t>(sql_payload(int64,left));
            const auto y=std::get<std::int64_t>(sql_payload(int64,right));
            if(y==0)throw runtime_error(RuntimeError::division_zero,location_);
            if(x==INT64_MIN && y==-1)throw runtime_error(RuntimeError::integer_overflow,location_);
            const Value remainder{x%y};
            const auto narrowed=small_integer(primitive) ? checked_small_integer(primitive,x%y,location_)
                : checked_signed_integer(primitive,remainder,location_);
            return sql_construct(owner,narrowed);
        }
        const auto x=sql_payload(owner,left),y=sql_payload(owner,right);
        if(op==Symbol::divide && real(y)==0)throw runtime_error(RuntimeError::division_zero,location_);
        return sql_construct(owner,arithmetic(op,x,y));
    }
    int sql_compare(Type owner,const Value& left,const Value& right,bool equality) {
        const auto base=sql_base(owner);
        if(base==Type::string)
            return compare(Value{text(left)},Value{text(right)},equality);
        return compare(sql_payload(owner,left),sql_payload(owner,right),equality);
    }
    int compare(const Value& left,const Value& right,bool equality=true) {
        const auto a=kind(left),b=kind(right);
        if(sql_value(a) || sql_value(b)){
            const auto primitive=sql_result_base(a,b);
            if(primitive==decimal_){
                const auto x=as_decimal(sql_value(a) ? sql_payload(a,left) : left);
                const auto y=as_decimal(sql_value(b) ? sql_payload(b,right) : right);
                return compare_decimal_values(x,y,location_);
            }
            return sql_compare(sql_storage(primitive),left,right,equality);
        }
        if(a==offset_date_ || b==offset_date_){
            if(a!=b)operands();
            return compare_date_offsets(left,right);
        }
        if(a==guid_ || b==guid_){
            if(a==guid_ && b==guid_)return compare_guid(left,right);
            if(a!=Type::string && b!=Type::string)operands();
            return compare_culture_strings(comparison_text(text(left)),comparison_text(text(right)),location_,options_,true);
        }
        if(a==date_ || b==date_)return compare_calendar_values(date_value(left),date_value(right));
        if((a==Type::string && b==Type::character) || (a==Type::character && b==Type::string))
            return compare(Value{text(left)},Value{text(right)},equality);
        if(a==Type::character && b==Type::character){
            const auto x=std::get<char16_t>(left),y=std::get<char16_t>(right);
            return (x>y)-(x<y);
        }
        if(a==Type::string && b==Type::string)
            return compare_culture_strings(comparison_text(std::get<std::u16string>(left)),
                comparison_text(std::get<std::u16string>(right)),location_,options_,true);
        if((a==Type::boolean || a==Type::string) && (b==Type::boolean || b==Type::string)){
            if(!equality)operands();
            return static_cast<int>(expression_boolean(left))-static_cast<int>(expression_boolean(right));
        }
        const auto type=numeric_type(a,b);
        if(type==Type::real)return floating_order(real(left),real(right));
        if(type==Type::single)return floating_order(single(left),single(right));
        if(type==decimal_ || type==Type::int64 || wide_unsigned(type))
            return compare_decimal_values(as_decimal(left),as_decimal(right),location_);
        if(integer_type(type)){
            const auto x=std::get<std::int32_t>(signed_integer(left,Type::integer));
            const auto y=std::get<std::int32_t>(signed_integer(right,Type::integer));
            return (x>y)-(x<y);
        }
        operands();
    }
    bool boolean(const Value& value) {
        if(const auto* logical=std::get_if<bool>(&value))return *logical;
        operands();
    }
    bool logic_flag(const Value& value) {
        const auto type=kind(value);
        if(sql_flag(type))return std::get<bool>(sql_payload(type,value));
        return boolean(value);
    }
    bool expression_boolean(const Value& value) {
        if(const auto* text=std::get_if<std::u16string>(&value)){
            if(const auto converted=boolean_text(*text))return *converted;
            operands();
        }
        return boolean(value);
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
            return expression_boolean(first) ? second : third;
        }
        if(null(first))return first;
        const auto* text=std::get_if<std::u16string>(&first);
        if(!text)operands();
        if(node.kind==Symbol::length)return static_cast<std::int32_t>(text->size());
        if(node.kind==Symbol::trim)return std::u16string(trim(*text));
        if(null(second) || null(third))return NilString{};
        const auto index=[&](const Value& argument){
            if(!integer_type(kind(argument)))operands();
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
    Value evaluate_membership(const Node& node,unsigned depth) {
        const auto left=evaluate(node.left,depth);
        if(node.members.empty()){
            (void)evaluate(node.right,depth);
            throw Error("E_UNSUPPORTED",location_,"IN requires an expression list as its right operand",
                "Правая часть IN должна быть списком выражений");
        }
        std::vector<Value> candidates;
        candidates.reserve(node.members.size());
        for(const auto member:node.members)candidates.push_back(evaluate(member,depth));
        if(null(left))return left;
        const auto type=kind(left);
        for(const auto& candidate:candidates){
            if(null(candidate))continue;
            if(type==Type::string){
                if(compare(left,Value{text(candidate)})==0)return true;
            }else if(type==date_){
                if(compare_calendar_values(left,date_value(candidate))==0)return true;
            }else if(type==guid_){
                if(kind(candidate)!=guid_)operands();
                if(compare_guid(left,candidate)==0)return true;
            }else if(type==offset_date_){
                if(kind(candidate)!=offset_date_)operands();
                if(compare_date_offsets(left,candidate)==0)return true;
            }else if(sql_value(type)){
                if(sql_compare(type,left,candidate,true)==0)return true;
            }else if(type==Type::real){
                if(floating_order(real(left),real(candidate))==0)return true;
            }else if(type==Type::single){
                if(floating_order(single(left),single(candidate))==0)return true;
            }else if(type==decimal_ || type==Type::int64 || wide_unsigned(type)){
                if(compare_decimal_values(as_decimal(left),as_decimal(candidate),location_)==0)return true;
            }else if(integer_type(type) || type==Type::character){
                if(integral_number(left,location_)==std::get<std::int32_t>(signed_integer(candidate,Type::integer)))return true;
            }else if(type==Type::boolean){
                if(std::get<bool>(left)==expression_boolean(candidate))return true;
            }else operands();
        }
        return false;
    }
    Value evaluate_like(const Value& left,const Value& right) {
        if(null(left))return left;
        const auto type=kind(left);
        if(sql_value(type) && sql_base(type)==Type::string)return evaluate_like(sql_payload(type,left),right);
        const auto* source=std::get_if<std::u16string>(&left);
        const auto* pattern=std::get_if<std::u16string>(&right);
        if(!source || !pattern)operands();
        std::u16string literal;
        bool leading=false,trailing=false;
        const auto wildcard=[](char16_t ch){return ch==u'*' || ch==u'%';};
        const auto invalid=[&]{
            throw Error("E_FORMAT",location_,"Invalid LIKE pattern","Некорректный шаблон LIKE");
        };
        for(std::size_t i=0;i<pattern->size();){
            const auto ch=(*pattern)[i];
            if(wildcard(ch)){
                const bool starts=i==0;
                do{++i;}while(i<pattern->size() && wildcard((*pattern)[i]));
                if(starts)leading=true;
                else if(i==pattern->size())trailing=true;
                else invalid();
            }else if(ch==u'['){
                if(i+2>=pattern->size() || (*pattern)[i+2]!=u']')invalid();
                literal.push_back((*pattern)[i+1]);
                i+=3;
            }else{
                literal.push_back(ch);
                ++i;
            }
        }
        const auto value=comparison_text(*source);
        if(!leading && !trailing)
            return compare_culture_strings(value,comparison_text(literal),location_,options_,true)==0;
        if(leading && !trailing)
            return find_culture_text(value,comparison_text(literal),TextSearch::suffix,location_,options_);
        return find_culture_text(value,literal,leading ? TextSearch::anywhere : TextSearch::prefix,location_,options_);
    }
    bool convert_boolean(const Value& value,bool accept_digit) {
        if(const auto* source=std::get_if<std::u16string>(&value)){
            if(accept_digit && (*source==u"0" || *source==u"1"))return *source==u"1";
            if(const auto converted=boolean_text(*source))return *converted;
            throw runtime_error(RuntimeError::number_text,location_,"boolean");
        }
        if(kind(value)==decimal_)
            return compare_decimal_values(value,decimal_call(DecimalMethod::zero,std::array<Value,0>{}),location_)!=0;
        return real(value)!=0;
    }
    Value evaluate_conversion(Value value,Type target) {
        if(null(value) || target==Type::invalid)return value;
        const auto source=kind(value);
        if(sql_value(target))return sql_scalar_convert(values_,target,source,value);
        if(sql_value(source))operands();
        if(target==Type::nil)operands();
        if(target==Type::string)return kind(value)==interval_ ? calendar_xml_duration(value) : text(value);
        if(target==Type::integer || target==Type::int64)return signed_integer(value,target);
        if(small_integer(target))
            return checked_small_integer(target,std::get<std::int64_t>(signed_integer(value,Type::int64)),location_);
        if(wide_unsigned(target))return unsigned_integer(value,target);
        if(target==Type::real)return real(value);
        if(target==Type::single)return single(value);
        if(target==decimal_)return as_decimal(value);
        if(target==date_)return date_value(value);
        if(target==interval_)return duration_value(value);
        if(target==guid_){
            if(kind(value)==guid_)return value;
            if(kind(value)!=Type::string)operands();
            const std::array<Value,1> arguments{std::move(value)};
            return guid_invoke(values_,guid_,GuidMethod::parse,arguments);
        }
        if(target==offset_date_){
            if(kind(value)==offset_date_)return value;
            const auto* input=std::get_if<std::u16string>(&value);
            if(!input)operands();
            const std::array<Value,1> arguments{normalized_date(*input)};
            return offset_invoke(values_,offset_date_,OffsetMethod::parse,arguments);
        }
        if(target==Type::boolean)return convert_boolean(value,true);
        if(target!=Type::character)throw runtime_error(RuntimeError::invalid_bytecode,location_);
        if(kind(value)==Type::character)return value;
        if(const auto* source=std::get_if<std::u16string>(&value)){
            if(source->size()!=1)
                throw Error("E_FORMAT",location_,"CONVERT to Char requires exactly one UTF-16 code unit",
                    "Для CONVERT в Char требуется ровно одна кодовая единица UTF-16");
            return source->front();
        }
        if(!integer_type(kind(value)))operands();
        const auto number=unsigned_number(value,location_,"Char");
        if(number>UINT16_MAX)throw runtime_error(RuntimeError::integer_overflow,location_);
        return static_cast<char16_t>(number);
    }
    Value evaluate(std::size_t index,unsigned depth) {
        step(depth);
        const auto& node=nodes_[index];
        if(node.literal)return node.value;
        if(node.kind==Symbol::membership_arguments)
            throw Error("E_UNSUPPORTED",location_,"An IN list cannot be evaluated as a standalone value",
                "Список IN нельзя вычислить как самостоятельное значение");
        if(argument_count(node.kind))return function(node,depth+1);
        if(node.kind==Symbol::membership)return evaluate_membership(node,depth+1);
        auto left=evaluate(node.left,depth+1);
        if(node.kind==Symbol::grouped)return left;
        if(node.kind==Symbol::invalid_is)
            throw Error("E_FORMAT",location_,"IS requires NULL or NOT NULL in a Utils expression",
                "Оператор IS требует NULL или NOT NULL в выражении Utils");
        if(node.kind==Symbol::conversion)
            return evaluate_conversion(std::move(left),conversion_type(string_text(node.value)));
        if(node.kind==Symbol::is_null)return null(left);
        if(node.kind==Symbol::is_not_null)return !null(left);
        if(node.kind==Symbol::negate){
            if(null(left))return left;
            const auto type=kind(left);
            if(sql_flag(type))return sql_construct(type,!logic_flag(left));
            return !expression_boolean(left);
        }
        if(node.right==0){
            if(null(left))return left;
            const auto type=kind(left);
            if(sql_value(type)){
                const auto primitive=sql_base(type);
                if(!numeric_rank(primitive))operands();
                if(node.kind!=Symbol::minus)return left;
                if(primitive==Type::byte)return NilString{};
                const auto payload=sql_payload(type,left);
                if(primitive==Type::real)return sql_construct(type,-std::get<double>(payload));
                if(primitive==Type::single)return sql_construct(type,-std::get<float>(payload));
                const auto integer=integral_number(payload,location_);
                if(integer==INT64_MIN)throw runtime_error(RuntimeError::integer_overflow,location_);
                return sql_construct(type,small_integer(primitive) ? checked_small_integer(primitive,-integer,location_)
                    : checked_signed_integer(primitive,Value{-integer},location_));
            }
            if(!numeric_rank(kind(left)))operands();
            if(node.kind!=Symbol::minus)return left;
            if(kind(left)==Type::real)return -std::get<double>(left);
            if(kind(left)==Type::single)return -std::get<float>(left);
            if(kind(left)==decimal_)return decimal_call(DecimalMethod::negate,left);
            if(kind(left)==Type::shortint || wide_unsigned(kind(left)))return NilString{};
            return arithmetic(Symbol::minus,Value{std::int32_t{0}},left);
        }
        auto right=evaluate(node.right,depth+1);
        if(node.kind==Symbol::like)return evaluate_like(left,right);
        if(node.kind==Symbol::conjunction){
            if(null(left))return left;
            if(!logic_flag(left))return false;
            return null(right) ? right : Value{logic_flag(right)};
        }
        if(node.kind==Symbol::disjunction){
            if(null(left))return right;
            if(boolean(left) || null(right))return left;
            return logic_flag(right);
        }
        if(null(left) || null(right))return NilString{};
        switch(node.kind){
        case Symbol::equal:return compare(left,right)==0;
        case Symbol::unequal:return compare(left,right)!=0;
        case Symbol::less:return compare(left,right,false)<0;
        case Symbol::less_equal:return compare(left,right,false)<=0;
        case Symbol::greater:return compare(left,right,false)>0;
        case Symbol::greater_equal:return compare(left,right,false)>=0;
        default:return arithmetic(node.kind,left,right);
        }
    }
public:
    Evaluation(Values& values,const ExecutionOptions& options,Type decimal,Type date,Type guid,Type date_offset,
        std::u16string_view source)
        :values_(values),options_(options),location_(values.location),decimal_(decimal),date_(date),
        interval_(date==Type::invalid ? Type::invalid : type_info(values.program,date).element),guid_(guid),
        offset_date_(date_offset),source_(source){}
    Value compute() {
        advance();
        Value value=NilString{};
        if(token_.kind!=Symbol::end){
            const auto root=parse(0);
            if(token_.kind!=Symbol::end)syntax();
            check_binding();
            value=evaluate(root,0);
        }
        return value;
    }
    Value run(Type result) {
        auto value=compute();
        if(result==Type::string)return text(value);
        if(null(value))throw Error("E_TYPE",location_,"A null evaluation result cannot be converted to the requested type",
            "Пустой результат выражения нельзя преобразовать к запрошенному типу");
        if(result==Type::real)return real(value);
        if(result==Type::boolean)return convert_boolean(value,false);
        return signed_integer(value,Type::integer);
    }
};

Type evaluator_native_type(const Program& program,NativeRecord type) {
    for(std::size_t i=0;i<program.types.size();++i)
        if(program.types[i].native_record==type)return static_cast<Type>(64+i);
    return Type::invalid;
}
Evaluation evaluate_text(Values& values,const Value& expression,const ExecutionOptions& options) {
    return {values,options,evaluator_native_type(values.program,NativeRecord::decimal_value),
        evaluator_native_type(values.program,NativeRecord::date_time),
        evaluator_native_type(values.program,NativeRecord::guid),
        evaluator_native_type(values.program,NativeRecord::date_time_offset),string_text(expression)};
}
}

Value utils_eval(Values& values,const Value& expression,Type result,const ExecutionOptions& options) {
    return evaluate_text(values,expression,options).run(result);
}
Value utils_eval_object(Values& values,const Value& expression,Type object,Type missing,const ExecutionOptions& options) {
    auto value=evaluate_text(values,expression,options).compute();
    if(null(value))value=values.database_null(missing);
    const auto type=value_type(value);
    return values.box_value(object,type,std::move(value));
}
}

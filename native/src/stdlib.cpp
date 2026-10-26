#include "stdlib.hpp"
#include "diagnostics.hpp"
#include "files.hpp"
#include "text_file.hpp"
#include "file_operations.hpp"
#include "integer_values.hpp"
#include "random.hpp"
#include "console_input.hpp"
#include "formatting.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <numeric>
#include <type_traits>

namespace abc::stdlib {
std::span<const Signature> signatures() {
    static const auto catalog = [] {
        std::vector<Signature> entries;
        auto add = [&](std::string_view name, std::vector<Type> parameters, Type result, Function function, std::string_view unit = {}) {
            entries.push_back({name, std::move(parameters), result, function,unit});
        };
        add("readfiletoend",{Type::string},Type::string,Function::files_read_to_end,"filesoperations");
        add("writestringtofile",{Type::string,Type::string},Type::unit,Function::files_write_string,"filesoperations");
        add("randomize",{},Type::unit,Function::random_reset);
        add("randomize",{Type::integer},Type::unit,Function::random_reset);
        add("setrandomseed",{Type::integer},Type::unit,Function::random_reset);
        struct ConsoleEntry { std::string_view read, readln; Type type; };
        for (auto [read,readln,type]:{ConsoleEntry{"readinteger","readlninteger",Type::integer},
            {"readint64","readlnint64",Type::int64},{"readreal","readlnreal",Type::real},
            {"readboolean","readlnboolean",Type::boolean},{"readchar","readlnchar",Type::character},
            {"readstring","readlnstring",Type::string}}) {
            for (auto prompt:{false,true}) {
                const auto parameters=prompt ? std::vector<Type>{Type::string} : std::vector<Type>{};
                add(read,parameters,type,type==Type::string ? Function::console_line : Function::console_read);
                add(readln,parameters,type,type==Type::string ? Function::console_line : Function::console_readln);
            }
        }
        add("readlexem",{},Type::string,Function::console_token);
        for (auto type:{Type::integer,Type::int64,Type::real,Type::boolean,Type::character,Type::string,Type::byte,Type::shortint,Type::smallint,Type::word,Type::longword,Type::uint64})
            add("formatvalue",{type,Type::integer},Type::string,Function::format_value);
        add("formatvalue",{Type::real,Type::integer,Type::integer},Type::string,Function::format_value);
        add("random",{},Type::real,Function::random_real);
        add("random",{Type::integer},Type::integer,Function::random_integer);
        add("random",{Type::integer,Type::integer},Type::integer,Function::random_integer);
        add("random",{Type::real},Type::real,Function::random_real);
        add("random",{Type::real,Type::real},Type::real,Function::random_real);
        add("random",{Type::character,Type::character},Type::character,Function::random_char);
        add("randomreal",{Type::real,Type::real},Type::real,Function::random_rounded);
        add("randomreal",{Type::real,Type::real,Type::integer},Type::real,Function::random_rounded);
        add("atan2",{Type::real,Type::real},Type::real,Function::atan2);
        add("hypot",{Type::real,Type::real},Type::real,Function::hypot);
        add("lerp",{Type::real,Type::real,Type::real},Type::real,Function::lerp);
        add("remap",std::vector<Type>(5,Type::real),Type::real,Function::remap);
        add("gcd",{Type::integer,Type::integer},Type::integer,Function::gcd);
        add("gcd",{Type::int64,Type::int64},Type::int64,Function::gcd);
        add("lcm",{Type::int64,Type::int64},Type::int64,Function::lcm);
        for (auto type:{Type::byte,Type::shortint,Type::smallint,Type::word}) {
            add("abs",{type},type,Function::abs);
            add("sign",{type},Type::integer,Function::sign);
            add("min",{type,type},type,Function::minimum);
            add("max",{type,type},type,Function::maximum);
            if (type!=Type::word) add("sqr",{type},Type::integer,Function::sqr);
        }
        add("sqr",{Type::word},Type::uint64,Function::sqr);
        for (auto type:{Type::longword,Type::uint64}) {
            add("abs",{type},type,Function::abs);
            add("sqr",{type},Type::uint64,Function::sqr);
            add("sign",{type},Type::integer,Function::sign);
            for (std::size_t count=2;count<=4;++count) {
                add("min",std::vector<Type>(count,type),type,Function::minimum);
                add("max",std::vector<Type>(count,type),type,Function::maximum);
            }
        }
        for (auto type : {Type::integer, Type::int64, Type::real}) {
            add("abs", {type}, type, Function::abs);
            add("sqr", {type}, type == Type::integer ? Type::int64 : type, Function::sqr);
            add("sign", {type}, Type::integer, Function::sign);
            for (std::size_t count = 2; count <= 4; ++count) {
                // int64 also has the source's generic params-array overload.
                add("min", std::vector<Type>(count, type), type, Function::minimum);
                add("max", std::vector<Type>(count, type), type, Function::maximum);
            }
        }
        for (auto [name, function] : {
            std::pair{"sin", Function::sin}, {"sinh", Function::sinh}, {"cos", Function::cos},
            {"cosh", Function::cosh}, {"tan", Function::tan}, {"tanh", Function::tanh},
            {"arcsin", Function::arcsin}, {"arccos", Function::arccos}, {"arctan", Function::arctan},
            {"exp", Function::exp}, {"ln", Function::ln}, {"log", Function::ln},
            {"log2", Function::log2}, {"log10", Function::log10}, {"sqrt", Function::sqrt},
            {"cbrt", Function::cbrt}, {"int", Function::int_part}, {"frac", Function::frac},
            {"degtorad", Function::deg_to_rad}, {"radtodeg", Function::rad_to_deg}})
            add(name, {Type::real}, Type::real, function);
        for (auto [name, function] : {std::pair{"round", Function::round}, {"trunc", Function::trunc},
            {"floor", Function::floor}, {"ceil", Function::ceil}})
            add(name, {Type::real}, Type::integer, function);
        add("round", {Type::real, Type::integer}, Type::real, Function::round);
        add("isqrt", {Type::int64}, Type::int64, Function::isqrt);
        add("power", {Type::real, Type::integer}, Type::real, Function::power);
        add("power", {Type::real, Type::real}, Type::real, Function::power);
        add("logn", {Type::real, Type::real}, Type::real, Function::logn);
        add("length", {Type::string}, Type::integer, Function::length);
        add("stringofchar",{Type::character,Type::integer},Type::string,Function::string_of_char);
        add("$repeat",{Type::string,Type::integer},Type::string,Function::repeat_string);
        add("$repeat",{Type::integer,Type::string},Type::string,Function::repeat_string);
        add("todigit",{Type::character},Type::integer,Function::to_digit);
        add("$string_length",{Type::string},Type::integer,Function::string_length);
        for (auto name:{"chr","chrunicode","char"}) {
            add(name,{Type::word},Type::character,Function::character);
            add(name,{Type::integer},Type::character,Function::character);
            add(name,{Type::int64},Type::character,Function::character);
            add(name,{Type::longword},Type::character,Function::character);
            add(name,{Type::uint64},Type::character,Function::character);
        }
        add("char",{Type::character},Type::character,Function::character);
        for (auto name:{"ord","ordunicode"}) add(name,{Type::character},Type::word,Function::ordinal);
        for (auto type:{Type::integer,Type::int64,Type::word,Type::byte,Type::shortint,Type::smallint,Type::longword,Type::uint64}) add("ord",{type},type,Function::ordinal);
        add("ord",{Type::boolean},Type::integer,Function::ordinal);
        for (auto target:{Type::word,Type::byte,Type::shortint,Type::smallint,Type::integer,Type::int64,Type::longword,Type::uint64})
            for (auto source:{Type::integer,Type::int64,Type::word,Type::character,Type::byte,Type::shortint,Type::smallint,Type::longword,Type::uint64})
                add(type_name(target),{source},target,Function::integer_cast);
        add("integer",{Type::boolean},Type::integer,Function::integer_cast);
        for (auto type:{Type::integer,Type::int64,Type::word,Type::character,Type::boolean,Type::byte,Type::shortint,Type::smallint,Type::longword,Type::uint64}) {
            add("succ",{type},type,Function::successor);
            add("pred",{type},type,Function::predecessor);
        }
        add("copy", {Type::string, Type::integer, Type::integer}, Type::string, Function::copy);
        for (auto name : {"pos", "posex", "lastpos"}) {
            const auto function = std::string_view(name) == "lastpos" ? Function::last_pos : Function::pos;
            add(name, {Type::string, Type::string}, Type::integer, function);
            add(name, {Type::string, Type::string, Type::integer}, Type::integer, function);
        }
        add("concat", {Type::string, Type::string}, Type::string, Function::concat);
        for (auto [name, function] : {std::pair{"trim", Function::trim}, {"trimleft", Function::trim_left},
            {"trimright", Function::trim_right}, {"reversestring", Function::reverse_string}})
            add(name, {Type::string}, Type::string, function);
        add("leftstr", {Type::string, Type::integer}, Type::string, Function::left_str);
        add("rightstr", {Type::string, Type::integer}, Type::string, Function::right_str);
        add("reversestring", {Type::string, Type::integer, Type::integer}, Type::string, Function::reverse_string);
        add("comparestr", {Type::string, Type::string}, Type::integer, Function::compare_str);
        add("inttostr", {Type::integer}, Type::string, Function::int_to_str);
        add("inttostr", {Type::int64}, Type::string, Function::int_to_str);
        add("strtoint", {Type::string}, Type::integer, Function::str_to_int);
        add("tointeger", {Type::string}, Type::integer, Function::str_to_int);
        add("tointeger", {Type::string,Type::integer}, Type::integer, Function::to_integer_default);
        add("toreal", {Type::string}, Type::real, Function::parse_real);
        add("toreal", {Type::string,Type::string}, Type::real, Function::parse_real);
        add("toreal", {Type::string,Type::real}, Type::real, Function::to_real_default);
        add("isinteger", {Type::string}, Type::boolean, Function::is_integer);
        add("isreal", {Type::string}, Type::boolean, Function::is_real);
        add("strtoint64", {Type::string}, Type::int64, Function::str_to_int64);
        for (auto name : {"strtoreal", "strtofloat"}) {
            add(name, {Type::string}, Type::real, Function::str_to_real);
            add(name, {Type::string, Type::string}, Type::real, Function::str_to_real);
        }
        add("floattostr", {Type::real}, Type::string, Function::float_to_str);
        // Compiler-only value operations: the frontend checks the var argument,
        // then stores the resulting string after the call completes successfully.
        add("$setlength", {Type::string, Type::integer}, Type::string, Function::resize_string);
        add("$insert", {Type::string, Type::string, Type::integer}, Type::string, Function::insert_string);
        add("$delete", {Type::string, Type::integer, Type::integer}, Type::string, Function::delete_string);
        add("readalltext", {Type::string}, Type::string, Function::read_all_text);
        add("writealltext", {Type::string, Type::string}, Type::unit, Function::write_all_text);
        add("fileexists", {Type::string}, Type::boolean, Function::file_exists);
        add("getdir", {}, Type::string, Function::get_dir);
        add("getcurrentdir", {}, Type::string, Function::get_dir);
        add("chdir", {Type::string}, Type::unit, Function::change_dir);
        add("mkdir", {Type::string}, Type::unit, Function::make_dir);
        add("rmdir", {Type::string}, Type::unit, Function::remove_dir);
        add("createdir", {Type::string}, Type::boolean, Function::create_dir);
        add("removedir", {Type::string}, Type::boolean, Function::try_remove_dir);
        add("deletefile", {Type::string}, Type::boolean, Function::delete_file);
        add("setcurrentdir", {Type::string}, Type::boolean, Function::set_current_dir);
        add("changefilenameextension", {Type::string,Type::string}, Type::string, Function::change_extension);
        add("renamefile", {Type::string,Type::string}, Type::boolean, Function::rename_file);
        add("renamedirectory", {Type::string,Type::string}, Type::boolean, Function::rename_directory);
        for (auto name:{"assign","assignfile"}) add(name,{Type::text_file,Type::string},Type::unit,Function::text_assign);
        for (auto [name,function]:{std::pair{"reset",Function::text_reset},{"rewrite",Function::text_rewrite},{"append",Function::text_append}}) {
            add(name,{Type::text_file},Type::unit,function);
            add(name,{Type::text_file,Type::string},Type::unit,function);
        }
        for (auto name:{"close","closefile"}) add(name,{Type::text_file},Type::unit,Function::text_close);
        add("flush",{Type::text_file},Type::unit,Function::text_flush);
        for (auto [name,function]:{std::pair{"eof",Function::text_eof},{"eoln",Function::text_eoln},
            {"seekeof",Function::text_seek_eof},{"seekeoln",Function::text_seek_eoln}}) add(name,{Type::text_file},Type::boolean,function);
        for (auto [name,function]:{std::pair{"openread",Function::text_open_read},{"openwrite",Function::text_open_write},
            {"openappend",Function::text_open_append}}) add(name,{Type::string},Type::text_file,function);
        for (auto [name,type]:{std::pair{"readinteger",Type::integer},{"readint64",Type::int64},{"readreal",Type::real},{"readboolean",Type::boolean},{"readchar",Type::character}})
            add(name,{Type::text_file},type,Function::text_read);
        for (auto [name,type]:{std::pair{"readlninteger",Type::integer},{"readlnint64",Type::int64},{"readlnreal",Type::real},
            {"readlnboolean",Type::boolean},{"readlnchar",Type::character},{"readstring",Type::string},{"readlnstring",Type::string}}) add(name,{Type::text_file},type,Function::text_read_line);
        add("readlexem",{Type::text_file},Type::string,Function::text_read_word);
        add("erase",{Type::text_file},Type::unit,Function::text_erase);
        add("rename",{Type::text_file,Type::string},Type::unit,Function::text_rename);
        // Internal signatures are shared by Text member calls and bytecode verification.
        add("$text_skip_line",{Type::text_file},Type::unit,Function::text_skip_line);
        add("$text_name",{Type::text_file},Type::string,Function::text_name);
        add("$text_full_name",{Type::text_file},Type::string,Function::text_full_name);
        add("$text_read_end",{Type::text_file},Type::string,Function::text_read_end);
        return entries;
    }();
    return catalog;
}

std::optional<Value> constant(std::string_view name) {
    if (name=="maxbyte") return std::numeric_limits<std::uint8_t>::max();
    if (name=="maxshortint") return std::numeric_limits<std::int8_t>::max();
    if (name=="maxsmallint") return std::numeric_limits<std::int16_t>::max();
    if (name=="maxword") return std::numeric_limits<std::uint16_t>::max();
    if (name=="maxlongword") return std::numeric_limits<std::uint32_t>::max();
    if (name=="maxuint64") return std::numeric_limits<std::uint64_t>::max();
    if (name=="__is_system_module") return true;
    if (name=="end_of_line_symbol") return char16_t{10};
    if (name=="alldelimiters") return std::u16string(u" <>=^`|~$№§!\"#%&'()*,+-./:;?@[\\]_{}«­·»\t\n\r");
    if (name=="newline") return std::u16string(
#ifdef _WIN32
        u"\r\n"
#else
        u"\n"
#endif
    );
    if (name == "pi") return std::numbers::pi;
    if (name == "e") return std::numbers::e;
    if (name == "maxint") return std::numeric_limits<std::int32_t>::max();
    if (name == "maxint64") return std::numeric_limits<std::int64_t>::max();
    if (name == "maxreal" || name == "maxdouble") return std::numeric_limits<double>::max();
    if (name == "minreal" || name == "mindouble") return std::numeric_limits<double>::denorm_min();
    return std::nullopt;
}

namespace {
// Linear-time ordinal UTF-16 search, including repetitive input up to the string limit.
std::size_t search(std::u16string_view pattern, std::u16string_view text, bool last) {
    std::vector<std::size_t> prefix(pattern.size());
    for (std::size_t i = 1, matched = 0; i < pattern.size(); ++i) {
        while (matched && pattern[i] != pattern[matched]) matched = prefix[matched - 1];
        if (pattern[i] == pattern[matched]) ++matched;
        prefix[i] = matched;
    }
    auto result = std::u16string_view::npos;
    for (std::size_t i = 0, matched = 0; i < text.size(); ++i) {
        while (matched && text[i] != pattern[matched]) matched = prefix[matched - 1];
        if (text[i] == pattern[matched]) ++matched;
        if (matched == pattern.size()) {
            result = i + 1 - matched;
            if (!last) return result;
            matched = prefix[matched - 1];
        }
    }
    return result;
}

double checked_real(double value, SourceLocation location) {
    if (std::isnan(value)) throw runtime_error(RuntimeError::math_domain, location);
    if (!std::isfinite(value)) throw runtime_error(RuntimeError::real_overflow, location);
    return value;
}
std::int32_t checked_integer(double value, SourceLocation location) {
    if (!std::isfinite(value) || value < std::numeric_limits<std::int32_t>::min()
        || value > std::numeric_limits<std::int32_t>::max())
        throw runtime_error(RuntimeError::integer_overflow, location);
    return static_cast<std::int32_t>(value);
}
// Do not depend on the host rounding mode. PascalABC.NET uses ties-to-even.
double round_even(double value) {
    const double lower = std::floor(value), fraction = value - lower;
    const double rounded = fraction > 0.5 || (fraction == 0.5 && std::fmod(lower, 2.0) != 0)
        ? lower + 1.0 : lower;
    return rounded == 0 ? std::copysign(0.0, value) : rounded;
}
double round_digits(double value, std::int32_t digits, SourceLocation location) {
    if (digits<0 || digits>15) throw runtime_error(RuntimeError::round_digits,location);
    static constexpr double scales[] = {1,10,100,1000,10000,100000,1000000,10000000,
        100000000,1000000000,10000000000,100000000000,1000000000000,10000000000000,
        100000000000000,1000000000000000};
    return std::abs(value)<1e16 ? round_even(value*scales[digits])/scales[digits] : value;
}
} // namespace

Value invoke(std::size_t id, std::span<const Value> arguments, SourceLocation location, Context& context) {
    auto& files=context.files; auto& random=context.random;
    const auto& signature = signatures()[id]; // Verified before execution.
    const auto function = signature.function;
    if (function>=Function::console_read && function<=Function::console_token) {
        if (!arguments.empty()) {
            context.write_text(std::holds_alternative<NilString>(arguments[0]) ? std::u16string_view{u"nil"} : std::u16string_view{string_text(arguments[0])},location);
            const auto& delimiter=context.print_delimiter();
            if (std::holds_alternative<NilString>(delimiter)) context.write_text(u"nil",location);
            else if (!string_text(delimiter).empty()) context.write_text(string_text(delimiter),location);
        }
        if (function==Function::console_line) return context.console.read_line(location);
        if (function==Function::console_token) return context.console.token(location);
        auto value=context.console.read(signature.result,location);
        if (function==Function::console_readln) context.console.skip_line(location);
        return value;
    }
    if (function == Function::get_dir) return files.directory(location);
    if (function==Function::random_reset) {
        if (arguments.empty()) random.randomize(location);
        else random.seed(std::get<std::int32_t>(arguments[0]));
        return false;
    }
    if (function==Function::random_integer || function==Function::random_char) {
        if (arguments.size()==1) return random.range(0,std::get<std::int32_t>(arguments[0]),location);
        auto first=function==Function::random_char ? std::int32_t(std::get<char16_t>(arguments[0])) : std::get<std::int32_t>(arguments[0]);
        auto last=function==Function::random_char ? std::int32_t(std::get<char16_t>(arguments[1])) : std::get<std::int32_t>(arguments[1]);
        if (first>last) std::swap(first,last);
        // PABCSystem uses an inclusive right bound by calling Next(first,last+1).
        if (last==INT32_MAX) throw runtime_error(RuntimeError::integer_overflow,location);
        const auto value=random.range(first,last+1,location);
        return function==Function::random_char ? Value{static_cast<char16_t>(value)} : Value{value};
    }
    if (function==Function::random_real || function==Function::random_rounded) {
        if (arguments.empty()) return random.unit(location);
        if (arguments.size()==1) return checked_real(random.unit(location)*std::abs(std::get<double>(arguments[0])),location);
        const auto original_first=std::get<double>(arguments[0]), original_last=std::get<double>(arguments[1]);
        const auto first=std::min(original_first,original_last), last=std::max(original_first,original_last);
        const auto digits=arguments.size()==3 ? std::get<std::int32_t>(arguments[2]) : 2;
        const auto step=function==Function::random_rounded && digits>=0 ? 1/std::pow(10.0,digits) : 0.0;
        const auto weighted=random.unit(location)*(last-first);
        const auto sampled=checked_real(first+weighted,location);
        if (function==Function::random_real || digits<0) return sampled;
        auto rounded=round_digits(sampled,digits,location);
        // Keep the source clamp order, including its behavior for reversed bounds.
        if (rounded<original_first) {
            rounded+=step; if (rounded>original_last) rounded=original_last;
        } else if (rounded>original_last) {
            rounded-=step; if (rounded<original_first) rounded=original_first;
        }
        return checked_real(rounded,location);
    }
    const auto& value = arguments.front();
    if (function==Function::format_value) return format_field(value,std::get<std::int32_t>(arguments[1]),arguments.size()==3 ? std::optional<std::int32_t>{std::get<std::int32_t>(arguments[2])} : std::nullopt,location);
    if (function==Function::gcd) {
        const auto calculate=[&]<class T>(T a,T b)->Value {
            if (a==std::numeric_limits<T>::min() || b==std::numeric_limits<T>::min())
                throw runtime_error(RuntimeError::integer_overflow,location);
            return std::gcd(a,b);
        };
        if (signature.result==Type::integer) return calculate(std::get<std::int32_t>(value),std::get<std::int32_t>(arguments[1]));
        return calculate(std::get<std::int64_t>(value),std::get<std::int64_t>(arguments[1]));
    }
    if (function==Function::lcm) {
        auto a=std::get<std::int64_t>(value), b=std::get<std::int64_t>(arguments[1]);
        if (!a || !b) return std::int64_t{0};
        if (a==INT64_MIN || b==INT64_MIN) throw runtime_error(RuntimeError::integer_overflow,location);
        if (a<0) a=-a;
        if (b<0) b=-b;
        const auto quotient=a/std::gcd(a,b);
        if (quotient>INT64_MAX/b) throw runtime_error(RuntimeError::integer_overflow,location);
        return quotient*b;
    }
    if (function==Function::files_read_to_end) return file_operations_read(files,value,location);
    if (function==Function::files_write_string) { file_operations_write(files,value,arguments[1],location); return false; }
    if (function==Function::ordinal) {
        if (const auto* ch=std::get_if<char16_t>(&value)) return static_cast<std::uint16_t>(*ch);
        if (const auto* flag=std::get_if<bool>(&value)) return std::int32_t(*flag);
        return value;
    }
    if (function==Function::integer_cast) {
        if (wide_unsigned(signature.result)) return checked_unsigned_integer(signature.result,value,location);
        if (signature.result==Type::integer || signature.result==Type::int64) return checked_signed_integer(signature.result,value,location);
        return checked_small_integer(signature.result,integral_number(value,location,type_name(signature.result)),location);
    }
    if (function==Function::character) {
        const auto number=integral_number(value,location,"char");
        if (number<0 || number>65535) throw runtime_error(RuntimeError::number_range,location,"char");
        return static_cast<char16_t>(number);
    }
    if (function==Function::successor || function==Function::predecessor) {
        if (const auto* flag=std::get_if<bool>(&value)) return !*flag;
        return std::visit([&](const auto& v)->Value {
            using T=std::decay_t<decltype(v)>;
            if constexpr (std::is_integral_v<T>) {
                const bool up=function==Function::successor;
                if (up ? v==std::numeric_limits<T>::max() : v==std::numeric_limits<T>::min())
                    throw runtime_error(RuntimeError::integer_overflow,location);
                return static_cast<T>(up ? v+1 : v-1);
            } else throw runtime_error(RuntimeError::invalid_bytecode,location);
        },value);
    }
    if (function>=Function::text_assign && function<=Function::text_rename) {
        if (function==Function::text_open_read || function==Function::text_open_write || function==Function::text_open_append) {
            auto file=std::make_shared<TextFile>(); text_assign(files,*file,value,location);
            text_open(files,file,function==Function::text_open_read ? 0 : function==Function::text_open_write ? 1 : 2,location);
            return file;
        }
        auto& file=text_file(value,location);
        switch (function) {
        case Function::text_assign: text_assign(files,file,arguments[1],location); return false;
        case Function::text_reset: case Function::text_rewrite: case Function::text_append:
            if (arguments.size()==2) text_assign(files,file,arguments[1],location);
            text_open(files,std::get<std::shared_ptr<TextFile>>(value),function==Function::text_reset ? 0 : function==Function::text_rewrite ? 1 : 2,location); return false;
        case Function::text_close: file.close(location); return false;
        case Function::text_flush: file.flush(location); return false;
        case Function::text_eof: case Function::text_eoln: case Function::text_seek_eof: case Function::text_seek_eoln:
            return text_eof(file,function==Function::text_eoln || function==Function::text_seek_eoln,
                function==Function::text_seek_eof || function==Function::text_seek_eoln,location);
        case Function::text_read: return text_read(file,signature.result,location);
        case Function::text_read_line: { auto result=text_read(file,signature.result,location); text_skip_line(file,location); return result; }
        case Function::text_read_word: return text_word(file,location);
        case Function::text_skip_line: text_skip_line(file,location); return false;
        case Function::text_name: return text_name(file,false,location);
        case Function::text_full_name: return text_name(file,true,location);
        case Function::text_read_end: return text_read_end(file,location);
        case Function::text_erase: text_erase(files,file,location); return false;
        case Function::text_rename: text_rename(files,file,arguments[1],location); return false;
        default: throw runtime_error(RuntimeError::invalid_bytecode,location);
        }
    }
    if (function == Function::change_dir) { files.change_directory(string_text(value),location); return false; }
    if (function == Function::make_dir) { create_directory(files,string_text(value),location); return false; }
    if (function == Function::remove_dir) { remove_directory(files,string_text(value),location); return false; }
    if (function == Function::create_dir || function == Function::try_remove_dir || function == Function::set_current_dir) {
        try {
            if (function == Function::create_dir) create_directory(files,string_text(value),location);
            else if (function == Function::try_remove_dir) remove_directory(files,string_text(value),location);
            else files.change_directory(string_text(value),location);
            return true;
        } catch (const Error& error) { if (error.code!="E_IO") throw; return false; }
    }
    if (function == Function::delete_file) return delete_file(files,string_text(value));
    if (function == Function::rename_file || function == Function::rename_directory)
        return files.rename(string_text(value),string_text(arguments[1]),function==Function::rename_directory);
    if (function == Function::change_extension) return change_file_extension(value,arguments[1],location);
    if (function == Function::read_all_text) return read_text_file(files,string_text(value),location);
    if (function == Function::file_exists) return file_exists(files,string_text(value));
    if (function == Function::write_all_text) {
        write_text_file(files,string_text(value),string_text(arguments[1]),location); return false;
    }
    if (auto result = invoke_text(function, arguments, location)) return std::move(*result);
    if (function == Function::minimum || function == Function::maximum) {
        Value result = value;
        for (const auto& next : arguments.subspan(1))
            if (function == Function::minimum ? next < result : next > result) result = next;
        return result;
    }
    if (function == Function::sign) {
        if (wide_unsigned(value_type(value))) return std::int32_t(unsigned_number(value,location)>0);
        if (small_integer(value_type(value))) { const auto n=integral_number(value,location); return std::int32_t((n>0)-(n<0)); }
        if (const auto* n = std::get_if<std::int32_t>(&value)) return std::int32_t((*n > 0) - (*n < 0));
        if (const auto* n = std::get_if<std::int64_t>(&value)) return std::int32_t((*n > 0) - (*n < 0));
        const auto n = std::get<double>(value);
        return std::int32_t((n > 0) - (n < 0));
    }
    if (function == Function::abs || function == Function::sqr) {
        if (function==Function::sqr && signature.result==Type::uint64) {
            const auto n=unsigned_number(value,location);
            if (n && n>UINT64_MAX/n) throw runtime_error(RuntimeError::integer_overflow,location);
            return n*n;
        }
        if (wide_unsigned(value_type(value))) return value;
        if (small_integer(value_type(value))) {
            const auto n=integral_number(value,location);
            if (function==Function::sqr) return static_cast<std::int32_t>(n*n);
            const auto magnitude=n<0 ? -n : n;
            if (magnitude>small_integer_bounds(value_type(value)).second) throw runtime_error(RuntimeError::integer_overflow,location);
            return checked_small_integer(value_type(value),magnitude,location);
        }
        if (const auto* n = std::get_if<std::int32_t>(&value)) {
            if (function == Function::sqr) return std::int64_t(*n) * std::int64_t(*n);
            if (*n == std::numeric_limits<std::int32_t>::min()) throw runtime_error(RuntimeError::integer_overflow, location);
            return std::int32_t(*n < 0 ? -*n : *n);
        }
        if (const auto* n = std::get_if<std::int64_t>(&value)) {
            if (function == Function::sqr) {
                if (*n < -3'037'000'499LL || *n > 3'037'000'499LL)
                    throw runtime_error(RuntimeError::integer_overflow, location);
                return *n * *n;
            }
            if (*n == std::numeric_limits<std::int64_t>::min()) throw runtime_error(RuntimeError::integer_overflow, location);
            return std::int64_t(*n < 0 ? -*n : *n);
        }
        const auto n = std::get<double>(value);
        return checked_real(function == Function::abs ? std::abs(n) : n * n, location);
    }
    if (function == Function::isqrt) {
        const auto n = std::get<std::int64_t>(value);
        if (n < 0) throw runtime_error(RuntimeError::math_domain, location);
        std::int64_t lower = 0, upper = 3'037'000'500LL;
        while (lower + 1 < upper) {
            const auto middle = lower + (upper - lower) / 2;
            if (middle <= n / middle) lower = middle;
            else upper = middle;
        }
        return lower;
    }
    if (signature.parameters.front() == Type::string) {
        const auto& text = string_text(value);
        if (function == Function::length) {
            if (text.size() > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
                throw runtime_error(RuntimeError::integer_overflow, location);
            return static_cast<std::int32_t>(text.size());
        }
        if (function == Function::copy) {
            const auto index = std::max(std::get<std::int32_t>(arguments[1]), std::int32_t{1});
            const auto count = std::get<std::int32_t>(arguments[2]);
            const auto start = static_cast<std::size_t>(index - 1);
            if (start >= text.size() || count <= 0) return std::u16string{};
            return text.substr(start, static_cast<std::size_t>(count));
        }
        const auto& other = string_text(arguments[1]);
        if (function == Function::concat) {
            constexpr std::size_t limit = 1'048'576;
            if (text.size() > limit || other.size() > limit - text.size())
                throw runtime_error(RuntimeError::string_limit, location);
            return text + other;
        }
        // Pos/LastPos take (substring, text, optional one-based starting index).
        if (text.empty()) return std::int32_t{0};
        if (std::holds_alternative<NilString>(arguments[1])) throw runtime_error(RuntimeError::nil_reference,location);
        const auto from = arguments.size() == 3 ? std::get<std::int32_t>(arguments[2])
            : function == Function::pos ? 1 : static_cast<std::int32_t>(other.size());
        if (text.empty() || from > static_cast<std::int64_t>(other.size())) return std::int32_t{0};
        // LastPos('', '') is handled above, and empty haystacks have no match.
        if (other.empty() && (arguments.size() == 2 || (function == Function::last_pos && from == 0))) return std::int32_t{0};
        if (from < 1) throw runtime_error(RuntimeError::string_index, location);
        const auto start = static_cast<std::size_t>(from - 1);
        const bool last = function == Function::last_pos;
        const auto haystack = last ? std::u16string_view(other).substr(0, start + 1)
            : std::u16string_view(other).substr(start);
        const auto found = search(text, haystack, last);
        return found == std::u16string::npos ? std::int32_t{0}
            : static_cast<std::int32_t>(found + (last ? 0 : start) + 1);
    }
    const auto x = std::get<double>(value);
    if (((function == Function::ln || function == Function::log2 || function == Function::log10) && x <= 0)
        || (function == Function::logn && (x <= 0 || x == 1 || std::get<double>(arguments[1]) <= 0)))
        throw runtime_error(RuntimeError::math_domain, location);
    double result = 0;
    switch (function) {
    case Function::atan2: result=std::atan2(x,std::get<double>(arguments[1])); break;
    case Function::hypot: {
        const auto a=std::abs(x), b=std::abs(std::get<double>(arguments[1]));
        const auto high=std::max(a,b), low=std::min(a,b);
        const auto ratio=high>0 ? low/high : 0;
        result=high*std::sqrt(1+ratio*ratio); break;
    }
    case Function::lerp: result=x+(std::get<double>(arguments[1])-x)*std::get<double>(arguments[2]); break;
    case Function::remap: {
        const auto a=std::get<double>(arguments[1]), b=std::get<double>(arguments[2]);
        if (a==b) throw runtime_error(RuntimeError::division_zero,location);
        const auto first=std::get<double>(arguments[3]), last=std::get<double>(arguments[4]);
        result=first+((x-a)/(b-a))*(last-first); break;
    }
    case Function::sin: result = std::sin(x); break;
    case Function::sinh: result = std::sinh(x); break;
    case Function::cos: result = std::cos(x); break;
    case Function::cosh: result = std::cosh(x); break;
    case Function::tan: result = std::tan(x); break;
    case Function::tanh: result = std::tanh(x); break;
    case Function::arcsin: result = std::asin(x); break;
    case Function::arccos: result = std::acos(x); break;
    case Function::arctan: result = std::atan(x); break;
    case Function::exp: result = std::exp(x); break;
    case Function::ln: result = std::log(x); break;
    case Function::log2: result = std::log2(x); break;
    case Function::log10: result = std::log10(x); break;
    case Function::logn: result = std::log(std::get<double>(arguments[1])) / std::log(x); break;
    case Function::sqrt: result = std::sqrt(x); break;
    case Function::cbrt: result = std::cbrt(x); break;
    case Function::power:
        if (signature.parameters[1] == Type::integer) result = std::pow(x, std::get<std::int32_t>(arguments[1]));
        else result = std::pow(x, std::get<double>(arguments[1]));
        break;
    case Function::round:
        if (arguments.size() == 1) return checked_integer(round_even(x), location);
        else {
            const auto digits = std::get<std::int32_t>(arguments[1]);
            result = round_digits(x,digits,location);
            break;
        }
    case Function::trunc: return checked_integer(std::trunc(x), location);
    case Function::floor: return checked_integer(std::floor(x), location);
    case Function::ceil: return checked_integer(std::ceil(x), location);
    case Function::int_part: result = std::trunc(x); break;
    case Function::frac: result = x - std::trunc(x); break;
    case Function::deg_to_rad: result = x * std::numbers::pi / 180.0; break;
    case Function::rad_to_deg: result = x * 180.0 / std::numbers::pi; break;
    default: throw runtime_error(RuntimeError::invalid_bytecode, location);
    }
    return checked_real(result, location);
}
} // namespace abc::stdlib

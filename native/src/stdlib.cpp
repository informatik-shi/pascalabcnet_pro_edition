#include "stdlib.hpp"
#include "diagnostics.hpp"
#include "files.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace abc::stdlib {
std::span<const Signature> signatures() {
    static const auto catalog = [] {
        std::vector<Signature> entries;
        auto add = [&](std::string_view name, std::vector<Type> parameters, Type result, Function function) {
            entries.push_back({name, std::move(parameters), result, function});
        };
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
        return entries;
    }();
    return catalog;
}

std::optional<Value> constant(std::string_view name) {
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
} // namespace

Value invoke(std::size_t id, std::span<const Value> arguments, SourceLocation location) {
    const auto& signature = signatures()[id]; // Verified before execution.
    const auto& value = arguments.front();
    const auto function = signature.function;
    if (function == Function::read_all_text) return read_text_file(std::get<std::u16string>(value),location);
    if (function == Function::file_exists) return file_exists(std::get<std::u16string>(value));
    if (function == Function::write_all_text) {
        write_text_file(std::get<std::u16string>(value),std::get<std::u16string>(arguments[1]),location); return false;
    }
    if (auto result = invoke_text(function, arguments, location)) return std::move(*result);
    if (function == Function::minimum || function == Function::maximum) {
        Value result = value;
        for (const auto& next : arguments.subspan(1))
            if (function == Function::minimum ? next < result : next > result) result = next;
        return result;
    }
    if (function == Function::sign) {
        if (const auto* n = std::get_if<std::int32_t>(&value)) return std::int32_t((*n > 0) - (*n < 0));
        if (const auto* n = std::get_if<std::int64_t>(&value)) return std::int32_t((*n > 0) - (*n < 0));
        const auto n = std::get<double>(value);
        return std::int32_t((n > 0) - (n < 0));
    }
    if (function == Function::abs || function == Function::sqr) {
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
        const auto& text = std::get<std::u16string>(value);
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
        const auto& other = std::get<std::u16string>(arguments[1]);
        if (function == Function::concat) {
            constexpr std::size_t limit = 1'048'576;
            if (text.size() > limit || other.size() > limit - text.size())
                throw runtime_error(RuntimeError::string_limit, location);
            return text + other;
        }
        // Pos/LastPos take (substring, text, optional one-based starting index).
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
            if (digits < 0 || digits > 15) throw runtime_error(RuntimeError::round_digits, location);
            static constexpr double scales[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000,
                100000000, 1000000000, 10000000000, 100000000000, 1000000000000, 10000000000000,
                100000000000000, 1000000000000000};
            result = std::abs(x) < 1e16 ? round_even(x * scales[digits]) / scales[digits] : x;
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

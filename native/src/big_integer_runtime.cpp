#include "big_integer_runtime.hpp"
#include "integer_values.hpp"
#include <cmath>
#include <type_traits>

namespace abc {
Value big_cast(const Value& value,Type target,SourceLocation location) {
    return checked_big(location,[&]()->Value {
        if (target==Type::real) {
            const auto result=std::visit([&](const auto& n)->double {
                using T=std::decay_t<decltype(n)>;
                if constexpr (std::is_same_v<T,BigInteger>) return n.to_real();
                else if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T,bool>) return static_cast<double>(n);
                else throw runtime_error(RuntimeError::invalid_bytecode,location);
            },value);
            if (!std::isfinite(result)) throw runtime_error(RuntimeError::real_overflow,location);
            return result;
        }
        if (target==Type::big_integer) return std::visit([&](const auto& n)->Value {
            using T=std::decay_t<decltype(n)>;
            if constexpr (std::is_same_v<T,BigInteger>) return n;
            else if constexpr (std::is_same_v<T,double>) return BigInteger::from_real(n);
            else if constexpr (std::is_integral_v<T> && !std::is_same_v<T,bool>) {
                if constexpr (std::is_unsigned_v<T>) return BigInteger::from_unsigned(n);
                else return BigInteger::from_signed(n);
            } else throw runtime_error(RuntimeError::invalid_bytecode,location);
        },value);
        const auto& big=std::get<BigInteger>(value);
        if (wide_unsigned(target)) return checked_unsigned_integer(target,Value{big.to_unsigned()},location);
        if (target==Type::word || target==Type::byte || target==Type::character) {
            const auto number=big.to_unsigned();
            const auto bound=target==Type::byte ? std::uint64_t{255} : std::uint64_t{65535};
            if (number>bound) throw runtime_error(RuntimeError::integer_overflow,location);
            if (target==Type::character) return static_cast<char16_t>(number);
            return checked_small_integer(target,static_cast<std::int64_t>(number),location);
        }
        const auto number=big.to_signed();
        if (target==Type::integer || target==Type::int64) return checked_signed_integer(target,Value{number},location);
        return checked_small_integer(target,number,location);
    });
}
BigInteger parse_big(std::u16string_view text,SourceLocation location,const ExecutionOptions* options) {
    const auto space=[](char16_t c) { return c==u' ' || (c>=9 && c<=13); };
    while (!text.empty() && text.back()==0) text.remove_suffix(1);
    while (!text.empty() && space(text.front())) text.remove_prefix(1);
    while (!text.empty() && space(text.back())) text.remove_suffix(1);
    std::string bytes; bytes.reserve(text.size());
    for (std::size_t i=0;i<text.size();++i) {
        if (options) {
            if (i>=options->instruction_limit) throw runtime_error(RuntimeError::instruction_limit,location);
            if ((i&255U)==0 && options->cancelled && options->cancelled()) throw runtime_error(RuntimeError::cancelled,location);
        }
        if (text[i]>127) throw runtime_error(RuntimeError::number_text,location,"BigInteger");
        bytes+=static_cast<char>(text[i]);
    }
    return checked_big(location,[&]{return BigInteger::parse(bytes);});
}
void big_work(std::uint64_t work,SourceLocation location,const ExecutionOptions& options) {
    if (work>options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,location);
    if (options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,location);
}
BigInteger big_binary(Op operation,const BigInteger& a,const BigInteger& b,SourceLocation location,const ExecutionOptions& options) {
    if ((operation==Op::divide_int || operation==Op::modulo) && b.zero()) throw runtime_error(RuntimeError::division_zero,location);
    const auto x=(a.bits()+27)/28+1,y=(b.bits()+27)/28+1;
    big_work(operation==Op::multiply || operation==Op::divide_int || operation==Op::modulo ? x*y : x+y,location,options);
    auto result=checked_big(location,[&] {
        switch (operation) {
        case Op::add: return a.add(b);
        case Op::subtract: return a.subtract(b);
        case Op::multiply: return a.multiply(b);
        case Op::divide_int: return a.divide(b).first;
        case Op::modulo: return a.divide(b).second;
        case Op::bit_and: return a.bit_and(b);
        case Op::bit_or: return a.bit_or(b);
        case Op::bit_xor: return a.bit_xor(b);
        default: throw runtime_error(RuntimeError::invalid_bytecode,location);
        }
    });
    big_work(0,location,options); return result;
}
BigInteger big_unary(Op operation,const BigInteger& value,SourceLocation location,const ExecutionOptions& options) {
    big_work(value.bits()/28+1,location,options);
    return checked_big(location,[&]{return operation==Op::negate ? value.negated() : value.complement();});
}
BigInteger big_shift(Op operation,const BigInteger& value,std::int32_t count,SourceLocation location,const ExecutionOptions& options) {
    big_work(value.bits()/28+1,location,options);
    return checked_big(location,[&]{return operation==Op::shift_left ? value.shift_left(count) : value.shift_right(count);});
}
} // namespace abc

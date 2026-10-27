#pragma once
#include <compare>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace abc {
// Immutable value. Copies share the representation; every operation creates a
// new value. The C backend and its configuration stay outside the public ABI.
class BigInteger {
    struct Rep;
    std::shared_ptr<const Rep> rep_;
    explicit BigInteger(std::shared_ptr<const Rep> rep) : rep_(std::move(rep)) {}
    static BigInteger finish(std::shared_ptr<Rep> rep);
    const Rep& rep() const;
    BigInteger shift(bool left,std::uint64_t count) const;
    enum class Binary { add,subtract,multiply,bit_and,bit_or,bit_xor,gcd };
    BigInteger combine(Binary operation,const BigInteger& other) const;
public:
    static constexpr std::size_t max_bits=65'536;
    BigInteger() = default;
    static BigInteger from_signed(std::int64_t value);
    static BigInteger from_unsigned(std::uint64_t value);
    static BigInteger from_real(double value);
    static BigInteger parse(std::string_view text,unsigned base=10);
    std::string text(unsigned base=10) const;
    std::size_t bits() const;
    bool negative() const;
    bool odd() const;
    bool zero() const { return !rep_; }
    bool shares_storage(const BigInteger& other) const { return rep_==other.rep_; }
    std::int64_t to_signed() const;
    std::uint64_t to_unsigned() const;
    double to_real() const;
    int compare(const BigInteger& other) const;
    bool operator==(const BigInteger& other) const { return compare(other)==0; }
    std::strong_ordering operator<=>(const BigInteger& other) const {
        const auto order=compare(other);
        return order<0 ? std::strong_ordering::less : order>0 ? std::strong_ordering::greater : std::strong_ordering::equal;
    }
    BigInteger negated() const;
    BigInteger absolute() const;
    BigInteger add(const BigInteger& other) const;
    BigInteger subtract(const BigInteger& other) const;
    BigInteger multiply(const BigInteger& other) const;
    std::pair<BigInteger,BigInteger> divide(const BigInteger& other) const;
    BigInteger bit_and(const BigInteger& other) const;
    BigInteger bit_or(const BigInteger& other) const;
    BigInteger bit_xor(const BigInteger& other) const;
    BigInteger complement() const;
    BigInteger shift_left(std::int32_t count) const;
    BigInteger shift_right(std::int32_t count) const;
    BigInteger gcd(const BigInteger& other) const;
    BigInteger square_root() const;
    BigInteger power(std::uint32_t exponent) const;
};
} // namespace abc

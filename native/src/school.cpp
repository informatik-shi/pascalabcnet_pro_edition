#include "school.hpp"
#include "diagnostics.hpp"
#include "runtime_values.hpp"
#include "unicode.hpp"
#include "big_integer_runtime.hpp"
#include "sequences.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <array>
#include <numeric>
#include <type_traits>

namespace abc::stdlib {
namespace {
bool small_prime(std::int32_t value, SourceLocation location, const ExecutionOptions& options, const SchoolContext& school) {
    if (value<2) return false;
    if (school.primes) {
        for (std::size_t i=0;i<school.primes->items.size();++i) {
            if ((i&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,location);
            const auto prime=std::int64_t(std::get<std::int32_t>(school.primes->items[i]->value));
            if (prime*prime>value) break;
            if (!prime) throw runtime_error(RuntimeError::division_zero,location);
            if (value%prime==0) return false;
        }
        return true;
    }
    if (!(value&1)) return value==2;
    const auto number=static_cast<std::uint32_t>(value);
    for (std::uint32_t divisor=3,steps=0;divisor<=number/divisor;divisor+=2,++steps) {
        if ((steps&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,location);
        if (number%divisor==0) return false;
    }
    return true;
}
class Modular63 {
    std::uint64_t modulus, steps=0;
    SourceLocation location;
    const ExecutionOptions& options;
public:
    Modular63(std::uint64_t n, SourceLocation l, const ExecutionOptions& o):modulus(n),location(l),options(o) {}
    std::uint64_t multiply(std::uint64_t a, std::uint64_t b) {
        std::uint64_t result=0;
        while (b) {
            if ((steps++&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,location);
            // Every residue is below 2^63, so both sums fit uint64 on every target.
            if (b&1U) result=(result+a)%modulus;
            b>>=1;
            if (b) a=(a+a)%modulus;
        }
        return result;
    }
    std::uint64_t power(std::uint64_t a, std::uint64_t exponent) {
        std::uint64_t result=1;
        while (exponent) {
            if (exponent&1U) result=multiply(result,a);
            exponent>>=1;
            if (exponent) a=multiply(a,a);
        }
        return result;
    }
};
bool large_prime(std::uint64_t number, SourceLocation location, const ExecutionOptions& options) {
    constexpr std::array<unsigned,12> bases{2,3,5,7,11,13,17,19,23,29,31,37};
    for (auto base:bases) if (number%base==0) return false;
    auto odd=number-1; unsigned twos=0;
    while (!(odd&1U)) { odd>>=1; ++twos; }
    Modular63 arithmetic(number,location,options);
    // Sorenson/Webster, Theorem 1.1: the first twelve prime bases cover int64.
    // https://arxiv.org/abs/1509.00864
    for (auto base:bases) {
        auto residue=arithmetic.power(base,odd);
        if (residue==1 || residue==number-1) continue;
        bool passed=false;
        for (unsigned i=1;i<twos;++i) {
            residue=arithmetic.multiply(residue,residue);
            if (residue==number-1) { passed=true; break; }
            if (residue==1) break;
        }
        if (!passed) return false;
    }
    return true;
}
}
Value invoke_school(Function function, std::span<const Value> arguments, SourceLocation location, const ExecutionOptions& options, SchoolContext& school) {
    if (function==Function::school_imp) return !std::get<bool>(arguments[0]) || std::get<bool>(arguments[1]);
    if (function==Function::school_gcd_alias || function==Function::school_lcm_alias)
        return school_pair(std::get<std::int64_t>(arguments[0]),std::get<std::int64_t>(arguments[1]),function==Function::school_lcm_alias,location);
    if (function==Function::school_replace_many || function==Function::school_translate) return invoke_school_text(function,arguments,location,options);
    constexpr std::u16string_view alphabet=u"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const auto check=[&](std::size_t index) {
        if ((index&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,location);
    };
    if (function==Function::school_is_prime) {
        const auto number=std::holds_alternative<std::int32_t>(arguments[0]) ? std::int64_t(std::get<std::int32_t>(arguments[0])) : std::get<std::int64_t>(arguments[0]);
        // The original BPSW entry delegates every n <= MaxInt to integer(n).
        if (number<INT32_MIN) throw runtime_error(RuntimeError::integer_overflow,location);
        return number<=INT32_MAX ? small_prime(static_cast<std::int32_t>(number),location,options,school)
            : large_prime(static_cast<std::uint64_t>(number),location,options);
    }
    if (function>=Function::school_sin_degrees && function<=Function::school_tan_degrees) {
        const auto radians=std::get<double>(arguments[0])*std::numbers::pi/180.0;
        if (!std::isfinite(radians)) throw runtime_error(RuntimeError::real_overflow,location);
        const auto result=function==Function::school_sin_degrees ? std::sin(radians) : function==Function::school_cos_degrees ? std::cos(radians) : std::tan(radians);
        if (!std::isfinite(result)) throw runtime_error(RuntimeError::real_overflow,location);
        return result;
    }
    if (function==Function::school_dec || function==Function::school_dec_big) {
        const auto base=std::get<std::int32_t>(arguments[1]);
        if (base<2 || base>36) throw runtime_error(RuntimeError::school_base,location);
        if (std::holds_alternative<NilString>(arguments[0])) throw runtime_error(RuntimeError::nil_reference,location);
        const auto& text=string_text(arguments[0]);
        const auto digit=[](char16_t c)->unsigned {
            c=static_cast<char16_t>(unicode::to_upper(c));
            if (c>=u'0' && c<=u'9') return c-u'0';
            if (c>=u'A' && c<=u'Z') return c-u'A'+10;
            return 36;
        };
        // The source validates every character before its BigInteger accumulation.
        for (std::size_t i=0;i<text.size();++i) {
            check(i); if (digit(text[i])>=static_cast<unsigned>(base)) throw runtime_error(RuntimeError::school_digit,location);
        }
        if (function==Function::school_dec_big) {
            std::string bytes; bytes.reserve(text.size());
            for (std::size_t i=0;i<text.size();++i) { check(i); bytes+=static_cast<char>(alphabet[digit(text[i])]); }
            big_work(text.size(),location,options);
            return checked_big(location,[&] {
                // DecBig advances rank_weight after the final digit as well.
                if (!text.empty()) (void)BigInteger::from_signed(base).power(static_cast<std::uint32_t>(text.size()));
                return bytes.empty() ? BigInteger{} : BigInteger::parse(bytes,base);
            });
        }
        std::int64_t result=0;
        for (std::size_t i=0;i<text.size();++i) {
            check(i); const auto d=digit(text[i]);
            if (result>(INT64_MAX-d)/base) throw runtime_error(RuntimeError::integer_overflow,location);
            result=result*base+d;
        }
        return result;
    }
    if (std::holds_alternative<BigInteger>(arguments[0]) || (function==Function::school_to_base &&
        (std::holds_alternative<std::u16string>(arguments[0]) || std::holds_alternative<NilString>(arguments[0])))) {
        const auto base=function==Function::school_bin || function==Function::school_bin_format ? 2
            : function==Function::school_oct ? 8 : function==Function::school_hex ? 16 : std::get<std::int32_t>(arguments[1]);
        if (base<2 || base>36) throw runtime_error(RuntimeError::school_base,location);
        BigInteger number;
        if (const auto* big=std::get_if<BigInteger>(&arguments[0])) number=*big;
        else {
            if (std::holds_alternative<NilString>(arguments[0])) return std::u16string{};
            try { number=parse_big(string_text(arguments[0]),location,&options); }
            catch (const Error& error) { if (error.code=="E_FORMAT") return std::u16string{}; throw; }
        }
        if (function==Function::school_to_base && (number.negative() || number.zero())) return std::u16string{u"0"};
        big_work(number.bits()+1,location,options);
        const auto bytes=checked_big(location,[&]{return number.absolute().text(base);});
        if (function!=Function::school_bin_format) return std::u16string(bytes.begin(),bytes.end());
        const auto natural=std::max<std::size_t>(1,(bytes.size()+7)/8);
        const auto requested=arguments.size()>1 ? std::get<std::int32_t>(arguments[1]) : 0;
        const auto count=requested>0 ? std::max(natural,static_cast<std::size_t>(requested)) : natural;
        const auto separator=arguments.size()>2 ? std::u16string_view(string_text(arguments[2])) : std::u16string_view{u" "};
        constexpr std::size_t limit=1'048'576;
        if (count>limit/8 || (count>1 && separator.size()>(limit-count*8)/(count-1))) throw runtime_error(RuntimeError::string_limit,location);
        std::u16string result; result.reserve(count*8+(count-1)*separator.size());
        const auto padding=count*8-bytes.size();
        for (std::size_t i=0;i<count;++i) {
            check(i); if (i) result+=separator;
            for (std::size_t bit=0;bit<8;++bit) {
                const auto position=i*8+bit; result+=position<padding ? u'0' : static_cast<char16_t>(bytes[position-padding]);
            }
        }
        return result;
    }
    const auto number=std::get<std::int64_t>(arguments[0]);
    if (function==Function::school_perfect_square) {
        if (number<0) return std::int64_t{-1};
        const auto low=number&15;
        if (low!=0 && low!=1 && low!=4 && low!=9) return std::int64_t{-1};
        const auto root=static_cast<std::uint64_t>(std::floor(std::sqrt(static_cast<double>(number))+0.5));
        return root*root==static_cast<std::uint64_t>(number) ? static_cast<std::int64_t>(root) : std::int64_t{-1};
    }
    if (function==Function::school_bin_format) {
        const auto magnitude=number<0 ? std::uint64_t{0}-static_cast<std::uint64_t>(number) : static_cast<std::uint64_t>(number);
        std::size_t natural=1; for (auto n=magnitude;n>255;n>>=8) ++natural;
        const auto requested=arguments.size()>1 ? std::get<std::int32_t>(arguments[1]) : 0;
        const auto count=requested>0 ? std::max(natural,static_cast<std::size_t>(requested)) : natural;
        const auto separator=arguments.size()>2 ? std::u16string_view(string_text(arguments[2])) : std::u16string_view{u" "};
        constexpr std::size_t limit=1'048'576;
        if (count>limit/8 || (count>1 && separator.size()>(limit-count*8)/(count-1))) throw runtime_error(RuntimeError::string_limit,location);
        std::u16string result; result.reserve(count*8+(count-1)*separator.size());
        for (std::size_t i=0;i<count;++i) {
            check(i); if (i) result+=separator;
            const auto index=count-i-1; const auto byte=index<8 ? (magnitude>>(index*8))&255U : 0;
            for (int bit=7;bit>=0;--bit) result+=((byte>>bit)&1U) ? u'1' : u'0';
        }
        return result;
    }
    const auto base=function==Function::school_bin ? 2 : function==Function::school_oct ? 8 : function==Function::school_hex ? 16 : std::get<std::int32_t>(arguments[1]);
    if (base<2 || base>36) throw runtime_error(RuntimeError::school_base,location);
    if (function==Function::school_to_base && number<=0) return std::u16string{u"0"};
    if (number==INT64_MIN) throw runtime_error(RuntimeError::integer_overflow,location);
    auto magnitude=static_cast<std::uint64_t>(number<0 ? -number : number);
    std::u16string result;
    do { result+=alphabet[static_cast<std::size_t>(magnitude%base)]; magnitude/=base; } while (magnitude);
    std::reverse(result.begin(),result.end()); return result;
}
} // namespace abc::stdlib

namespace abc {
namespace {
std::int64_t school_multiply(std::int64_t a, std::int64_t b, SourceLocation location) {
    const auto magnitude=[](std::int64_t n) { return n<0 ? std::uint64_t{0}-static_cast<std::uint64_t>(n) : static_cast<std::uint64_t>(n); };
    const bool negative=(a<0)!=(b<0);
    const auto limit=negative ? std::uint64_t{1}<<63 : static_cast<std::uint64_t>(INT64_MAX);
    const auto left=magnitude(a), right=magnitude(b);
    if (right && left>limit/right) throw runtime_error(RuntimeError::integer_overflow,location);
    const auto product=left*right;
    if (negative && product==(std::uint64_t{1}<<63)) return INT64_MIN;
    return negative ? -static_cast<std::int64_t>(product) : static_cast<std::int64_t>(product);
}
std::int64_t school_add(std::int64_t a, std::int64_t b, SourceLocation location) {
    if ((b>0 && a>INT64_MAX-b) || (b<0 && a<INT64_MIN-b)) throw runtime_error(RuntimeError::integer_overflow,location);
    return a+b;
}
std::int64_t school_subtract(std::int64_t a, std::int64_t b, SourceLocation location) {
    if ((b>0 && a<INT64_MIN+b) || (b<0 && a>INT64_MAX+b)) throw runtime_error(RuntimeError::integer_overflow,location);
    return a-b;
}
std::int64_t school_divide(std::int64_t a, std::int64_t b, SourceLocation location) {
    if (!b) throw runtime_error(RuntimeError::division_zero,location);
    if (a==INT64_MIN && b== -1) throw runtime_error(RuntimeError::integer_overflow,location);
    return a/b;
}
std::int64_t school_sqrt(std::int64_t n, SourceLocation location) {
    if (n<0) throw runtime_error(RuntimeError::math_domain,location);
    auto root=static_cast<std::uint64_t>(std::sqrt(static_cast<double>(n)));
    const auto number=static_cast<std::uint64_t>(n);
    while (root && root>number/root) --root;
    while (root<3'037'000'499ULL && root+1<=number/(root+1)) ++root;
    return static_cast<std::int64_t>(root);
}
struct SchoolLists {
    Values& values;
    SchoolContext& context;
    Type type;
    const ExecutionOptions& options;
    std::uint64_t work=0;
    static constexpr std::int32_t bound=46349;
    void check(std::uint64_t index=0) {
        if (work++>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
        if ((index&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
    }
    std::vector<std::uint8_t> sieve(std::size_t size) {
        check();
        // Native scratch storage is charged like the source boolean array.
        if (size>values.remaining) throw runtime_error(RuntimeError::allocation_limit,values.location);
        values.remaining-=size; return std::vector<std::uint8_t>(size);
    }
    void add(const std::shared_ptr<Aggregate>& list, std::int32_t value) {
        auto cell=values.cell(Type::integer,value); values.own_list_cell(cell,list);
        list->items.push_back(std::move(cell)); ++list->version;
    }
    std::shared_ptr<Aggregate> cache() {
        if (context.primes) return context.primes;
        auto composite=sieve(bound+1); auto result=values.array(type,0);
        for (std::int32_t n=2;n<=bound;++n) {
            check(n);
            if (composite[n]) continue;
            add(result,n);
            for (std::int32_t multiple=n+n;multiple<=bound;multiple+=n) { check(multiple); composite[multiple]=1; }
        }
        context.initial_prime_count=static_cast<std::int32_t>(result->items.size()); context.primes=result; return result;
    }
    std::shared_ptr<Aggregate> primes(std::int32_t n) {
        auto result=values.array(type,0);
        if (n<2) return result;
        auto composite=sieve(static_cast<std::size_t>(n));
        for (std::int64_t p=2;p<=n/p;++p) if (!composite[static_cast<std::size_t>(p-1)]) {
            for (auto multiple=p*p;multiple<=n;multiple+=p) { check(multiple); composite[static_cast<std::size_t>(multiple-1)]=1; }
        }
        for (std::int64_t p=2;p<=n;++p) { check(p); if (!composite[static_cast<std::size_t>(p-1)]) add(result,static_cast<std::int32_t>(p)); }
        return result;
    }
    std::shared_ptr<Aggregate> range(std::int32_t low, std::int32_t high) {
        const auto divisors=cache(); auto result=values.array(type,0);
        if (low>high) std::swap(low,high);
        if (divisors->items.empty()) throw runtime_error(RuntimeError::index_bounds,values.location);
        if (low<=std::get<std::int32_t>(divisors->items.back()->value)) {
            for (std::size_t i=0;i<divisors->items.size();++i) {
                check(i); const auto prime=std::get<std::int32_t>(divisors->items[i]->value);
                if (prime>high) break;
                if (prime>=low) add(result,prime);
            }
            if (result->items.empty()) return result;
            const auto last=std::get<std::int32_t>(result->items.back()->value);
            if (last==INT32_MAX) throw runtime_error(RuntimeError::integer_overflow,values.location);
            low=last+1;
            if (low>=high) return result;
        }
        const auto ceiling=[](std::int32_t n) { return n>=0 ? (static_cast<std::int64_t>(n)+bound-1)/bound : static_cast<std::int64_t>(n)/bound; };
        const auto first=ceiling(low), last=ceiling(high);
        for (auto segment=first;segment<=last;++segment) {
            const auto a=std::max((segment-1)*bound+1,std::int64_t{2}), b=segment*bound;
            auto composite=sieve(bound);
            for (std::size_t i=0;i<divisors->items.size();++i) {
                check(i); const auto prime=std::int64_t(std::get<std::int32_t>(divisors->items[i]->value));
                if (prime*prime>b) break;
                if (!prime) throw runtime_error(RuntimeError::division_zero,values.location);
                auto multiple=a; const auto remainder=multiple%prime;
                if (remainder) multiple+=prime-remainder;
                // Mutation of the public cache can supply an invalid divisor.
                if (prime<0) throw runtime_error(RuntimeError::index_bounds,values.location);
                for (;multiple<=b;multiple+=prime) { check(multiple); composite[static_cast<std::size_t>(multiple-a)]=1; }
            }
            for (std::size_t i=0;i<composite.size();++i) {
                check(i); const auto n=a+static_cast<std::int64_t>(i);
                if (!composite[i] && n>=low && n<=high) add(result,static_cast<std::int32_t>(n));
            }
        }
        return result;
    }
    std::shared_ptr<Aggregate> first(std::int32_t count) {
        const auto primes=cache();
        const auto initial_count=context.initial_prime_count;
        if (count<=initial_count) {
            if (count< -1 || (count>0 && static_cast<std::size_t>(count)>primes->items.size())) throw runtime_error(RuntimeError::index_bounds,values.location);
            auto result=values.array(type,0);
            for (std::int32_t i=0;i<count;++i) { check(i); add(result,std::get<std::int32_t>(primes->items[i]->value)); }
            return result;
        }
        if (static_cast<std::size_t>(count-initial_count)>values.remaining) throw runtime_error(RuntimeError::allocation_limit,values.location);
        auto found=initial_count; std::int32_t n=bound+2;
        do {
            check();
            if (stdlib::small_prime(n,values.location,options,context)) { add(primes,n); ++found; }
            if (n>INT32_MAX-2) throw runtime_error(RuntimeError::integer_overflow,values.location);
            n+=2;
        } while (found!=count);
        return primes;
    }
};
struct SchoolFactors {
    SchoolLists& service;
    bool wide;
    void append(const std::shared_ptr<Aggregate>& list, std::int64_t n) {
        const auto element=wide ? Type::int64 : Type::integer;
        if (!wide && (n<INT32_MIN || n>INT32_MAX)) throw runtime_error(RuntimeError::integer_overflow,service.values.location);
        auto cell=service.values.cell(element,wide ? Value{n} : Value{static_cast<std::int32_t>(n)});
        service.values.own_list_cell(cell,list); list->items.push_back(std::move(cell)); ++list->version;
    }
    std::int64_t number(const std::shared_ptr<Cell>& cell) const {
        return wide ? std::get<std::int64_t>(cell->value) : std::int64_t(std::get<std::int32_t>(cell->value));
    }
    void sort(const std::shared_ptr<Aggregate>& list) const {
        std::sort(list->items.begin(),list->items.end(),[&](const auto& a,const auto& b) { return number(a)<number(b); });
        ++list->version;
    }
    std::vector<std::int64_t> factors32(std::int32_t input) {
        std::vector<std::int64_t> result;
        if (input<2) return result;
        const auto primes=wide ? service.context.primes : service.cache();
        std::int64_t n=input; std::size_t index=0;
        const auto prime_at=[&](std::size_t i) {
            if (!primes) return i==0 ? std::int64_t{2} : static_cast<std::int64_t>(i)*2+1;
            if (i>=primes->items.size()) throw runtime_error(RuntimeError::index_bounds,service.values.location);
            return std::int64_t(std::get<std::int32_t>(primes->items[i]->value));
        };
        auto prime=prime_at(index);
        while (prime*prime<=n) {
            service.check();
            if (!prime) throw runtime_error(RuntimeError::division_zero,service.values.location);
            if (prime==1 || prime== -1) throw runtime_error(RuntimeError::instruction_limit,service.values.location);
            while (n%prime==0) { service.check(); result.push_back(prime); n/=prime; }
            prime=prime_at(++index);
        }
        if (result.empty()) result.push_back(input);
        else if (n>1) result.push_back(n);
        return result;
    }
    bool prime(std::uint64_t n, const SchoolContext& context) {
        service.check();
        return n<=INT32_MAX ? stdlib::small_prime(static_cast<std::int32_t>(n),service.values.location,service.options,context)
            : stdlib::large_prime(n,service.values.location,service.options);
    }
    std::int64_t squfof(std::int64_t n) {
        if (prime(static_cast<std::uint64_t>(n),service.context)) return n;
        if (n%2==0) return 2;
        const auto location=service.values.location;
        const auto mul=[&](std::int64_t a,std::int64_t b) { return school_multiply(a,b,location); };
        const auto add=[&](std::int64_t a,std::int64_t b) { return school_add(a,b,location); };
        const auto sub=[&](std::int64_t a,std::int64_t b) { return school_subtract(a,b,location); };
        const auto div=[&](std::int64_t a,std::int64_t b) { return school_divide(a,b,location); };
        const auto sqrt=[&](std::int64_t a) { return school_sqrt(a,location); };
        const auto root=sqrt(n);
        if (mul(root,root)==n) return root;
        for (auto multiplier:{1,3,5,7,11,15,21,33,35,55,77,105,165,231,385,1155}) {
            service.check();
            if (multiplier>1 && n%multiplier==0) return multiplier;
            if (n>INT64_MAX/multiplier) break;
            const auto mn=mul(multiplier,n);
            auto r=sqrt(mn); const auto rn=r;
            auto b=r; std::int64_t a=1;
            const auto h=sub(mul(div(add(rn,b),a),a),b);
            auto c=div(sub(mn,mul(h,h)),a);
            const auto limit=mul(4,sqrt(mul(2,r)));
            for (std::int64_t i=2;i<=limit;++i) {
                service.check(static_cast<std::uint64_t>(i));
                std::swap(a,c); auto q=div(add(rn,b),a);
                const auto t=b; b=sub(mul(q,a),b); c=add(c,mul(q,sub(t,b)));
                if (i%2) continue;
                r=sqrt(c);
                if (mul(r,r)!=c) continue;
                q=div(sub(rn,b),r); auto v=add(mul(q,r),b);
                auto w=div(sub(mn,mul(v,v)),r),u=r;
                for (;;) {
                    service.check(); std::swap(w,u); r=v; q=div(add(rn,v),u);
                    v=sub(mul(q,u),v);
                    if (v==r) break;
                    w=add(w,mul(q,sub(r,v)));
                }
                if (u==INT64_MIN) throw runtime_error(RuntimeError::integer_overflow,location);
                const auto divisor=std::gcd(n,u<0 ? -u : u);
                if (divisor!=1) return divisor;
            }
        }
        return -1;
    }
    std::uint64_t pollard(std::uint64_t n) {
        for (auto p:{2U,3U,5U,7U,11U,13U,17U,19U,23U,29U,31U,37U}) if (n%p==0) return p;
        std::uint64_t low=1,high=3'037'000'500ULL;
        while (low+1<high) { const auto mid=low+(high-low)/2; if (mid<=n/mid) low=mid; else high=mid; }
        if (low*low==n) return low;
        stdlib::Modular63 arithmetic(n,service.values.location,service.options);
        // Same deterministic polynomial constants and restart bound as School.pas.
        // Reduce multiplication before addition instead of overflowing Sqr(x).
        for (auto c:{1U,3U,5U,7U,11U,13U,17U,19U,23U,29U}) {
            std::uint64_t x=2,y=2;
            const auto next=[&](std::uint64_t value) { return (arithmetic.multiply(value,value)+c)%n; };
            for (unsigned step=0;step<500'000;++step) {
                service.check(step); x=next(x); y=next(next(y));
                const auto divisor=std::gcd(x>y ? x-y : y-x,n);
                if (divisor==1) continue;
                if (divisor==n) break;
                return divisor;
            }
        }
        throw runtime_error(RuntimeError::school_factorization,service.values.location);
    }
    std::vector<std::int64_t> factors64(std::int64_t input, bool count_strategy=false) {
        if (input<INT32_MIN) throw runtime_error(RuntimeError::integer_overflow,service.values.location);
        if (input<2) return {};
        auto n=input;
        if (prime(static_cast<std::uint64_t>(n),service.context)) return {input};
        std::vector<std::int64_t> result;
        while (n>1) {
            service.check(); auto divisor=squfof(n);
            if (divisor== -1) divisor=static_cast<std::int64_t>(pollard(static_cast<std::uint64_t>(n)));
            n/=divisor;
            if (n==1 || prime(static_cast<std::uint64_t>(divisor),service.context)) result.push_back(divisor);
            else if (divisor<=INT32_MAX) {
                const auto factors=count_strategy ? factors32(static_cast<std::int32_t>(divisor)) : factors64(divisor);
                result.insert(result.end(),factors.begin(),factors.end());
            } else throw runtime_error(RuntimeError::school_factorization,service.values.location);
        }
        std::sort(result.begin(),result.end()); return result;
    }
    Value divisors32(std::int32_t input, bool count_only) {
        auto list=service.values.array(service.type,0);
        std::int32_t count=1; if (!count_only) append(list,1);
        std::int64_t n=input; const auto primes=service.cache();
        const auto extend=[&](std::int32_t old_count, std::int64_t prime) {
            service.check();
            if (old_count>INT32_MAX-count) throw runtime_error(RuntimeError::integer_overflow,service.values.location);
            if (!count_only) {
                const auto end=list->items.size();
                for (auto i=end-static_cast<std::size_t>(old_count);i<end;++i) {
                    service.check(i); append(list,school_multiply(number(list->items[i]),prime,service.values.location));
                }
            }
            count+=old_count;
        };
        for (std::size_t i=0;i<primes->items.size();++i) {
            service.check(i); const auto p=std::int64_t(std::get<std::int32_t>(primes->items[i]->value));
            if (p*p>n) break;
            if (!p) throw runtime_error(RuntimeError::division_zero,service.values.location);
            if (p==1 || p== -1) throw runtime_error(RuntimeError::instruction_limit,service.values.location);
            const auto old_count=count;
            while (n%p==0) { extend(old_count,p); n/=p; }
        }
        if (n>1) extend(count,n);
        if (count_only) return count;
        sort(list); return list;
    }
    Value invoke(std::int64_t input, SchoolList method) {
        if (!wide && (method==SchoolList::divisors || method==SchoolList::divisors_count))
            return divisors32(static_cast<std::int32_t>(input),method==SchoolList::divisors_count);
        auto factors=wide ? factors64(input,method==SchoolList::factors_count) : factors32(static_cast<std::int32_t>(input));
        if (method==SchoolList::prime_divisors || method==SchoolList::prime_divisors_count)
            factors.erase(std::unique(factors.begin(),factors.end()),factors.end());
        if (method==SchoolList::factors_count || method==SchoolList::prime_divisors_count) return static_cast<std::int32_t>(factors.size());
        auto list=service.values.array(service.type,0);
        if (method!=SchoolList::divisors && method!=SchoolList::divisors_count) {
            for (const auto factor:factors) { service.check(); append(list,factor); }
            return list;
        }
        append(list,1);
        for (std::size_t i=0;i<factors.size();) {
            const auto p=factors[i]; std::size_t exponent=0;
            while (i<factors.size() && factors[i]==p) { ++i; ++exponent; }
            const auto current=list->items.size(); std::int64_t power=1;
            for (std::size_t k=0;k<exponent;++k) {
                power=school_multiply(power,p,service.values.location);
                for (std::size_t j=0;j<current;++j) { service.check(j); append(list,school_multiply(number(list->items[j]),power,service.values.location)); }
            }
        }
        if (method==SchoolList::divisors_count) return static_cast<std::int32_t>(list->items.size());
        sort(list); return list;
    }
};
}
Value invoke_school_list(Values& values, SchoolContext& context, SchoolList method, Type list,
    std::span<const Value> arguments, const ExecutionOptions& options) {
    SchoolLists service{values,context,list,options};
    if (school_factor_method(method)) {
        const bool wide=type_info(values.program,list).element==Type::int64;
        SchoolFactors factors{service,wide};
        return factors.invoke(wide ? std::get<std::int64_t>(arguments[0]) : std::int64_t(std::get<std::int32_t>(arguments[0])),method);
    }
    if (method==SchoolList::primes) return service.primes(std::get<std::int32_t>(arguments[0]));
    if (method==SchoolList::primes_range) return service.range(std::get<std::int32_t>(arguments[0]),std::get<std::int32_t>(arguments[1]));
    if (method==SchoolList::first_primes) return service.first(std::get<std::int32_t>(arguments[0]));
    const auto base=std::get<std::int32_t>(arguments[1]);
    if (method==SchoolList::digits_big) {
        auto result=values.array(list,0); auto n=std::get<BigInteger>(arguments[0]);
        if (n.negative() || base<2) return result;
        if (n.zero()) service.add(result,0);
        const auto divisor=BigInteger::from_signed(base);
        while (!n.zero()) {
            service.check(); big_work(n.bits()/28+1,values.location,options);
            auto divided=checked_big(values.location,[&]{return n.divide(divisor);});
            service.add(result,static_cast<std::int32_t>(divided.second.to_signed())); n=std::move(divided.first);
        }
        std::reverse(result->items.begin(),result->items.end()); return result;
    }
    if (method==SchoolList::digits) {
        auto result=values.array(list,0); auto n=std::get<std::int64_t>(arguments[0]);
        if (n<0 || base<2) return result;
        if (!n) service.add(result,0);
        while (n>0) { service.check(); service.add(result,static_cast<std::int32_t>(n%base)); n/=base; }
        std::reverse(result->items.begin(),result->items.end()); return result;
    }
    const auto digits=std::get<std::shared_ptr<Aggregate>>(arguments[0]);
    if (digits->nil) throw runtime_error(RuntimeError::nil_reference,values.location);
    for (std::size_t i=0;i<digits->items.size();++i) {
        service.check(i);
        if (std::get<std::int32_t>(digits->items[i]->value)>=base) throw runtime_error(RuntimeError::school_expanded_digit,values.location);
    }
    if (method==SchoolList::digits_to_big) {
        BigInteger result; auto power=BigInteger::from_signed(1),radix=BigInteger::from_signed(base);
        for (auto i=digits->items.size();i>0;--i) {
            service.check(i);
            const auto digit=BigInteger::from_signed(std::get<std::int32_t>(digits->items[i-1]->value));
            const auto term=big_binary(Op::multiply,digit,power,values.location,options);
            result=big_binary(Op::add,result,term,values.location,options);
            power=big_binary(Op::multiply,power,radix,values.location,options);
            values.number(term); values.number(result); values.number(power);
        }
        return result;
    }
    std::int64_t result=0,power=1;
    for (auto i=digits->items.size();i>0;--i) {
        service.check(i);
        const auto term=school_multiply(std::get<std::int32_t>(digits->items[i-1]->value),power,values.location);
        result=school_add(result,term,values.location);
        // The original updates this power after every digit, including the last.
        power=school_multiply(power,base,values.location);
    }
    return result;
}
namespace {
std::int64_t school_signed_gcd(std::int64_t a,std::int64_t b,SourceLocation location) {
    while (b) {
        if (a==INT64_MIN && b== -1) throw runtime_error(RuntimeError::integer_overflow,location);
        const auto remainder=a%b; a=b; b=remainder;
    }
    return a;
}
std::int64_t school_absolute(std::int64_t result,SourceLocation location) {
    if (result==INT64_MIN) throw runtime_error(RuntimeError::integer_overflow,location);
    return result<0 ? -result : result;
}
}
std::int64_t school_pair(std::int64_t a, std::int64_t b, bool lcm, SourceLocation location) {
    const auto gcd=school_signed_gcd(a,b,location);
    return school_absolute(lcm ? school_multiply(school_divide(a,gcd,location),b,location) : gcd,location);
}
std::pair<std::int64_t,std::int64_t> school_pair_values(std::int64_t a,std::int64_t b,SourceLocation location) {
    const auto gcd=school_signed_gcd(a,b,location);
    return {gcd,school_absolute(school_multiply(school_divide(a,gcd,location),b,location),location)};
}
std::shared_ptr<Aggregate> school_minmax(Values& values,const std::shared_ptr<Aggregate>& sequence,Type tuple,const ExecutionOptions& options) {
    if (sequence->nil) throw runtime_error(RuntimeError::nil_reference,values.location);
    const auto& info=type_info(values.program,sequence->type); const auto version=sequence->version;
    const bool lazy=info.kind==CompoundKind::sequence;
    const auto iterator=lazy ? sequence_begin(values,sequence) : sequence;
    const auto count=lazy ? sequence_length(iterator,values.location) : sequence->items.size();
    if (lazy) (void)sequence_resume(iterator,0,values.location);
    const auto calculate=[&]<class T>() {
        T low{},high{};
        if constexpr (!std::is_same_v<T,BigInteger>) { low=std::numeric_limits<T>::max(); high=std::numeric_limits<T>::lowest(); }
        for (std::uint64_t i=0;i<count;++i) {
            if (i>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
            if ((i&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
            if (info.kind==CompoundKind::list && sequence->version!=version) throw runtime_error(RuntimeError::collection_changed,values.location);
            const auto number=std::get<T>(lazy ? sequence_at(values,iterator,static_cast<std::int64_t>(i)) : sequence->items[static_cast<std::size_t>(i)]->value);
            if constexpr (std::is_same_v<T,BigInteger>) if (!i) low=high=number;
            if (number<low) low=number;
            if (number>high) high=number;
        }
        if (lazy) (void)sequence_resume(iterator,count,values.location);
        auto result=std::make_shared<Aggregate>(); result->type=tuple;
        result->items.push_back(values.cell(info.element,low)); result->items.push_back(values.cell(info.element,high));
        return result;
    };
    if (info.element==Type::integer) return calculate.template operator()<std::int32_t>();
    if (info.element==Type::int64) return calculate.template operator()<std::int64_t>();
    if (info.element==Type::big_integer) return calculate.template operator()<BigInteger>();
    return calculate.template operator()<double>();
}
Value school_fold(Values& values,const std::shared_ptr<Aggregate>& array, SchoolFold method, const ExecutionOptions& options) {
    const auto location=values.location;
    const auto count=array->nil ? std::size_t{0} : array->items.size();
    if (!count) throw runtime_error(RuntimeError::school_empty_arguments,location);
    if (type_info(values.program,array->type).element==Type::big_integer) {
        const auto number=[&](std::size_t i)->const BigInteger& { return std::get<BigInteger>(array->items[i]->value); };
        if (count==1) return number(0);
        const auto combine=[&](const BigInteger& a,const BigInteger& b) {
            big_work((a.bits()/28+1)*(b.bits()/28+1),location,options);
            const auto product=method==SchoolFold::lcm ? big_binary(Op::multiply,a,b,location,options) : BigInteger{};
            const auto gcd=checked_big(location,[&]{return a.gcd(b);});
            if (method==SchoolFold::gcd) { values.number(gcd); return gcd; }
            values.number(product); values.number(gcd);
            auto result=big_binary(Op::divide_int,product,gcd,location,options); values.number(result); return result;
        };
        auto result=combine(number(0),number(1));
        for (std::size_t i=1;i<count;++i) {
            if (i>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,location);
            result=combine(result,number(i));
        }
        return result;
    }
    const auto number=[&](std::size_t index) { return std::get<std::int64_t>(array->items[index]->value); };
    if (count==1) return number(0);
    const auto gcd=[&](std::int64_t a,std::int64_t b) {
        if (a==INT64_MIN || b==INT64_MIN) throw runtime_error(RuntimeError::integer_overflow,location);
        return std::gcd(a,b);
    };
    const auto combine=[&](std::int64_t a,std::int64_t b) {
        if (method==SchoolFold::gcd) return gcd(a,b);
        // Preserve multiplication-before-GCD and the sign of the source formula.
        const auto product=school_multiply(a,b,location);
        return school_divide(product,gcd(a,b),location);
    };
    auto result=combine(number(0),number(1));
    // The original starts at one, so it processes the second element again.
    for (std::size_t i=1;i<count;++i) {
        if (i>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,location);
        if ((i&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,location);
        result=combine(result,number(i));
    }
    return result;
}
} // namespace abc

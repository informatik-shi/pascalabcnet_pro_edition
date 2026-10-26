#include "school.hpp"
#include "diagnostics.hpp"
#include "runtime_values.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <array>

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
    if (function==Function::school_dec) {
        const auto base=std::get<std::int32_t>(arguments[1]);
        if (base<2 || base>36) throw runtime_error(RuntimeError::school_base,location);
        if (std::holds_alternative<NilString>(arguments[0])) throw runtime_error(RuntimeError::nil_reference,location);
        const auto& text=string_text(arguments[0]);
        const auto digit=[](char16_t c)->unsigned {
            if (c>=u'0' && c<=u'9') return c-u'0';
            if (c>=u'A' && c<=u'Z') return c-u'A'+10;
            if (c>=u'a' && c<=u'z') return c-u'a'+10;
            return 36;
        };
        // The source validates every character before its BigInteger accumulation.
        for (std::size_t i=0;i<text.size();++i) {
            check(i); if (digit(text[i])>=static_cast<unsigned>(base)) throw runtime_error(RuntimeError::school_digit,location);
        }
        std::int64_t result=0;
        for (std::size_t i=0;i<text.size();++i) {
            check(i); const auto d=digit(text[i]);
            if (result>(INT64_MAX-d)/base) throw runtime_error(RuntimeError::integer_overflow,location);
            result=result*base+d;
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
}
Value invoke_school_list(Values& values, SchoolContext& context, SchoolList method, Type list,
    std::span<const Value> arguments, const ExecutionOptions& options) {
    SchoolLists service{values,context,list,options};
    if (method==SchoolList::primes) return service.primes(std::get<std::int32_t>(arguments[0]));
    if (method==SchoolList::primes_range) return service.range(std::get<std::int32_t>(arguments[0]),std::get<std::int32_t>(arguments[1]));
    if (method==SchoolList::first_primes) return service.first(std::get<std::int32_t>(arguments[0]));
    const auto base=std::get<std::int32_t>(arguments[1]);
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
} // namespace abc

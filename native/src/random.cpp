// Seeded compatibility algorithm adapted from dotnet/runtime v10.0.0,
// System/Random.CompatImpl.cs. MIT notice: licenses/DotNet-Runtime-MIT.txt.
// xoshiro256**: David Blackman and Sebastiano Vigna, 2018, public domain/CC0.
#include "random.hpp"
#include "diagnostics.hpp"
#include <bit>
#include <random>

namespace abc {
namespace {
std::int32_t wrap(std::int64_t value) noexcept {
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value));
}
}

void RandomContext::seed(std::int32_t value) noexcept {
    compatible_=true; initialized_=true; subtractive_.fill(0);
    const auto magnitude=value==INT32_MIN ? INT32_MAX : value<0 ? -value : value;
    std::int32_t previous=161803398-magnitude, current=1;
    subtractive_[55]=previous;
    int position=0;
    for (int i=1;i<55;++i) {
        position+=21; if (position>=55) position-=55;
        subtractive_[position]=current;
        current=wrap(std::int64_t(previous)-current);
        if (current<0) current=wrap(std::int64_t(current)+INT32_MAX);
        previous=subtractive_[position];
    }
    for (int pass=0;pass<4;++pass) for (int i=1;i<56;++i) {
        int other=i+30; if (other>=55) other-=55;
        auto n=wrap(std::int64_t(subtractive_[i])-subtractive_[1+other]);
        if (n<0) n=wrap(std::int64_t(n)+INT32_MAX);
        subtractive_[i]=n;
    }
    next_=0; partner_=21;
}

void RandomContext::randomize(SourceLocation location) {
    std::array<std::uint64_t,4> fresh{};
    try {
        std::random_device source;
        do {
            for (auto& word:fresh) {
                const auto high=std::uint64_t(source()), low=std::uint64_t(source());
                word=(high<<32)^low;
            }
        } while (!(fresh[0]|fresh[1]|fresh[2]|fresh[3]));
    } catch (const std::exception&) { throw runtime_error(RuntimeError::random_source,location); }
    state_=fresh; compatible_=false; initialized_=true;
}

std::int32_t RandomContext::sample() noexcept {
    if (++next_>=56) next_=1;
    if (++partner_>=56) partner_=1;
    auto value=wrap(std::int64_t(subtractive_[next_])-subtractive_[partner_]);
    if (value==INT32_MAX) --value;
    if (value<0) value=wrap(std::int64_t(value)+INT32_MAX);
    subtractive_[next_]=value;
    return value;
}

std::uint64_t RandomContext::bits(SourceLocation location) {
    if (!initialized_) randomize(location);
    const auto result=std::rotl(state_[1]*5,7)*9;
    const auto shifted=state_[1]<<17;
    state_[2]^=state_[0]; state_[3]^=state_[1]; state_[1]^=state_[2]; state_[0]^=state_[3];
    state_[2]^=shifted; state_[3]=std::rotl(state_[3],45);
    return result;
}

double RandomContext::unit(SourceLocation location) {
    if (compatible_) return sample()*(1.0/INT32_MAX);
    return static_cast<double>(bits(location)>>11)*(1.0/9007199254740992.0);
}

std::uint32_t RandomContext::bounded(std::uint32_t count, SourceLocation location) {
    auto product=(bits(location)>>32)*count;
    auto low=static_cast<std::uint32_t>(product);
    if (low<count) {
        const auto threshold=static_cast<std::uint32_t>(0U-count)%count;
        while (low<threshold) {
            product=(bits(location)>>32)*count; low=static_cast<std::uint32_t>(product);
        }
    }
    return static_cast<std::uint32_t>(product>>32);
}

std::int32_t RandomContext::range(std::int32_t first, std::int32_t exclusive_last, SourceLocation location) {
    if (first>exclusive_last) throw runtime_error(RuntimeError::random_range,location);
    const auto count=std::int64_t(exclusive_last)-first;
    if (!compatible_) return static_cast<std::int32_t>(std::int64_t(bounded(static_cast<std::uint32_t>(count),location))+first);
    double fraction;
    if (count<=INT32_MAX) fraction=unit(location);
    else {
        auto n=sample(); if (sample()%2==0) n=-n;
        fraction=(static_cast<double>(n)+(INT32_MAX-1))/4294967293.0;
    }
    return static_cast<std::int32_t>(static_cast<std::int64_t>(fraction*static_cast<double>(count))+first);
}
} // namespace abc

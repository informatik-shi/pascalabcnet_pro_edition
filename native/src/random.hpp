#pragma once
#include "abc/core.hpp"
#include <array>

namespace abc {
// Execution-owned state: seeded compatibility PRNG and unseeded xoshiro256**.
class RandomContext {
public:
    void seed(std::int32_t value) noexcept;
    void randomize(SourceLocation location);
    double unit(SourceLocation location);
    std::int32_t range(std::int32_t first, std::int32_t exclusive_last, SourceLocation location);
private:
    bool compatible_=false, initialized_=false;
    std::array<std::int32_t,56> subtractive_{};
    int next_=0, partner_=21;
    std::array<std::uint64_t,4> state_{};
    std::int32_t sample() noexcept;
    std::uint64_t bits(SourceLocation location);
    std::uint32_t bounded(std::uint32_t count, SourceLocation location);
};
}

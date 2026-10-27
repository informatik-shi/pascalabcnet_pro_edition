#pragma once
#include "abc/core.hpp"

namespace abc {
struct Values;
std::u16string regex_swap(Values& values, std::u16string_view text, std::u16string_view first,
    std::u16string_view second, const ExecutionOptions& options, std::uint64_t& work);
}

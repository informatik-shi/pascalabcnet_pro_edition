#pragma once
#include "abc/core.hpp"
#include "diagnostics.hpp"
#include <type_traits>

namespace abc {
inline std::int64_t integral_number(const Value& value, SourceLocation location) {
    return std::visit([&](const auto& v)->std::int64_t {
        using T=std::decay_t<decltype(v)>;
        if constexpr (std::is_integral_v<T>) return static_cast<std::int64_t>(v);
        else throw runtime_error(RuntimeError::invalid_bytecode,location);
    },value);
}
inline Value checked_small_integer(Type type, std::int64_t number, SourceLocation location) {
    const auto [minimum,maximum]=small_integer_bounds(type);
    if (number<minimum || number>maximum) throw runtime_error(RuntimeError::number_range,location,type_name(type));
    switch (type) {
    case Type::byte: return static_cast<std::uint8_t>(number);
    case Type::shortint: return static_cast<std::int8_t>(number);
    case Type::smallint: return static_cast<std::int16_t>(number);
    default: return static_cast<std::uint16_t>(number);
    }
}
}

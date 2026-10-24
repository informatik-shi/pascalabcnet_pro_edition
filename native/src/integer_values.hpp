#pragma once
#include "abc/core.hpp"
#include "diagnostics.hpp"
#include <type_traits>
#include <limits>

namespace abc {
inline std::int64_t integral_number(const Value& value, SourceLocation location, std::string_view target = "int64") {
    return std::visit([&](const auto& v)->std::int64_t {
        using T=std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T,std::uint64_t>) {
            if (v>static_cast<std::uint64_t>(INT64_MAX)) throw runtime_error(RuntimeError::number_range,location,target);
            return static_cast<std::int64_t>(v);
        } else if constexpr (std::is_integral_v<T>) return static_cast<std::int64_t>(v);
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
inline std::uint64_t unsigned_number(const Value& value, SourceLocation location, std::string_view target = "uint64") {
    if (const auto* n=std::get_if<std::uint64_t>(&value)) return *n;
    const auto n=integral_number(value,location,target);
    if (n<0) throw runtime_error(RuntimeError::number_range,location,target);
    return static_cast<std::uint64_t>(n);
}
inline Value checked_unsigned_integer(Type type, const Value& value, SourceLocation location) {
    const auto n=unsigned_number(value,location,type_name(type));
    if (type==Type::longword) {
        if (n>UINT32_MAX) throw runtime_error(RuntimeError::number_range,location,"longword");
        return static_cast<std::uint32_t>(n);
    }
    return n;
}
inline Value checked_signed_integer(Type type, const Value& value, SourceLocation location) {
    const auto n=integral_number(value,location,type_name(type));
    if (type==Type::integer) {
        if (n<INT32_MIN || n>INT32_MAX) throw runtime_error(RuntimeError::number_range,location,"integer");
        return static_cast<std::int32_t>(n);
    }
    return n;
}
inline Value shifted_unsigned(Type type, const Value& value, std::int64_t step, bool descending, SourceLocation location) {
    const auto n=unsigned_number(value,location);
    const auto magnitude=step<0 ? std::uint64_t(-(step+1))+1 : static_cast<std::uint64_t>(step);
    const auto maximum=type==Type::longword ? std::uint64_t{UINT32_MAX} : UINT64_MAX;
    const bool subtract=descending!=(step<0);
    if (subtract ? n<magnitude : magnitude>maximum || n>maximum-magnitude)
        throw runtime_error(RuntimeError::integer_overflow,location);
    const auto result=subtract ? n-magnitude : n+magnitude;
    return type==Type::longword ? Value{static_cast<std::uint32_t>(result)} : Value{result};
}
}

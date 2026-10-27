#pragma once
#include "abc/core.hpp"
#include <array>
#include <span>

namespace abc {
struct Values;
enum class SchoolIPMethod {
    address_empty,address_text,address_suffix,address_value,address_binary,address_number,address_set,
    address_format,address_string,address_and,address_or,address_not,address_add,
    ip_empty,ip_text,ip_suffix,ip_address,ip_netmask,ip_bitmask,ip_hosts,ip_wildcard,ip_network,ip_broadcast,ip_hostmin,ip_hostmax,
    ip_string,ip_generate,ip_generate_binary
};
inline bool school_ip_kind(CompoundKind kind) { return kind==CompoundKind::school_address || kind==CompoundKind::school_ip; }
inline bool school_ip_generator(SchoolIPMethod method) { return method==SchoolIPMethod::ip_generate || method==SchoolIPMethod::ip_generate_binary; }
inline bool school_ip_allowed(CompoundKind kind,SchoolIPMethod method) {
    return kind==CompoundKind::school_address ? method<=SchoolIPMethod::address_add
        : kind==CompoundKind::school_ip && method>=SchoolIPMethod::ip_empty && method<=SchoolIPMethod::ip_generate_binary;
}
inline std::vector<Type> school_ip_parameters(Type owner,SchoolIPMethod method) {
    switch (method) {
    case SchoolIPMethod::address_empty: case SchoolIPMethod::ip_empty: return {};
    case SchoolIPMethod::address_text: return {Type::string};
    case SchoolIPMethod::address_suffix: return {Type::integer};
    case SchoolIPMethod::ip_text: return {Type::string,Type::string};
    case SchoolIPMethod::ip_suffix: return {Type::string,Type::byte};
    case SchoolIPMethod::address_set: case SchoolIPMethod::address_format: return {owner,Type::string};
    case SchoolIPMethod::address_and: case SchoolIPMethod::address_or: return {owner,owner};
    case SchoolIPMethod::address_add: return {owner,Type::longword};
    default: return {owner};
    }
}
inline Type school_ip_result(Type owner,const TypeInfo& info,SchoolIPMethod method,Type generated=Type::invalid) {
    if (school_ip_generator(method)) return generated;
    if (method>=SchoolIPMethod::ip_address && method<=SchoolIPMethod::ip_hostmax)
        return info.fields[static_cast<std::size_t>(method)-static_cast<std::size_t>(SchoolIPMethod::ip_address)].type;
    switch (method) {
    case SchoolIPMethod::address_value: case SchoolIPMethod::address_binary: case SchoolIPMethod::address_format:
    case SchoolIPMethod::address_string: case SchoolIPMethod::ip_string: return Type::string;
    case SchoolIPMethod::address_number: return Type::longword;
    case SchoolIPMethod::address_set: return Type::unit;
    default: return owner;
    }
}
inline constexpr std::array<std::string_view,3> school_address_fields{"$address","$binary","$number"};
inline constexpr std::array<std::string_view,9> school_ip_fields{"$address","$netmask","$bitmask","$hosts","$wildcard","$network","$broadcast","$hostmin","$hostmax"};
Value invoke_school_ip(Values& values,Type owner,SchoolIPMethod method,Type generated,std::span<const Value> arguments,const ExecutionOptions& options);
std::u16string school_ip_text(const Program& program,const std::shared_ptr<Aggregate>& object,SourceLocation location);
} // namespace abc

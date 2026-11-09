#pragma once
#include "abc/core.hpp"
#include <span>
namespace abc {
struct Values;
bool interface_kind(const TypeInfo&);
std::vector<InterfaceMember> interface_contract(const std::vector<TypeInfo>&,Type);
bool interface_property_match(const InterfaceMember&,const InterfaceMember&);
bool interface_member_key_equal(const InterfaceMember&,const InterfaceMember&);
bool interface_compatible(const std::vector<TypeInfo>&,Type from,Type to);
bool interface_cast_valid(const Program&,Type from,Type to,bool checked);
void verify_interfaces(const Program&);
Value interface_cast(Values&,Type from,Type to,bool checked,const Value&);
std::size_t interface_target(Values&,Type owner,std::size_t slot,const Value& receiver,const ExecutionOptions&);
Value interface_invoke(Values&,Type owner,std::size_t slot,const Value& receiver,std::span<const Value> arguments,const ExecutionOptions&);
}

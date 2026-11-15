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
Type class_parent(const std::vector<TypeInfo>&,Type);
bool valid_class_ancestry(const std::vector<TypeInfo>&,Type);
bool class_reference_compatible(const std::vector<TypeInfo>&,Type from,Type to);
bool virtual_signature_matches(const Routine&,const Routine&);
bool has_abstract_methods(const std::vector<TypeInfo>&,const std::vector<Routine>&,Type);
void verify_virtual_methods(const Program&);
std::size_t virtual_method_target(const Program&,std::size_t,Type,SourceLocation);
bool interface_cast_valid(const Program&,Type from,Type to,bool checked);
void verify_interfaces(const Program&);
Value interface_cast(Values&,Type from,Type to,bool checked,const Value&);
std::size_t interface_target(Values&,Type owner,std::size_t slot,const Value& receiver,const ExecutionOptions&);
Value interface_invoke(Values&,Type owner,std::size_t slot,const Value& receiver,std::span<const Value> arguments,const ExecutionOptions&);
}

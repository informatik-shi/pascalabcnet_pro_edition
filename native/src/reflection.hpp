#pragma once

#include "abc/core.hpp"
#include <span>

namespace abc {
struct Values;
inline constexpr std::string_view property_info_class_name="$system.reflection.propertyinfo";
enum class ReflectionMethod {
    get_property, get_properties, name, property_type, declaring_type, reflected_type,
    can_read, can_write, get_value, get_value_indexed, attributes, attributes_typed,
    is_defined, to_string, base_type, set_value, set_value_indexed,
    attribute_equals, attribute_hash, attribute_default, attribute_type_id
};
struct ReflectionSignature { std::vector<Type> parameters;Type result; };
TypeInfo property_info_class();
bool property_info_shape(const TypeInfo& shape);
void verify_property_metadata(const Program& program);
std::optional<ReflectionSignature> reflection_signature(const std::vector<TypeInfo>& types,Type owner,ReflectionMethod method);
Value reflection_invoke(Values& values,Type owner,ReflectionMethod method,std::span<const Value> arguments,const ExecutionOptions& options);
}

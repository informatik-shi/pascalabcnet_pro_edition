#pragma once

#include "abc/core.hpp"
#include <span>

namespace abc {
class FileContext;
struct Values;

enum class CountriesMethod { construct, load, name, capital, population, continent };
inline constexpr std::string_view country_class_name="$countries.country";
inline constexpr std::string_view country_attribute_name="$countries.att";
inline constexpr std::string_view system_attribute_name="$system.attribute";

TypeInfo countries_class(std::string_view name,Type parent=Type::invalid);
bool countries_class_shape(const std::vector<TypeInfo>& types,const TypeInfo& shape);
bool countries_method_valid(const std::vector<TypeInfo>& types,Type owner,CountriesMethod method);
std::vector<Type> countries_parameters(Type owner,const TypeInfo& shape,CountriesMethod method);
Type countries_result(Type owner,CountriesMethod method);
std::optional<CountriesMethod> country_property(std::string_view name);
Value countries_invoke(Values& values,FileContext& files,const ExecutionOptions& options,
    Type owner,CountriesMethod method,std::span<const Value> arguments);
}

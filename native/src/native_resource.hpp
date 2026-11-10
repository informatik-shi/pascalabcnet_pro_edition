#pragma once
#include "encoding.hpp"
namespace abc {
bool native_resource_signature_valid(const Program&,Type,std::size_t);
std::vector<Type> native_resource_parameters(const Program&,Type,std::size_t);
Type native_resource_result(const Program&,Type,std::size_t);
Value native_resource_invoke(EncodingContext&,Values&,FileContext&,Type,std::size_t,std::span<const Value>);
}

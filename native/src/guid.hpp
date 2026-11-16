#pragma once
#include "abc/core.hpp"
#include <array>
#include <span>

namespace abc {
struct Values;
enum class GuidMethod : std::uint8_t { parse, from_bytes, to_string, to_string_format, to_bytes, compare_to, equals };
inline bool guid_record_type(const TypeInfo& type) {return type.native_record==NativeRecord::guid;}
bool guid_record_shape(std::span<const TypeInfo>,const TypeInfo&);
std::vector<Type> guid_parameters(Type owner,Type byte_array,GuidMethod method);
Type guid_result(Type owner,Type byte_array,GuidMethod method);
Value guid_invoke(Values&,Type,GuidMethod,std::span<const Value>);
std::u16string guid_text(const Value&,std::u16string_view,SourceLocation);
int compare_guid(const Value&,const Value&);
std::array<std::uint8_t,16> guid_bytes(const Value&);
}

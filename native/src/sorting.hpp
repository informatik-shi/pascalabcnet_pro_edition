#pragma once
#include "abc/core.hpp"
namespace abc {
struct Values;
enum class SortMethod:unsigned { default_order,ordinal_order,comparison,less,key };
constexpr std::size_t sort_descending=16,sort_extension=32,sort_generic_key=64,sort_flags=127;
inline SortMethod sort_method(std::size_t code){return static_cast<SortMethod>(code&15U);}
inline Type sort_callback(std::size_t code){return code>>8U ? static_cast<Type>(code>>8U) : Type::invalid;}
bool sort_signature(const Program&,Type,std::size_t);
void sort_array(Values&,Type,std::size_t,const Value&,const Value*,const ExecutionOptions&);
}

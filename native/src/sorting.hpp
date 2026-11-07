#pragma once
#include "abc/core.hpp"
namespace abc {
struct Values;
enum class SortMethod:unsigned { default_order,ordinal_order,comparison,less,key,comparer,system_default,system_comparison,system_comparer,system_pair_default,system_pair_comparer };
constexpr std::size_t sort_descending=16,sort_extension=32,sort_generic_key=64,sort_range=128,sort_flags=255;
inline SortMethod sort_method(std::size_t code){return static_cast<SortMethod>(code&15U);}
inline bool sort_system(std::size_t code){return sort_method(code)>=SortMethod::system_default;}
inline bool sort_paired(std::size_t code){return sort_method(code)==SortMethod::system_pair_default || sort_method(code)==SortMethod::system_pair_comparer;}
inline SortMethod sort_algorithm(std::size_t code){const auto method=sort_method(code);return method==SortMethod::system_default || method==SortMethod::system_pair_default ? SortMethod::default_order : method==SortMethod::system_comparison ? SortMethod::comparison : method==SortMethod::system_comparer || method==SortMethod::system_pair_comparer ? SortMethod::comparer : method;}
// Closed type IDs are bounded below 4096 by the compiler type limit.
inline Type sort_callback(std::size_t code){const auto type=(code>>8U)&4095U;return type ? static_cast<Type>(type) : Type::invalid;}
inline Type sort_items(std::size_t code){return code>>20U ? static_cast<Type>(code>>20U) : Type::invalid;}
bool sort_signature(const Program&,Type,std::size_t);
bool sort_reference(const Program&,Type,std::size_t);
std::vector<Type> sort_parameters(const Program&,Type,std::size_t);
void sort_array(Values&,Type,std::size_t,const Value&,const Value*,const ExecutionOptions&,std::int32_t index=0,std::int32_t count=-1,const Value* items=nullptr);
}

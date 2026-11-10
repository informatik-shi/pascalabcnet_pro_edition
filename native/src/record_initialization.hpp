#pragma once
#include "abc/core.hpp"
namespace abc {
struct Values;
// Declaration literals allocate CLR-style zero storage before evaluating values.
inline constexpr std::size_t raw_array_initializer = ~std::size_t{0};
// These operands are resolved from the closed program by generation/verification.
std::size_t record_initializer_operand(const Program&,Type);
Value initialized_default(Values&,Type,bool constructed,bool generic,std::size_t initializer);
void initialize_record_array(Values&,const std::shared_ptr<Aggregate>&,std::size_t initializer,std::size_t first=0);
}

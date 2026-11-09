#pragma once
#include "abc/core.hpp"
namespace abc {
struct Values;
bool list_user_bulk(Values&,const std::shared_ptr<Aggregate>& target,const std::shared_ptr<Aggregate>& input,std::size_t position,const ExecutionOptions&);
}

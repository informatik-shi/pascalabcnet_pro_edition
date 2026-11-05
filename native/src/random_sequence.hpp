#pragma once
#include "abc/core.hpp"
#include <array>
#include <span>

namespace abc {
struct Values;
enum class RandomSequenceMethod { integer,real };
struct RandomSequenceState {
    std::array<Value,3> arguments{};
    std::size_t builtin=0,arity=0;
    std::uint32_t count=0;
    std::uint64_t index=0;
    bool positioned=false,available=false;
    Value current=std::int32_t{0};
};
bool random_sequence_signature(const Program& program,Type type,RandomSequenceMethod method);
std::vector<Type> random_sequence_parameters(RandomSequenceMethod method);
std::shared_ptr<Aggregate> random_sequence_create(Values& values,Type type,RandomSequenceMethod method,std::span<const Value> arguments);
bool random_sequence_resume(Values& values,const std::shared_ptr<Aggregate>& cursor,std::uint64_t index,const ExecutionOptions& options);
}

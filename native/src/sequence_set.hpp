#pragma once
#include "abc/core.hpp"
#include "hash_set.hpp"
namespace abc {
struct SequenceSetState {
    HashSetState set;
    Value iterator=std::shared_ptr<Aggregate>{},current=std::int32_t{0};
    Type protocol=Type::invalid;
    std::uint64_t index=0,work=0;
    unsigned phase=1;
    bool initialized=false,scope=false,armed=false,yield_scope=false,positioned=false,available=false,ended=false,running=false;
};
inline bool sequence_set_binary(SequenceSetMethod method){return method!=SequenceSetMethod::distinct;}
inline bool sequence_set_preload(SequenceSetMethod method){return method==SequenceSetMethod::intersect || method==SequenceSetMethod::subtract;}
bool sequence_set_signature(const Program&,Type,std::size_t);
std::shared_ptr<Aggregate> sequence_set(Values&,Type,SequenceSetMethod,const std::shared_ptr<Aggregate>&,const std::shared_ptr<Aggregate>&,const std::shared_ptr<Aggregate>&,const ExecutionOptions&);
bool sequence_set_resume(Values&,const std::shared_ptr<Aggregate>&,std::uint64_t,const ExecutionOptions&);
void sequence_set_running_dispose(Values&,SequenceSetState&,const ExecutionOptions&);
void sequence_set_dispose(Values&,SequenceSetState&,const ExecutionOptions&);
}

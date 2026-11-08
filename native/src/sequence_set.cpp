#include "sequence_set.hpp"
#include "runtime_values.hpp"
#include "sequences.hpp"
#include "sequence_scope.hpp"
#include "sequence_guard.hpp"
#include "equality.hpp"
#include "enumerator.hpp"
#include "value_hash.hpp"
#include <algorithm>
#include <limits>
namespace abc {
namespace {
Type protocol_type(const Program& p,Type element){for(std::size_t n=0;n<p.types.size();++n)if(p.types[n].kind==CompoundKind::enumerator && p.types[n].element==element)return static_cast<Type>(64+n);return Type::invalid;}
void step(Values& values,SequenceSetState& cursor,const ExecutionOptions& options){ValueHashWork work{values,options,cursor.work};work.step();cursor.work=work.done;}
struct Budget {SequenceSetState& cursor;ValueHashWork work;~Budget(){cursor.work=work.done;}};
void resize(Values& values,HashSetState& set,std::size_t size,ValueHashWork& work){
    if(size>static_cast<std::size_t>(INT32_MAX) || size>values.remaining)throw runtime_error(RuntimeError::allocation_limit,values.location);values.remaining-=size;
    auto slots=set.slots;slots.resize(size);auto codes=set.codes,next=set.next;codes.resize(size);next.resize(size);std::vector<std::int32_t> buckets(size,0);
    for(std::int32_t n=0;n<set.last_index;++n){work.step();const auto at=static_cast<std::size_t>(n);const auto bucket=static_cast<std::size_t>(codes[at])%size;next[at]=buckets[bucket]-1;buckets[bucket]=n+1;}
    set.slots=std::move(slots);set.codes=std::move(codes);set.next=std::move(next);set.buckets=std::move(buckets);set.capacity=size;
}
bool modify(Values& values,SequenceSetState& cursor,Type element,const Value& value,bool remove,const ExecutionOptions& options){
    Budget budget{cursor,{values,options,cursor.work}};auto& work=budget.work;auto& set=cursor.set;work.step();
    const auto hash=equality_nil(value) ? 0 : equality_policy_hash(values,element,set.comparer,value,options)&0x7fffffff;
    auto bucket=static_cast<std::size_t>(hash)%set.capacity;std::int32_t previous=-1;
    for(auto n=set.buckets[bucket]-1;n>=0;previous=n,n=set.next[static_cast<std::size_t>(n)]){
        work.step();const auto at=static_cast<std::size_t>(n);if(at>=set.slots.size() || !set.slots[at])throw runtime_error(RuntimeError::invalid_bytecode,values.location);
        if(set.codes[at]!=hash || !equality_policy_value(values,element,set.comparer,set.slots[at]->value,value,options))continue;
        if(!remove)return false;if(previous<0)set.buckets[bucket]=set.next[at]+1;else set.next[static_cast<std::size_t>(previous)]=set.next[at];
        set.codes[at]=-1;set.slots[at]->value=values.zero(element,false,true);set.next[at]=set.free_head;set.free_head=n;return true;
    }
    if(remove)return false;const bool reuse=set.free_head>=0;
    if(!reuse && static_cast<std::size_t>(set.last_index)==set.capacity){if(set.last_index>(INT32_MAX-1)/2)throw runtime_error(RuntimeError::integer_overflow,values.location);resize(values,set,static_cast<std::size_t>(set.last_index)*2+1,work);bucket=static_cast<std::size_t>(hash)%set.capacity;}
    const auto n=reuse ? set.free_head : set.last_index;const auto at=static_cast<std::size_t>(n);const auto copied=values.copy(value);
    if(!set.slots[at])set.slots[at]=values.cell(element,copied);else set.slots[at]->value=copied;
    if(reuse)set.free_head=set.next[at];else ++set.last_index;set.codes[at]=hash;set.next[at]=set.buckets[bucket]-1;set.buckets[bucket]=n+1;return true;
}
void close(Values& values,SequenceSetState& cursor,const ExecutionOptions& options){if(!cursor.scope)return;cursor.scope=false;cursor.armed=false;auto iterator=std::move(cursor.iterator);cursor.iterator=std::shared_ptr<Aggregate>{};enumerator_finish(values,cursor.protocol,iterator,options,{});}
struct Running {bool& flag;explicit Running(bool& f):flag(f){flag=true;}~Running(){flag=false;}};
}
bool sequence_set_signature(const Program& p,Type type,std::size_t operand){
    if(!compound(type) || !valid_value_type(p,type))return false;const auto& shape=type_info(p,type);if(shape.kind!=CompoundKind::sequence || shape.rank!=1 || protocol_type(p,shape.element)==Type::invalid)return false;
    const auto method=operand&255U,policy=operand>>8U;if(method>static_cast<std::size_t>(SequenceSetMethod::subtract))return false;if(!policy)return true;
    if(policy<64 || policy>=64+p.types.size())return false;const auto& payload=type_info(p,static_cast<Type>(policy));return payload.kind==CompoundKind::equality_comparer_interface && payload.element==shape.element;
}
std::shared_ptr<Aggregate> sequence_set(Values& values,Type type,SequenceSetMethod method,const std::shared_ptr<Aggregate>& source,const std::shared_ptr<Aggregate>& second,const std::shared_ptr<Aggregate>& comparer,const ExecutionOptions& options){
    if(!source || source->nil || (sequence_set_binary(method) && (!second || second->nil)))throw runtime_error(RuntimeError::nil_reference,values.location);
    const auto nesting=std::max(source->sequence ? source->sequence->nesting : 0,second && second->sequence ? second->sequence->nesting : 0);if(nesting>=options.call_limit)throw runtime_error(RuntimeError::sequence_depth,values.location);
    auto result=sequence_composed(values,type,source,SequenceProvider::set_query,options);auto& state=*result->sequence;state.nesting=nesting+1;state.second_source=second;state.callable=comparer;state.set_method=method;return result;
}
void sequence_set_running_dispose(Values& values,SequenceSetState& cursor,const ExecutionOptions& options){if(!cursor.scope || !cursor.armed || !cursor.yield_scope)return;cursor.armed=false;enumerator_finish(values,cursor.protocol,cursor.iterator,options,{});}
void sequence_set_dispose(Values& values,SequenceSetState& cursor,const ExecutionOptions& options){cursor.ended=true;cursor.available=false;if(cursor.armed)close(values,cursor,options);else{cursor.scope=false;cursor.iterator=std::shared_ptr<Aggregate>{};}}
bool sequence_set_resume(Values& values,const std::shared_ptr<Aggregate>& source,std::uint64_t index,const ExecutionOptions& options){
    SequenceDepthGuard depth(values,options);auto& state=*source->sequence;if(!state.set_query)throw runtime_error(RuntimeError::invalid_bytecode,values.location);auto& cursor=*state.set_query;
    if(cursor.running)return false;if(cursor.positioned && index==cursor.index)return cursor.available;if(cursor.ended)return false;
    if((!cursor.positioned && index) || (cursor.positioned && (index<=cursor.index || index-cursor.index!=1)))throw runtime_error(RuntimeError::invalid_bytecode,values.location);Running running(cursor.running);
    const auto element=type_info(values.program,source->type).element;const auto method=state.set_method;
    if(!cursor.initialized){cursor.protocol=protocol_type(values.program,element);if(cursor.protocol==Type::invalid)throw runtime_error(RuntimeError::invalid_bytecode,values.location);cursor.set.comparer=state.callable;ValueHashWork work{values,options,cursor.work};resize(values,cursor.set,7,work);cursor.work=work.done;cursor.initialized=true;}
    while(!cursor.ended){
        step(values,cursor,options);
        if(!cursor.scope){const auto input=sequence_set_preload(method) ? (cursor.phase==1 ? state.second_source : state.source) : (cursor.phase==2 ? state.second_source : state.source);cursor.iterator=enumerator_invoke(values,input->type,cursor.protocol,EnumeratorMethod::get,input,options);cursor.scope=true;cursor.armed=true;cursor.yield_scope=!(sequence_set_preload(method) && cursor.phase==1);}
        if(!std::get<bool>(enumerator_invoke(values,cursor.protocol,Type::invalid,EnumeratorMethod::move,cursor.iterator,options))){close(values,cursor,options);if(cursor.phase==1 && sequence_set_binary(method)){cursor.phase=2;continue;}cursor.ended=true;break;}
        auto item=enumerator_invoke(values,cursor.protocol,Type::invalid,EnumeratorMethod::current,cursor.iterator,options);
        const bool emit=!(sequence_set_preload(method) && cursor.phase==1);const bool accepted=modify(values,cursor,element,item,method==SequenceSetMethod::intersect && emit,options);
        if(emit && accepted){cursor.armed=true;cursor.current=std::move(item);cursor.positioned=true;cursor.available=true;cursor.index=index;return true;}
    }
    cursor.available=false;cursor.positioned=true;cursor.index=index;return false;
}
}

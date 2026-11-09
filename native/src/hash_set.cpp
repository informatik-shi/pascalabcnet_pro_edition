#include "hash_set.hpp"
#include "value_hash.hpp"
#include "equality.hpp"
#include "ordinal_range.hpp"
#include "real_interval.hpp"
#include "runtime_values.hpp"
#include "sequences.hpp"
#include "collection_interface.hpp"
#include "interfaces.hpp"
#include "sequence_scope.hpp"
#include "big_integer_runtime.hpp"
#include <cmath>
#include <array>
#include <limits>
#include <type_traits>

namespace abc {
// Source contract: PABCSystem set operators and .NET Framework HashSet slot/version rules.
// Storage, hashing, typed dispatch and execution guards are implemented natively.
bool hash_set_reference(HashSetMethod method){return method>=HashSetMethod::ref_add && method<=HashSetMethod::ref_except;}
namespace {
bool sequence_method(HashSetMethod m){return m==HashSetMethod::construct_sequence || (m>=HashSetMethod::union_with && m<=HashSetMethod::overlaps) || m==HashSetMethod::ref_union || m==HashSetMethod::ref_except;}
const TypeInfo& info(const std::vector<TypeInfo>& types,Type owner){return types.at(static_cast<unsigned>(owner)-64U);}
Type array_type(const std::vector<TypeInfo>& types,Type element){for(std::size_t i=0;i<types.size();++i)if(types[i].kind==CompoundKind::array && types[i].rank==1 && types[i].element==element)return static_cast<Type>(64+i);return Type::invalid;}
}
bool hash_set_sequence_method(HashSetMethod method){return sequence_method(method);}
bool hash_set_signature(const Program& p,Type owner,Type sequence,HashSetMethod m){
    if(m>HashSetMethod::comparer || !compound(owner) || static_cast<unsigned>(owner)-64U>=p.types.size() || type_info(p,owner).kind!=CompoundKind::hash_set)return false;
    const auto element=type_info(p,owner).element;
    if(m==HashSetMethod::construct_comparer || m==HashSetMethod::construct_sequence_comparer || m==HashSetMethod::comparer)return compound(sequence) && valid_value_type(p,sequence) && type_info(p,sequence).kind==CompoundKind::equality_comparer_interface && type_info(p,sequence).element==element;
    if(m==HashSetMethod::remove_where){
        if(!compound(sequence) || static_cast<unsigned>(sequence)-64U>=p.types.size())return false;
        const auto& callback=type_info(p,sequence);
        return callback.kind==CompoundKind::callable && callback.element==Type::boolean && callback.parameters==std::vector<CallableParameter>{{element,false,false}};
    }
    if(m>=HashSetMethod::copy_array && m<=HashSetMethod::copy_count && array_type(p.types,element)==Type::invalid)return false;
    if(!sequence_method(m))return sequence==Type::invalid;
    return compound(sequence) && static_cast<unsigned>(sequence)-64U<p.types.size() && type_info(p,sequence).kind==CompoundKind::sequence && type_info(p,sequence).element==type_info(p,owner).element;
}
std::vector<Type> hash_set_parameters(const std::vector<TypeInfo>& types,Type owner,Type sequence,HashSetMethod m){
    if(m==HashSetMethod::construct_comparer)return {sequence};
    if(m==HashSetMethod::construct_sequence_comparer)return {info(types,owner).key,sequence};
    if(m==HashSetMethod::comparer)return {owner};
    if(m==HashSetMethod::construct)return {};
    if(m==HashSetMethod::construct_sequence)return {sequence};
    if(m==HashSetMethod::construct_copy)return {owner};
    if(m==HashSetMethod::membership)return {info(types,owner).element,owner};
    std::vector<Type> result{hash_set_reference(m) ? reference_type(owner) : owner};
    if(m==HashSetMethod::count || m==HashSetMethod::clear || m==HashSetMethod::trim_excess || m==HashSetMethod::copy_value)return result;
    if(m>=HashSetMethod::copy_array && m<=HashSetMethod::copy_count){result.push_back(array_type(types,info(types,owner).element));if(m>=HashSetMethod::copy_index)result.push_back(Type::integer);if(m==HashSetMethod::copy_count)result.push_back(Type::integer);return result;}
    if(m==HashSetMethod::remove_where){result.push_back(sequence);return result;}
    if(m==HashSetMethod::try_get_value){const auto element=info(types,owner).element;result.push_back(element);result.push_back(reference_type(element));return result;}
    if(sequence_method(m))result.push_back(sequence);
    else if(m==HashSetMethod::add || m==HashSetMethod::remove || m==HashSetMethod::contains || m==HashSetMethod::add_copy || m==HashSetMethod::remove_copy || m==HashSetMethod::ref_add || m==HashSetMethod::ref_remove)result.push_back(info(types,owner).element);
    else result.push_back(owner);
    return result;
}
Type hash_set_result(Type owner,HashSetMethod m,Type payload){
    if(m==HashSetMethod::comparer)return payload;
    if(m==HashSetMethod::construct_comparer || m==HashSetMethod::construct_sequence_comparer)return owner;
    if(m<=HashSetMethod::construct_copy || m==HashSetMethod::copy_value || (m>=HashSetMethod::unite && m<=HashSetMethod::remove_copy))return owner;
    if(m==HashSetMethod::count || m==HashSetMethod::remove_where)return Type::integer;
    if((m>=HashSetMethod::copy_array && m<=HashSetMethod::copy_count) || m==HashSetMethod::trim_excess || m==HashSetMethod::clear || (m>=HashSetMethod::union_with && m<=HashSetMethod::symmetric_with) || hash_set_reference(m))return Type::unit;
    return Type::boolean;
}
namespace {
using Work=ValueHashWork;
void require(const std::shared_ptr<Aggregate>& set,SourceLocation location){if(!set || set->nil)throw runtime_error(RuntimeError::nil_reference,location);if(!set->hash_set)throw runtime_error(RuntimeError::invalid_bytecode,location);}
std::size_t prime(std::size_t minimum){
    static constexpr std::size_t table[]={3,7,11,17,23,29,37,47,59,71,89,107,131,163,197,239,293,353,431,521,631,761,919,1103,1327,1597,1931,2333,2801,3371,4049,4861,5839,7013,8419,10103,12143,14591,17519,21023,25229,30293,36353,43627,52361,62851,75431,90523,108631,130363,156437,187751,225307,270371,324449,389357,467237,560689,672827,807403,968897,1162687,1395263,1674319,2009191,2411033,2893249,3471899,4166287,4999559,5999471,7199369};
    for(const auto n:table)if(n>=minimum)return n;
    throw std::length_error("HashSet storage limit");
}
constexpr auto absent=std::numeric_limits<std::size_t>::max();
Type element(Work& work,const std::shared_ptr<Aggregate>& set){return type_info(work.values.program,set->type).element;}
bool null_item(const Value& v){if(std::holds_alternative<NilString>(v))return true;if(const auto* p=std::get_if<std::shared_ptr<Aggregate>>(&v))return !*p || (*p)->nil;if(const auto* p=std::get_if<std::shared_ptr<TextFile>>(&v))return !*p;if(const auto* p=std::get_if<ReflectedType>(&v))return p->target==Type::invalid;return false;}
std::size_t hash(Work& work,const std::shared_ptr<Aggregate>& set,const Value& value){work.step();return null_item(value) ? 0U : static_cast<std::uint32_t>(equality_policy_hash(work.values,element(work,set),set->hash_set->comparer,value,work.options))&0x7fffffffU;}
bool same_policy(Work& work,const std::shared_ptr<Aggregate>& a,const std::shared_ptr<Aggregate>& b){return equality_policy_same(work.values.program,a->hash_set->comparer,b->hash_set->comparer);}
std::shared_ptr<Cell> physical(Work& work,const std::shared_ptr<Aggregate>& set,std::size_t index){auto& state=*set->hash_set;if(!state.capacity)throw runtime_error(RuntimeError::nil_reference,work.values.location);if(index>=state.slots.size())throw runtime_error(RuntimeError::index_bounds,work.values.location);auto& cell=state.slots[index];if(!cell)cell=work.values.cell(element(work,set),work.values.zero(element(work,set),false,true));return cell;}
std::int32_t& bucket_at(Work& work,const std::shared_ptr<Aggregate>& set,std::size_t index){auto& buckets=set->hash_set->buckets;if(buckets.empty())throw runtime_error(RuntimeError::nil_reference,work.values.location);if(index>=buckets.size())throw runtime_error(RuntimeError::index_bounds,work.values.location);return buckets[index];}
std::int32_t next_at(Work& work,const std::shared_ptr<Aggregate>& set,std::size_t index){(void)physical(work,set,index);return set->hash_set->next[index];}
void reserve(Work& work,const std::shared_ptr<Aggregate>& set,std::size_t size){
 auto& state=*set->hash_set;if(size<static_cast<std::size_t>(state.last_index) || size>INT32_MAX || size>work.values.list_storage_remaining)throw runtime_error(RuntimeError::allocation_limit,work.values.location);
 std::vector<std::shared_ptr<Cell>> cells(size);std::vector<std::int32_t> codes(size),next(size),buckets(size);
 for(std::size_t i=0;i<static_cast<std::size_t>(state.last_index);++i){work.step();codes[i]=state.codes[i];next[i]=state.next[i];if(state.slots[i])cells[i]=work.values.cell(element(work,set),work.values.copy(state.slots[i]->value));if(codes[i]>=0){const auto bucket=static_cast<std::size_t>(codes[i])%size;next[i]=buckets[bucket]-1;buckets[bucket]=static_cast<std::int32_t>(i+1);}}
 set->items.reserve(size);state.slots=std::move(cells);state.codes=std::move(codes);state.next=std::move(next);state.buckets=std::move(buckets);state.capacity=size;set->items.resize(static_cast<std::size_t>(state.last_index));for(std::size_t i=0;i<set->items.size();++i)set->items[i]=state.codes[i]>=0 ? state.slots[i] : nullptr;work.values.list_storage_remaining-=size;
}
std::size_t bucket(Work& work,const std::shared_ptr<Aggregate>& set,std::size_t code){if(set->hash_set->buckets.empty())throw runtime_error(RuntimeError::nil_reference,work.values.location);return code%set->hash_set->buckets.size();}
bool matches(Work& work,const std::shared_ptr<Aggregate>& set,std::size_t index,const Value& value,std::size_t code){auto cell=physical(work,set,index);return set->hash_set->codes[index]==static_cast<std::int32_t>(code) && equality_policy_value(work.values,element(work,set),set->hash_set->comparer,cell->value,value,work.options);}
std::size_t find(Work& work,const std::shared_ptr<Aggregate>& set,const Value& item,std::size_t code){
 const auto start=bucket(work,set,code);for(auto i=bucket_at(work,set,start)-1;i>=0;i=next_at(work,set,static_cast<std::size_t>(i))){work.step();if(matches(work,set,static_cast<std::size_t>(i),item,code))return static_cast<std::size_t>(i);}return absent;
}
std::size_t find(Work& work,const std::shared_ptr<Aggregate>& set,const Value& item){if(set->hash_set->buckets.empty())return absent;return find(work,set,item,hash(work,set,item));}
std::pair<bool,std::size_t> add(Work& work,const std::shared_ptr<Aggregate>& set,const Value& item){
 auto& state=*set->hash_set;if(state.buckets.empty())reserve(work,set,prime(0));const auto code=hash(work,set,item);auto destination=bucket(work,set,code);const auto found=find(work,set,item,code);if(found!=absent)return {false,found};
 std::int32_t index;std::shared_ptr<Cell> cell;
 if(state.free_head>=0){index=state.free_head;cell=physical(work,set,static_cast<std::size_t>(index));state.free_head=next_at(work,set,static_cast<std::size_t>(index));}
 else{if(static_cast<std::size_t>(state.last_index)==state.slots.size()){reserve(work,set,prime(set->count*2));destination=bucket(work,set,code);}index=state.last_index;cell=physical(work,set,static_cast<std::size_t>(index));++state.last_index;set->items.resize(static_cast<std::size_t>(state.last_index));}
 state.codes[static_cast<std::size_t>(index)]=static_cast<std::int32_t>(code);work.values.write(cell,item,false);state.next[static_cast<std::size_t>(index)]=bucket_at(work,set,destination)-1;bucket_at(work,set,destination)=index+1;
 if(static_cast<std::size_t>(index)<set->items.size())set->items[static_cast<std::size_t>(index)]=cell;++set->count;++set->version;return {true,static_cast<std::size_t>(index)};
}
bool remove(Work& work,const std::shared_ptr<Aggregate>& set,const Value& item){
 auto& state=*set->hash_set;if(state.buckets.empty())return false;const auto code=hash(work,set,item),destination=bucket(work,set,code);std::int32_t last=-1;
 for(auto i=bucket_at(work,set,destination)-1;i>=0;last=i,i=next_at(work,set,static_cast<std::size_t>(i))){work.step();const auto index=static_cast<std::size_t>(i);if(!matches(work,set,index,item,code))continue;
  if(last<0)bucket_at(work,set,destination)=next_at(work,set,index)+1;else{(void)physical(work,set,static_cast<std::size_t>(last));state.next[static_cast<std::size_t>(last)]=next_at(work,set,index);}
  state.codes[index]=-1;work.values.write(physical(work,set,index),work.values.zero(element(work,set),false,true),false);state.next[index]=state.free_head;if(index<set->items.size())set->items[index].reset();--set->count;++set->version;
  if(!set->count){state.last_index=0;state.free_head=-1;set->items.clear();}else state.free_head=i;return true;
 }return false;
}
void clear(Work& work,const std::shared_ptr<Aggregate>& set){auto& state=*set->hash_set;if(state.last_index>0){for(std::size_t i=0;i<static_cast<std::size_t>(state.last_index);++i){work.step();if(state.slots[i])work.values.write(state.slots[i],work.values.zero(element(work,set),false,true),false);state.codes[i]=state.next[i]=0;}std::fill(state.buckets.begin(),state.buckets.end(),0);state.last_index=0;state.free_head=-1;set->items.clear();set->count=0;}++set->version;}
void append_cached(Work& work,const std::shared_ptr<Aggregate>& set,std::size_t code,const Value& value){auto& state=*set->hash_set;const auto i=static_cast<std::size_t>(state.last_index++);auto cell=physical(work,set,i);work.values.write(cell,value,false);state.codes[i]=static_cast<std::int32_t>(code);const auto b=bucket(work,set,code);state.next[i]=state.buckets[b]-1;state.buckets[b]=static_cast<std::int32_t>(i+1);set->items.push_back(cell);++set->count;}
void update(Work&,const std::shared_ptr<Aggregate>&,const std::shared_ptr<Aggregate>&,HashSetMethod);
void trim_storage(Work&,const std::shared_ptr<Aggregate>&);
std::shared_ptr<Aggregate> copy(Work& work,Type owner,const std::shared_ptr<Aggregate>& source,std::shared_ptr<Aggregate> comparer={}){
 auto result=std::get<std::shared_ptr<Aggregate>>(work.values.zero(owner,true));result->hash_set->comparer=std::move(comparer);
 if(!same_policy(work,result,source)){reserve(work,result,prime(source->count));auto sequence=sequence_create(work.values,type_info(work.values.program,owner).key,source,SequenceProvider::hash_set);update(work,result,sequence,HashSetMethod::union_with);if(result->count && result->hash_set->capacity/result->count>3)trim_storage(work,result);return result;}
 if(!source->count)return result;
 const auto capacity=source->hash_set->capacity;const bool clone=prime((source->count+1)*2)>=capacity;
 reserve(work,result,clone ? capacity : prime(source->count));
 if(clone){auto& state=*result->hash_set;const auto& original=*source->hash_set;state.codes=original.codes;state.next=original.next;state.buckets=original.buckets;state.last_index=original.last_index;state.free_head=original.free_head;result->items.resize(static_cast<std::size_t>(state.last_index));for(std::size_t i=0;i<result->items.size();++i){work.step();if(original.slots[i])state.slots[i]=work.values.cell(element(work,source),work.values.copy(original.slots[i]->value));if(state.codes[i]>=0)result->items[i]=state.slots[i];}result->count=source->count;}
 else for(std::size_t i=0;i<source->items.size();++i){work.step();if(source->items[i])append_cached(work,result,static_cast<std::size_t>(source->hash_set->codes[i]),source->items[i]->value);}
 return result;
}
void trim_storage(Work& work,const std::shared_ptr<Aggregate>& set){
 if(!set->count){auto policy=set->hash_set->comparer;HashSetState fresh;fresh.comparer=std::move(policy);std::swap(*set->hash_set,fresh);set->items.clear();++set->version;return;}
 auto compact=std::get<std::shared_ptr<Aggregate>>(work.values.zero(set->type,true));compact->hash_set->comparer=set->hash_set->comparer;reserve(work,compact,prime(set->count));
 for(std::size_t i=0;i<set->items.size();++i){work.step();if(set->items[i])append_cached(work,compact,static_cast<std::size_t>(set->hash_set->codes[i]),set->items[i]->value);}
 set->items.swap(compact->items);std::swap(*set->hash_set,*compact->hash_set);
}
std::shared_ptr<Aggregate> direct_set(const std::shared_ptr<Aggregate>& sequence){return sequence && sequence->sequence && sequence->sequence->provider==SequenceProvider::hash_set ? sequence->sequence->source : nullptr;}
std::optional<std::size_t> collection_count(const std::shared_ptr<Aggregate>& sequence){
    if(!sequence || !sequence->sequence)return {};
    const auto& s=*sequence->sequence;
    if(s.provider==SequenceProvider::hash_set)return s.source->count;
    if(s.provider==SequenceProvider::ordinal_range)return static_cast<std::size_t>(ordinal_range_count(s.source));
    if(collection_sequence(s.provider))return s.source->items.size();
    return {};
}
std::optional<std::int64_t> collection_count(Work& work,const std::shared_ptr<Aggregate>& sequence){
    if(const auto count=collection_count(sequence))return static_cast<std::int64_t>(*count);
    const auto owner=sequence_collection_owner(work.values.program,sequence);
    if(owner==Type::invalid)return {};
    return std::get<std::int32_t>(interface_invoke(work.values,owner,0,sequence->sequence->source,{},work.options));
}
template<class F> bool each(Work& work,const std::shared_ptr<Aggregate>& source,F action){
    const auto cursor=sequence_begin(work.values,source,work.options);
    return sequence_finally(work.values,cursor,work.options,[&]() -> bool {
    for(std::uint64_t i=0;sequence_resume(work.values,cursor,i,work.options);++i){work.step();auto value=sequence_at(work.values,cursor,static_cast<std::int64_t>(i),work.options);if(!action(value))return false;}
    return true;
    });
}
std::pair<std::size_t,std::size_t> unique(Work& work,const std::shared_ptr<Aggregate>& set,const std::shared_ptr<Aggregate>& other,bool stop){
    if(!set->count){bool found=false;each(work,other,[&](const Value&){found=true;return false;});return {0,found ? 1U : 0U};}
    std::vector<bool> marked((set->items.size()+31)/32*32);std::size_t match=0,missing=0;
    each(work,other,[&](const Value& item){const auto at=find(work,set,item);if(at==absent){++missing;return !stop;}if(at>=marked.size() || !marked[at]){++match;if(at<marked.size())marked[at]=true;}return true;});
    return {match,missing};
}
bool relation(Work& work,const std::shared_ptr<Aggregate>& set,const std::shared_ptr<Aggregate>& other,HashSetMethod method){
    if(!other || other->nil)throw runtime_error(RuntimeError::nil_reference,work.values.location);
    auto direct=direct_set(other);if(direct && !same_policy(work,set,direct))direct.reset();
    if(method==HashSetMethod::subset && !set->count)return true;
    if(method==HashSetMethod::proper_superset && !set->count)return false;
    if(method==HashSetMethod::overlaps){if(!set->count)return false;bool found=false;each(work,other,[&](const Value& item){found=find(work,set,item)!=absent;return !found;});return found;}
    if(method==HashSetMethod::proper_subset && !set->count){if(const auto count=collection_count(work,other))return *count>0;}
    if(method==HashSetMethod::superset || method==HashSetMethod::proper_superset){if(const auto count=collection_count(work,other);count&&!*count)return true;}
    if(method==HashSetMethod::set_equals && !direct && !set->count){if(const auto count=collection_count(work,other);count&&*count>0)return false;}
    if(direct){
        if((method==HashSetMethod::set_equals && set->count!=direct->count) || (method==HashSetMethod::subset && set->count>direct->count) || (method==HashSetMethod::proper_subset && set->count>=direct->count) || (method==HashSetMethod::superset && set->count<direct->count) || (method==HashSetMethod::proper_superset && set->count<=direct->count))return false;
        if(method==HashSetMethod::subset || method==HashSetMethod::proper_subset){auto source=sequence_create(work.values,type_info(work.values.program,set->type).key,set,SequenceProvider::hash_set);return each(work,source,[&](const Value& item){return find(work,direct,item)!=absent;});}
    }
    if(method==HashSetMethod::superset || (direct && (method==HashSetMethod::set_equals || method==HashSetMethod::proper_superset))){return each(work,other,[&](const Value& item){return find(work,set,item)!=absent;});}
    const auto [matched,missing]=unique(work,set,other,method==HashSetMethod::set_equals || method==HashSetMethod::proper_superset);
    if(method==HashSetMethod::proper_superset)return matched<set->count && !missing;
    return matched==set->count && (method==HashSetMethod::set_equals ? missing==0 : method==HashSetMethod::proper_subset ? missing>0 : true);
}
void update(Work& work,const std::shared_ptr<Aggregate>& set,const std::shared_ptr<Aggregate>& other,HashSetMethod method){
    if(!other || other->nil)throw runtime_error(RuntimeError::nil_reference,work.values.location);
    const auto direct=direct_set(other);
    if(method==HashSetMethod::union_with){each(work,other,[&](const Value& item){(void)add(work,set,item);return true;});return;}
    if(!set->count){if(method==HashSetMethod::symmetric_with)update(work,set,other,HashSetMethod::union_with);return;}
    if(method==HashSetMethod::except_with){if(direct==set){clear(work,set);return;}each(work,other,[&](const Value& item){(void)remove(work,set,item);return true;});return;}
    if(method==HashSetMethod::intersect_with){
        if(const auto count=collection_count(work,other);count && !*count){clear(work,set);return;}
        if(direct && same_policy(work,set,direct)){for(std::size_t i=0;i<set->items.size();++i){work.step();if(i<set->items.size() && set->items[i]){const auto item=work.values.copy(set->items[i]->value);if(find(work,direct,item)==absent)(void)remove(work,set,item);}}return;}
        const auto size=set->items.size();std::vector<bool> marked((size+31)/32*32);
        each(work,other,[&](const Value& item){const auto at=find(work,set,item);if(at<marked.size())marked[at]=true;return true;});
        for(std::size_t i=0;i<size;++i){work.step();if(i<set->items.size() && set->items[i] && !marked[i]){const auto item=work.values.copy(set->items[i]->value);(void)remove(work,set,item);}}return;
    }
    if(direct==set){clear(work,set);return;}
    if(direct && same_policy(work,set,direct)){each(work,other,[&](const Value& item){if(!remove(work,set,item))(void)add(work,set,item);return true;});return;}
    const auto size=set->items.size();std::vector<bool> marked((size+31)/32*32),added(marked.size());
    each(work,other,[&](const Value& item){const auto [fresh,at]=add(work,set,item);if(fresh){if(at<added.size())added[at]=true;}else if(at<size && !added[at])marked[at]=true;return true;});
    for(std::size_t i=0;i<size;++i){work.step();if(marked[i]){auto item=i<set->items.size() && set->items[i] ? work.values.copy(set->items[i]->value) : work.values.zero(type_info(work.values.program,set->type).element,false,true);(void)remove(work,set,item);}}
}
}
Value hash_set_invoke(Values& values,Type owner,Type sequence,HashSetMethod method,std::span<const Value> arguments,const ExecutionOptions& options){
    if(!hash_set_signature(values.program,owner,sequence,method) || arguments.size()!=hash_set_parameters(values.program.types,owner,sequence,method).size())throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    try{
        Work work{values,options};
        if(method==HashSetMethod::construct)return values.zero(owner,true);
        if(method==HashSetMethod::construct_comparer){auto result=std::get<std::shared_ptr<Aggregate>>(values.zero(owner,true));auto comparer=std::get<std::shared_ptr<Aggregate>>(arguments[0]);if(comparer && !comparer->nil)result->hash_set->comparer=std::move(comparer);return result;}
        if(method==HashSetMethod::construct_copy){const auto source=std::get<std::shared_ptr<Aggregate>>(arguments[0]);require(source,values.location);return copy(work,owner,source);}
        if(method==HashSetMethod::construct_sequence || method==HashSetMethod::construct_sequence_comparer){
            const auto source=std::get<std::shared_ptr<Aggregate>>(arguments[0]);if(!source || source->nil)throw runtime_error(RuntimeError::nil_reference,values.location);
            auto policy=method==HashSetMethod::construct_sequence_comparer ? std::get<std::shared_ptr<Aggregate>>(arguments[1]) : std::shared_ptr<Aggregate>{};if(policy && policy->nil)policy.reset();
            if(const auto direct=direct_set(source))return copy(work,owner,direct,policy);
            auto result=std::get<std::shared_ptr<Aggregate>>(values.zero(owner,true));result->hash_set->comparer=std::move(policy);const auto count=collection_count(work,source).value_or(0);reserve(work,result,prime(count>0?static_cast<std::size_t>(count):0));update(work,result,source,HashSetMethod::union_with);
            if(result->count && result->hash_set->capacity/result->count>3)trim_storage(work,result);return result;
        }
        if(method==HashSetMethod::membership){const auto set=std::get<std::shared_ptr<Aggregate>>(arguments[1]);require(set,values.location);return find(work,set,arguments[0])!=absent;}
        const auto object=hash_set_reference(method) ? std::get<std::shared_ptr<Aggregate>>(std::get<std::shared_ptr<Cell>>(arguments[0])->value) : std::get<std::shared_ptr<Aggregate>>(arguments[0]);
        if(method==HashSetMethod::copy_value){if(!object || object->nil)return object;require(object,values.location);return copy(work,owner,object,object->hash_set->comparer);}
        if(method==HashSetMethod::equal || method==HashSetMethod::not_equal){const auto right=std::get<std::shared_ptr<Aggregate>>(arguments[1]);bool equal=(object && !object->nil)==(right && !right->nil);if(equal && object && !object->nil){require(object,values.location);require(right,values.location);const auto seq=sequence_create(values,sequence==Type::invalid ? type_info(values.program,owner).key : sequence,right,SequenceProvider::hash_set);equal=relation(work,object,seq,HashSetMethod::set_equals);}return method==HashSetMethod::equal ? equal : !equal;}
        require(object,values.location);
        if(method==HashSetMethod::comparer){if(!object->hash_set->comparer){Type factory=Type::invalid;for(std::size_t i=0;i<values.program.types.size();++i)if(values.program.types[i].kind==CompoundKind::equality_comparer && values.program.types[i].element==element(work,object)){factory=static_cast<Type>(64+i);break;}if(factory==Type::invalid)throw runtime_error(RuntimeError::invalid_bytecode,values.location);object->hash_set->comparer=std::get<std::shared_ptr<Aggregate>>(equality_create(values,factory,options));}return object->hash_set->comparer;}
        if(method==HashSetMethod::count)return static_cast<std::int32_t>(object->count);
        if(method>=HashSetMethod::copy_array && method<=HashSetMethod::copy_count){
            const auto array=std::get<std::shared_ptr<Aggregate>>(arguments[1]);
            if(!array || array->nil)throw runtime_error(RuntimeError::nil_reference,values.location);
            const auto index=method>=HashSetMethod::copy_index ? std::get<std::int32_t>(arguments[2]) : 0;
            const auto count=method==HashSetMethod::copy_count ? std::get<std::int32_t>(arguments[3]) : static_cast<std::int32_t>(object->count);
            if(index<0)throw runtime_error(RuntimeError::hash_set_copy_index,values.location);
            if(count<0)throw runtime_error(RuntimeError::hash_set_copy_count,values.location);
            if(static_cast<std::size_t>(index)>array->items.size() || static_cast<std::size_t>(count)>array->items.size()-static_cast<std::size_t>(index))throw runtime_error(RuntimeError::hash_set_copy_range,values.location);
            std::size_t copied=0;
            for(std::size_t i=0;i<object->items.size() && copied<static_cast<std::size_t>(count);++i){work.step();if(const auto& cell=object->items[i]){values.write(array->items[static_cast<std::size_t>(index)+copied],values.copy(cell->value));++copied;}}
            return false;
        }
        if(method==HashSetMethod::try_get_value){
            const auto slot=find(work,object,arguments[1]);
            const auto target=std::get<std::shared_ptr<Cell>>(arguments[2]);
            values.write(target,slot==absent ? values.zero(type_info(values.program,owner).element,false,true) : values.copy(physical(work,object,slot)->value));
            return slot!=absent;
        }
        if(method==HashSetMethod::remove_where){
            const auto predicate=std::get<std::shared_ptr<Aggregate>>(arguments[1]);(void)values.callable_target(predicate);
            std::int32_t removed=0;
            // The source scans live slots, not a version-checked enumerator. Callbacks may resize or compact them.
            for(std::size_t i=0;i<object->items.size();++i){work.step();if(object->items[i]){
                auto item=values.copy(object->items[i]->value);const std::array<Value,1> call{values.copy(item)};
                if(std::get<bool>(values.invoke_value(predicate,call)) && remove(work,object,item))++removed;
            }}
            return removed;
        }
        if(method==HashSetMethod::trim_excess){trim_storage(work,object);return false;}
        if(method==HashSetMethod::contains)return find(work,object,arguments[1])!=absent;
        if(method==HashSetMethod::add || method==HashSetMethod::ref_add){const auto added=add(work,object,arguments[1]).first;return method==HashSetMethod::add ? Value{added} : Value{false};}
        if(method==HashSetMethod::remove || method==HashSetMethod::ref_remove){const auto removed=remove(work,object,arguments[1]);return method==HashSetMethod::remove ? Value{removed} : Value{false};}
        if(method==HashSetMethod::clear){clear(work,object);return false;}
        if(method>=HashSetMethod::union_with && method<=HashSetMethod::overlaps){const auto source=std::get<std::shared_ptr<Aggregate>>(arguments[1]);if(method>=HashSetMethod::set_equals)return relation(work,object,source,method);update(work,object,source,method);return false;}
        if(method==HashSetMethod::ref_union || method==HashSetMethod::ref_except){update(work,object,std::get<std::shared_ptr<Aggregate>>(arguments[1]),method==HashSetMethod::ref_union ? HashSetMethod::union_with : HashSetMethod::except_with);return false;}
        if(method==HashSetMethod::add_copy || method==HashSetMethod::remove_copy){auto result=copy(work,owner,object);if(method==HashSetMethod::add_copy)(void)add(work,result,arguments[1]);else(void)remove(work,result,arguments[1]);return result;}
        const auto right=std::get<std::shared_ptr<Aggregate>>(arguments[1]);require(right,values.location);
        const auto seq=sequence_create(values,type_info(values.program,owner).key,right,SequenceProvider::hash_set);
        if(method>=HashSetMethod::less && method<=HashSetMethod::greater_equal)return relation(work,object,seq,method==HashSetMethod::less ? HashSetMethod::proper_subset : method==HashSetMethod::less_equal ? HashSetMethod::subset : method==HashSetMethod::greater ? HashSetMethod::proper_superset : HashSetMethod::superset);
        auto result=copy(work,owner,object);update(work,result,seq,method==HashSetMethod::unite ? HashSetMethod::union_with : method==HashSetMethod::subtract ? HashSetMethod::except_with : HashSetMethod::intersect_with);return result;
    }catch(const std::bad_alloc&){throw runtime_error(RuntimeError::allocation_limit,values.location);}catch(const std::length_error&){throw runtime_error(RuntimeError::allocation_limit,values.location);}
}
bool hash_set_resume(Values& values,const std::shared_ptr<Aggregate>& cursor,std::uint64_t index,const ExecutionOptions& options){
    auto& state=*cursor->sequence;auto& position=*state.hash_cursor;if(position.positioned && position.index==index)return position.available;
    if(position.positioned && index<position.index)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    std::uint64_t work=0;auto logical=position.positioned ? position.index+1 : 0;
    for(;logical<=index;++logical){
        bool found=false;
        while(position.slot<state.source->items.size()){
            if(work>=options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,values.location);
            if((work++&255U)==0 && options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,values.location);
            const auto& cell=state.source->items[position.slot++];if(cell){position.current=values.copy(cell->value);found=true;break;}
        }
        position.positioned=true;position.index=logical;position.available=found;if(!found)return false;
    }
    return true;
}
}

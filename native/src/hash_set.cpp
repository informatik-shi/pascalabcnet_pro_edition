#include "hash_set.hpp"
#include "runtime_values.hpp"
#include "sequences.hpp"
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
    if(m>HashSetMethod::copy_value || !compound(owner) || static_cast<unsigned>(owner)-64U>=p.types.size() || type_info(p,owner).kind!=CompoundKind::hash_set)return false;
    const auto element=type_info(p,owner).element;
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
Type hash_set_result(Type owner,HashSetMethod m){
    if(m<=HashSetMethod::construct_copy || m==HashSetMethod::copy_value || (m>=HashSetMethod::unite && m<=HashSetMethod::remove_copy))return owner;
    if(m==HashSetMethod::count || m==HashSetMethod::remove_where)return Type::integer;
    if((m>=HashSetMethod::copy_array && m<=HashSetMethod::copy_count) || m==HashSetMethod::trim_excess || m==HashSetMethod::clear || (m>=HashSetMethod::union_with && m<=HashSetMethod::symmetric_with) || hash_set_reference(m))return Type::unit;
    return Type::boolean;
}
namespace {
struct Work {
    Values& values;const ExecutionOptions& options;std::uint64_t done=0;
    void step(){if(done>=options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,values.location);if((done++&255U)==0 && options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,values.location);}
};
void require(const std::shared_ptr<Aggregate>& set,SourceLocation location){if(!set || set->nil)throw runtime_error(RuntimeError::nil_reference,location);if(!set->hash_set)throw runtime_error(RuntimeError::invalid_bytecode,location);}
std::size_t mix(std::size_t seed,std::size_t value){return (seed^value)*static_cast<std::size_t>(1099511628211ULL);}
bool same(Work& work,const Value& a,const Value& b){
    work.step();
    if(const auto* x=std::get_if<double>(&a)){const auto y=std::get<double>(b);return *x==y || (std::isnan(*x) && std::isnan(y));}
    if(const auto* x=std::get_if<std::u16string>(&a)){const auto* y=std::get_if<std::u16string>(&b);if(!y || x->size()!=y->size())return false;for(std::size_t i=0;i<x->size();++i){work.step();if((*x)[i]!=(*y)[i])return false;}return true;}
    if(const auto* x=std::get_if<std::shared_ptr<Aggregate>>(&a)){
        const auto& y=std::get<std::shared_ptr<Aggregate>>(b);const auto kind=type_info(work.values.program,(*x)->type).kind;
        if(kind==CompoundKind::record || kind==CompoundKind::tuple){if((*x)->nil || y->nil)return (*x)->nil==y->nil;for(std::size_t i=0;i<(*x)->items.size();++i)if(!same(work,(*x)->items[i]->value,y->items[i]->value))return false;return true;}
    }
    return work.values.equal(a,b);
}
std::size_t hash(Work& work,const Value& value){
    work.step();
    return std::visit([&](const auto& v)->std::size_t{
        using T=std::decay_t<decltype(v)>;
        if constexpr(std::is_same_v<T,std::u16string>){std::size_t result=1469598103934665603ULL;for(const auto c:v){work.step();result=mix(result,c);}return result;}
        else if constexpr(std::is_same_v<T,NewSetValue>)return hash(work,v.storage->value);
        else if constexpr(std::is_same_v<T,NilString>)return 0;
        else if constexpr(std::is_same_v<T,EnumValue>)return std::hash<std::int32_t>{}(v.value);
        else if constexpr(std::is_same_v<T,BigInteger>){big_work(v.bits()/16+1,work.values.location,work.options);std::size_t result=0;for(const auto c:v.text(16)){work.step();result=mix(result,static_cast<unsigned char>(c));}return result;}
        else if constexpr(std::is_same_v<T,std::shared_ptr<Aggregate>>){
            if(!v || v->nil)return 0;const auto kind=type_info(work.values.program,v->type).kind;
            if(kind==CompoundKind::record || kind==CompoundKind::tuple){std::size_t result=0;for(const auto& field:v->items)result=mix(result,hash(work,field->value));return result;}
            if(kind==CompoundKind::callable){std::size_t result=v->routine.value_or(0);for(const auto& cell:v->captures)result=mix(result,std::hash<const void*>{}(cell.get()));return result;}
            if(kind==CompoundKind::sequence && v->sequence && (collection_sequence(v->sequence->provider) || v->sequence->provider==SequenceProvider::hash_set))return std::hash<const void*>{}(v->sequence->source.get());
            return std::hash<const void*>{}(v.get());
        }else if constexpr(std::is_same_v<T,std::shared_ptr<Cell>> || std::is_same_v<T,std::shared_ptr<TextFile>>)return std::hash<const void*>{}(v.get());
        else if constexpr(std::is_same_v<T,double>)return std::isnan(v) ? 0x7ff80000U : std::hash<double>{}(v);
        else return std::hash<T>{}(v);
    },value);
}
std::size_t prime(std::size_t minimum){
    static constexpr std::size_t table[]={3,7,11,17,23,29,37,47,59,71,89,107,131,163,197,239,293,353,431,521,631,761,919,1103,1327,1597,1931,2333,2801,3371,4049,4861,5839,7013,8419,10103,12143,14591,17519,21023,25229,30293,36353,43627,52361,62851,75431,90523,108631,130363,156437,187751,225307,270371,324449,389357,467237,560689,672827,807403,968897,1162687,1395263,1674319,2009191,2411033,2893249,3471899,4166287,4999559,5999471,7199369};
    for(const auto n:table)if(n>=minimum)return n;
    throw std::length_error("HashSet storage limit");
}
void reserve(const std::shared_ptr<Aggregate>& set,std::size_t size){set->items.reserve(size);set->hash_set->hashes.reserve(size);set->hash_set->free.reserve(size);set->hash_set->capacity=size;}
constexpr auto absent=std::numeric_limits<std::size_t>::max();
std::size_t find(Work& work,const std::shared_ptr<Aggregate>& set,const Value& item,std::size_t code){
    const auto range=set->hash_set->lookup.equal_range(code);
    for(auto it=range.first;it!=range.second;++it){work.step();if(same(work,set->items[it->second]->value,item))return it->second;}
    return absent;
}
std::size_t find(Work& work,const std::shared_ptr<Aggregate>& set,const Value& item){return find(work,set,item,hash(work,item));}
std::pair<bool,std::size_t> add(Work& work,const std::shared_ptr<Aggregate>& set,const Value& item){
    const auto code=hash(work,item),found=find(work,set,item,code);if(found!=absent)return {false,found};
    auto& state=*set->hash_set;const bool reuse=!state.free.empty();const auto slot=reuse ? state.free.back() : set->items.size();
    if(!reuse && slot>=state.capacity)reserve(set,prime(state.capacity ? state.capacity*2 : 0));
    auto cell=work.values.cell(type_info(work.values.program,set->type).element,work.values.copy(item));
    state.lookup.emplace(code,slot);
    if(reuse){state.free.pop_back();set->items[slot]=std::move(cell);state.hashes[slot]=code;}
    else{set->items.push_back(std::move(cell));state.hashes.push_back(code);}
    ++set->count;++set->version;return {true,slot};
}
bool remove(Work& work,const std::shared_ptr<Aggregate>& set,const Value& item){
    const auto code=hash(work,item),slot=find(work,set,item,code);if(slot==absent)return false;
    auto& state=*set->hash_set;const auto range=state.lookup.equal_range(code);for(auto it=range.first;it!=range.second;++it)if(it->second==slot){state.lookup.erase(it);break;}
    set->items[slot].reset();--set->count;++set->version;
    if(!set->count){set->items.clear();state.hashes.clear();state.free.clear();}else state.free.push_back(slot);
    return true;
}
void clear(const std::shared_ptr<Aggregate>& set){set->items.clear();set->hash_set->hashes.clear();set->hash_set->free.clear();set->hash_set->lookup.clear();set->count=0;++set->version;}
std::shared_ptr<Aggregate> copy(Work& work,Type owner,const std::shared_ptr<Aggregate>& source){
    auto result=std::get<std::shared_ptr<Aggregate>>(work.values.zero(owner,true));if(!source->count)return result;
    const auto capacity=source->hash_set->capacity;const bool clone=prime((source->count+1)*2)>=capacity;
    reserve(result,clone ? capacity : prime(source->count));
    if(clone){result->hash_set->free=source->hash_set->free;result->hash_set->hashes=source->hash_set->hashes;
        for(std::size_t i=0;i<source->items.size();++i){work.step();const auto& cell=source->items[i];if(cell){result->items.push_back(work.values.cell(cell->type,work.values.copy(cell->value)));result->hash_set->lookup.emplace(source->hash_set->hashes[i],i);}else result->items.push_back({});}
        result->count=source->count;
    }else{for(const auto& cell:source->items){work.step();if(cell)(void)add(work,result,cell->value);}result->version=0;}
    return result;
}
std::shared_ptr<Aggregate> direct_set(const std::shared_ptr<Aggregate>& sequence){return sequence && sequence->sequence && sequence->sequence->provider==SequenceProvider::hash_set ? sequence->sequence->source : nullptr;}
std::optional<std::size_t> collection_count(const std::shared_ptr<Aggregate>& sequence){
    if(!sequence || !sequence->sequence)return {};
    const auto& s=*sequence->sequence;
    if(s.provider==SequenceProvider::hash_set)return s.source->count;
    if(collection_sequence(s.provider))return s.source->items.size();
    return {};
}
template<class F> bool each(Work& work,const std::shared_ptr<Aggregate>& source,F action){
    const auto cursor=sequence_begin(work.values,source);
    for(std::uint64_t i=0;sequence_resume(work.values,cursor,i,work.options);++i){work.step();auto value=sequence_at(work.values,cursor,static_cast<std::int64_t>(i),work.options);if(!action(value))return false;}
    return true;
}
std::pair<std::size_t,std::size_t> unique(Work& work,const std::shared_ptr<Aggregate>& set,const std::shared_ptr<Aggregate>& other,bool stop){
    if(!set->count){bool found=false;each(work,other,[&](const Value&){found=true;return false;});return {0,found ? 1U : 0U};}
    std::vector<bool> marked((set->items.size()+31)/32*32);std::size_t match=0,missing=0;
    each(work,other,[&](const Value& item){const auto at=find(work,set,item);if(at==absent){++missing;return !stop;}if(at>=marked.size() || !marked[at]){++match;if(at<marked.size())marked[at]=true;}return true;});
    return {match,missing};
}
bool relation(Work& work,const std::shared_ptr<Aggregate>& set,const std::shared_ptr<Aggregate>& other,HashSetMethod method){
    if(!other || other->nil)throw runtime_error(RuntimeError::nil_reference,work.values.location);
    const auto direct=direct_set(other);const auto count=collection_count(other);
    if(method==HashSetMethod::subset && !set->count)return true;
    if(method==HashSetMethod::proper_superset && !set->count)return false;
    if(method==HashSetMethod::overlaps){if(!set->count)return false;bool found=false;each(work,other,[&](const Value& item){found=find(work,set,item)!=absent;return !found;});return found;}
    if(method==HashSetMethod::proper_subset && !set->count && count)return *count>0;
    if((method==HashSetMethod::superset || method==HashSetMethod::proper_superset) && count && !*count)return method==HashSetMethod::superset || set->count>0;
    if(method==HashSetMethod::set_equals && !set->count && count && *count>0)return false;
    if(direct){
        if((method==HashSetMethod::set_equals && set->count!=direct->count) || (method==HashSetMethod::subset && set->count>direct->count) || (method==HashSetMethod::proper_subset && set->count>=direct->count) || (method==HashSetMethod::superset && set->count<direct->count) || (method==HashSetMethod::proper_superset && set->count<=direct->count))return false;
        if(method==HashSetMethod::subset || method==HashSetMethod::proper_subset){for(const auto& cell:set->items){work.step();if(cell && find(work,direct,cell->value)==absent)return false;}return true;}
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
    if(method==HashSetMethod::except_with){if(direct==set){clear(set);return;}each(work,other,[&](const Value& item){(void)remove(work,set,item);return true;});return;}
    if(method==HashSetMethod::intersect_with){
        if(const auto count=collection_count(other);count && !*count){clear(set);return;}
        if(direct){const auto size=set->items.size();for(std::size_t i=0;i<size;++i){work.step();if(i<set->items.size() && set->items[i]){const auto item=work.values.copy(set->items[i]->value);if(find(work,direct,item)==absent)(void)remove(work,set,item);}}return;}
        const auto size=set->items.size();std::vector<bool> marked((size+31)/32*32);
        each(work,other,[&](const Value& item){const auto at=find(work,set,item);if(at<marked.size())marked[at]=true;return true;});
        for(std::size_t i=0;i<size;++i){work.step();if(i<set->items.size() && set->items[i] && !marked[i]){const auto item=work.values.copy(set->items[i]->value);(void)remove(work,set,item);}}return;
    }
    if(direct==set){clear(set);return;}
    if(direct){each(work,other,[&](const Value& item){if(!remove(work,set,item))(void)add(work,set,item);return true;});return;}
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
        if(method==HashSetMethod::construct_copy){const auto source=std::get<std::shared_ptr<Aggregate>>(arguments[0]);require(source,values.location);return copy(work,owner,source);}
        if(method==HashSetMethod::construct_sequence){const auto source=std::get<std::shared_ptr<Aggregate>>(arguments[0]);if(!source || source->nil)throw runtime_error(RuntimeError::nil_reference,values.location);if(const auto direct=direct_set(source))return copy(work,owner,direct);auto result=std::get<std::shared_ptr<Aggregate>>(values.zero(owner,true));if(const auto count=collection_count(source);count && *count)reserve(result,prime(*count));update(work,result,source,HashSetMethod::union_with);if(result->count && result->hash_set->capacity/result->count>3){std::vector<std::shared_ptr<Cell>> compact;for(const auto& cell:result->items)if(cell)compact.push_back(cell);result->items=std::move(compact);result->hash_set->hashes.clear();result->hash_set->lookup.clear();result->hash_set->free.clear();reserve(result,prime(result->count));for(std::size_t i=0;i<result->items.size();++i){const auto code=hash(work,result->items[i]->value);result->hash_set->hashes.push_back(code);result->hash_set->lookup.emplace(code,i);}}return result;}
        if(method==HashSetMethod::membership){const auto set=std::get<std::shared_ptr<Aggregate>>(arguments[1]);require(set,values.location);return find(work,set,arguments[0])!=absent;}
        const auto object=hash_set_reference(method) ? std::get<std::shared_ptr<Aggregate>>(std::get<std::shared_ptr<Cell>>(arguments[0])->value) : std::get<std::shared_ptr<Aggregate>>(arguments[0]);
        if(method==HashSetMethod::copy_value){if(!object || object->nil)return object;require(object,values.location);return copy(work,owner,object);}
        if(method==HashSetMethod::equal || method==HashSetMethod::not_equal){const auto right=std::get<std::shared_ptr<Aggregate>>(arguments[1]);bool equal=(object && !object->nil)==(right && !right->nil);if(equal && object && !object->nil){require(object,values.location);require(right,values.location);const auto seq=sequence_create(values,sequence==Type::invalid ? type_info(values.program,owner).key : sequence,right,SequenceProvider::hash_set);equal=relation(work,object,seq,HashSetMethod::set_equals);}return method==HashSetMethod::equal ? equal : !equal;}
        require(object,values.location);
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
            values.write(target,slot==absent ? values.zero(type_info(values.program,owner).element,false,true) : values.copy(object->items[slot]->value));
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
        if(method==HashSetMethod::trim_excess){
            if(!object->count){
                std::vector<std::shared_ptr<Cell>>{}.swap(object->items);
                HashSetState fresh;std::swap(*object->hash_set,fresh);++object->version;return false;
            }
            auto compact=std::get<std::shared_ptr<Aggregate>>(values.zero(owner,true));reserve(compact,prime(object->count));
            // Cached hashes and physical order survive compaction; the nonempty source does not change version.
            for(std::size_t i=0;i<object->items.size();++i){work.step();if(const auto& cell=object->items[i]){
                const auto at=compact->items.size(),code=object->hash_set->hashes[i];
                compact->items.push_back(values.cell(cell->type,values.copy(cell->value)));
                compact->hash_set->hashes.push_back(code);compact->hash_set->lookup.emplace(code,at);
            }}
            object->items.swap(compact->items);std::swap(*object->hash_set,*compact->hash_set);return false;
        }
        if(method==HashSetMethod::contains)return find(work,object,arguments[1])!=absent;
        if(method==HashSetMethod::add || method==HashSetMethod::ref_add){const auto added=add(work,object,arguments[1]).first;return method==HashSetMethod::add ? Value{added} : Value{false};}
        if(method==HashSetMethod::remove || method==HashSetMethod::ref_remove){const auto removed=remove(work,object,arguments[1]);return method==HashSetMethod::remove ? Value{removed} : Value{false};}
        if(method==HashSetMethod::clear){clear(object);return false;}
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

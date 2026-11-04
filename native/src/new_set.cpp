#include "new_set.hpp"
#include "hash_set.hpp"
#include "runtime_values.hpp"
#include "sequences.hpp"
#include "formatting.hpp"

namespace abc {
namespace {
const TypeInfo& shape(const std::vector<TypeInfo>& types,Type owner){return types.at(static_cast<unsigned>(owner)-64U);}
Type array_type(const std::vector<TypeInfo>& types,Type element){for(std::size_t i=0;i<types.size();++i)if(types[i].kind==CompoundKind::array && types[i].rank==1 && types[i].element==element)return static_cast<Type>(64+i);return Type::invalid;}
bool instance(NewSetMethod m){return m>=NewSetMethod::clone && m<=NewSetMethod::to_hash;}
}
bool new_set_signature(const Program& p,Type owner,NewSetMethod m,bool reference){
    if(m>NewSetMethod::membership || !compound(owner) || static_cast<unsigned>(owner)-64U>=p.types.size() || type_info(p,owner).kind!=CompoundKind::new_set)return false;
    if((m==NewSetMethod::construct_array || m==NewSetMethod::cast_array) && array_type(p.types,type_info(p,owner).element)==Type::invalid)return false;
    if(m==NewSetMethod::assign || m==NewSetMethod::raw_reference)return reference;
    return !reference || instance(m);
}
std::vector<Type> new_set_parameters(const std::vector<TypeInfo>& types,Type owner,NewSetMethod m,bool reference){
    const auto& type=shape(types,owner);const auto hash=type.key;const auto sequence=shape(types,hash).key;
    if(m==NewSetMethod::construct)return {};
    if(m==NewSetMethod::construct_array || m==NewSetMethod::cast_array)return {array_type(types,type.element)};
    if(m==NewSetMethod::cast_hash)return {hash};
    if(m==NewSetMethod::membership)return {type.element,owner};
    std::vector<Type> result{reference ? reference_type(owner) : owner};
    if(m==NewSetMethod::add || m==NewSetMethod::remove || m==NewSetMethod::contains || m==NewSetMethod::item_add || m==NewSetMethod::item_remove)result.push_back(type.element);
    else if(m==NewSetMethod::add_range)result.push_back(sequence);
    else if(m>=NewSetMethod::assign && m<=NewSetMethod::greater_equal)result.push_back(owner);
    return result;
}
Type new_set_result(const std::vector<TypeInfo>& types,Type owner,NewSetMethod m){
    if(m==NewSetMethod::count)return Type::integer;
    if(m==NewSetMethod::add || m==NewSetMethod::remove || m==NewSetMethod::contains || m>=NewSetMethod::equal)return Type::boolean;
    if(m==NewSetMethod::to_string)return Type::string;
    if(m==NewSetMethod::raw_reference)return reference_type(shape(types,owner).key);
    if(m==NewSetMethod::hash || m==NewSetMethod::raw_hash || m==NewSetMethod::to_hash)return shape(types,owner).key;
    if(m==NewSetMethod::add_range || (m>=NewSetMethod::assign && m<=NewSetMethod::intersect_assign))return Type::unit;
    return owner;
}
std::shared_ptr<Aggregate> new_set_hash(Values& values,const NewSetValue& value){
    if(!value.storage || value.storage->type!=type_info(values.program,value.type).key)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    auto object=std::get<std::shared_ptr<Aggregate>>(value.storage->value);
    if(!object || object->nil){object=std::get<std::shared_ptr<Aggregate>>(values.zero(value.storage->type,true));values.write(value.storage,object);}
    return object;
}
std::shared_ptr<Aggregate> new_set_sequence(Values& values,Type sequence,NewSetValue value){
    if(!values.remaining)throw runtime_error(RuntimeError::allocation_limit,values.location);--values.remaining;
    auto result=std::make_shared<Aggregate>();result->type=sequence;result->sequence=std::make_unique<SequenceState>();result->sequence->provider=SequenceProvider::new_set;result->sequence->seed=values.cell(value.type,std::move(value));return result;
}
Value new_set_invoke(Values& values,Type owner,NewSetMethod m,bool reference,std::span<const Value> arguments,const ExecutionOptions& options){
    if(!new_set_signature(values.program,owner,m,reference) || arguments.size()!=new_set_parameters(values.program.types,owner,m,reference).size())throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    try{
        const auto hash=type_info(values.program,owner).key,sequence=type_info(values.program,hash).key;
        const auto call=[&](HashSetMethod method,std::span<const Value> args){return hash_set_invoke(values,hash,hash_set_sequence_method(method) ? sequence : Type::invalid,method,args,options);};
        const auto wrap=[&](Value value)->Value{return NewSetValue{owner,values.cell(hash,std::move(value))};};
        const auto seq_hash=[&](const std::shared_ptr<Aggregate>& h){return sequence_create(values,sequence,h,SequenceProvider::hash_set);};
        const auto update=[&](const std::shared_ptr<Aggregate>& h,const Value& source,HashSetMethod method){const std::array<Value,2> args{h,source};(void)call(method,args);};
        if(m==NewSetMethod::construct)return values.zero(owner);
        if(m==NewSetMethod::construct_array || m==NewSetMethod::cast_array){
            const auto source=std::get<std::shared_ptr<Aggregate>>(arguments[0]);const auto seq=sequence_create(values,sequence,source,SequenceProvider::array);
            if(m==NewSetMethod::cast_array){const std::array<Value,1> args{seq};return wrap(call(HashSetMethod::construct_sequence,args));}
            auto result=std::get<NewSetValue>(values.zero(owner));update(new_set_hash(values,result),seq,HashSetMethod::union_with);return result;
        }
        if(m==NewSetMethod::cast_hash){const auto source=std::get<std::shared_ptr<Aggregate>>(arguments[0]);auto result=std::get<NewSetValue>(values.zero(owner));update(new_set_hash(values,result),seq_hash(source),HashSetMethod::union_with);return result;}
        if(m==NewSetMethod::membership){const auto& set=std::get<NewSetValue>(arguments[1]);const std::array<Value,2> args{new_set_hash(values,set),arguments[0]};return call(HashSetMethod::contains,args);}
        const auto& self=reference ? std::get<NewSetValue>(std::get<std::shared_ptr<Cell>>(arguments[0])->value) : std::get<NewSetValue>(arguments[0]);
        if(m==NewSetMethod::raw_reference)return self.storage;
        if(m==NewSetMethod::raw_hash)return self.storage->value;
        if(m==NewSetMethod::assign){const auto& source=std::get<NewSetValue>(arguments[1]);const std::array<Value,1> args{new_set_hash(values,source)};auto result=call(HashSetMethod::construct_copy,args);values.write(self.storage,result);return false;}
        const auto object=new_set_hash(values,self);
        if(m==NewSetMethod::copy_value){const std::array<Value,1> args{object};return wrap(call(HashSetMethod::construct_copy,args));}
        if(m==NewSetMethod::hash)return object;
        if(m==NewSetMethod::clone || m==NewSetMethod::to_hash){
            if(m==NewSetMethod::to_hash){auto boxed=std::get<NewSetValue>(values.copy(Value{self}));const std::array<Value,1> args{new_set_sequence(values,sequence,std::move(boxed))};return call(HashSetMethod::construct_sequence,args);}
            auto result=std::get<NewSetValue>(values.zero(owner));update(new_set_hash(values,result),seq_hash(object),HashSetMethod::union_with);return result;
        }
        if(m==NewSetMethod::to_string)return object_text(values.program,Value{object},hash,values.location,options);
        if(m==NewSetMethod::count){const std::array<Value,1> args{object};return call(HashSetMethod::count,args);}
        if(m==NewSetMethod::add || m==NewSetMethod::remove || m==NewSetMethod::contains || m==NewSetMethod::item_add || m==NewSetMethod::item_remove){
            const std::array<Value,2> args{object,arguments[1]};const auto result=call(m==NewSetMethod::add || m==NewSetMethod::item_add ? HashSetMethod::add : m==NewSetMethod::remove || m==NewSetMethod::item_remove ? HashSetMethod::remove : HashSetMethod::contains,args);return m==NewSetMethod::item_add || m==NewSetMethod::item_remove ? Value{false} : result;
        }
        if(m==NewSetMethod::add_range){update(object,arguments[1],HashSetMethod::union_with);return false;}
        const auto& other=std::get<NewSetValue>(arguments[1]);auto boxed=std::get<NewSetValue>(values.copy(Value{other}));const auto seq=(m==NewSetMethod::unite || m==NewSetMethod::subtract || m==NewSetMethod::intersect) ? new_set_sequence(values,sequence,std::move(boxed)) : seq_hash(new_set_hash(values,other));
        if(m==NewSetMethod::union_assign || m==NewSetMethod::except_assign || m==NewSetMethod::intersect_assign){update(object,seq,m==NewSetMethod::union_assign ? HashSetMethod::union_with : m==NewSetMethod::except_assign ? HashSetMethod::except_with : HashSetMethod::intersect_with);return false;}
        if(m>=NewSetMethod::equal){
            const auto method=m==NewSetMethod::equal || m==NewSetMethod::not_equal ? HashSetMethod::set_equals : m==NewSetMethod::less ? HashSetMethod::proper_subset : m==NewSetMethod::less_equal ? HashSetMethod::subset : m==NewSetMethod::greater ? HashSetMethod::proper_superset : HashSetMethod::superset;
            const std::array<Value,2> args{object,seq};const auto answer=call(method,args);return m==NewSetMethod::not_equal ? Value{!std::get<bool>(answer)} : answer;
        }
        auto result=std::get<NewSetValue>(values.zero(owner));auto first=std::get<NewSetValue>(values.copy(Value{self}));update(new_set_hash(values,result),new_set_sequence(values,sequence,std::move(first)),HashSetMethod::union_with);
        update(new_set_hash(values,result),seq,m==NewSetMethod::unite ? HashSetMethod::union_with : m==NewSetMethod::subtract ? HashSetMethod::except_with : HashSetMethod::intersect_with);return result;
    }catch(const std::bad_alloc&){throw runtime_error(RuntimeError::allocation_limit,values.location);}catch(const std::length_error&){throw runtime_error(RuntimeError::allocation_limit,values.location);}
}
}

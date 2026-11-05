#include "new_set.hpp"
#include "hash_set.hpp"
#include "runtime_values.hpp"
#include "sequences.hpp"
#include "formatting.hpp"

namespace abc {
namespace {
const TypeInfo& shape(const std::vector<TypeInfo>& types,Type owner){return types.at(static_cast<unsigned>(owner)-64U);}
Type array_type(const std::vector<TypeInfo>& types,Type element){for(std::size_t i=0;i<types.size();++i)if(types[i].kind==CompoundKind::array && types[i].rank==1 && types[i].element==element)return static_cast<Type>(64+i);return Type::invalid;}
bool empty_operation(NewSetMethod m){return m>=NewSetMethod::empty_unite_left;}
bool empty_left(NewSetMethod m){return (static_cast<unsigned>(m)-static_cast<unsigned>(NewSetMethod::empty_unite_left))%2==0;}
bool range_element(const std::vector<TypeInfo>& types,Type element){return element==Type::integer || element==Type::character || element==Type::boolean || (compound(element) && shape(types,element).kind==CompoundKind::enumeration);}
bool instance(NewSetMethod m){return m>=NewSetMethod::clone && m<=NewSetMethod::to_hash;}
}
Type empty_collection_type(const std::vector<TypeInfo>& types){for(std::size_t i=0;i<types.size();++i)if(types[i].kind==CompoundKind::empty_collection)return static_cast<Type>(64+i);return Type::invalid;}
bool empty_signature(const Program& p,Type target,EmptyMethod method){
    if(method>EmptyMethod::to_set || empty_collection_type(p.types)==Type::invalid || !valid_value_type(p,target))return false;
    if(method==EmptyMethod::to_string)return target==Type::string;
    if(!compound(target))return false;const auto& type=type_info(p,target);
    if(method==EmptyMethod::singleton || method==EmptyMethod::binary)return type.kind==CompoundKind::empty_collection;
    if(method==EmptyMethod::to_set)return type.kind==CompoundKind::new_set;
    return type.kind==CompoundKind::new_set || type.kind==CompoundKind::hash_set || type.kind==CompoundKind::list || (type.kind==CompoundKind::array && type.rank==1) || (type.kind==CompoundKind::sequence && array_type(p.types,type.element)!=Type::invalid);
}
std::vector<Type> empty_parameters(const std::vector<TypeInfo>& types,EmptyMethod method){const auto empty=empty_collection_type(types);if(method==EmptyMethod::singleton)return {};if(method==EmptyMethod::binary)return {empty,empty};return {empty};}
Value empty_invoke(Values& values,Type target,EmptyMethod method,std::span<const Value> arguments,const ExecutionOptions& options){
    if(!empty_signature(values.program,target,method) || arguments.size()!=empty_parameters(values.program.types,method).size())throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    if(options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,values.location);
    if(method==EmptyMethod::singleton){if(!values.empty_collection)values.empty_collection=std::get<std::shared_ptr<Aggregate>>(values.zero(target,true));return values.empty_collection;}
    if(method==EmptyMethod::to_string || method==EmptyMethod::to_set){const auto object=std::get<std::shared_ptr<Aggregate>>(arguments[0]);if(!object || object->nil)throw runtime_error(RuntimeError::nil_reference,values.location);if(method==EmptyMethod::to_string)return std::u16string{u"{}"};}
    const auto& type=type_info(values.program,target);
    if(type.kind==CompoundKind::array)return values.array(target,0,true);
    if(type.kind==CompoundKind::sequence){const auto array=array_type(values.program.types,type.element);return sequence_create(values,target,values.array(array,0,true),SequenceProvider::array);}
    return values.zero(target,true);
}
bool new_set_signature(const Program& p,Type owner,NewSetMethod m,bool reference){
    if(m>NewSetMethod::empty_array_intersect_right || !compound(owner) || static_cast<unsigned>(owner)-64U>=p.types.size() || type_info(p,owner).kind!=CompoundKind::new_set)return false;
    if((m==NewSetMethod::construct_array || m==NewSetMethod::cast_array) && array_type(p.types,type_info(p,owner).element)==Type::invalid)return false;
    if(m==NewSetMethod::literal_ranges && (!range_element(p.types,type_info(p,owner).element) || array_type(p.types,type_info(p,owner).element)==Type::invalid))return false;
    if(empty_operation(m) && (empty_collection_type(p.types)==Type::invalid || (m>=NewSetMethod::empty_array_unite_left && array_type(p.types,type_info(p,owner).element)==Type::invalid)))return false;
    if(m==NewSetMethod::assign || m==NewSetMethod::raw_reference)return reference;
    return !reference || instance(m);
}
std::vector<Type> new_set_parameters(const std::vector<TypeInfo>& types,Type owner,NewSetMethod m,bool reference){
    const auto& type=shape(types,owner);const auto hash=type.key;const auto sequence=shape(types,hash).key;
    if(empty_operation(m)){const auto empty=empty_collection_type(types),other=m>=NewSetMethod::empty_array_unite_left ? array_type(types,type.element) : owner;return empty_left(m) ? std::vector<Type>{empty,other} : std::vector<Type>{other,empty};}
    if(m==NewSetMethod::literal_ranges)return {array_type(types,type.element),array_type(types,type.element)};
    if(m==NewSetMethod::from_sequence)return {sequence};
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
    if(m==NewSetMethod::add || m==NewSetMethod::remove || m==NewSetMethod::contains || (m>=NewSetMethod::equal && m<=NewSetMethod::membership) || (m>=NewSetMethod::empty_equal_left && m<=NewSetMethod::empty_greater_equal_right))return Type::boolean;
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
        if(m==NewSetMethod::from_sequence){const std::array<Value,1> args{arguments[0]};return wrap(call(HashSetMethod::construct_sequence,args));}
        if(m==NewSetMethod::literal_ranges){
            auto result=std::get<NewSetValue>(values.zero(owner));const auto object=new_set_hash(values,result),scalars=std::get<std::shared_ptr<Aggregate>>(arguments[0]),ranges=std::get<std::shared_ptr<Aggregate>>(arguments[1]);
            update(object,sequence_create(values,sequence,scalars,SequenceProvider::array),HashSetMethod::union_with);
            if(!ranges || ranges->nil)throw runtime_error(RuntimeError::nil_reference,values.location);if(ranges->items.size()%2)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
            std::uint64_t work=0;const auto step=[&]{if(++work>options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,values.location);if((work==1 || !(work&255U)) && options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,values.location);};
            const auto add=[&](Value item){step();const std::array<Value,2> args{object,std::move(item)};(void)call(HashSetMethod::add,args);};
            const auto element=type_info(values.program,owner).element;std::vector<std::int32_t> enums;
            if(compound(element)){for(const auto& member:type_info(values.program,element).enum_members){step();enums.push_back(member.value);}std::stable_sort(enums.begin(),enums.end(),[&](auto a,auto b){step();return static_cast<std::uint32_t>(a)<static_cast<std::uint32_t>(b);});}
            for(std::size_t i=0;i<ranges->items.size();i+=2){step();const auto& a=ranges->items[i]->value;const auto& b=ranges->items[i+1]->value;
                if(compound(element)){const auto first=std::find(enums.begin(),enums.end(),std::get<EnumValue>(a).value),last=std::find(enums.begin(),enums.end(),std::get<EnumValue>(b).value);const auto from=first==enums.end() ? -1 : std::distance(enums.begin(),first),to=last==enums.end() ? -1 : std::distance(enums.begin(),last);for(auto n=from;n<=to;++n){if(n<0)throw runtime_error(RuntimeError::index_bounds,values.location);add(EnumValue{element,enums[static_cast<std::size_t>(n)]});}}
                else if(element==Type::integer){for(std::int64_t n=std::get<std::int32_t>(a),last=std::get<std::int32_t>(b);n<=last;++n)add(static_cast<std::int32_t>(n));}
                else if(element==Type::character){for(std::uint32_t n=std::get<char16_t>(a),last=std::get<char16_t>(b);n<=last;++n)add(static_cast<char16_t>(n));}
                else{for(int n=std::get<bool>(a),last=std::get<bool>(b);n<=last;++n)add(n!=0);}
            }return result;
        }
        if(empty_operation(m)){
            const bool left=empty_left(m);const auto& other=arguments[left ? 1 : 0];
            if(m>=NewSetMethod::empty_array_unite_left){const auto group=(static_cast<unsigned>(m)-static_cast<unsigned>(NewSetMethod::empty_array_unite_left))/2;auto result=std::get<NewSetValue>(values.zero(owner));if(group==0 || (group==1 && !left))update(new_set_hash(values,result),sequence_create(values,sequence,std::get<std::shared_ptr<Aggregate>>(other),SequenceProvider::array),HashSetMethod::union_with);return result;}
            const auto group=(static_cast<unsigned>(m)-static_cast<unsigned>(NewSetMethod::empty_unite_left))/2;
            if(group<=2){if(group==0 || (group==1 && !left)){const std::array<Value,1> args{new_set_hash(values,std::get<NewSetValue>(other))};return wrap(call(HashSetMethod::construct_copy,args));}return values.zero(owner);}
            const bool needs_count=group==3 || group==4 || (group==5 && left) || (group==6 && left) || (group==7 && !left) || (group==8 && !left);
            const auto count=needs_count ? new_set_hash(values,std::get<NewSetValue>(other))->count : 0;
            if(group==3)return count==0;if(group==4)return count!=0;if(group==5)return left && count>0;if(group==6)return left;if(group==7)return !left && count>0;return !left;
        }
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

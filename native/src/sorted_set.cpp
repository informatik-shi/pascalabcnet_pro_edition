#include "sorted_set.hpp"
#include "comparison.hpp"
#include "collection_interface.hpp"
#include "dictionary.hpp"
#include "interfaces.hpp"
#include "runtime_values.hpp"
#include "sequence_scope.hpp"
#include "sequences.hpp"
#include "sorting.hpp"
#include <array>

namespace abc {
namespace {
using Object=std::shared_ptr<Aggregate>;
using Method=SortedSetMethod;
bool uses_source(Method method){
    return method==Method::construct_source || method==Method::construct_source_comparer;
}
bool uses_comparer(Method method){
    return method==Method::construct_comparer || method==Method::construct_source_comparer;
}
bool copies_items(Method method){
    return method>=Method::copy_array && method<=Method::copy_count;
}
bool relation_method(Method method){
    return method>=Method::set_equal && method<=Method::overlaps;
}

class OrderedSet {
    Values& values;
    const ExecutionOptions& options;
    Type policy;
    std::uint64_t work=0;

public:
    OrderedSet(Values& values,const ExecutionOptions& options,Type policy)
        : values(values),options(options),policy(policy){}

    void step(std::size_t amount=1){
        if(amount>options.instruction_limit-work)
            throw runtime_error(RuntimeError::instruction_limit,values.location);
        work+=amount;
        if(options.cancelled && options.cancelled())
            throw runtime_error(RuntimeError::cancelled,values.location);
    }

    int compare(const Object& set,const Value& left,const Value& right){
        step();
        const auto version=set->version;
        const std::array<Value,2> arguments{left,right};
        const auto result=std::get<std::int32_t>(comparer_invoke(values,policy,set->ordering,arguments,options));
        if(set->version!=version)throw runtime_error(RuntimeError::collection_changed,values.location);
        return result;
    }

    std::pair<std::size_t,bool> locate(const Object& set,const Value& item){
        std::size_t first=0,last=set->items.size();
        while(first<last){
            const auto middle=first+(last-first)/2;
            const auto order=compare(set,item,set->items[middle]->value);
            if(order==0)return {middle,true};
            if(order<0)last=middle;
            else first=middle+1;
        }
        return {first,false};
    }
};

Object as_sorted(const Object& input){
    return input && input->sequence && input->sequence->provider==SequenceProvider::sorted_set
        ? input->sequence->source : Object{};
}

std::optional<std::int32_t> collection_count(Values& values,const Object& input,bool generic,const ExecutionOptions& options){
    const auto& state=*input->sequence;
    if(collection_sequence(state.provider))return static_cast<std::int32_t>(state.source->items.size());
    if(dictionary_provider(state.provider))
        return static_cast<std::int32_t>(dictionary_sequence_source(state,values.location)->count);
    if(!generic)return std::nullopt;
    if(state.provider==SequenceProvider::hash_set)return static_cast<std::int32_t>(state.source->count);
    if(const auto contract=sequence_collection_owner(values.program,input);contract!=Type::invalid)
        return std::get<std::int32_t>(interface_invoke(values,contract,0,state.source,{},options));
    return std::nullopt;
}

template<class Visit>
bool for_each_item(Values& values,OrderedSet& order,const Object& source,const ExecutionOptions& options,Visit&& visit){
    const auto iterator=sequence_begin(values,source,options);
    return sequence_finally(values,iterator,options,[&]{
        for(std::uint64_t i=0;sequence_resume(values,iterator,i,options);++i){
            order.step();
            if(!visit(sequence_at(values,iterator,static_cast<std::int64_t>(i),options)))return false;
        }
        return true;
    });
}

bool relation(Values& values,OrderedSet& order,const Object& left,const Object& input,Method method,const ExecutionOptions& options){
    if(!input || input->nil)
        throw Error("E_NIL",values.location,"SortedSet comparison collection is nil","Коллекция для сравнения с SortedSet равна nil");
    if(!input->sequence)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    const auto& items=left->items;
    if(method==Method::subset && items.empty())return true;
    if((method==Method::overlaps || method==Method::proper_superset) && items.empty())return false;
    if(method==Method::superset || method==Method::proper_superset || method==Method::overlaps){
        if(const auto size=collection_count(values,input,method==Method::overlaps,options);size && *size==0)
            return method!=Method::overlaps;
    }
    if(method==Method::proper_subset && items.empty()){
        if(const auto size=collection_count(values,input,false,options))return *size>0;
    }
    const auto right=as_sorted(input);
    if(right && right->ordering==left->ordering){
        if(method==Method::set_equal){
            const auto version=right->version;
            const auto size=std::min(items.size(),right->items.size());
            for(std::size_t i=0;i<size;++i){
                if(order.compare(left,items[i]->value,right->items[i]->value)!=0)return false;
                if(right->version!=version)throw runtime_error(RuntimeError::collection_changed,values.location);
            }
            return items.size()==right->items.size();
        }
        if((method==Method::subset && items.size()>right->items.size())
            || (method==Method::proper_subset && items.size()>=right->items.size())
            || (method==Method::superset && items.size()<right->items.size())
            || (method==Method::proper_superset && items.size()<=right->items.size()))
            return false;
        if(method==Method::overlaps){
            const auto element=type_info(values.program,left->type).element;
            const auto bound=[&](bool minimum){
                if(right->items.empty())return values.zero(element,false,true);
                return values.copy((minimum ? right->items.front() : right->items.back())->value);
            };
            if(order.compare(left,items.front()->value,bound(false))>0
                || order.compare(left,items.back()->value,bound(true))<0)return false;
        }
    }
    if(method==Method::superset)
        return for_each_item(values,order,input,options,[&](const Value& item){return order.locate(left,item).second;});
    if(method==Method::overlaps)
        return !for_each_item(values,order,input,options,[&](const Value& item){return !order.locate(left,item).second;});
    if(items.empty()){
        const auto empty=for_each_item(values,order,input,options,[](const Value&){return false;});
        return method==Method::set_equal ? empty : !empty;
    }
    const auto words=(items.size()+31)/32;
    if(words>values.remaining)throw runtime_error(RuntimeError::allocation_limit,values.location);
    values.remaining-=words;
    std::vector<std::uint32_t> seen(words,0);
    std::size_t unique=0;
    bool missing=false;
    for_each_item(values,order,input,options,[&](const Value& item){
        const auto [index,present]=order.locate(left,item);
        if(!present){
            missing=true;
            return method==Method::subset || method==Method::proper_subset;
        }
        const auto word=index/32;
        const auto mask=std::uint32_t{1}<<(index%32);
        if(word>=seen.size())++unique;
        else if(!(seen[word]&mask)){seen[word]|=mask;++unique;}
        return true;
    });
    if(method==Method::proper_superset)return !missing && unique<items.size();
    if(method==Method::proper_subset)return missing && unique==items.size();
    return unique==items.size() && (method==Method::subset || !missing);
}
}

Type sorted_set_type_for(const std::vector<TypeInfo>& types,Type element,CompoundKind kind){
    for(std::size_t i=0;i<types.size();++i)
        if(types[i].kind==kind && types[i].element==element && types[i].rank==1)return static_cast<Type>(64+i);
    return Type::invalid;
}

bool sorted_set_signature(const Program& program,Type owner,SortedSetMethod method){
    if(method>Method::overlaps || !compound(owner) || !valid_value_type(program,owner))return false;
    const auto& shape=type_info(program,owner);
    return shape.kind==CompoundKind::sorted_set
        && sorted_set_type_for(program.types,shape.element,CompoundKind::comparer_interface)!=Type::invalid
        && sorted_set_type_for(program.types,shape.element,CompoundKind::comparer)!=Type::invalid
        && sorted_set_type_for(program.types,shape.element,CompoundKind::list)!=Type::invalid
        && (!copies_items(method) || sorted_set_type_for(program.types,shape.element,CompoundKind::array)!=Type::invalid);
}

std::vector<Type> sorted_set_parameters(const std::vector<TypeInfo>& types,Type owner,SortedSetMethod method){
    const auto& shape=types.at(static_cast<unsigned>(owner)-64U);
    if(method<=Method::construct_source_comparer){
        std::vector<Type> parameters;
        if(uses_source(method))parameters.push_back(shape.key);
        if(uses_comparer(method))parameters.push_back(sorted_set_type_for(types,shape.element,CompoundKind::comparer_interface));
        return parameters;
    }
    if(method==Method::membership)return {shape.element,owner};
    if(relation_method(method))return {owner,shape.key};
    if(method==Method::try_get_value)return {owner,shape.element,reference_type(shape.element)};
    if(copies_items(method)){
        std::vector<Type> parameters{owner,sorted_set_type_for(types,shape.element,CompoundKind::array)};
        if(method>=Method::copy_index)parameters.push_back(Type::integer);
        if(method==Method::copy_count)parameters.push_back(Type::integer);
        return parameters;
    }
    if(method==Method::contains || method==Method::add || method==Method::remove)return {owner,shape.element};
    return {owner};
}

Type sorted_set_result(const std::vector<TypeInfo>& types,Type owner,SortedSetMethod method){
    if(method<=Method::construct_source_comparer || method==Method::copy)return owner;
    const auto& shape=types.at(static_cast<unsigned>(owner)-64U);
    const auto element=shape.element;
    switch(method){
    case Method::minimum: case Method::maximum:return element;
    case Method::count:return Type::integer;
    case Method::comparer:return sorted_set_type_for(types,element,CompoundKind::comparer_interface);
    case Method::clear: case Method::copy_array: case Method::copy_index: case Method::copy_count:return Type::unit;
    case Method::reverse:return shape.key;
    default:return Type::boolean;
    }
}

Value sorted_set_invoke(Values& values,Type owner,SortedSetMethod method,std::span<const Value> args,const ExecutionOptions& options){
    if(!sorted_set_signature(values.program,owner,method)
        || args.size()!=sorted_set_parameters(values.program.types,owner,method).size())
        throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    const auto& shape=type_info(values.program,owner);
    const auto policy=sorted_set_type_for(values.program.types,shape.element,CompoundKind::comparer_interface);
    OrderedSet order(values,options,policy);
    try {
        if(method<=Method::construct_source_comparer){
            const auto result=std::get<Object>(values.zero(owner,true));
            if(uses_comparer(method))result->ordering=std::get<Object>(args.back());
            if(!result->ordering || result->ordering->nil){
                const auto factory=sorted_set_type_for(values.program.types,shape.element,CompoundKind::comparer);
                result->ordering=std::get<Object>(comparer_create(values,factory,nullptr,options));
            }
            if(!uses_source(method))return result;
            const auto sequence=std::get<Object>(args.front());
            if(!sequence || sequence->nil)
                throw Error("E_NIL",values.location,"SortedSet source collection is nil","Исходная коллекция SortedSet равна nil");
            const auto source=as_sorted(sequence);
            if(source && source->ordering==result->ordering){
                order.step(source->items.size());
                if(source->items.size()>values.remaining)
                    throw runtime_error(RuntimeError::allocation_limit,values.location);
                result->items.reserve(source->items.size());
                for(const auto& cell:source->items)result->items.push_back(values.cell(shape.element,values.copy(cell->value)));
                return result;
            }
            const auto list_type=sorted_set_type_for(values.program.types,shape.element,CompoundKind::list);
            const auto list=std::get<Object>(sequence_collect(values,sequence,list_type,options));
            sort_legacy_list(values,list_type,policy,list,result->ordering,options);
            for(const auto& cell:list->items){
                if(result->items.empty() || order.compare(result,cell->value,result->items.back()->value)!=0)
                    result->items.push_back(cell);
            }
            return result;
        }
        const auto receiver=std::get<Object>(args[method==Method::membership ? 1U : 0U]);
        if(method==Method::copy && (!receiver || receiver->nil))return receiver;
        if(!receiver || receiver->nil)throw runtime_error(RuntimeError::nil_reference,values.location);
        auto& items=receiver->items;
        if(relation_method(method))return relation(values,order,receiver,std::get<Object>(args[1]),method,options);
        if(method==Method::copy){
            const auto source=sequence_create(values,shape.key,receiver,SequenceProvider::sorted_set);
            const std::array<Value,2> input{source,receiver->ordering};
            return sorted_set_invoke(values,owner,Method::construct_source_comparer,input,options);
        }
        if(method==Method::count)return static_cast<std::int32_t>(items.size());
        if(method==Method::minimum || method==Method::maximum)
            return items.empty() ? values.zero(shape.element,false,true)
                : values.copy((method==Method::minimum ? items.front() : items.back())->value);
        if(method==Method::comparer)return receiver->ordering;
        if(method==Method::clear){order.step(items.size());items.clear();++receiver->version;return false;}
        if(method==Method::reverse)return sequence_create(values,shape.key,receiver,SequenceProvider::sorted_set_reverse);
        if(copies_items(method)){
            const auto target=std::get<Object>(args[1]);
            if(!target || target->nil)throw runtime_error(RuntimeError::nil_reference,values.location);
            const auto index=method>=Method::copy_index ? std::get<std::int32_t>(args[2]) : 0;
            const auto count=method==Method::copy_count ? std::get<std::int32_t>(args[3]) : static_cast<std::int32_t>(items.size());
            if(index<0)throw Error("E_RANGE",values.location,"SortedSet copy index must be nonnegative","Индекс копирования SortedSet должен быть неотрицательным");
            if(count<0)throw Error("E_RANGE",values.location,"SortedSet copy count must be nonnegative","Число копируемых элементов SortedSet должно быть неотрицательным");
            const auto start=static_cast<std::size_t>(index);
            const auto length=static_cast<std::size_t>(count);
            if(start>target->items.size() || length>target->items.size()-start)
                throw Error("E_RANGE",values.location,"Destination array is too small for the SortedSet copy","В массиве недостаточно места для копирования SortedSet");
            for(std::size_t i=0;i<std::min(length,items.size());++i){
                order.step();
                values.write(target->items[start+i],items[i]->value);
            }
            return false;
        }

        const bool adding=method==Method::add;
        const bool removing=method==Method::remove;
        if(!items.empty() && (adding || removing))++receiver->version;
        const auto& item=args[method==Method::membership ? 0U : 1U];
        const auto [position,present]=order.locate(receiver,item);
        if(method==Method::try_get_value){
            const auto output=std::get<std::shared_ptr<Cell>>(args[2]);
            values.write(output,present ? items[position]->value : values.zero(shape.element,false,true));
            return present;
        }
        if(!adding && !removing)return present;
        if(adding==present)return false;
        if(adding && items.size()>=INT32_MAX)throw runtime_error(RuntimeError::integer_overflow,values.location);
        order.step(items.size()-position);
        const auto at=items.begin()+static_cast<std::ptrdiff_t>(position);
        if(adding){
            const auto was_empty=items.empty();
            items.insert(at,values.cell(shape.element,values.copy(item)));
            if(was_empty)++receiver->version;
        }else items.erase(at);
        return true;
    } catch(const std::bad_alloc&){
        throw runtime_error(RuntimeError::allocation_limit,values.location);
    } catch(const std::length_error&){
        throw runtime_error(RuntimeError::allocation_limit,values.location);
    }
}
}

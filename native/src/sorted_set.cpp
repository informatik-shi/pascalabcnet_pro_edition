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
struct SortedSetViewState {
    std::shared_ptr<Aggregate> underlying;
    Value lower,upper;
    bool cache_valid=false;
};
std::uint64_t sorted_set_version(const Aggregate& set){
    return set.sorted_view ? set.sorted_view->underlying->version : set.version;
}
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
bool update_method(Method method){
    return method>=Method::union_with && method<=Method::symmetric_with;
}
bool binary_operator(Method method){
    return method>=Method::equal_operator && method<=Method::remove_copy;
}
bool pair_operator(Method method){
    return method>=Method::equal_operator && method<=Method::intersect_copy;
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
        const auto version=sorted_set_version(*set);
        const std::array<Value,2> arguments{left,right};
        const auto result=std::get<std::int32_t>(comparer_invoke(values,policy,set->ordering,arguments,options));
        if(sorted_set_version(*set)!=version)throw runtime_error(RuntimeError::collection_changed,values.location);
        return result;
    }

    std::size_t boundary(const Object& set,const Value& bound,bool upper){
        std::size_t first=0,last=set->items.size();
        while(first<last){
            const auto middle=first+(last-first)/2;
            const auto comparison=compare(set,bound,set->items[middle]->value);
            if(comparison>0 || (upper && comparison==0))first=middle+1;
            else last=middle;
        }
        return first;
    }

    void refresh(const Object& set){
        const auto& view=set->sorted_view;
        if(!view || (view->cache_valid && set->version==view->underlying->version))return;
        const auto& source=view->underlying;
        const auto first=boundary(source,view->lower,false);
        const auto last=boundary(source,view->upper,true);
        if(last<first)throw Error("E_COMPARE",values.location,"SortedSet view bounds are inconsistent","Границы представления SortedSet несогласованны");
        const auto length=last-first;
        step(length);
        if(length>set->items.capacity()){
            const auto growth=length-set->items.capacity();
            if(growth>values.remaining)throw runtime_error(RuntimeError::allocation_limit,values.location);
            set->items.reserve(length);
            values.remaining-=growth;
        }
        set->items.assign(source->items.begin()+static_cast<std::ptrdiff_t>(first),
            source->items.begin()+static_cast<std::ptrdiff_t>(last));
        set->version=source->version;
        view->cache_valid=true;
    }

    bool within(const Object& set,const Value& item){
        const auto& view=*set->sorted_view;
        return compare(set,view.lower,item)<=0 && compare(set,view.upper,item)>=0;
    }

    std::pair<std::size_t,bool> locate(const Object& set,const Value& item){
        if(set->sorted_view){
            refresh(set);
            if(!within(set,item))return {0,false};
        }
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

    bool change(const Object& set,const Value& item,bool adding){
        if(set->sorted_view){
            if(!within(set,item)){
                if(adding)throw Error("E_RANGE",values.location,"Element is outside the SortedSet view","Элемент находится за пределами представления SortedSet");
                return false;
            }
            const auto changed=change(set->sorted_view->underlying,item,adding);
            refresh(set);
            return changed;
        }
        auto& items=set->items;
        if(!items.empty())++set->version;
        const auto [position,present]=locate(set,item);
        if(adding==present)return false;
        if(adding && items.size()>=INT32_MAX)
            throw runtime_error(RuntimeError::integer_overflow,values.location);
        step(items.size()-position);
        const auto at=items.begin()+static_cast<std::ptrdiff_t>(position);
        if(adding){
            const auto was_empty=items.empty();
            const auto element=type_info(values.program,set->type).element;
            items.insert(at,values.cell(element,values.copy(item)));
            if(was_empty)++set->version;
        }else items.erase(at);
        return true;
    }

    void clear(const Object& set){
        if(set->sorted_view){
            if(set->items.empty())return;
            refresh(set);
            if(set->items.size()>values.remaining)throw runtime_error(RuntimeError::allocation_limit,values.location);
            values.remaining-=set->items.size();
            const auto selected=set->items;
            for(auto item=selected.rbegin();item!=selected.rend();++item)
                (void)change(set->sorted_view->underlying,(*item)->value,false);
            refresh(set);
            return;
        }
        step(set->items.size());
        set->items.clear();
        ++set->version;
    }
};

Value create_view(Values& values,OrderedSet& order,const Object& receiver,const Value& lower,const Value& upper){
    auto root=receiver;
    if(const auto& parent=receiver->sorted_view){
        if(order.compare(receiver,parent->lower,lower)>0)
            throw Error("E_RANGE",values.location,"Lower bound is outside the parent SortedSet view","Нижняя граница выходит за пределы исходного представления SortedSet");
        if(order.compare(receiver,parent->upper,upper)<0)
            throw Error("E_RANGE",values.location,"Upper bound is outside the parent SortedSet view","Верхняя граница выходит за пределы исходного представления SortedSet");
        root=parent->underlying;
    }
    if(order.compare(root,lower,upper)>0)
        throw Error("E_RANGE",values.location,"SortedSet lower bound exceeds the upper bound","Нижняя граница SortedSet больше верхней");
    auto result=std::get<Object>(values.zero(root->type,true));
    if(!values.remaining)throw runtime_error(RuntimeError::allocation_limit,values.location);
    --values.remaining;
    result->ordering=root->ordering;
    result->sorted_view=std::make_shared<SortedSetViewState>(
        SortedSetViewState{root,values.copy(lower),values.copy(upper)});
    order.refresh(result);
    return result;
}

Object as_sorted(const Object& input){
    return input && input->sequence && input->sequence->provider==SequenceProvider::sorted_set
        ? input->sequence->source : Object{};
}

std::optional<std::int32_t> collection_count(Values& values,const Object& input,bool generic,const ExecutionOptions& options){
    const auto& state=*input->sequence;
    if(state.provider==SequenceProvider::sorted_set)sorted_set_refresh(values,state.source,options);
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
    order.refresh(left);
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
    if(right)order.refresh(right);
    if(right && right->ordering==left->ordering){
        if(method==Method::set_equal){
            const auto version=sorted_set_version(*right);
            const auto size=std::min(items.size(),right->items.size());
            for(std::size_t i=0;i<size;++i){
                if(order.compare(left,items[i]->value,right->items[i]->value)!=0)return false;
                if(sorted_set_version(*right)!=version)throw runtime_error(RuntimeError::collection_changed,values.location);
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

void merge_sorted(Values& values,OrderedSet& order,const Object& left,const Object& right,bool intersection){
    const auto left_count=left->items.size();
    const auto right_count=right->items.size();
    if(!intersection && right_count>static_cast<std::size_t>(INT32_MAX)-left_count)
        throw runtime_error(RuntimeError::integer_overflow,values.location);
    const auto capacity=intersection ? left_count : left_count+right_count;
    if(capacity>values.remaining)throw runtime_error(RuntimeError::allocation_limit,values.location);
    values.remaining-=capacity;
    std::vector<std::shared_ptr<Cell>> result;
    result.reserve(capacity);
    const auto maximum=intersection ? values.copy(left->items.back()->value) : Value{std::int32_t{0}};
    const auto version=sorted_set_version(*right);
    const auto check_right=[&]{
        if(sorted_set_version(*right)!=version)throw runtime_error(RuntimeError::collection_changed,values.location);
    };
    std::size_t i=0,j=0;
    while(i<left_count && j<right_count){
        if(intersection){
            const auto beyond_maximum=order.compare(left,right->items[j]->value,maximum)>0;
            check_right();
            if(beyond_maximum)break;
        }
        const auto comparison=order.compare(left,left->items[i]->value,right->items[j]->value);
        check_right();
        if(comparison<0){
            if(!intersection)result.push_back(left->items[i]);
            ++i;
        }else{
            if(!intersection || comparison==0)result.push_back(right->items[j]);
            ++j;
            if(comparison==0)++i;
        }
    }
    if(!intersection){
        order.step(left_count-i+right_count-j);
        result.insert(result.end(),left->items.begin()+static_cast<std::ptrdiff_t>(i),left->items.end());
        result.insert(result.end(),right->items.begin()+static_cast<std::ptrdiff_t>(j),right->items.end());
    }
    left->items=std::move(result);
    ++left->version;
}

void update(Values& values,OrderedSet& order,const Object& left,const Object& input,Method method,const ExecutionOptions& options){
    if(!input || input->nil)
        throw Error("E_NIL",values.location,"SortedSet source collection is nil","Исходная коллекция SortedSet равна nil");
    if(method==Method::except_with && left->items.empty())return;
    order.refresh(left);
    if(method==Method::intersect_with && left->items.empty())return;
    const auto right=as_sorted(input);
    if(right)order.refresh(right);
    const bool same_policy=right && right->ordering==left->ordering;
    const bool can_merge=!left->sorted_view && same_policy;
    const auto& shape=type_info(values.program,left->type);
    const auto include=[&](const Value& item){
        if(!order.locate(left,item).second)(void)order.change(left,item,true);
        return true;
    };
    const auto toggle=[&](const Value& item){
        (void)order.change(left,item,!order.locate(left,item).second);
        return true;
    };
    if(method==Method::symmetric_with && left->items.empty())method=Method::union_with;
    if(method==Method::union_with){
        if(right && !left->sorted_view && left->items.empty()){
            const std::array<Value,2> args{input,left->ordering};
            auto copy=std::get<Object>(sorted_set_invoke(values,left->type,Method::construct_source_comparer,args,options));
            left->items=std::move(copy->items);
            ++left->version;
        }else if(can_merge && right->items.size()>left->items.size()/2)
            merge_sorted(values,order,left,right,false);
        else for_each_item(values,order,input,options,include);
        return;
    }
    if(method==Method::intersect_with){
        if(can_merge){merge_sorted(values,order,left,right,true);return;}
        std::vector<Value> saved;
        for_each_item(values,order,input,options,[&](const Value& item){
            if(order.locate(left,item).second){
                if(!values.remaining)throw runtime_error(RuntimeError::allocation_limit,values.location);
                --values.remaining;
                saved.push_back(values.copy(item));
                (void)order.change(left,item,false);
            }
            return true;
        });
        order.clear(left);
        for(const auto& item:saved)include(item);
        return;
    }
    if(right==left){order.clear(left);return;}
    if(method==Method::except_with){
        const auto min=values.copy(left->items.front()->value);
        const auto max=values.copy(left->items.back()->value);
        if(same_policy){
            if(right->items.empty() || order.compare(left,right->items.back()->value,min)<0)return;
            if(right->items.empty() || order.compare(left,right->items.front()->value,max)>0)return;
        }
        for_each_item(values,order,input,options,[&](const Value& item){
            if(order.compare(left,item,min)<0)return true;
            if(order.compare(left,item,max)>0)return !same_policy;
            if(same_policy || order.locate(left,item).second)(void)order.change(left,item,false);
            return true;
        });
        return;
    }
    if(same_policy){for_each_item(values,order,input,options,toggle);return;}
    const auto list_type=find_element_type(values.program.types,shape.element,CompoundKind::list);
    const auto policy=find_element_type(values.program.types,shape.element,CompoundKind::comparer_interface);
    const auto list=std::get<Object>(sequence_collect(values,input,list_type,options));
    sort_legacy_list(values,list_type,policy,list,left->ordering,options);
    const Cell* previous=nullptr;
    for(const auto& cell:list->items){
        if(previous && order.compare(left,cell->value,previous->value)==0)continue;
        toggle(cell->value);
        previous=cell.get();
    }
}

void require_set(const Object& set,SourceLocation location){
    if(!set || set->nil)
        throw Error("E_NIL",location,"SortedSet is nil","Упорядоченное множество равно nil");
}

Value apply_operator(Values& values,OrderedSet& order,Type owner,Method method,std::span<const Value> args,const ExecutionOptions& options){
    const auto sequence_type=type_info(values.program,owner).key;
    if(sorted_set_reference(method)){
        // A var target may be assigned by the right operand before the call starts.
        const auto reference=std::get<std::shared_ptr<Cell>>(args[0]);
        if(!reference || reference->type!=owner)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
        const auto set=std::get<Object>(reference->value);
        require_set(set,values.location);
        if(method==Method::ref_add || method==Method::ref_remove)
            (void)order.change(set,args[1],method==Method::ref_add);
        else update(values,order,set,std::get<Object>(args[1]),method==Method::ref_union ? Method::union_with : Method::except_with,options);
        return false;
    }
    const auto left=std::get<Object>(args[0]);
    const auto right=pair_operator(method) ? std::get<Object>(args[1]) : Object{};
    const bool left_nil=!left || left->nil;
    const bool right_nil=!right || right->nil;
    if(method==Method::equal_operator || method==Method::not_equal_operator){
        const bool equal=left_nil || right_nil ? left_nil==right_nil
            : relation(values,order,left,sequence_create(values,sequence_type,right,SequenceProvider::sorted_set),Method::set_equal,options);
        return method==Method::equal_operator ? equal : !equal;
    }
    require_set(left,values.location);
    if(pair_operator(method))require_set(right,values.location);
    if(method<=Method::greater_equal_operator){
        constexpr std::array<Method,4> methods{Method::proper_subset,Method::subset,Method::proper_superset,Method::superset};
        const auto index=static_cast<unsigned>(method)-static_cast<unsigned>(Method::less_operator);
        return relation(values,order,left,sequence_create(values,sequence_type,right,SequenceProvider::sorted_set),methods[index],options);
    }
    const std::array<Value,1> copy_source{sequence_create(values,sequence_type,left,SequenceProvider::sorted_set)};
    const auto copy=std::get<Object>(sorted_set_invoke(values,owner,Method::construct_source,copy_source,options));
    if(method==Method::add_copy || method==Method::remove_copy)
        (void)order.change(copy,args[1],method==Method::add_copy);
    else {
        const auto operation=method==Method::union_copy ? Method::union_with
            : method==Method::except_copy ? Method::except_with : Method::intersect_with;
        update(values,order,copy,sequence_create(values,sequence_type,right,SequenceProvider::sorted_set),operation,options);
    }
    return copy;
}
}

void sorted_set_refresh(Values& values,const std::shared_ptr<Aggregate>& set,const ExecutionOptions& options){
    if(!set || !set->sorted_view)return;
    const auto element=type_info(values.program,set->type).element;
    const auto policy=find_element_type(values.program.types,element,CompoundKind::comparer_interface);
    OrderedSet order(values,options,policy);
    order.refresh(set);
}

Type find_element_type(const std::vector<TypeInfo>& types,Type element,CompoundKind kind){
    for(std::size_t i=0;i<types.size();++i)
        if(types[i].kind==kind && types[i].element==element && types[i].rank==1)return static_cast<Type>(64+i);
    return Type::invalid;
}

bool sorted_set_signature(const Program& program,Type owner,SortedSetMethod method){
    if(method>Method::view_between || !compound(owner) || !valid_value_type(program,owner))return false;
    const auto& shape=type_info(program,owner);
    return shape.kind==CompoundKind::sorted_set
        && find_element_type(program.types,shape.element,CompoundKind::comparer_interface)!=Type::invalid
        && find_element_type(program.types,shape.element,CompoundKind::comparer)!=Type::invalid
        && find_element_type(program.types,shape.element,CompoundKind::list)!=Type::invalid
        && (!copies_items(method) || find_element_type(program.types,shape.element,CompoundKind::array)!=Type::invalid);
}
bool sorted_set_reference(SortedSetMethod method){
    return method>=Method::ref_add && method<=Method::ref_except;
}

std::vector<Type> sorted_set_parameters(const std::vector<TypeInfo>& types,Type owner,SortedSetMethod method){
    const auto& shape=types.at(static_cast<unsigned>(owner)-64U);
    if(method<=Method::construct_source_comparer){
        std::vector<Type> parameters;
        if(uses_source(method))parameters.push_back(shape.key);
        if(uses_comparer(method))parameters.push_back(find_element_type(types,shape.element,CompoundKind::comparer_interface));
        return parameters;
    }
    if(method==Method::membership)return {shape.element,owner};
    if(method==Method::view_between)return {owner,shape.element,shape.element};
    if(sorted_set_reference(method))
        return {reference_type(owner),method<=Method::ref_remove ? shape.element : shape.key};
    if(pair_operator(method))return {owner,owner};
    if(binary_operator(method))return {owner,shape.element};
    if(relation_method(method) || update_method(method))return {owner,shape.key};
    if(method==Method::try_get_value)return {owner,shape.element,reference_type(shape.element)};
    if(copies_items(method)){
        std::vector<Type> parameters{owner,find_element_type(types,shape.element,CompoundKind::array)};
        if(method>=Method::copy_index)parameters.push_back(Type::integer);
        if(method==Method::copy_count)parameters.push_back(Type::integer);
        return parameters;
    }
    if(method==Method::contains || method==Method::add || method==Method::remove)return {owner,shape.element};
    return {owner};
}

Type sorted_set_result(const std::vector<TypeInfo>& types,Type owner,SortedSetMethod method){
    if(method<=Method::construct_source_comparer || method==Method::copy || method==Method::view_between)return owner;
    if(method>=Method::union_copy && method<=Method::remove_copy)return owner;
    if(update_method(method) || sorted_set_reference(method))return Type::unit;
    const auto& shape=types.at(static_cast<unsigned>(owner)-64U);
    const auto element=shape.element;
    switch(method){
    case Method::minimum: case Method::maximum:return element;
    case Method::count:return Type::integer;
    case Method::comparer:return find_element_type(types,element,CompoundKind::comparer_interface);
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
    const auto policy=find_element_type(values.program.types,shape.element,CompoundKind::comparer_interface);
    OrderedSet order(values,options,policy);
    try {
        if(method<=Method::construct_source_comparer){
            const auto result=std::get<Object>(values.zero(owner,true));
            if(uses_comparer(method))result->ordering=std::get<Object>(args.back());
            if(!result->ordering || result->ordering->nil){
                const auto factory=find_element_type(values.program.types,shape.element,CompoundKind::comparer);
                result->ordering=std::get<Object>(comparer_create(values,factory,nullptr,options));
            }
            if(!uses_source(method))return result;
            const auto sequence=std::get<Object>(args.front());
            if(!sequence || sequence->nil)
                throw Error("E_NIL",values.location,"SortedSet source collection is nil","Исходная коллекция SortedSet равна nil");
            const auto source=as_sorted(sequence);
            if(source && !source->sorted_view && source->ordering==result->ordering){
                order.step(source->items.size());
                if(source->items.size()>values.remaining)
                    throw runtime_error(RuntimeError::allocation_limit,values.location);
                result->items.reserve(source->items.size());
                for(const auto& cell:source->items)result->items.push_back(values.cell(shape.element,values.copy(cell->value)));
                return result;
            }
            const auto list_type=find_element_type(values.program.types,shape.element,CompoundKind::list);
            const auto list=std::get<Object>(sequence_collect(values,sequence,list_type,options));
            sort_legacy_list(values,list_type,policy,list,result->ordering,options);
            for(const auto& cell:list->items){
                if(result->items.empty() || order.compare(result,cell->value,result->items.back()->value)!=0)
                    result->items.push_back(cell);
            }
            return result;
        }
        if(binary_operator(method) || sorted_set_reference(method))
            return apply_operator(values,order,owner,method,args,options);
        const auto receiver=std::get<Object>(args[method==Method::membership ? 1U : 0U]);
        if(method==Method::membership)require_set(receiver,values.location);
        if(method==Method::copy && (!receiver || receiver->nil))return receiver;
        if(!receiver || receiver->nil)throw runtime_error(RuntimeError::nil_reference,values.location);
        auto& items=receiver->items;
        if(relation_method(method))return relation(values,order,receiver,std::get<Object>(args[1]),method,options);
        if(update_method(method)){update(values,order,receiver,std::get<Object>(args[1]),method,options);return false;}
        if(method==Method::copy){
            const auto source=sequence_create(values,shape.key,receiver,SequenceProvider::sorted_set);
            const std::array<Value,2> input{source,receiver->ordering};
            return sorted_set_invoke(values,owner,Method::construct_source_comparer,input,options);
        }
        if(method==Method::comparer)return receiver->ordering;
        if(method==Method::clear){order.clear(receiver);return false;}
        if(method==Method::reverse)return sequence_create(values,shape.key,receiver,SequenceProvider::sorted_set_reverse);
        if(method==Method::view_between)return create_view(values,order,receiver,args[1],args[2]);
        if(method==Method::count || method==Method::minimum || method==Method::maximum){
            order.refresh(receiver);
            if(method==Method::count)return static_cast<std::int32_t>(items.size());
            return items.empty() ? values.zero(shape.element,false,true)
                : values.copy((method==Method::minimum ? items.front() : items.back())->value);
        }
        if(copies_items(method)){
            if(method!=Method::copy_count)order.refresh(receiver);
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
            order.refresh(receiver);
            for(std::size_t i=0;i<std::min(length,items.size());++i){
                order.step();
                values.write(target->items[start+i],items[i]->value);
            }
            return false;
        }

        if(method==Method::add || method==Method::remove)
            return order.change(receiver,args[1],method==Method::add);
        const auto& item=args[method==Method::membership ? 0U : 1U];
        const auto [position,present]=order.locate(receiver,item);
        if(method==Method::try_get_value){
            const auto output=std::get<std::shared_ptr<Cell>>(args[2]);
            values.write(output,present ? items[position]->value : values.zero(shape.element,false,true));
            return present;
        }
        return present;
    } catch(const std::bad_alloc&){
        throw runtime_error(RuntimeError::allocation_limit,values.location);
    } catch(const std::length_error&){
        throw runtime_error(RuntimeError::allocation_limit,values.location);
    }
}
}

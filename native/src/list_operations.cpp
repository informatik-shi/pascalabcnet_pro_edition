#include "list_operations.hpp"
#include "sequences.hpp"
#include "ordinal_range.hpp"
#include <bit>

namespace abc {
namespace {
void work(Values& values,std::uint64_t& used,const ExecutionOptions& options) {
    if (used>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
    if ((used&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
    ++used;
}
void charge(Values& values,std::uint64_t& used,std::size_t count,const ExecutionOptions& options) {
    for (std::size_t i=0;i<count;++i) work(values,used,options);
}
void range(std::int32_t index,std::int32_t count,std::int32_t size,SourceLocation location) {
    if (index<0) throw runtime_error(RuntimeError::list_index,location);
    if (count<0) throw runtime_error(RuntimeError::list_count,location);
    if (index>size || count>size-index) throw runtime_error(RuntimeError::list_range,location);
}
}
bool list_copy_signature_valid(const Program& program,Type list,Type array,ListCopy method) {
    if (static_cast<unsigned>(method)>static_cast<unsigned>(ListCopy::range) || !compound(list) || !compound(array)) return false;
    const auto& source=type_info(program,list);const auto& target=type_info(program,array);
    return source.kind==CompoundKind::list && source.rank==1 && target.kind==CompoundKind::array && target.rank==1 && source.element==target.element;
}
void list_copy(Values& values,const std::shared_ptr<Aggregate>& source,ListCopy method,std::span<const Value> arguments,const ExecutionOptions& options) {
    if (!source || source->nil) throw runtime_error(RuntimeError::nil_reference,values.location);
    const bool range_copy=method==ListCopy::range;
    const auto size=static_cast<std::int32_t>(source->items.size());
    const auto index=range_copy ? std::get<std::int32_t>(arguments[0]) : 0;
    const auto count=range_copy ? std::get<std::int32_t>(arguments[3]) : size;
    if (range_copy) {
        // The BCL preliminary check uses unchecked Int32 subtraction before Array.Copy.
        const auto available=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(size)-static_cast<std::uint32_t>(index));
        if (available<count) throw runtime_error(RuntimeError::list_range,values.location);
    }
    const auto target=std::get<std::shared_ptr<Aggregate>>(arguments[range_copy ? 1 : 0]);
    if (!target || target->nil) throw runtime_error(RuntimeError::nil_reference,values.location);
    if (count<0) throw runtime_error(RuntimeError::list_count,values.location);
    if (index<0) throw runtime_error(RuntimeError::list_index,values.location);
    if (index>size || count>size-index) throw runtime_error(RuntimeError::list_range,values.location);
    const auto destination=method==ListCopy::all ? 0 : std::get<std::int32_t>(arguments[range_copy ? 2 : 1]);
    if (destination<0) throw runtime_error(RuntimeError::list_copy_destination_index,values.location);
    if (static_cast<std::size_t>(destination)>target->items.size() || static_cast<std::size_t>(count)>target->items.size()-static_cast<std::size_t>(destination))
        throw runtime_error(RuntimeError::list_copy_destination_range,values.location);
    std::uint64_t used=0;charge(values,used,static_cast<std::size_t>(count),options);
    for (std::int32_t i=0;i<count;++i) values.write(target->items[static_cast<std::size_t>(destination)+static_cast<std::size_t>(i)],source->items[static_cast<std::size_t>(index)+static_cast<std::size_t>(i)]->value);
}
Value list_capacity(Values& values,Type type,const std::shared_ptr<Aggregate>& source,ListCapacity method,std::int32_t capacity,const ExecutionOptions& options) {
    const auto list=method==ListCapacity::construct ? values.array(type,0) : source;
    if (!list || list->nil) throw runtime_error(RuntimeError::nil_reference,values.location);
    if (method==ListCapacity::get) return static_cast<std::int32_t>(list->list_capacity);
    if (list_capacity_argument(method) && capacity<0)
        throw runtime_error(method==ListCapacity::set ? RuntimeError::list_capacity : RuntimeError::list_capacity_negative,values.location);
    if (method==ListCapacity::trim) {
        if (list->items.size()>=list->list_capacity*9/10) return std::int32_t{0};
        capacity=static_cast<std::int32_t>(list->items.size());
    }
    if (method==ListCapacity::set && static_cast<std::size_t>(capacity)<list->items.size())
        throw runtime_error(RuntimeError::list_capacity,values.location);
    const bool change=method==ListCapacity::ensure ? static_cast<std::size_t>(capacity)>list->list_capacity
        : static_cast<std::size_t>(capacity)!=list->list_capacity;
    if (change) {
        std::uint64_t used=0;charge(values,used,list->items.size()+1,options);
        if (method==ListCapacity::ensure) values.grow_list(list,static_cast<std::size_t>(capacity));
        else values.set_list_capacity(list,static_cast<std::size_t>(capacity));
    }
    if (method==ListCapacity::construct) return list;
    return method==ListCapacity::ensure ? static_cast<std::int32_t>(list->list_capacity) : std::int32_t{0};
}
bool list_bulk_signature_valid(const Program& program,Type target,Type input) {
    if (!compound(target) || !compound(input)) return false;
    const auto& list=type_info(program,target);const auto& source=type_info(program,input);
    return list.kind==CompoundKind::list && list.rank==1 && source.rank==1 && source.element==list.element
        && (source.kind==CompoundKind::array || source.kind==CompoundKind::list || source.kind==CompoundKind::sequence);
}
void list_bulk(Values& values,const std::shared_ptr<Aggregate>& target,const std::shared_ptr<Aggregate>& input,bool insert,std::int32_t index,const ExecutionOptions& options) {
    if (!target || target->nil || !input || input->nil) throw runtime_error(RuntimeError::nil_reference,values.location);
    if (insert && (index<0 || static_cast<std::size_t>(index)>target->items.size())) throw runtime_error(RuntimeError::list_index,values.location);
    std::uint64_t used=0;const auto element=type_info(values.program,target->type).element;
    std::shared_ptr<Aggregate> collection;
    const auto kind=type_info(values.program,input->type).kind;
    if (kind==CompoundKind::array || kind==CompoundKind::list) collection=input;
    else if (input->sequence && collection_sequence(input->sequence->provider)) collection=input->sequence->source;
    else if(input->sequence && input->sequence->provider==SequenceProvider::hash_set){
        Type array=Type::invalid;for(std::size_t i=0;i<values.program.types.size();++i){const auto& type=values.program.types[i];if(type.kind==CompoundKind::array && type.rank==1 && type.element==element){array=static_cast<Type>(64+i);break;}}
        if(array==Type::invalid)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
        collection=std::get<std::shared_ptr<Aggregate>>(sequence_collect(values,input,array,options));
    }
    else if(input->sequence && input->sequence->provider==SequenceProvider::ordinal_range){
        const auto owner=input->sequence->source->type;const auto array=ordinal_range_array(values.program.types,owner);
        collection=std::get<std::shared_ptr<Aggregate>>(ordinal_range_collect(values,input->sequence->source,array,options));
    }
    if (collection) {
        const auto count=collection->items.size();if (!count) return;
        if (target->items.size()>INT32_MAX || count>static_cast<std::size_t>(INT32_MAX)-target->items.size() || count>values.remaining)
            throw runtime_error(RuntimeError::allocation_limit,values.location);
        const auto position=insert ? static_cast<std::size_t>(index) : target->items.size();
        charge(values,used,target->items.size()-position,options);
        std::vector<std::shared_ptr<Cell>> added;added.reserve(count);
        for (const auto& item:collection->items) {
            work(values,used,options);auto cell=values.cell(element,values.copy(item->value));values.own_list_cell(cell,target);added.push_back(std::move(cell));
        }
        values.grow_list(target,target->items.size()+count);
        // Freeze source cells before insertion, including insertion into Self.
        target->items.insert(target->items.begin()+static_cast<std::ptrdiff_t>(position),added.begin(),added.end());++target->version;return;
    }
    auto cursor=sequence_begin(values,input);std::uint64_t position=static_cast<std::uint32_t>(index);
    for (std::uint64_t i=0;sequence_resume(values,cursor,i,options);++i) {
        work(values,used,options);
        if (i>static_cast<std::uint64_t>(INT64_MAX)) throw runtime_error(RuntimeError::integer_overflow,values.location);
        auto value=sequence_at(values,cursor,static_cast<std::int64_t>(i),options);
        const auto destination=insert ? position : target->items.size();
        if (destination>target->items.size()) throw runtime_error(RuntimeError::list_index,values.location);
        if (target->items.size()>=INT32_MAX) throw runtime_error(RuntimeError::allocation_limit,values.location);
        charge(values,used,target->items.size()-static_cast<std::size_t>(destination)+1,options);
        values.grow_list(target,target->items.size()+1);
        auto cell=values.cell(element,values.copy(value));values.own_list_cell(cell,target);
        target->items.insert(target->items.begin()+static_cast<std::ptrdiff_t>(destination),std::move(cell));++target->version;
        if (insert) ++position;
    }
}
bool list_operation_signature_valid(const Program& program,Type source,Type result,ListOperation method) {
    if (static_cast<unsigned>(method)>static_cast<unsigned>(ListOperation::last_range) || !compound(source)) return false;
    const auto& type=type_info(program,source);
    return type.kind==CompoundKind::list && type.rank==1 && result==list_operation_result(source,method);
}
Value list_operation(Values& values,const std::shared_ptr<Aggregate>& source,ListOperation method,std::span<const Value> arguments,const ExecutionOptions& options) {
    if (!source || source->nil) throw runtime_error(RuntimeError::nil_reference,values.location);
    const auto size=static_cast<std::int32_t>(source->items.size());std::uint64_t used=0;
    if (method==ListOperation::insert) {
        const auto index=std::get<std::int32_t>(arguments[0]);
        if (index<0 || index>size) throw runtime_error(RuntimeError::list_index,values.location);
        if (size==INT32_MAX) throw runtime_error(RuntimeError::allocation_limit,values.location);
        charge(values,used,static_cast<std::size_t>(size-index)+1,options);
        values.grow_list(source,source->items.size()+1);
        auto cell=values.cell(type_info(values.program,source->type).element,values.copy(arguments[1]));
        values.own_list_cell(cell,source);source->items.insert(source->items.begin()+index,std::move(cell));++source->version;
        return std::int32_t{0};
    }
    if (method==ListOperation::get_range || method==ListOperation::remove_range) {
        const auto index=std::get<std::int32_t>(arguments[0]),count=std::get<std::int32_t>(arguments[1]);
        range(index,count,size,values.location);
        if (method==ListOperation::get_range) {
            auto result=values.array(source->type,static_cast<std::size_t>(count),true);
            for (std::int32_t i=0;i<count;++i) {
                work(values,used,options);values.write(result->items[static_cast<std::size_t>(i)],source->items[static_cast<std::size_t>(index+i)]->value);
                values.own_list_cell(result->items[static_cast<std::size_t>(i)],result);
            }
            return result;
        }
        if (count) {
            charge(values,used,static_cast<std::size_t>(size-index),options);
            source->items.erase(source->items.begin()+index,source->items.begin()+index+count);++source->version;
        }
        return std::int32_t{0};
    }
    const bool last=method==ListOperation::last || method==ListOperation::last_from || method==ListOperation::last_range;
    const bool from=method==ListOperation::index_from || method==ListOperation::last_from;
    const bool explicit_range=method==ListOperation::index_range || method==ListOperation::last_range;
    std::int32_t start=last ? size-1 : 0,count=size;
    if (from || explicit_range) {
        start=std::get<std::int32_t>(arguments[1]);
        if (explicit_range) count=std::get<std::int32_t>(arguments[2]);
        if (last) {
            // The two-argument overload checks the upper index before delegating;
            // the three-argument overload returns -1 immediately for an empty List.
            if (from && start>=size) throw runtime_error(RuntimeError::list_index,values.location);
            if (!size) return std::int32_t{-1};
            if (start<0) throw runtime_error(RuntimeError::list_index,values.location);
            if (explicit_range && count<0) throw runtime_error(RuntimeError::list_count,values.location);
            if (start>=size) throw runtime_error(RuntimeError::list_index,values.location);
            if (from) count=start+1;
            if (count>start+1) throw runtime_error(RuntimeError::list_count,values.location);
        } else {
            if (start>size) throw runtime_error(RuntimeError::list_index,values.location);
            if (explicit_range && (count<0 || static_cast<std::int64_t>(start)>static_cast<std::int64_t>(size)-count))
                throw runtime_error(RuntimeError::list_count,values.location);
            if (start<0) throw runtime_error(RuntimeError::list_index,values.location);
            if (from) count=size-start;
        }
    }
    for (std::int32_t i=0;i<count;++i) {
        work(values,used,options);const auto index=last ? start-i : start+i;
        if (!values.equal(source->items[static_cast<std::size_t>(index)]->value,arguments[0])) continue;
        if (method!=ListOperation::remove) return index;
        charge(values,used,static_cast<std::size_t>(size-index),options);
        source->items.erase(source->items.begin()+index);++source->version;return true;
    }
    return method==ListOperation::remove ? Value{false} : Value{std::int32_t{-1}};
}
}

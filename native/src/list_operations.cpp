#include "list_operations.hpp"

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

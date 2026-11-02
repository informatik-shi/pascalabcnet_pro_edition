#include "array_views.hpp"
#include "sequence_guard.hpp"
#include <array>
#include <algorithm>

namespace abc {
namespace {
void work(Values& values,ArrayViewState& cursor,const ExecutionOptions& options) {
    if (cursor.work>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
    if ((cursor.work&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
    ++cursor.work;
}
std::shared_ptr<Aggregate> tuple(Values& values,Type type,std::span<const Value> fields) {
    const auto& signature=type_info(values.program,type).fields;
    auto result=std::make_shared<Aggregate>(); result->type=type;
    for (std::size_t i=0;i<fields.size();++i) result->items.push_back(values.cell(signature[i].type,values.copy(fields[i])));
    return result;
}
}
bool array_view_signature_valid(const Program& program,Type source,Type result,Type callback,ArrayViewMethod method) {
    if (method>ArrayViewMethod::by_column || !compound(source) || !compound(result)) return false;
    const auto& input=type_info(program,source); const auto& output=type_info(program,result);
    if ((input.kind!=CompoundKind::array && input.kind!=CompoundKind::list) || (input.rank!=1 && input.rank!=2)
        || (input.rank==2 && input.kind!=CompoundKind::array) || output.kind!=CompoundKind::sequence || output.rank!=1) return false;
    const bool matrix=input.rank==2,predicate=method==ArrayViewMethod::matching_indices;
    if ((method==ArrayViewMethod::indices && matrix) || ((method==ArrayViewMethod::with_indices || method==ArrayViewMethod::by_row || method==ArrayViewMethod::by_column) && !matrix)) return false;
    if (predicate) {
        if (!compound(callback)) return false;
        const auto& signature=type_info(program,callback); const auto count=signature.parameters.size();
        if (signature.kind!=CompoundKind::callable || signature.element!=Type::boolean || (count!=1 && count!=input.rank+1)
            || signature.parameters[0].type!=input.element || std::any_of(signature.parameters.begin(),signature.parameters.end(),[](const auto& p){return p.by_reference;})) return false;
        for (std::size_t i=1;i<count;++i) if (signature.parameters[i].type!=Type::integer) return false;
    } else if (callback!=Type::invalid) return false;
    if (method==ArrayViewMethod::by_row || method==ArrayViewMethod::by_column) return output.element==input.element;
    if (!matrix) return output.element==Type::integer;
    if (!compound(output.element)) return false;
    const auto& fields=type_info(program,output.element);
    const bool values=method==ArrayViewMethod::with_indices;
    if (fields.kind!=CompoundKind::tuple || fields.fields.size()!=(values ? 3U : 2U)) return false;
    for (std::size_t i=0;i<fields.fields.size();++i) if (fields.fields[i].type!=(values && !i ? input.element : Type::integer)) return false;
    return true;
}
std::shared_ptr<Aggregate> array_view(Values& values,Type type,const std::shared_ptr<Aggregate>& source,ArrayViewMethod method,const ExecutionOptions& options,std::shared_ptr<Aggregate> callable) {
    if (method==ArrayViewMethod::indices) {
        if (!source || source->nil) throw runtime_error(RuntimeError::nil_reference,values.location);
        if (source->items.size()>INT32_MAX) throw runtime_error(RuntimeError::integer_overflow,values.location);
        return sequence_range(values,type,std::int32_t{0},static_cast<std::int32_t>(source->items.size())-1);
    }
    auto result=sequence_composed(values,type,source,SequenceProvider::array_view,options);
    result->sequence->view_method=method; result->sequence->callable=std::move(callable); return result;
}
bool array_view_resume(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options) {
    SequenceDepthGuard guard(values,options); const auto& state=*sequence->sequence;
    if (!state.view) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    auto& cursor=*state.view;
    if (cursor.positioned && index==cursor.index) return cursor.available;
    if ((!cursor.positioned && index) || (cursor.positioned && (!cursor.available || index<=cursor.index || index-cursor.index!=1)))
        throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    if (index>static_cast<std::uint64_t>(INT64_MAX)) throw runtime_error(RuntimeError::integer_overflow,values.location);
    work(values,cursor,options);
    if (!state.source) throw runtime_error(RuntimeError::nil_reference,values.location);
    const auto& input=type_info(values.program,state.source->type);
    const bool matrix=input.rank==2,predicate=state.view_method==ArrayViewMethod::matching_indices;
    if (!cursor.initialized) {
        // Only the matrix overload checks its predicate before touching the source.
        if (matrix && predicate) (void)values.callable_target(state.callable);
        if (state.source->nil) throw runtime_error(RuntimeError::nil_reference,values.location);
        cursor.bound=state.source->items.size(); cursor.initialized=true;
    }
    for (;cursor.next<cursor.bound;) {
        work(values,cursor,options); const auto candidate=cursor.next++;
        const auto row=matrix ? state.view_method==ArrayViewMethod::by_column ? candidate%state.source->rows : candidate/state.source->columns : 0;
        const auto column=matrix ? state.view_method==ArrayViewMethod::by_column ? candidate/state.source->rows : candidate%state.source->columns : candidate;
        const auto offset=matrix ? row*state.source->columns+column : candidate;
        if (offset>=state.source->items.size()) throw runtime_error(RuntimeError::index_bounds,values.location);
        const auto value=state.source->items[offset]->value;
        if (predicate) {
            const std::array<Value,3> actual{value,static_cast<std::int32_t>(matrix ? row : candidate),static_cast<std::int32_t>(column)};
            if (!state.callable) throw runtime_error(RuntimeError::nil_reference,values.location);
            const auto count=type_info(values.program,state.callable->type).parameters.size();
            if (!std::get<bool>(values.invoke_value(state.callable,std::span<const Value>(actual).first(count)))) continue;
        }
        const auto element=type_info(values.program,sequence->type).element;
        if (state.view_method==ArrayViewMethod::by_row || state.view_method==ArrayViewMethod::by_column) cursor.current=values.copy(value);
        else if (!matrix) cursor.current=static_cast<std::int32_t>(candidate);
        else if (state.view_method==ArrayViewMethod::with_indices) {
            const std::array<Value,3> actual{value,static_cast<std::int32_t>(row),static_cast<std::int32_t>(column)};
            cursor.current=tuple(values,element,actual);
        } else {
            const std::array<Value,2> actual{static_cast<std::int32_t>(row),static_cast<std::int32_t>(column)};
            cursor.current=tuple(values,element,actual);
        }
        cursor.positioned=true; cursor.available=true; cursor.index=index; return true;
    }
    cursor.positioned=true; cursor.available=false; cursor.index=index; return false;
}
}

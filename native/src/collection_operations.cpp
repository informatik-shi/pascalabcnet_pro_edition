#include "collection_operations.hpp"
#include "sequences.hpp"
#include <array>
#include <algorithm>

namespace abc {
namespace {
void require(const std::shared_ptr<Aggregate>& source,SourceLocation location) {
    if (!source || source->nil) throw runtime_error(RuntimeError::nil_reference,location);
}
void work(Values& values,std::uint64_t& used,const ExecutionOptions& options) {
    if (used>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
    if ((used&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
    ++used;
}
}
bool collection_signature_valid(const Program& program,Type source,Type result,Type callback,CollectionOperation method) {
    if (static_cast<unsigned>(method)>static_cast<unsigned>(CollectionOperation::replace) || !compound(source)) return false;
    const auto& input=type_info(program,source); const bool list=input.kind==CompoundKind::list,sequence=input.kind==CompoundKind::sequence;
    if ((input.kind!=CompoundKind::array && !list && !sequence) || (input.rank!=1 && input.rank!=2)
        || ((list || sequence) && input.rank!=1) || (sequence && method!=CollectionOperation::for_each)) return false;
    if (method==CollectionOperation::replace) {
        if (result!=source || input.rank!=1 || sequence || !compound(callback)) return false;
        const auto& pair=type_info(program,callback);
        return pair.kind==CompoundKind::tuple && pair.fields.size()==2 && pair.fields[0].type==input.element && pair.fields[1].type==input.element;
    }
    const bool action=method==CollectionOperation::for_each,convert=method==CollectionOperation::convert_all;
    if (action) { if (result!=Type::unit) return false; }
    else if (convert) {
        if (!compound(result)) return false;
        const auto& output=type_info(program,result);
        if (output.kind!=input.kind || output.rank!=input.rank) return false;
    } else {
        const auto expected=list || (method==CollectionOperation::transform && input.rank==1) ? source : Type::unit;
        if (result!=expected || (method==CollectionOperation::fill_value && input.rank==2)) return false;
    }
    if (method==CollectionOperation::fill_value) return callback==Type::invalid;
    if (!compound(callback)) return false;
    const auto& signature=type_info(program,callback); const auto count=signature.parameters.size();
    const auto output=action ? Type::unit : convert ? type_info(program,result).element : input.element;
    if (signature.kind!=CompoundKind::callable || signature.element!=output
        || std::any_of(signature.parameters.begin(),signature.parameters.end(),[](const auto& p){return p.by_reference;})) return false;
    if (method==CollectionOperation::fill_function) {
        return count==input.rank && std::all_of(signature.parameters.begin(),signature.parameters.end(),[](const auto& p){return p.type==Type::integer;});
    }
    if ((count!=1 && count!=input.rank+1) || signature.parameters[0].type!=input.element || (convert && list && count!=1)) return false;
    for (std::size_t i=1;i<count;++i) if (signature.parameters[i].type!=Type::integer) return false;
    return true;
}
Value collection_operation(Values& values,Type type,const std::shared_ptr<Aggregate>& source,CollectionOperation method,const Value& argument,const ExecutionOptions& options) {
    if (!source) throw runtime_error(RuntimeError::nil_reference,values.location);
    const auto& input=type_info(values.program,source->type);
    const bool list=input.kind==CompoundKind::list,sequence=input.kind==CompoundKind::sequence,matrix=input.rank==2;
    if (method==CollectionOperation::replace) {
        require(source,values.location);const auto pair=std::get<std::shared_ptr<Aggregate>>(argument);
        const auto old_value=pair->items[0]->value,new_value=pair->items[1]->value;std::uint64_t used=0;
        for (const auto& cell:source->items) {
            work(values,used,options);
            if (values.equal(cell->value,old_value)) values.write(cell,new_value);
        }
        return source;
    }
    const bool fill=method==CollectionOperation::fill_value,generator=method==CollectionOperation::fill_function;
    const bool convert=method==CollectionOperation::convert_all,action=method==CollectionOperation::for_each;
    const auto callback=fill ? std::shared_ptr<Aggregate>{} : std::get<std::shared_ptr<Aggregate>>(argument);
    std::uint64_t used=0;
    // Native List.ConvertAll is an instance method; other checks follow Pascal bodies.
    if (convert && list) require(source,values.location);
    const bool eager=!fill && (convert || action || (generator ? list || matrix : !list));
    if (eager) (void)values.callable_target(callback);
    if (sequence) {
        auto cursor=sequence_begin(values,source); std::uint64_t index=0;
        const auto count=type_info(values.program,callback->type).parameters.size();
        while (sequence_resume(values,cursor,index,options)) {
            work(values,used,options);
            if (count==2 && index>INT32_MAX) throw runtime_error(RuntimeError::integer_overflow,values.location);
            const std::array<Value,2> actual{sequence_at(values,cursor,static_cast<std::int64_t>(index),options),static_cast<std::int32_t>(index)};
            (void)values.invoke_value(callback,std::span<const Value>(actual).first(count));
            // The source indexed overload increments after the action, even after its last element.
            if (index==static_cast<std::uint64_t>(INT64_MAX) || (count==2 && index==INT32_MAX)) throw runtime_error(RuntimeError::integer_overflow,values.location);
            ++index;
        }
        return std::int32_t{0};
    }
    require(source,values.location);
    const auto bound=source->items.size();
    auto output=convert ? matrix ? values.matrix(type,static_cast<std::int32_t>(source->rows),static_cast<std::int32_t>(source->columns)) : values.array(type,bound) : source;
    const auto count=fill ? 0U : type_info(values.program,callback->type).parameters.size();
    // List.ConvertAll follows the live Count but keeps the initial destination capacity.
    for (std::size_t i=0;i<(convert && list ? source->items.size() : bound);++i) {
        work(values,used,options);
        if (i>=source->items.size()) throw runtime_error(RuntimeError::index_bounds,values.location);
        Value value=argument;
        if (!fill) {
            const auto row=matrix ? i/source->columns : i,column=matrix ? i%source->columns : 0;
            std::array<Value,3> actual;
            if (generator) { actual[0]=static_cast<std::int32_t>(row); actual[1]=static_cast<std::int32_t>(column); }
            else { actual[0]=source->items[i]->value; actual[1]=static_cast<std::int32_t>(row); actual[2]=static_cast<std::int32_t>(column); }
            value=values.invoke_value(callback,std::span<const Value>(actual).first(count));
        }
        if (!action) {
            // A callback may clear a List; resolve the target after returning from it.
            if (i>=output->items.size()) throw runtime_error(RuntimeError::index_bounds,values.location);
            values.write(output->items[i],value);
        }
    }
    if (convert && list) {
        output->items.resize(source->items.size());
        for (const auto& cell:output->items) values.own_list_cell(cell,output);
    }
    return type==Type::unit ? Value{std::int32_t{0}} : Value{output};
}
}

#include "generators.hpp"
#include "sequence_guard.hpp"
#include <array>
#include <algorithm>

namespace abc {
namespace {
std::int32_t checked(std::int64_t value,SourceLocation location) {
    if (value<INT32_MIN || value>INT32_MAX) throw runtime_error(RuntimeError::integer_overflow,location);
    return static_cast<std::int32_t>(value);
}
void work(std::uint64_t index,SourceLocation location,const ExecutionOptions& options) {
    if (index>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,location);
    if ((index&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,location);
}
}
bool generator_signature_valid(const Program& program,Type result,Type callback,GeneratorMethod method) {
    if (static_cast<unsigned>(method)>static_cast<unsigned>(GeneratorMethod::while_two) || !compound(result) || !compound(callback)) return false;
    const auto& output=type_info(program,result); const auto& signature=type_info(program,callback);
    if (output.kind!=(generator_sequence(method) ? CompoundKind::sequence : CompoundKind::array)
        || output.rank!=(method==GeneratorMethod::matrix ? 2U : 1U) || signature.kind!=CompoundKind::callable || signature.element!=output.element) return false;
    const auto count=method==GeneratorMethod::matrix || generator_pair(method) ? 2U : 1U;
    const auto input=generator_index(method) || method==GeneratorMethod::matrix ? Type::integer : output.element;
    return signature.parameters.size()==count && std::all_of(signature.parameters.begin(),signature.parameters.end(),[&](const auto& p){return !p.by_reference && p.type==input;});
}
Value generate_values(Values& values,Type type,GeneratorMethod method,std::span<const Value> arguments,const ExecutionOptions& options,std::shared_ptr<Aggregate> predicate) {
    const auto element=type_info(values.program,type).element;
    const auto callback=std::get<std::shared_ptr<Aggregate>>(arguments[generator_index(method) ? 1 : arguments.size()-1]);
    if (generator_index(method)) {
        const auto count=std::get<std::int32_t>(arguments[0]), from=std::get<std::int32_t>(arguments[2]);
        if (count<0) throw runtime_error(RuntimeError::sequence_count_range,values.location);
        (void)values.callable_target(callback);
        if (method==GeneratorMethod::index_sequence) {
            // Source spells count + from - 1: both checked intermediate operations matter.
            (void)checked(std::int64_t(checked(std::int64_t(count)+from,values.location))-1,values.location);
            auto result=sequence_composed(values,type,{},SequenceProvider::generated,options);
            result->sequence->generator_method=method; result->sequence->generator_from=from;
            result->sequence->count=static_cast<std::uint64_t>(count); result->sequence->callable=callback; return result;
        }
        auto result=values.array(type,static_cast<std::size_t>(count));
        for (std::int32_t i=0;i<count;++i) {
            work(static_cast<std::uint64_t>(i),values.location,options);
            const std::array<Value,1> actual{checked(std::int64_t(i)+from,values.location)};
            values.write(result->items[static_cast<std::size_t>(i)],values.invoke_value(callback,actual));
        }
        return result;
    }
    if (method==GeneratorMethod::matrix) {
        const auto rows=std::get<std::int32_t>(arguments[0]),columns=std::get<std::int32_t>(arguments[1]);
        auto result=values.matrix(type,rows,columns); std::uint64_t index=0;
        if (!rows || !columns) return result;
        for (std::int32_t row=0;row<rows;++row) for (std::int32_t column=0;column<columns;++column) {
            work(index,values.location,options); const std::array<Value,2> actual{row,column};
            values.write(result->items[static_cast<std::size_t>(index++)],values.invoke_value(callback,actual));
        }
        return result;
    }
    const bool bounded=generator_bounded(method),array=method==GeneratorMethod::array_one || method==GeneratorMethod::array_two,pair=generator_pair(method);
    const auto count=bounded || array ? std::get<std::int32_t>(arguments[0]) : 0;
    if ((bounded || array) && count<(array && pair ? 2 : 1))
        throw runtime_error(array && pair ? RuntimeError::generator_pair_count : RuntimeError::generator_positive_count,values.location);
    const auto first=bounded || array ? 1U : 0U;
    if (array) {
        auto result=values.array(type,static_cast<std::size_t>(count));
        values.write(result->items[0],arguments[first]); if (pair) values.write(result->items[1],arguments[first+1]);
        for (std::int32_t i=pair ? 2 : 1;i<count;++i) {
            work(static_cast<std::uint64_t>(i),values.location,options);
            std::array<Value,2> actual;
            if (pair) { actual[0]=result->items[static_cast<std::size_t>(i-2)]->value; actual[1]=result->items[static_cast<std::size_t>(i-1)]->value; }
            else actual[0]=result->items[static_cast<std::size_t>(i-1)]->value;
            values.write(result->items[static_cast<std::size_t>(i)],values.invoke_value(callback,std::span<const Value>(actual).first(pair ? 2U : 1U)));
        }
        return result;
    }
    auto result=sequence_composed(values,type,{},SequenceProvider::generated,options); auto& state=*result->sequence;
    state.generator_method=generator_while(method) ? (pair ? GeneratorMethod::iterate_two : GeneratorMethod::iterate_one) : method;
    state.callable=callback; state.count=static_cast<std::uint64_t>(count);
    state.seed=values.cell(element,values.copy(arguments[first]));
    if (pair) state.second_seed=values.cell(element,values.copy(arguments[first+1]));
    if (generator_while(method)) return sequence_transform(values,type,result,0,SequenceTransform::take_while,options,std::move(predicate));
    return result;
}
bool generator_resume(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options) {
    SequenceDepthGuard guard(values,options); const auto& state=*sequence->sequence;
    if (!state.generator) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    auto& cursor=*state.generator;
    if (cursor.positioned && index==cursor.index) return cursor.available;
    if ((!cursor.positioned && index) || (cursor.positioned && (!cursor.available || index<=cursor.index || index-cursor.index!=1)))
        throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    if (index>static_cast<std::uint64_t>(INT64_MAX)) throw runtime_error(RuntimeError::integer_overflow,values.location);
    work(index,values.location,options);
    const bool indexed=state.generator_method==GeneratorMethod::index_sequence,pair=generator_pair(state.generator_method);
    if ((indexed || generator_bounded(state.generator_method)) && index>=state.count) {
        cursor.positioned=true; cursor.index=index; cursor.available=false; return false;
    }
    if (!cursor.positioned) {
        (void)values.callable_target(state.callable);
        if (!indexed) { cursor.first=values.copy(state.seed->value); if (pair) cursor.second=values.copy(state.second_seed->value); }
    }
    if (indexed) {
        const std::array<Value,1> actual{checked(std::int64_t(state.generator_from)+static_cast<std::int64_t>(index),values.location)};
        cursor.current=values.invoke_value(state.callable,actual);
    } else if (!index) cursor.current=values.copy(cursor.first);
    else if (pair && index==1) cursor.current=values.copy(cursor.second);
    else {
        if (pair && index>2) { cursor.first=values.copy(cursor.second); cursor.second=std::move(cursor.pending); }
        const std::array<Value,2> actual{cursor.first,cursor.second};
        auto next=values.invoke_value(state.callable,std::span<const Value>(actual).first(pair ? 2U : 1U));
        if (pair) { cursor.pending=std::move(next); cursor.current=values.copy(cursor.pending); }
        else { cursor.first=std::move(next); cursor.current=values.copy(cursor.first); }
    }
    cursor.positioned=true; cursor.available=true; cursor.index=index; return true;
}
}

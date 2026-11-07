#include "matrix_construction.hpp"
#include "sequences.hpp"
#include "sequence_scope.hpp"
#include <algorithm>

namespace abc {
namespace {
void require(const std::shared_ptr<Aggregate>& value,SourceLocation location) {
    if (!value || value->nil) throw runtime_error(RuntimeError::nil_reference,location);
}
void work(Values& values,std::uint64_t& used,const ExecutionOptions& options) {
    if (used>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
    if ((used&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
    ++used;
}
std::int32_t count(Values& values,const std::shared_ptr<Aggregate>& source,const ExecutionOptions& options) {
    return std::get<std::int32_t>(sequence_collect(values,source,Type::invalid,options));
}
template<class F> void visit(Values& values,const std::shared_ptr<Aggregate>& source,std::uint64_t& used,const ExecutionOptions& options,F action) {
    require(source,values.location);
    if (type_info(values.program,source->type).kind==CompoundKind::array) {
        for (std::size_t i=0;i<source->items.size();++i) {
            work(values,used,options);
            if (!action(source->items[i]->value,i)) return;
        }
        return;
    }
    auto cursor=sequence_begin(values,source,options);
    return sequence_finally(values,cursor,options,[&]() -> void {
    for (std::uint64_t i=0;sequence_resume(values,cursor,i,options);++i) {
        work(values,used,options);
        if (i>static_cast<std::uint64_t>(INT64_MAX)) throw runtime_error(RuntimeError::integer_overflow,values.location);
        auto value=sequence_at(values,cursor,static_cast<std::int64_t>(i),options);
        if (!action(value,i)) return;
    }
    });
}
}
bool matrix_build_signature_valid(const Program& program,Type result,Type input,MatrixBuild method) {
    if (static_cast<unsigned>(method)>static_cast<unsigned>(MatrixBuild::columns_flat) || !compound(result) || !compound(input)) return false;
    const auto& matrix=type_info(program,result);const auto& source=type_info(program,input);
    if (matrix.kind!=CompoundKind::array || matrix.rank!=2 || source.rank!=1) return false;
    if (matrix_build_flat(method)) return source.element==matrix.element && source.kind==(method==MatrixBuild::flat_values ? CompoundKind::array : CompoundKind::sequence);
    const bool jagged=method==MatrixBuild::jagged_values;
    const bool sequences=method==MatrixBuild::rows_sequences || method==MatrixBuild::columns_sequences;
    if ((jagged && source.kind!=CompoundKind::array) || (sequences && source.kind!=CompoundKind::sequence)
        || (!jagged && !sequences && source.kind!=CompoundKind::array && source.kind!=CompoundKind::sequence) || !compound(source.element)) return false;
    const auto& line=type_info(program,source.element);
    return line.rank==1 && line.kind==(sequences ? CompoundKind::sequence : CompoundKind::array) && line.element==matrix.element;
}
Value matrix_build(Values& values,Type type,MatrixBuild method,std::span<const Value> arguments,const ExecutionOptions& options) {
    std::uint64_t used=0;
    if (matrix_build_flat(method)) {
        const auto rows=std::get<std::int32_t>(arguments[0]),columns=std::get<std::int32_t>(arguments[1]);
        const auto source=std::get<std::shared_ptr<Aggregate>>(arguments[2]);
        if (method==MatrixBuild::flat_values) {
            require(source,values.location);
            const auto size=static_cast<std::int64_t>(rows)*columns;
            if (size<INT32_MIN || size>INT32_MAX) throw runtime_error(RuntimeError::integer_overflow,values.location);
            if (static_cast<std::int64_t>(source->items.size())!=size) throw runtime_error(RuntimeError::matrix_initializers,values.location);
        }
        auto result=values.matrix(type,rows,columns,true);
        const bool column=method==MatrixBuild::columns_flat;
        visit(values,source,used,options,[&](const Value& value,std::uint64_t i) {
            // The Pascal foreach fetches Current before testing the capacity.
            if (i>=result->items.size()) return false;
            const auto index=static_cast<std::size_t>(i);
            const auto offset=column ? (index%result->rows)*result->columns+index/result->rows : index;
            values.write(result->items[offset],value);return true;
        });
        return result;
    }
    const auto source=std::get<std::shared_ptr<Aggregate>>(arguments[0]);
    require(source,values.location);
    const bool column=method==MatrixBuild::columns_arrays || method==MatrixBuild::columns_sequences;
    const bool sequences=method==MatrixBuild::rows_sequences || method==MatrixBuild::columns_sequences;
    const bool jagged=method==MatrixBuild::jagged_values;
    const bool array=type_info(values.program,source->type).kind==CompoundKind::array;
    std::int32_t lines=0,length=0;
    if (!sequences && !jagged) {
        visit(values,source,used,options,[&](const Value& value,std::uint64_t) { require(std::get<std::shared_ptr<Aggregate>>(value),values.location);return true; });
    }
    if (array) {
        if (source->items.size()>INT32_MAX) throw runtime_error(RuntimeError::integer_overflow,values.location);
        lines=static_cast<std::int32_t>(source->items.size());
    } else lines=count(values,source,options);
    if (jagged && !lines) throw runtime_error(RuntimeError::sequence_empty,values.location);
    if (sequences && lines) {
        const auto first=std::get<std::shared_ptr<Aggregate>>(sequence_query(values,source,SequenceQuery::first,std::int32_t{0},options));
        length=count(values,first,options);
    } else if (!sequences && lines) {
        bool seen=false;
        visit(values,source,used,options,[&](const Value& value,std::uint64_t) {
            const auto row=std::get<std::shared_ptr<Aggregate>>(value);require(row,values.location);
            if (row->items.size()>INT32_MAX) throw runtime_error(RuntimeError::integer_overflow,values.location);
            seen=true;length=std::max(length,static_cast<std::int32_t>(row->items.size()));return true;
        });
        if (!seen) throw runtime_error(RuntimeError::sequence_empty,values.location);
    }
    auto result=values.matrix(type,column ? length : lines,column ? lines : length,true);
    visit(values,source,used,options,[&](const Value& value,std::uint64_t line) {
        const auto row=std::get<std::shared_ptr<Aggregate>>(value);
        visit(values,row,used,options,[&](const Value& item,std::uint64_t position) {
            // Sources can produce different shapes on each enumeration.
            if (line>=static_cast<std::uint64_t>(lines) || position>=static_cast<std::uint64_t>(length)) throw runtime_error(RuntimeError::index_bounds,values.location);
            const auto r=static_cast<std::size_t>(column ? position : line),c=static_cast<std::size_t>(column ? line : position);
            values.write(result->items[r*result->columns+c],item);return true;
        });
        if (line==INT32_MAX) throw runtime_error(RuntimeError::integer_overflow,values.location);
        return true;
    });
    return result;
}
}

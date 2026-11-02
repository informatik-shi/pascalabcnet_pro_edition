#include "matrix_operations.hpp"
#include "sequence_guard.hpp"
#include <array>

namespace abc {
namespace {
void require(const std::shared_ptr<Aggregate>& source,SourceLocation location) {
    if (!source || source->nil) throw runtime_error(RuntimeError::nil_reference,location);
}
void check_index(const std::shared_ptr<Aggregate>& source,std::int32_t index,bool column,SourceLocation location) {
    const auto error=column ? RuntimeError::matrix_column_index : RuntimeError::matrix_row_index;
    if (index<0) throw runtime_error(error,location);
    require(source,location);
    if (static_cast<std::size_t>(index)>=(column ? source->columns : source->rows)) throw runtime_error(error,location);
}
void work(Values& values,std::uint64_t& used,const ExecutionOptions& options) {
    if (used>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
    if ((used&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
    ++used;
}
std::size_t offset(const Aggregate& source,bool column,std::size_t line,std::size_t position) {
    return column ? position*source.columns+line : line*source.columns+position;
}
std::shared_ptr<Aggregate> lazy(Values& values,Type type,const std::shared_ptr<Aggregate>& source,MatrixMethod method,std::int32_t index,const ExecutionOptions& options) {
    auto result=sequence_composed(values,type,source,SequenceProvider::matrix_lines,options);
    result->sequence->matrix_method=method; result->sequence->matrix_index=index; return result;
}
}
bool matrix_signature_valid(const Program& program,Type source,Type result,Type array,MatrixMethod method) {
    if (static_cast<unsigned>(method)>static_cast<unsigned>(MatrixMethod::set_column) || !compound(source) || !compound(array)) return false;
    const auto& input=type_info(program,source); const auto& line=type_info(program,array);
    if (input.kind!=CompoundKind::array || input.rank!=2 || line.kind!=CompoundKind::array || line.rank!=1 || line.element!=input.element) return false;
    if (matrix_swap(method) || matrix_set(method)) return result==Type::unit;
    if (method==MatrixMethod::row || method==MatrixMethod::column) return result==array;
    if (!compound(result)) return false;
    const auto& output=type_info(program,result);
    if (output.rank!=1 || output.kind!=(matrix_sequence(method) ? CompoundKind::sequence : CompoundKind::array)) return false;
    if (!matrix_sequence(method)) return output.element==array;
    if (!matrix_outer(method)) return output.element==input.element;
    if (!compound(output.element)) return false;
    const auto& nested=type_info(program,output.element);
    return nested.kind==CompoundKind::sequence && nested.rank==1 && nested.element==input.element;
}
Value matrix_operation(Values& values,Type type,Type array_type,const std::shared_ptr<Aggregate>& source,MatrixMethod method,std::span<const Value> arguments,const ExecutionOptions& options) {
    const bool column=matrix_column(method),outer=matrix_outer(method);
    const auto index=outer ? 0 : std::get<std::int32_t>(arguments[0]);
    if (matrix_sequence(method)) return lazy(values,type,source,method,index,options);
    std::uint64_t used=0;
    if (matrix_set(method)) {
        const auto replacement=std::get<std::shared_ptr<Aggregate>>(arguments[1]);
        require(replacement,values.location); // Source checks a before validating k or Self.
        check_index(source,index,column,values.location);
        const auto length=column ? source->rows : source->columns;
        if (replacement->items.size()!=length) throw runtime_error(RuntimeError::matrix_line_length,values.location);
        for (std::size_t i=0;i<length;++i) {
            work(values,used,options); values.write(source->items[offset(*source,column,static_cast<std::size_t>(index),i)],replacement->items[i]->value);
        }
        return std::int32_t{0};
    }
    if (matrix_swap(method)) {
        const auto second=std::get<std::int32_t>(arguments[1]);
        check_index(source,index,column,values.location); check_index(source,second,column,values.location);
        const auto length=column ? source->rows : source->columns;
        for (std::size_t i=0;i<length;++i) {
            work(values,used,options);
            const auto& a=source->items[offset(*source,column,static_cast<std::size_t>(index),i)];
            const auto& b=source->items[offset(*source,column,static_cast<std::size_t>(second),i)];
            const auto saved=values.copy(a->value); values.write(a,b->value); values.write(b,saved);
        }
        return std::int32_t{0};
    }
    if (outer) {
        require(source,values.location);
        const auto count=column ? source->columns : source->rows,length=column ? source->rows : source->columns;
        auto result=values.array(type,count);
        // Original Rows/Cols allocates every row/column before copying any elements.
        for (std::size_t i=0;i<count;++i) { work(values,used,options); result->items[i]->value=values.array(array_type,length); }
        for (std::size_t i=0;i<count;++i) {
            const auto row=std::get<std::shared_ptr<Aggregate>>(result->items[i]->value);
            for (std::size_t j=0;j<length;++j) {
                work(values,used,options); values.write(row->items[j],source->items[offset(*source,column,i,j)]->value);
            }
        }
        return result;
    }
    check_index(source,index,column,values.location);
    const auto length=column ? source->rows : source->columns;
    auto result=values.array(type,length);
    for (std::size_t i=0;i<length;++i) {
        work(values,used,options); values.write(result->items[i],source->items[offset(*source,column,static_cast<std::size_t>(index),i)]->value);
    }
    return result;
}
bool matrix_line_resume(Values& values,const std::shared_ptr<Aggregate>& sequence,std::uint64_t index,const ExecutionOptions& options) {
    SequenceDepthGuard guard(values,options); const auto& state=*sequence->sequence;
    if (!state.matrix_line) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    auto& cursor=*state.matrix_line;
    if (cursor.positioned && index==cursor.index) return cursor.available;
    if ((!cursor.positioned && index) || (cursor.positioned && (!cursor.available || index<=cursor.index || index-cursor.index!=1)))
        throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    if (index>static_cast<std::uint64_t>(INT64_MAX)) throw runtime_error(RuntimeError::integer_overflow,values.location);
    if (index>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
    if ((index&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
    const bool column=matrix_column(state.matrix_method),outer=matrix_outer(state.matrix_method);
    if (!cursor.initialized) {
        if (outer) require(state.source,values.location);
        else check_index(state.source,state.matrix_index,column,values.location);
        cursor.bound=outer ? column ? state.source->columns : state.source->rows : column ? state.source->rows : state.source->columns;
        cursor.initialized=true;
    }
    cursor.positioned=true; cursor.index=index; cursor.available=index<cursor.bound;
    if (!cursor.available) return false;
    if (outer) {
        cursor.current=lazy(values,type_info(values.program,sequence->type).element,state.source,column ? MatrixMethod::column_sequence : MatrixMethod::row_sequence,static_cast<std::int32_t>(index),options);
    } else cursor.current=values.copy(state.source->items[offset(*state.source,column,static_cast<std::size_t>(state.matrix_index),static_cast<std::size_t>(index))]->value);
    return true;
}
}

#include "array_operations.hpp"
#include <array>

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
bool array_operation_signature_valid(const Program& program,Type array,Type result,ArrayOperation method) {
    if (static_cast<unsigned>(method)>static_cast<unsigned>(ArrayOperation::equal_matrix) || !compound(array)) return false;
    const auto& input=type_info(program,array);
    const auto rank=method==ArrayOperation::fill_array || method==ArrayOperation::equal_array ? 1U : 2U;
    if (input.kind!=CompoundKind::array || input.rank!=rank) return false;
    if (method==ArrayOperation::equal_array || method==ArrayOperation::equal_matrix) return result==Type::boolean;
    if (method!=ArrayOperation::size) return result==array;
    if (!compound(result)) return false;
    const auto& tuple=type_info(program,result);
    return tuple.kind==CompoundKind::tuple && tuple.fields.size()==2
        && tuple.fields[0].type==Type::integer && tuple.fields[1].type==Type::integer;
}
Value array_operation(Values& values,Type array,Type result_type,ArrayOperation method,std::span<const Value> arguments,const ExecutionOptions& options) {
    std::uint64_t used=0;
    if (method==ArrayOperation::fill_array || method==ArrayOperation::fill_matrix) {
        const bool matrix=method==ArrayOperation::fill_matrix;
        const auto count=std::get<std::int32_t>(arguments[0]);
        if (!matrix && count<0) throw runtime_error(RuntimeError::array_length,values.location);
        auto result=matrix ? values.matrix(array,count,std::get<std::int32_t>(arguments[1])) : values.array(array,static_cast<std::size_t>(count));
        const auto& value=arguments[matrix ? 2 : 1];
        for (const auto& cell:result->items) { work(values,used,options); values.write(cell,value); }
        return result;
    }
    const auto source=std::get<std::shared_ptr<Aggregate>>(arguments[0]);
    if (method==ArrayOperation::equal_array || method==ArrayOperation::equal_matrix) {
        const auto other=std::get<std::shared_ptr<Aggregate>>(arguments[1]);
        const bool a=!source || source->nil,b=!other || other->nil;
        if (method==ArrayOperation::equal_matrix && source==other) return true;
        if (a || b) return a==b;
        if (method==ArrayOperation::equal_matrix && (source->rows!=other->rows || source->columns!=other->columns)) return false;
        if (source->items.size()!=other->items.size()) return false;
        // ArrEqual has no identity shortcut: it still visits a shared array.
        for (std::size_t i=0;i<source->items.size();++i) {
            work(values,used,options); if (!values.equal(source->items[i]->value,other->items[i]->value)) return false;
        }
        return true;
    }
    require(source,values.location);
    if (method==ArrayOperation::size) {
        auto result=std::make_shared<Aggregate>(); result->type=result_type;
        result->items.push_back(values.cell(Type::integer,static_cast<std::int32_t>(source->rows)));
        result->items.push_back(values.cell(Type::integer,static_cast<std::int32_t>(source->columns)));
        return result;
    }
    auto result=values.matrix(array,static_cast<std::int32_t>(source->columns),static_cast<std::int32_t>(source->rows));
    if (!source->rows || !source->columns) return result;
    for (std::size_t row=0;row<result->rows;++row) for (std::size_t column=0;column<result->columns;++column) {
        work(values,used,options); values.write(result->items[row*result->columns+column],source->items[column*source->columns+row]->value);
    }
    return result;
}
}

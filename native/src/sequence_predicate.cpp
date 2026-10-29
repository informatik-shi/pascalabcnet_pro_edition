#include "sequences.hpp"
#include "runtime_values.hpp"
#include <array>

namespace abc {
Value sequence_predicate(Values& values,const std::shared_ptr<Aggregate>& source,std::size_t callback,PredicateQuery method,const ExecutionOptions& options) {
    if (!source || source->nil) throw runtime_error(RuntimeError::nil_reference,values.location);
    if (!values.invoke || callback>=values.program.routines.size()) throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    const auto iterator=sequence_begin(values,source);
    std::int32_t count=0; std::optional<Value> kept;
    for (std::uint64_t index=0;;++index) {
        if (index>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
        if ((index&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
        if (index>static_cast<std::uint64_t>(INT64_MAX)) throw runtime_error(RuntimeError::integer_overflow,values.location);
        if (!sequence_resume(values,iterator,index,options)) break;
        auto item=sequence_at(values,iterator,static_cast<std::int64_t>(index),options);
        const std::array<Value,1> arguments={item}; const auto matched=std::get<bool>(values.invoke(callback,arguments));
        if (method==PredicateQuery::all) { if (!matched) return false; continue; }
        if (!matched) continue;
        if (method==PredicateQuery::any) return true;
        if (method==PredicateQuery::count) { if (count==INT32_MAX) throw runtime_error(RuntimeError::integer_overflow,values.location); ++count; continue; }
        if (method==PredicateQuery::first || method==PredicateQuery::first_default) return item;
        if (kept && (method==PredicateQuery::single || method==PredicateQuery::single_default)) throw runtime_error(RuntimeError::sequence_multiple_matches,values.location);
        kept=std::move(item);
    }
    if (method==PredicateQuery::any) return false;
    if (method==PredicateQuery::all) return true;
    if (method==PredicateQuery::count) return count;
    if (kept) return std::move(*kept);
    if (method==PredicateQuery::first_default || method==PredicateQuery::last_default || method==PredicateQuery::single_default)
        return values.zero(type_info(values.program,source->type).element,false,true);
    throw runtime_error(RuntimeError::sequence_no_match,values.location);
}
} // namespace abc

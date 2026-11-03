#include "sequences.hpp"
#include "runtime_values.hpp"
#include "formatting.hpp"

namespace abc {
std::u16string sequence_join(Values& values,const std::shared_ptr<Aggregate>& source,const Value& delimiter,const ExecutionOptions& options) {
    auto cursor=sequence_begin(values,source);const auto element=type_info(values.program,source->type).element;
    std::u16string result;std::uint64_t used=0;bool first=true;
    const auto work=[&] {
        if (used>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
        if ((used&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
        ++used;
    };
    const auto append=[&](std::u16string_view part) {
        if (part.size()>1'048'576-result.size()) throw runtime_error(RuntimeError::string_limit,values.location);
        for (std::size_t i=0;i<part.size();++i) work();
        result.append(part);
    };
    for (std::uint64_t i=0;sequence_resume(values,cursor,i,options);++i) {
        work();
        if (i>static_cast<std::uint64_t>(INT64_MAX)) throw runtime_error(RuntimeError::integer_overflow,values.location);
        const auto value=sequence_at(values,cursor,static_cast<std::int64_t>(i),options);
        if (!first) append(string_text(delimiter));
        // Convert.ToString(null) is empty; ObjectToString(null) is the text nil.
        if (!std::holds_alternative<NilString>(value)) append(object_text(values.program,value,element,values.location,options));
        first=false;
    }
    return result;
}
}

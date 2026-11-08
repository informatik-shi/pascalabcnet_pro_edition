#include "sequences.hpp"
#include "sequence_scope.hpp"
#include "runtime_values.hpp"
#include "equality.hpp"
#include "enumerator.hpp"

namespace abc {
namespace {
Type iterator_type(const Program& p,Type element){for(std::size_t n=0;n<p.types.size();++n)if(p.types[n].kind==CompoundKind::enumerator && p.types[n].element==element)return static_cast<Type>(64+n);return Type::invalid;}
}
bool sequence_equal_signature_valid(const Program& p,Type owner,std::size_t policy){
    if(!compound(owner) || !valid_value_type(p,owner))return false;const auto& shape=type_info(p,owner);if(shape.kind!=CompoundKind::sequence || shape.rank!=1 || iterator_type(p,shape.element)==Type::invalid)return false;
    if(!policy)return true;if(policy<64 || policy>=64+p.types.size())return false;const auto& payload=type_info(p,static_cast<Type>(policy));return payload.kind==CompoundKind::equality_comparer_interface && payload.element==shape.element;
}
bool sequence_equal(Values& values,const std::shared_ptr<Aggregate>& first,const std::shared_ptr<Aggregate>& second,const ExecutionOptions& options,const std::shared_ptr<Aggregate>& comparer) {
    if (!first || first->nil || !second || second->nil) throw runtime_error(RuntimeError::nil_reference,values.location);
    const auto element=type_info(values.program,first->type).element;const auto type=iterator_type(values.program,element);if(type==Type::invalid)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    const auto left=enumerator_invoke(values,first->type,type,EnumeratorMethod::get,first,options);
    return enumerator_finally(values,type,left,options,[&]() -> bool {
    const auto right=enumerator_invoke(values,second->type,type,EnumeratorMethod::get,second,options);
    return enumerator_finally(values,type,right,options,[&]() -> bool {
    const auto move=[&](const Value& it){return std::get<bool>(enumerator_invoke(values,type,Type::invalid,EnumeratorMethod::move,it,options));};
    for (std::uint64_t index=0;;++index) {
        if (index>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,values.location);
        if ((index&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,values.location);
        if (!move(left)) return !move(right);
        if (!move(right)) return false;
        const auto a=enumerator_invoke(values,type,Type::invalid,EnumeratorMethod::current,left,options);
        const auto b=enumerator_invoke(values,type,Type::invalid,EnumeratorMethod::current,right,options);
        if (!equality_policy_value(values,element,comparer,a,b,options)) return false;
    }
    });
    });
}
} // namespace abc

#include "random_sequence.hpp"
#include "runtime_values.hpp"
#include "stdlib.hpp"
#include <new>

namespace abc {
bool random_sequence_signature(const Program& program,Type type,RandomSequenceMethod method){
    return method>=RandomSequenceMethod::integer && method<=RandomSequenceMethod::real && compound(type)
        && static_cast<unsigned>(type)-64U<program.types.size() && type_info(program,type).kind==CompoundKind::sequence
        && type_info(program,type).element==(method==RandomSequenceMethod::integer ? Type::integer : Type::real);
}
std::vector<Type> random_sequence_parameters(RandomSequenceMethod method){
    return method==RandomSequenceMethod::integer ? std::vector<Type>{Type::integer,Type::integer,Type::integer}
        : std::vector<Type>{Type::integer,Type::real,Type::real,Type::integer};
}
namespace {
std::size_t builtin(RandomSequenceMethod method,SourceLocation location){
    const auto catalog=stdlib::signatures();const bool real=method==RandomSequenceMethod::real;
    const auto arguments=real ? std::vector<Type>{Type::real,Type::real,Type::integer} : std::vector<Type>{Type::integer,Type::integer};
    for(std::size_t i=0;i<catalog.size();++i)if(catalog[i].unit.empty() && catalog[i].name==(real ? "randomreal" : "random") && catalog[i].parameters==arguments)return i;
    throw runtime_error(RuntimeError::invalid_bytecode,location);
}
}
std::shared_ptr<Aggregate> random_sequence_create(Values& values,Type type,RandomSequenceMethod method,std::span<const Value> arguments){
    if(!random_sequence_signature(values.program,type,method) || arguments.size()!=random_sequence_parameters(method).size())throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    if(values.remaining<4)throw runtime_error(RuntimeError::allocation_limit,values.location);values.remaining-=4;
    try{
        auto result=std::make_shared<Aggregate>();result->type=type;result->sequence=std::make_unique<SequenceState>();result->sequence->provider=SequenceProvider::random;
        auto state=std::make_shared<RandomSequenceState>();state->builtin=builtin(method,values.location);state->arity=arguments.size()-1;state->current=values.zero(type_info(values.program,type).element);
        const auto count=std::get<std::int32_t>(arguments[0]);state->count=count>0 ? static_cast<std::uint32_t>(count) : 0;
        for(std::size_t i=0;i<state->arity;++i)state->arguments[i]=arguments[i+1];
        result->sequence->random=std::move(state);return result;
    }catch(const std::bad_alloc&){throw runtime_error(RuntimeError::allocation_limit,values.location);}
}
bool random_sequence_resume(Values& values,const std::shared_ptr<Aggregate>& cursor,std::uint64_t index,const ExecutionOptions& options){
    if(!cursor || !cursor->sequence || !cursor->sequence->random)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    auto& state=*cursor->sequence->random;
    if(state.positioned && index==state.index)return state.available;
    if(state.positioned && index<state.index)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    const auto first=state.positioned ? state.index+1 : 0;std::uint64_t work=0;
    for(auto i=first;i<=index;++i){
        if(i>=state.count){state.positioned=true;state.index=i;state.available=false;return false;}
        if(work>=options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,values.location);
        if((work++&255U)==0 && options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,values.location);
        if(!values.invoke_builtin)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
        auto value=values.invoke_builtin(state.builtin,std::span<const Value>(state.arguments.data(),state.arity));
        state.current=std::move(value);state.index=i;state.positioned=true;state.available=true;
    }
    return state.available;
}
}

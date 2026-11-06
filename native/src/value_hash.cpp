#include "value_hash.hpp"
#include "runtime_values.hpp"
#include "ordinal_range.hpp"
#include "real_interval.hpp"
#include "big_integer_runtime.hpp"
#include <cmath>
#include <type_traits>
namespace abc {
void ValueHashWork::step(){if(done>=options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,values.location);if((done++&255U)==0 && options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,values.location);}
namespace {std::size_t mix(std::size_t seed,std::size_t value){return (seed^value)*static_cast<std::size_t>(1099511628211ULL);}}
bool value_equal(ValueHashWork& work,const Value& a,const Value& b){
    work.step();
    if(const auto* x=std::get_if<double>(&a)){const auto y=std::get<double>(b);return *x==y || (std::isnan(*x) && std::isnan(y));}
    if(const auto* x=std::get_if<std::u16string>(&a)){const auto* y=std::get_if<std::u16string>(&b);if(!y || x->size()!=y->size())return false;for(std::size_t i=0;i<x->size();++i){work.step();if((*x)[i]!=(*y)[i])return false;}return true;}
    if(const auto* x=std::get_if<std::shared_ptr<Aggregate>>(&a)){
        const auto& y=std::get<std::shared_ptr<Aggregate>>(b);const auto kind=type_info(work.values.program,(*x)->type).kind;
        if(kind==CompoundKind::record || kind==CompoundKind::tuple){if((*x)->nil || y->nil)return (*x)->nil==y->nil;for(std::size_t i=0;i<(*x)->items.size();++i)if(!value_equal(work,(*x)->items[i]->value,y->items[i]->value))return false;return true;}
    }
    return work.values.equal(a,b);
}
std::size_t value_hash(ValueHashWork& work,const Value& value){
    work.step();
    return std::visit([&](const auto& v)->std::size_t{
        using T=std::decay_t<decltype(v)>;
        if constexpr(std::is_same_v<T,std::u16string>){std::size_t result=1469598103934665603ULL;for(const auto c:v){work.step();result=mix(result,c);}return result;}
        else if constexpr(std::is_same_v<T,NewSetValue>)return value_hash(work,v.storage->value);
        else if constexpr(std::is_same_v<T,NilString>)return 0;
        else if constexpr(std::is_same_v<T,EnumValue>)return std::hash<std::int32_t>{}(v.value);
        else if constexpr(std::is_same_v<T,ReflectedType>)return mix(mix(static_cast<unsigned>(v.target),v.implementation),static_cast<unsigned>(v.source));
        else if constexpr(std::is_same_v<T,BigInteger>){big_work(v.bits()/16+1,work.values.location,work.options);std::size_t result=0;for(const auto c:v.text(16)){work.step();result=mix(result,static_cast<unsigned char>(c));}return result;}
        else if constexpr(std::is_same_v<T,std::shared_ptr<Aggregate>>){
            if(!v || v->nil)return 0;const auto kind=type_info(work.values.program,v->type).kind;
            if(ordinal_range_type(type_info(work.values.program,v->type)))return static_cast<std::uint32_t>(ordinal_range_hash(v));
            if(real_interval_type(type_info(work.values.program,v->type)))return static_cast<std::uint32_t>(real_interval_hash(v));
            if(kind==CompoundKind::record || kind==CompoundKind::tuple){std::size_t result=0;for(const auto& field:v->items)result=mix(result,value_hash(work,field->value));return result;}
            if(kind==CompoundKind::callable){std::size_t result=v->routine.value_or(0);for(const auto& cell:v->captures)result=mix(result,std::hash<const void*>{}(cell.get()));return result;}
            if(kind==CompoundKind::sequence && v->sequence && (collection_sequence(v->sequence->provider) || v->sequence->provider==SequenceProvider::hash_set || dictionary_provider(v->sequence->provider)))return std::hash<const void*>{}(v->sequence->source.get());
            return std::hash<const void*>{}(v.get());
        }else if constexpr(std::is_same_v<T,std::shared_ptr<Cell>> || std::is_same_v<T,std::shared_ptr<TextFile>>)return std::hash<const void*>{}(v.get());
        else if constexpr(std::is_floating_point_v<T>)return std::isnan(v) ? 0x7ff80000U : std::hash<T>{}(v);
        else return std::hash<T>{}(v);
    },value);
}
}

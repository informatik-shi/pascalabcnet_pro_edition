#include "print.hpp"
#include "runtime_values.hpp"
#include "sequences.hpp"
#include "formatting.hpp"
#include <algorithm>
#include <array>
#include <bit>
namespace abc {
Type print_sequence_type(const std::vector<TypeInfo>& types,Type element){for(std::size_t i=0;i<types.size();++i)if(types[i].kind==CompoundKind::sequence && types[i].element==element)return static_cast<Type>(64+i);return Type::invalid;}
bool print_signature(const Program& p,Type self,PrintMethod method,Type callback){
    if(method>PrintMethod::matrix_real || !valid_value_type(p,self))return false;
    if(method==PrintMethod::scalar_print || method==PrintMethod::scalar_println)return callback==Type::invalid && (self==Type::integer || self==Type::int64 || self==Type::real || self==Type::boolean || self==Type::character || self==Type::string || self==Type::big_integer || self==Type::longword || self==Type::uint64);
    if(!compound(self))return false;const auto& type=type_info(p,self);
    if(method==PrintMethod::matrix || method==PrintMethod::matrix_real)return callback==Type::invalid && type.kind==CompoundKind::array && type.rank==2 && (method!=PrintMethod::matrix_real || type.element==Type::real) && object_text_supported(p.types,type.element);
    if(type.kind!=CompoundKind::sequence)return false;
    if(method!=PrintMethod::sequence_map_lines)return callback==Type::invalid && object_text_supported(p.types,type.element);
    if(!compound(callback) || !valid_value_type(p,callback))return false;const auto& fn=type_info(p,callback);
    return fn.kind==CompoundKind::callable && fn.parameters.size()==1 && !fn.parameters[0].by_reference && fn.parameters[0].type==type.element && object_text_supported(p.types,fn.element) && print_sequence_type(p.types,fn.element)!=Type::invalid;
}
Type print_result(Type self,PrintMethod method){return (method==PrintMethod::scalar_print || method==PrintMethod::scalar_println) && (self==Type::longword || self==Type::uint64) ? Type::integer : self;}
std::vector<Type> print_parameters(Type self,PrintMethod method,Type callback){
    if(method==PrintMethod::scalar_print || method==PrintMethod::scalar_println)return {self};
    if(method==PrintMethod::sequence_map_lines)return {self,callback};
    if(method==PrintMethod::matrix)return {self,Type::integer};
    if(method==PrintMethod::matrix_real)return {self,Type::integer,Type::integer};
    return {self,Type::string};
}
Value print_invoke(Values& values,Type self,PrintMethod method,Type callback,std::span<const Value> args,
    const std::function<const Value&()>& delimiter,const std::function<bool()>& matrix_format,
    const std::function<void(std::u16string_view)>& emit,const ExecutionOptions& options){
    if(!print_signature(values.program,self,method,callback) || args.size()!=print_parameters(self,method,callback).size())throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    std::uint64_t used=0;const auto step=[&]{if(used>=options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,values.location);if((used++&255U)==0 && options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,values.location);};
    const auto text=[&](const Value& value,Type type){return object_text(values.program,value,type,values.location,options);};
    const auto separator=[&](const Value& value){if(std::holds_alternative<NilString>(value) || !string_text(value).empty())emit(text(value,Type::string));};
    step();
    if(method==PrintMethod::scalar_print || method==PrintMethod::scalar_println){Value value=args[0];if(self==Type::longword || self==Type::uint64){const auto bits=self==Type::longword ? std::get<std::uint32_t>(value) : static_cast<std::uint32_t>(std::get<std::uint64_t>(value));value=std::bit_cast<std::int32_t>(bits);}emit(text(value,print_result(self,method)));if(method==PrintMethod::scalar_print)separator(delimiter());else emit(u"\n");return value;}
    auto source=std::get<std::shared_ptr<Aggregate>>(args[0]);
    if(method==PrintMethod::matrix || method==PrintMethod::matrix_real){
        if(!source || source->nil)throw runtime_error(RuntimeError::nil_reference,values.location);const auto element=type_info(values.program,self).element;const auto width=std::get<std::int32_t>(args[1]);
        const bool formatted=matrix_format();std::vector<std::int32_t> widths;
        if(method==PrintMethod::matrix_real && formatted){if(source->columns>values.remaining)throw runtime_error(RuntimeError::allocation_limit,values.location);values.remaining-=source->columns;widths.assign(source->columns,width);
            for(std::size_t c=0;c<source->columns;++c){step();for(std::size_t r=0;r<source->rows;++r){step();const auto value=format_field(source->items[r*source->columns+c]->value,0,std::get<std::int32_t>(args[2]),values.location);widths[c]=std::max(widths[c],static_cast<std::int32_t>(value.size()+1));}}
        }
        for(std::size_t r=0;r<source->rows;++r){step();for(std::size_t c=0;c<source->columns;++c){step();const auto& value=source->items[r*source->columns+c]->value;
                if(method==PrintMethod::matrix_real && formatted)emit(pad_field(format_field(value,0,std::get<std::int32_t>(args[2]),values.location),widths[c],values.location));
                else if(method==PrintMethod::matrix && matrix_format())emit(pad_field(text(value,element),width,values.location));
                else{emit(text(value,element));separator(delimiter());}
            }emit(u"\n");}
        return args[0];
    }
    Type element=type_info(values.program,self).element;Value delim;
    if(method==PrintMethod::sequence_map_lines){
        const auto fn=std::get<std::shared_ptr<Aggregate>>(args[1]);element=type_info(values.program,callback).element;source=sequence_transform(values,print_sequence_type(values.program.types,element),source,0,SequenceTransform::select,options,fn);
#ifdef _WIN32
        delim=std::u16string{u"\r\n"};
#else
        delim=std::u16string{u"\n"};
#endif
    }else delim=args[1];
    const auto cursor=sequence_begin(values,source);for(std::uint64_t i=0;sequence_resume(values,cursor,i,options);++i){step();if(i>static_cast<std::uint64_t>(INT64_MAX))throw runtime_error(RuntimeError::integer_overflow,values.location);const auto value=sequence_at(values,cursor,static_cast<std::int64_t>(i),options);if(i)separator(delim);emit(text(value,element));}
    if(method!=PrintMethod::sequence_print)emit(u"\n");return args[0];
}
}

#include "formatting.hpp"
#include "diagnostics.hpp"
#include "stdlib.hpp"
#include "big_integer_runtime.hpp"
#include "school_ip.hpp"
#include "enumeration.hpp"
#include "ordinal_range.hpp"
#include "real_interval.hpp"
#include "complex_number.hpp"
#include "calendar.hpp"
#include "composite_format.hpp"
#include "decimal.hpp"
#include "number_format.hpp"
#include "runtime_values.hpp"
#include "interfaces.hpp"
#include "ini_file.hpp"
#include "string_builder.hpp"
#include "type_names.hpp"
#include <array>
#include <charconv>
#include <type_traits>
#include <algorithm>

namespace abc {
namespace {
std::optional<Value> class_to_string_override(Values& values,const Value& value,Type type) {
    for(;type!=Type::invalid;type=class_parent(values.program.types,type)){
        for(std::size_t id=0;id<values.program.routines.size();++id){
            const auto& method=values.program.routines[id];
            if(method.owner!=type || !method.object_to_string_override)continue;
            if(!values.invoke)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
            const std::array<Value,1> arguments{value};
            return values.invoke(id,arguments);
        }
    }
    return {};
}
bool supports(const std::vector<TypeInfo>& types, Type type, unsigned depth) {
    if (scalar(type) || type==Type::nil) return true;
    if (!compound(type) || depth>32) return false;
    const auto index=static_cast<std::size_t>(type)-static_cast<std::size_t>(Type::first_compound);
    if (index>=types.size()) return false;
    const auto& info=types[index];
    if(dictionary_enumerator_type(info) || collection_enumerator_type(info))return false;
    if(info.kind==CompoundKind::empty_collection || info.kind==CompoundKind::boxed_object
        || info.kind==CompoundKind::object_class || info.kind==CompoundKind::ini_file)return true;
    if(info.kind==CompoundKind::fixed_array)return supports(types,info.element,depth+1);
    if (school_ip_kind(info.kind) || info.kind==CompoundKind::enumeration) return true;
    if (info.kind==CompoundKind::record || info.kind==CompoundKind::tuple) {
        for (const auto& field:info.fields) if (!supports(types,field.type,depth+1)) return false;
        return true;
    }
    return (info.kind==CompoundKind::array || info.kind==CompoundKind::list || info.kind==CompoundKind::hash_set || info.kind==CompoundKind::new_set || info.kind==CompoundKind::dictionary || info.kind==CompoundKind::dictionary_keys || info.kind==CompoundKind::dictionary_values) && supports(types,info.element,depth+1);
}
class ObjectWriter {
    Values& values;
    const Program& program;
    SourceLocation location;
    const ExecutionOptions& options;
    const NumberFormatSettings* settings;
    std::u16string text;
    std::vector<const Aggregate*> path;
    void append(std::u16string_view part) {
        if (part.size()>1'048'576-text.size()) throw runtime_error(RuntimeError::string_limit,location);
        text+=part;
    }
    void class_contents(const std::shared_ptr<Aggregate>& object,const TypeInfo& shape) {
        append(u"(");
        bool has_value=false;
        for(std::size_t i=0;i<shape.fields.size();++i) {
            const auto& field=shape.fields[i];
            if(field.access!=MemberAccess::public_member)continue;
            if(has_value)append(u",");
            has_value=true;
            if(!supports(program.types,field.type,0))
                throw Error("E_UNSUPPORTED",location,"Cannot format this public field type",
                    "Форматирование типа этого открытого поля пока не поддерживается");
            value(object->items[i]->value,field.type);
        }
        if(ini_ancestor(program.types,object->type)!=Type::invalid) {
            if(has_value)append(u",");
            value(ini_file_name(*object),Type::string);
        }
        append(u")");
    }
    void check_collection_version(const Aggregate& source,std::uint64_t version,std::size_t seen,std::size_t size) {
        // The original prefetches MoveNext before rendering Current.
        if(seen<size && version!=source.version)
            throw runtime_error(RuntimeError::collection_changed,location);
    }
    void value(const Value& data, Type type) {
        if (options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,location);
        if (!compound(type)) { append(type==Type::nil ? u"nil" : format_field(data,0,std::nullopt,location)); return; }
        if(enumeration(program,type)){append(enum_text(program,std::get<EnumValue>(data),location));return;}
        const auto& info=program.types[static_cast<std::size_t>(type)-static_cast<std::size_t>(Type::first_compound)];
        if(info.kind==CompoundKind::new_set){
            const auto& set=std::get<NewSetValue>(data);const auto& hash=std::get<std::shared_ptr<Aggregate>>(set.storage->value);
            if(!hash || hash->nil)append(u"{}");else value(set.storage->value,set.storage->type);return;
        }
        const auto object=std::get<std::shared_ptr<Aggregate>>(data);
        if (!object || object->nil) { append(u"nil"); return; }
        if(database_null_type(info))return;
        if (info.kind==CompoundKind::boxed_object) {
            if (object->items.empty()) append(u"()");
            else value(object->items.front()->value,object->items.front()->type);
            return;
        }
        if(info.kind==CompoundKind::fixed_array){value(object->items[0]->value,info.key);return;}
        if(info.kind==CompoundKind::empty_collection){append(u"{}");return;}
        if (std::find(path.begin(),path.end(),object.get())!=path.end()) { append(u"(...)"); return; }
        path.push_back(object.get());
        if(info.kind==CompoundKind::object_class || info.kind==CompoundKind::ini_file){
            const auto& actual=type_info(program,object->type);
            if(const auto rendered=class_to_string_override(values,Value{object},object->type))
                append(string_text(*rendered));
            else class_contents(object,actual);
        }
        else if(ordinal_range_type(info))append(ordinal_range_text(object));
        else if(real_interval_type(info))append(real_interval_text(object,location));
        else if(complex_number_type(info))append(complex_pascal_text(data,settings,location));
        else if(calendar_record(info))append(calendar_text(data,{},location));
        else if(decimal_type(info)) {
            const auto rendered=decimal_text(data);
            append(settings?localize_number(rendered,*settings,location):rendered);
        }
        else if (school_ip_kind(info.kind)) append(school_ip_text(program,object,location));
        else if (info.kind==CompoundKind::record || info.kind==CompoundKind::tuple) {
            append(u"(");
            for (std::size_t i=0;i<info.fields.size();++i) {
                if (i) append(u",");
                value(object->items[i]->value,info.fields[i].type);
            }
            append(u")");
        } else if (info.kind==CompoundKind::array && info.rank==2) {
            if (object->items.empty()) append(u"[[]]");
            else sequence(object->rows,99,[&](std::size_t row) {
                sequence(object->columns,99,[&](std::size_t column) { value(object->items[row*object->columns+column]->value,info.element); });
            });
        } else if(info.kind==CompoundKind::dictionary || info.kind==CompoundKind::dictionary_keys || info.kind==CompoundKind::dictionary_values){
            const bool dictionary=info.kind==CompoundKind::dictionary;const auto owner=dictionary ? object : object->items.size()==1 ? std::get<std::shared_ptr<Aggregate>>(object->items[0]->value) : std::shared_ptr<Aggregate>{};if(!owner || owner->nil)throw runtime_error(RuntimeError::invalid_bytecode,location);
            const auto size=owner->count;
            const auto version=owner->version;
            append(dictionary ? u"{" : u"[");const auto count=size>101 ? 100 : size;std::size_t shown=0;
            for(std::size_t i=0;i<owner->items.size() && shown<count;++i){if(i>=options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,location);if((i&255U)==0 && options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,location);const auto cell=owner->items[i];if(!cell)continue;if(shown++)append(u",");if(dictionary)value(cell->value,info.element);else value(std::get<std::shared_ptr<Aggregate>>(cell->value)->items[info.kind==CompoundKind::dictionary_keys ? 0U : 1U]->value,info.element);check_collection_version(*owner,version,shown,size);}
            if(size>101)append(u",...");append(dictionary ? u"}" : u"]");
        } else if(info.kind==CompoundKind::hash_set){
            const auto size=object->count;
            const auto version=object->version;
            append(u"{");const auto count=size>101 ? 100 : size;std::size_t shown=0;
            for(std::size_t i=0;i<object->items.size() && shown<count;++i){if(i>=options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,location);if((i&255U)==0 && options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,location);const auto cell=object->items[i];if(cell){if(shown++)append(u",");value(cell->value,info.element);check_collection_version(*object,version,shown,size);}}
            if(size>101)append(u",...");append(u"}");
        } else if(info.kind==CompoundKind::list){
            const auto size=object->items.size();
            const auto version=object->version;
            sequence(size,100,[&](std::size_t i) {
                value(object->items[i]->value,info.element);
                check_collection_version(*object,version,i+1,size);
            });
        } else sequence(object->items.size(),99,
            [&](std::size_t i) { value(object->items[i]->value,info.element); });
        path.pop_back();
    }
    template<class F> void sequence(std::size_t size, std::size_t cutoff, F element) {
        append(u"[");
        const bool trimmed=size>cutoff+1;
        const auto count=trimmed ? cutoff : size;
        for (std::size_t i=0;i<count;++i) { if (i) append(u","); element(i); }
        if (trimmed) append(u",...");
        append(u"]");
    }
public:
    ObjectWriter(Values& state, SourceLocation l, const ExecutionOptions& o,const NumberFormatSettings* format)
        :values(state),program(state.program),location(l),options(o),settings(format) {}
    std::u16string render(const Value& data, Type type) { value(data,type); return std::move(text); }
};
}
bool object_text_supported(const std::vector<TypeInfo>& types, Type type) { return supports(types,type,0); }
std::u16string object_text(Values& values, const Value& value, Type type, SourceLocation location,
    const ExecutionOptions& options,const NumberFormatSettings* settings) {
    return ObjectWriter(values,location,options,settings).render(value,type);
}
Value class_object_to_string(Values& values,const Value& value,const ExecutionOptions& options){
    const auto& object=std::get<std::shared_ptr<Aggregate>>(value);
    if(!object || object->nil)throw runtime_error(RuntimeError::nil_reference,values.location);
    if(database_null_type(type_info(values.program,object->type)))return std::u16string{};
    if(auto rendered=class_to_string_override(values,value,object->type))return std::move(*rendered);
    return reflected_type_property(values.program,{object->type},TypeOperation::full_name,values.location,options);
}
Value object_method_to_string(Values& values,const Value& value,const ExecutionOptions& options){
    const auto& object=std::get<std::shared_ptr<Aggregate>>(value);
    if(!object || object->nil)throw runtime_error(RuntimeError::nil_reference,values.location);
    if(type_info(values.program,object->type).kind!=CompoundKind::boxed_object)
        return class_object_to_string(values,value,options);
    if(!object->items.empty()){
        const auto& item=*object->items.front();
        if(valid_class_ancestry(values.program.types,item.type))
            return class_object_to_string(values,item.value,options);
    }
    return framework_object_text(values,value,object->type,options);
}
namespace {
class FrameworkWriter {
    Values& values;
    const ExecutionOptions& options;
    std::uint64_t work=0;
    std::u16string text;

    void append(std::u16string_view part){
        if(part.size()>1'048'576-text.size())throw runtime_error(RuntimeError::string_limit,values.location);
        text+=part;
    }

    void write(const Value& value,Type type,unsigned depth){
        if(work++>=options.instruction_limit || depth>32)throw runtime_error(RuntimeError::instruction_limit,values.location);
        if(options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,values.location);
        const auto location=values.location;
        if(std::holds_alternative<NilString>(value))return;
        const auto& format=number_settings(values);
        if(integer_type(type) || type==Type::real || type==Type::single){
            append(format_number(value,type,{},format,location));
            return;
        }
        if(scalar(type)){append(format_scalar(value,location));return;}
        if(type==Type::type_descriptor){
            const auto descriptor=std::get<ReflectedType>(value);
            if(descriptor.target!=Type::invalid)append(reflected_type_text(values.program,descriptor,location,options));
            return;
        }
        if(enumeration(values.program,type)){append(enum_text(values.program,std::get<EnumValue>(value),location));return;}
        if(const auto* aggregate=std::get_if<std::shared_ptr<Aggregate>>(&value)){
            const auto& object=*aggregate;
            if(!object || object->nil)return;
            const auto& shape=type_info(values.program,object->type);
            if(shape.kind==CompoundKind::object_class || shape.kind==CompoundKind::ini_file){
                append(string_text(class_object_to_string(values,value,options)));
                return;
            }
            if(shape.kind==CompoundKind::boxed_object && !object->items.empty()){
                const auto& item=object->items.front();
                write(item->value,item->type,depth+1);
                return;
            }
            if(calendar_record(shape)){append(calendar_text(value,{},location));return;}
            if(decimal_type(shape)){append(format_decimal(value,{},format,location));return;}
            if(complex_number_type(shape)){append(complex_dotnet_text(value,{},format,location));return;}
            if(ordinal_range_type(shape)){append(ordinal_range_text(object));return;}
            if(real_interval_type(shape)){append(real_interval_text(object,location));return;}
            if(shape.kind==CompoundKind::string_builder){
                const std::array<Value,1> input{value};
                append(std::get<std::u16string>(string_builder_invoke(values,object->type,StringBuilderMethod::text,input)));
                return;
            }
            const bool pair=key_value_pair_type(shape);
            if(pair || shape.kind==CompoundKind::tuple){
                append(pair ? u"[" : u"(");
                for(std::size_t i=0;i<object->items.size();++i){
                    if(i)append(u", ");
                    const auto& item=object->items[i];
                    write(item->value,item->type,depth+1);
                }
                append(pair ? u"]" : u")");
                return;
            }
        }
        const auto descriptor=reflected_value_type(values.program,value,type,location,options);
        if(descriptor.target!=Type::invalid)append(reflected_type_text(values.program,descriptor,location,options));
    }

public:
    FrameworkWriter(Values& context,const ExecutionOptions& limits):values(context),options(limits){}
    std::u16string render(const Value& value,Type type){
        write(value,type,0);
        return std::move(text);
    }
};
}
std::u16string framework_object_text(Values& values,const Value& value,Type type,const ExecutionOptions& options){
    return FrameworkWriter(values,options).render(value,type);
}
std::u16string pad_field(std::u16string text, std::int32_t width, SourceLocation location) {
    if (width<0) throw runtime_error(RuntimeError::field_width,location);
    if (static_cast<std::size_t>(width)>1'048'576) throw runtime_error(RuntimeError::string_limit,location);
    if (static_cast<std::size_t>(width)>text.size()) text.insert(0,static_cast<std::size_t>(width)-text.size(),u' ');
    return text;
}
std::u16string format_field(const Value& value, std::int32_t width, std::optional<std::int32_t> digits, SourceLocation location) {
    constexpr std::size_t limit=1'048'576;
    if (!digits && width<0) throw runtime_error(RuntimeError::field_width,location);
    const auto size=static_cast<std::uint64_t>(width<0 ? -std::int64_t(width) : width);
    if (size>limit) throw runtime_error(RuntimeError::string_limit,location);
    std::u16string text;
    if (digits) {
        if (*digits==INT32_MIN) throw runtime_error(RuntimeError::integer_overflow,location);
        const auto precision=*digits<0 ? -*digits : *digits;
        if (static_cast<std::size_t>(precision)>limit-2) throw runtime_error(RuntimeError::string_limit,location);
        std::string bytes(static_cast<std::size_t>(precision)+512,'\0');
        const auto converted=std::to_chars(bytes.data(),bytes.data()+bytes.size(),(std::holds_alternative<float>(value) ? static_cast<double>(std::get<float>(value)) : std::get<double>(value)),std::chars_format::fixed,precision);
        if (converted.ec!=std::errc{}) throw runtime_error(RuntimeError::number_format,location);
        text=from_utf8(std::string_view(bytes.data(),static_cast<std::size_t>(converted.ptr-bytes.data())));
    } else if (const auto* big=std::get_if<BigInteger>(&value)) text=checked_big(location,[&]{return from_utf8(big->text());});
    else if (const auto* string=std::get_if<std::u16string>(&value)) text=*string;
    else if (std::holds_alternative<NilString>(value)) text=u"nil";
    else if (const auto* ch=std::get_if<char16_t>(&value)) text+=*ch;
    else if (const auto* flag=std::get_if<bool>(&value)) text=*flag ? u"True" : u"False";
    else if (std::holds_alternative<double>(value) || std::holds_alternative<float>(value)) {
        const std::array<Value,1> arguments{value};
        text=std::get<std::u16string>(*stdlib::invoke_text(stdlib::Function::float_to_str,arguments,location));
    } else text=std::visit([&](const auto& n)->std::u16string {
        using T=std::decay_t<decltype(n)>;
        if constexpr (std::is_integral_v<T> && !std::is_same_v<T,bool> && !std::is_same_v<T,char16_t>) {
            char bytes[32]; const auto converted=std::to_chars(bytes,bytes+sizeof bytes,n);
            if (converted.ec!=std::errc{}) throw runtime_error(RuntimeError::number_format,location);
            return from_utf8(std::string_view(bytes,static_cast<std::size_t>(converted.ptr-bytes)));
        } else throw runtime_error(RuntimeError::invalid_bytecode,location);
    },value);
    if (text.size()>limit) throw runtime_error(RuntimeError::string_limit,location);
    if (size>text.size()) {
        const auto count=static_cast<std::size_t>(size)-text.size();
        if (width<0) text.append(count,u' '); else text.insert(0,count,u' ');
    }
    return text;
}
} // namespace abc

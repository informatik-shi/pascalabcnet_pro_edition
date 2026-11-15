#include "number_format.hpp"
#include "composite_format.hpp"
#include "runtime_values.hpp"
#include <array>

namespace abc {
struct NumberFormatState : NumberFormatSettings {
    bool readonly=false;
};
namespace {
using M=NumberFormatMethod;
using F=NumberFormatField;
constexpr std::array string_fields{
    &NumberFormatState::decimal,&NumberFormatState::group,&NumberFormatState::positive,
    &NumberFormatState::negative,&NumberFormatState::nan,&NumberFormatState::positive_infinity,
    &NumberFormatState::negative_infinity
};
M method(std::size_t operation) {return static_cast<M>(operation&255U);}
F field(std::size_t operation) {return static_cast<F>(operation>>8U);}
Type field_type(F member,Type groups) {
    if (member<F::decimal_digits) return Type::string;
    return member==F::group_sizes?groups:Type::integer;
}
Error argument_error(SourceLocation location) {
    return Error("E_ARGUMENT",location,"Invalid NumberFormatInfo value","Недопустимое значение NumberFormatInfo");
}
NumberFormatState& state(const Value& value,SourceLocation location) {
    const auto& object=std::get<std::shared_ptr<Aggregate>>(value);
    if (!object || object->nil) throw runtime_error(RuntimeError::nil_reference,location);
    if (!object->number_format) throw runtime_error(RuntimeError::invalid_bytecode,location);
    return *object->number_format;
}
std::shared_ptr<Aggregate> create(Values& values,Type type,NumberFormatState format) {
    if (!values.remaining) throw runtime_error(RuntimeError::allocation_limit,values.location);
    auto object=std::make_shared<Aggregate>();
    object->type=type;
    object->number_format=std::make_shared<NumberFormatState>(std::move(format));
    --values.remaining;
    return object;
}
const std::u16string& require_text(const Value& input,F member,SourceLocation location) {
    if (std::holds_alternative<NilString>(input)) throw runtime_error(RuntimeError::nil_reference,location);
    const auto& text=string_text(input);
    if (member==F::decimal_separator && text.empty()) throw argument_error(location);
    return text;
}
std::shared_ptr<Aggregate> culture_format(Values& values,Type owner,bool invariant) {
    auto& cached=invariant?values.invariant_number_format:values.current_number_format;
    if (!cached) {
        NumberFormatState initial;
        initial.readonly=invariant;
        cached=create(values,owner,std::move(initial));
    }
    return cached;
}
Value property_value(Values& values,const NumberFormatState& format,Type owner,F member) {
    if (member<F::decimal_digits) return format.*string_fields[static_cast<std::size_t>(member)];
    if (member==F::decimal_digits) return format.digits;
    if (member==F::negative_pattern) return format.pattern;
    auto result=values.array(type_info(values.program,owner).key,format.groups.size());
    for (std::size_t i=0;i<format.groups.size();++i) result->items[i]->value=format.groups[i];
    return result;
}
void set_property(NumberFormatState& format,F member,const Value& value,SourceLocation location) {
    if (format.readonly)
        throw Error("E_ACCESS",location,"NumberFormatInfo is read-only","NumberFormatInfo доступен только для чтения");
    if (member<F::decimal_digits) {
        format.*string_fields[static_cast<std::size_t>(member)]=require_text(value,member,location);
        return;
    }
    if (member==F::group_sizes) {
        const auto& array=std::get<std::shared_ptr<Aggregate>>(value);
        if (!array || array->nil) throw runtime_error(RuntimeError::nil_reference,location);
        std::vector<std::int32_t> groups;
        groups.reserve(array->items.size());
        for (std::size_t i=0;i<array->items.size();++i) {
            const auto size=std::get<std::int32_t>(array->items[i]->value);
            if (size<0 || size>9 || (size==0 && i+1!=array->items.size())) throw argument_error(location);
            groups.push_back(size);
        }
        format.groups=std::move(groups);
        return;
    }
    const auto number=std::get<std::int32_t>(value);
    if (number<0 || number>(member==F::decimal_digits?99:4)) throw argument_error(location);
    if (member==F::decimal_digits) format.digits=number;
    else format.pattern=number;
}
std::u16string real_text(const Value& value,const NumberFormatState& format,SourceLocation location) {
    return localize_number(format_scalar(value,location),format,location);
}
}
bool number_format_signature(const Program& program,Type owner,std::size_t operation) {
    if (!compound(owner) || static_cast<unsigned>(owner)-64U>=program.types.size()
        || type_info(program,owner).kind!=CompoundKind::number_format) return false;
    const auto call=method(operation);
    if (call==M::get || call==M::set) return (operation>>8U)<=static_cast<unsigned>(F::group_sizes);
    return call<=M::float_text && (operation>>8U)==0;
}
Type known_number_format_type(const Program& program) {
    for (std::size_t i=0;i<program.types.size();++i)
        if (program.types[i].kind==CompoundKind::number_format) return static_cast<Type>(64+i);
    return Type::invalid;
}
const NumberFormatSettings& number_settings(const Values& values,const Value* provider) {
    if (provider) {
        const auto& object=std::get<std::shared_ptr<Aggregate>>(*provider);
        if (object && !object->nil) return state(*provider,values.location);
    }
    if (values.current_number_format)
        return state(Value{values.current_number_format},values.location);
    static const NumberFormatSettings en_us;
    return en_us;
}
std::vector<Type> number_format_parameters(Type owner,Type groups,std::size_t operation) {
    switch (method(operation)) {
    case M::create: case M::invariant: case M::current: return {};
    case M::factory: return {Type::string,Type::string};
    case M::set: return {owner,field_type(field(operation),groups)};
    case M::float_text: return {Type::real,owner};
    default: return {owner};
    }
}
Type number_format_result(Type owner,Type groups,std::size_t operation) {
    switch (method(operation)) {
    case M::get: return field_type(field(operation),groups);
    case M::set: return Type::unit;
    case M::is_readonly: return Type::boolean;
    case M::to_string: case M::float_text: return Type::string;
    default: return owner;
    }
}
Value number_format_invoke(Values& values,Type owner,std::size_t operation,std::span<const Value> args) {
    const auto location=values.location;
    const auto call=method(operation);
    switch (call) {
    case M::create: return create(values,owner,{});
    case M::factory: {
        NumberFormatState format;
        format.decimal=require_text(args[0],F::decimal_separator,location);
        format.group=require_text(args[1],F::group_separator,location);
        return create(values,owner,std::move(format));
    }
    case M::invariant: case M::current: return culture_format(values,owner,call==M::invariant);
    case M::float_text: {
        const auto& object=std::get<std::shared_ptr<Aggregate>>(args[1]);
        const auto provider=object && !object->nil?args[1]:Value{culture_format(values,owner,false)};
        return real_text(args[0],state(provider,location),location);
    }
    default: break;
    }
    auto& format=state(args[0],location);
    switch (call) {
    case M::get: return property_value(values,format,owner,field(operation));
    case M::set: set_property(format,field(operation),args[1],location); return false;
    case M::is_readonly: return format.readonly;
    case M::readonly_copy: {
        if (format.readonly) return args[0];
        auto clone=format;
        clone.readonly=true;
        return create(values,owner,std::move(clone));
    }
    case M::to_string: return std::u16string(u"System.Globalization.NumberFormatInfo");
    default: throw runtime_error(RuntimeError::invalid_bytecode,location);
    }
}
}

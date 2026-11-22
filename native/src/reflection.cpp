#include "reflection.hpp"
#include "countries.hpp"
#include "interfaces.hpp"
#include "runtime_values.hpp"
#include "type_names.hpp"

namespace abc {
namespace {
constexpr std::array<std::u16string_view,4> property_names{u"Название",u"Столица",u"Население",u"Континент"};
constexpr std::array<Type,4> property_types{Type::string,Type::string,Type::integer,Type::string};
constexpr std::array<std::int32_t,4> attribute_numbers{1,2,4,3};

Type named_type(const std::vector<TypeInfo>& types,std::string_view name) {
    for(std::size_t i=0;i<types.size();++i)if(types[i].name==name)return static_cast<Type>(64+i);
    return Type::invalid;
}
Type array_type(const std::vector<TypeInfo>& types,Type element) {
    for(std::size_t i=0;i<types.size();++i){
        const auto& shape=types[i];
        if(shape.kind==CompoundKind::array && shape.rank==1 && shape.element==element)return static_cast<Type>(64+i);
    }
    return Type::invalid;
}
[[noreturn]] void unsupported(SourceLocation location) {
    throw Error("E_UNSUPPORTED",location,"Property reflection for this type is not implemented",
        "Отражение свойств этого типа ещё не перенесено");
}
ReflectedType descriptor(Values& values,const Value& value) {
    const auto result=std::get<ReflectedType>(value);
    if(result.target==Type::invalid)throw runtime_error(RuntimeError::nil_reference,values.location);
    return result;
}
Type country_owner(Values& values,Type type) {
    if(compound(type) && type_info(values.program,type).name==country_class_name)return type;
    unsupported(values.location);
}
std::shared_ptr<Aggregate> property(Values& values,Type owner,ReflectedType target,std::int32_t index) {
    const auto key=std::pair{target,index};
    if(auto cached=values.reflected_properties[key].lock())return cached;
    auto result=std::get<std::shared_ptr<Aggregate>>(values.zero(owner,true));
    result->items[0]->value=target;
    result->items[1]->value=index;
    values.reflected_properties[key]=result;
    return result;
}
const Aggregate& require_property(Values& values,const Value& value) {
    const auto& result=std::get<std::shared_ptr<Aggregate>>(value);
    if(!result || result->nil)throw runtime_error(RuntimeError::nil_reference,values.location);
    return *result;
}
}

TypeInfo property_info_class() {
    TypeInfo result{std::string(property_info_class_name),CompoundKind::object_class,Type::invalid,
        {{"$owner",Type::type_descriptor,false,MemberAccess::private_member},
         {"$index",Type::integer,false,MemberAccess::private_member}}};
    result.display_name="System.Reflection.PropertyInfo";
    result.abstract_class=true;
    return result;
}
bool property_info_shape(const TypeInfo& shape) {
    const auto expected=property_info_class();
    return shape.name==expected.name && shape.kind==expected.kind && shape.element==Type::invalid
        && shape.key==Type::invalid && shape.abstract_class && shape.display_name==expected.display_name
        && shape.fields.size()==expected.fields.size()
        && std::ranges::equal(shape.fields,expected.fields,[](const auto& left,const auto& right){
            return left.name==right.name && left.type==right.type && left.access==right.access && !left.global;
        });
}

std::optional<ReflectionSignature> reflection_signature(const std::vector<TypeInfo>& types,Type owner,ReflectionMethod method) {
    if(!compound(owner) || static_cast<unsigned>(owner)-64U>=types.size()
        || !property_info_shape(types[static_cast<unsigned>(owner)-64U]))return {};
    const auto object=named_type(types,"object");
    const auto objects=array_type(types,object);
    const auto properties=array_type(types,owner);
    const auto attribute=named_type(types,country_attribute_name);
    if(object==Type::invalid || types[static_cast<unsigned>(object)-64U].kind!=CompoundKind::boxed_object
        || objects==Type::invalid || properties==Type::invalid || attribute==Type::invalid
        || !countries_class_shape(types,types[static_cast<unsigned>(attribute)-64U]))return {};
    using M=ReflectionMethod;
    switch(method){
    case M::get_property:return ReflectionSignature{{Type::type_descriptor,Type::string},owner};
    case M::get_properties:return ReflectionSignature{{Type::type_descriptor},properties};
    case M::name:case M::to_string:return ReflectionSignature{{owner},Type::string};
    case M::property_type:case M::declaring_type:case M::reflected_type:return ReflectionSignature{{owner},Type::type_descriptor};
    case M::can_read:case M::can_write:return ReflectionSignature{{owner},Type::boolean};
    case M::get_value:return ReflectionSignature{{owner,object},object};
    case M::get_value_indexed:return ReflectionSignature{{owner,object,objects},object};
    case M::attributes:return ReflectionSignature{{owner,Type::boolean},objects};
    case M::attributes_typed:return ReflectionSignature{{owner,Type::type_descriptor,Type::boolean},objects};
    case M::is_defined:return ReflectionSignature{{owner,Type::type_descriptor,Type::boolean},Type::boolean};
    case M::base_type:return ReflectionSignature{{Type::type_descriptor},Type::type_descriptor};
    case M::attribute_equals:return ReflectionSignature{{named_type(types,system_attribute_name),object},Type::boolean};
    case M::attribute_hash:return ReflectionSignature{{named_type(types,system_attribute_name)},Type::integer};
    case M::attribute_default:return ReflectionSignature{{named_type(types,system_attribute_name)},Type::boolean};
    case M::attribute_type_id:return ReflectionSignature{{named_type(types,system_attribute_name)},object};
    }
    return {};
}

Value reflection_invoke(Values& values,Type owner,ReflectionMethod method,std::span<const Value> arguments) {
    using M=ReflectionMethod;
    const auto& types=values.program.types;
    const auto signature=*reflection_signature(types,owner,method);
    if(method>=M::attribute_equals){
        const auto& receiver=std::get<std::shared_ptr<Aggregate>>(arguments[0]);
        if(!receiver || receiver->nil)throw runtime_error(RuntimeError::nil_reference,values.location);
        if(method==M::attribute_type_id)return values.box_value(signature.result,Type::type_descriptor,ReflectedType{receiver->type});
        if(method==M::attribute_default)return false;
        if(type_info(values.program,receiver->type).name!=country_attribute_name)
            throw Error("E_UNSUPPORTED",values.location,"Equality of this attribute type is not implemented",
                "Равенство атрибутов этого типа ещё не перенесено");
        if(method==M::attribute_hash)return receiver->items[0]->value;
        const auto& other=std::get<std::shared_ptr<Aggregate>>(arguments[1]);
        if(!other || other->nil || other->items.empty() || other->items[0]->type!=receiver->type)return false;
        const auto& attribute=std::get<std::shared_ptr<Aggregate>>(other->items[0]->value);
        return attribute && !attribute->nil && receiver->items[0]->value==attribute->items[0]->value;
    }
    if(method==M::base_type){
        const auto target=descriptor(values,arguments[0]);
        if(target.implementation || !compound(target.target))unsupported(values.location);
        const auto& shape=type_info(values.program,target.target);
        if(shape.kind==CompoundKind::boxed_object)return ReflectedType{};
        if(shape.kind!=CompoundKind::object_class)unsupported(values.location);
        return ReflectedType{shape.key==Type::invalid ? named_type(types,"object") : shape.key};
    }
    if(method==M::get_property || method==M::get_properties){
        const auto target=descriptor(values,arguments[0]);
        if(target.implementation || !compound(target.target))unsupported(values.location);
        (void)country_owner(values,target.target);
        if(method==M::get_property){
            if(std::holds_alternative<NilString>(arguments[1]))
                throw Error("E_ARGUMENT",values.location,"Property name cannot be null","Имя свойства не может быть nil");
            const auto name=std::ranges::find(property_names,string_text(arguments[1]));
            if(name==property_names.end())return values.zero(owner);
            return property(values,owner,target,static_cast<std::int32_t>(name-property_names.begin()));
        }
        auto result=values.array(signature.result,0);
        for(std::int32_t index=0;index<4;++index)result->items.push_back(values.cell(owner,property(values,owner,target,index)));
        return result;
    }
    const auto& metadata=require_property(values,arguments[0]);
    const auto reflected=std::get<ReflectedType>(metadata.items[0]->value);
    const auto index=static_cast<std::size_t>(std::get<std::int32_t>(metadata.items[1]->value));
    if(!reflected_type_valid(values.program,reflected) || !compound(reflected.target) || index>=property_names.size())
        throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    const auto country=country_owner(values,reflected.target);
    switch(method){
    case M::name:return std::u16string(property_names[index]);
    case M::property_type:return ReflectedType{property_types[index]};
    case M::declaring_type:return ReflectedType{country};
    case M::reflected_type:return reflected;
    case M::can_read:return true;
    case M::can_write:return false;
    case M::to_string:return (index==2 ? u"Int32 " : u"System.String ")+std::u16string(property_names[index]);
    case M::get_value:case M::get_value_indexed:{
        const auto& boxed=std::get<std::shared_ptr<Aggregate>>(arguments[1]);
        if(!boxed || boxed->nil || boxed->items.empty())
            throw Error("E_TARGET",values.location,"A property requires an object instance","Для свойства требуется экземпляр объекта");
        const auto& input=*boxed->items.front();
        if(input.type!=country && !class_reference_compatible(types,input.type,country))
            throw Error("E_TARGET",values.location,"Object does not match the property declaring type",
                "Тип объекта не соответствует типу, объявляющему свойство");
        if(method==M::get_value_indexed){
            const auto& indices=std::get<std::shared_ptr<Aggregate>>(arguments[2]);
            if(indices && !indices->nil && !indices->items.empty())
                throw Error("E_ARGUMENT",values.location,"Property has no index parameters","У свойства нет параметров индекса");
        }
        const auto& instance=std::get<std::shared_ptr<Aggregate>>(input.value);
        return values.box_value(signature.result,property_types[index],instance->items[index]->value);
    }
    case M::attributes:case M::attributes_typed:case M::is_defined:{
        const auto attribute=named_type(types,country_attribute_name);
        bool include=true;
        if(method!=M::attributes){
            const auto filter=descriptor(values,arguments[1]);
            const auto base=named_type(types,system_attribute_name);
            if(filter.implementation || (filter.target!=base && !class_reference_compatible(types,filter.target,base)))
                throw Error("E_ARGUMENT",values.location,"Attribute filter must derive from System.Attribute",
                    "Тип фильтра должен наследовать System.Attribute");
            include=filter.target==attribute || class_reference_compatible(types,attribute,filter.target);
        }
        if(method==M::is_defined)return include;
        auto result=values.array(signature.result,0);
        if(include){
            auto entry=values.zero(attribute,true);
            std::get<std::shared_ptr<Aggregate>>(entry)->items[0]->value=attribute_numbers[index];
            const auto object=type_info(values.program,signature.result).element;
            result->items.push_back(values.cell(object,values.box_value(object,attribute,std::move(entry))));
        }
        return result;
    }
    default:throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    }
}
}

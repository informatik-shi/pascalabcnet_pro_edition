#include "reflection.hpp"
#include "countries.hpp"
#include "interfaces.hpp"
#include "runtime_values.hpp"
#include "scalar_serialization.hpp"
#include "type_names.hpp"
#include <set>
#include <tuple>
#include <limits>
#include <algorithm>
#include <type_traits>

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
struct ReflectedProperty:PropertyMetadata {bool static_member=false;};
std::vector<ReflectedProperty> properties(Values& values,Type type) {
    std::vector<ReflectedProperty> result;
    std::set<std::tuple<std::string,Type,std::vector<Type>>> signatures;
    const auto append=[&](const PropertyMetadata& metadata,bool static_member=false,bool include=true){
        const auto& name=metadata.display_name.empty() ? metadata.name : metadata.display_name;
        if(!signatures.emplace(name,metadata.type,metadata.indices).second)return;
        if(!include || metadata.access!=MemberAccess::public_member)return;
        result.push_back({metadata,static_member});
    };
    std::size_t depth=0;
    for(auto current=type;current!=Type::invalid;current=class_parent(values.program.types,current)){
        if(++depth>values.program.types.size())throw runtime_error(RuntimeError::invalid_bytecode,values.location);
        const auto& shape=type_info(values.program,current);
        if(shape.kind==CompoundKind::boxed_object)break;
        if(shape.kind!=CompoundKind::object_class)unsupported(values.location);
        if(shape.name==country_class_name){
            const auto attribute=named_type(values.program.types,country_attribute_name);
            for(std::size_t index=0;index<property_names.size();++index){
                PropertyMetadata metadata;
                metadata.owner=current;
                metadata.type=property_types[index];
                metadata.name=to_utf8(property_names[index]);
                metadata.display_name=metadata.name;
                metadata.attributes.push_back({attribute,{attribute_numbers[index]}});
                append(metadata);
            }
            continue;
        }
        if(shape.name.starts_with('$'))unsupported(values.location);
        std::vector<std::pair<const PropertyMetadata*,bool>> declared;
        for(const auto& metadata:values.program.instance_properties)
            if(metadata.owner==current)declared.push_back({&metadata,false});
        for(const auto& metadata:values.program.static_properties)
            if(metadata.owner==current)declared.push_back({&metadata,true});
        std::stable_sort(declared.begin(),declared.end(),[](const auto& left,const auto& right){
            return left.first->declaration_order.value_or(std::numeric_limits<std::size_t>::max())
                <right.first->declaration_order.value_or(std::numeric_limits<std::size_t>::max());
        });
        for(const auto& [metadata,static_member]:declared)
            append(*metadata,static_member,!static_member || current==type);
    }
    return result;
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
std::u16string property_signature_type(Values& values,Type type,const ExecutionOptions& options) {
    auto root=type;
    std::u16string suffix;
    while(compound(root) && type_info(values.program,root).kind==CompoundKind::array){
        const auto& array=type_info(values.program,root);
        suffix=(array.rank==2 ? u"[,]" : u"[]")+suffix;
        root=array.element;
    }
    if(scalar(root) && !string_type(root)){
        const auto name=reflected_type_property(values.program,{root},TypeOperation::name,values.location,options);
        return string_text(name)+suffix;
    }
    return reflected_type_text(values.program,{type},values.location,options);
}
bool primitive_index_type(Type type) {
    switch(type){
    case Type::boolean:case Type::character:case Type::shortint:case Type::byte:
    case Type::smallint:case Type::word:case Type::integer:case Type::longword:
    case Type::int64:case Type::uint64:case Type::single:case Type::real:return true;
    default:return false;
    }
}
bool primitive_index_widen(Type from,Type to) {
    if(!primitive_index_type(from) || !primitive_index_type(to))return false;
    if(from==Type::character)return from==to || to==Type::word || to==Type::integer || to==Type::longword
        || to==Type::int64 || to==Type::uint64 || to==Type::single || to==Type::real;
    if(to==Type::character)return from==Type::byte || from==Type::word;
    return can_widen(from,to);
}
Value primitive_index_value(Type type,const Value& value,SourceLocation location) {
    return std::visit([&](const auto& number)->Value{
        using Number=std::decay_t<decltype(number)>;
        if constexpr(std::is_arithmetic_v<Number>){
            switch(type){
            case Type::character:return static_cast<char16_t>(number);
            case Type::shortint:return static_cast<std::int8_t>(number);
            case Type::byte:return static_cast<std::uint8_t>(number);
            case Type::smallint:return static_cast<std::int16_t>(number);
            case Type::word:return static_cast<std::uint16_t>(number);
            case Type::integer:return static_cast<std::int32_t>(number);
            case Type::longword:return static_cast<std::uint32_t>(number);
            case Type::int64:return static_cast<std::int64_t>(number);
            case Type::uint64:return static_cast<std::uint64_t>(number);
            case Type::single:return static_cast<float>(number);
            case Type::real:return static_cast<double>(number);
            default:break;
            }
        }
        throw runtime_error(RuntimeError::invalid_bytecode,location);
    },value);
}
Value index_argument(Values& values,Type type,const Value& value) {
    const auto& types=values.program.types;
    if(compound(type) && type_info(values.program,type).kind==CompoundKind::boxed_object)return value;
    const auto& boxed=std::get<std::shared_ptr<Aggregate>>(value);
    if(!boxed || boxed->nil)return values.zero(type,false,true);
    if(boxed->items.empty())throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    const auto& item=*boxed->items[0];
    if(item.type==type)return values.copy(item.value);
    if(valid_class_ancestry(types,type) && class_reference_compatible(types,item.type,type))return item.value;
    if(compound(type) && interface_kind(type_info(values.program,type))
        && interface_cast_valid(values.program,item.type,type,true)){
        try{return interface_cast(values,item.type,type,true,item.value);}
        catch(const Error& error){if(error.code!="E_CAST")throw;}
    }
    if(primitive_index_widen(item.type,type))return primitive_index_value(type,item.value,values.location);
    if((compound(type) && type_info(values.program,type).kind==CompoundKind::enumeration)
        || (compound(item.type) && type_info(values.program,item.type).kind==CompoundKind::enumeration))
        throw Error("E_UNSUPPORTED",values.location,"Numeric conversions of enum indices are not implemented",
            "Числовые преобразования индексов перечисляемого типа ещё не перенесены");
    if(compound(type) && compound(item.type) && type_info(values.program,type).kind==CompoundKind::array
        && type_info(values.program,item.type).kind==CompoundKind::array)
        throw Error("E_UNSUPPORTED",values.location,"Array covariance for property indices is not implemented",
            "Ковариантность массивов в индексах свойств ещё не перенесена");
    throw Error("E_ARGUMENT",values.location,"Index argument has an incompatible type",
        "Аргумент индекса имеет несовместимый тип");
}
std::vector<Value> index_arguments(Values& values,const PropertyMetadata& member,const Value* indices_value) {
    const auto indices=indices_value ? std::get<std::shared_ptr<Aggregate>>(*indices_value) : nullptr;
    const auto count=indices && !indices->nil ? indices->items.size() : 0;
    if(count!=member.indices.size())
        throw Error("E_ARGUMENT",values.location,"Incorrect number of property index arguments",
            "Неверное число аргументов индекса свойства");
    std::vector<Value> result;result.reserve(count+1);
    for(std::size_t index=0;index<count;++index)
        result.push_back(index_argument(values,member.indices[index],indices->items[index]->value));
    return result;
}
Value read_property(Values& values,const ReflectedProperty& member,Type result,
    std::span<const Value> receiver={},Type actual=Type::invalid) {
    const auto& types=values.program.types;
    if(!values.invoke)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    if(!scalar(member.type) && member.type!=Type::type_descriptor && !nrbf_array_type(types,member.type)
        && !object_structure_type(types,member.type) && !valid_class_ancestry(types,member.type)
        && !(compound(member.type) && type_info(values.program,member.type).kind==CompoundKind::boxed_object))
        unsupported(values.location);
    if(member.static_member){
        for(const auto& owner:values.program.static_classes)if(owner.owner==member.owner){
            (void)values.invoke(owner.initializer,{});
            break;
        }
    }
    const auto target=member.static_member ? *member.getter
        : virtual_method_target(values.program,*member.getter,actual,values.location);
    auto value=values.invoke(target,receiver);
    if(compound(member.type) && type_info(values.program,member.type).kind==CompoundKind::boxed_object)return value;
    return values.box_value(result,member.type,std::move(value));
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

void verify_property_metadata(const Program& program) {
    std::set<std::pair<Type,std::size_t>> declaration_order;
    const auto check_attributes=[&](const PropertyMetadata& property){
        if(property.declaration_order && !declaration_order.emplace(property.owner,*property.declaration_order).second)
            throw Error("E_BYTECODE",{},"Duplicate property declaration order","Повторяющийся порядок объявления свойств");
        std::set<Type> seen;
        for(const auto& attribute:property.attributes){
            if(!compound(attribute.type) || !valid_value_type(program,attribute.type)
                || type_info(program,attribute.type).name!=country_attribute_name
                || !countries_class_shape(program.types,type_info(program,attribute.type))
                || attribute.arguments.size()!=1 || !std::holds_alternative<std::int32_t>(attribute.arguments[0])
                || !seen.insert(attribute.type).second)
                throw Error("E_BYTECODE",{},"Invalid property attribute metadata","Недопустимые метаданные атрибута свойства");
        }
    };
    for(const auto& property:program.static_properties)check_attributes(property);
    std::set<std::pair<Type,std::string>> seen;
    std::set<Type> defaults;
    for(const auto& property:program.instance_properties){
        if(!compound(property.owner) || !valid_value_type(program,property.owner))
            throw Error("E_BYTECODE",{},"Invalid property owner","Недопустимый владелец свойства");
        const auto& owner=type_info(program,property.owner);
        if((owner.kind!=CompoundKind::object_class && !(owner.kind==CompoundKind::record && owner.native_record==NativeRecord::none))
            || property.name.empty() || !valid_value_type(program,property.type)
            || !valid_member_access(property.access) || (!property.getter && !property.setter)
            || property.indices.size()>14 || !seen.emplace(property.owner,property.name).second
            || (property.default_indexer && !defaults.insert(property.owner).second))
            throw Error("E_BYTECODE",{},"Invalid instance property metadata","Недопустимые метаданные свойства экземпляра");
        for(auto index:property.indices)if(!valid_value_type(program,index))
            throw Error("E_BYTECODE",{},"Invalid property index type","Недопустимый тип индекса свойства");
        const auto check_accessor=[&](std::size_t id,bool setter){
            if(id>=program.routines.size())throw Error("E_BYTECODE",{},"Invalid property accessor","Недопустимый метод свойства");
            const auto& routine=program.routines[id];
            if((routine.owner!=property.owner && !class_reference_compatible(program.types,property.owner,routine.owner))
                || routine.constructor || !routine.captures.empty() || routine.result!=(setter ? Type::unit : property.type)
                || routine.parameters.size()!=property.indices.size()+(setter ? 2U : 1U)
                || routine.parameters[0].type!=routine.owner || routine.parameters[0].variadic
                || routine.parameters[0].by_reference!=(owner.kind==CompoundKind::record))
                throw Error("E_BYTECODE",{},"Invalid instance property accessor signature","Недопустимая сигнатура метода свойства экземпляра");
            for(std::size_t index=1;index<routine.parameters.size();++index){
                const auto expected=index<=property.indices.size() ? property.indices[index-1] : property.type;
                const auto& parameter=routine.parameters[index];
                if(parameter.type!=expected || parameter.by_reference || parameter.variadic)
                    throw Error("E_BYTECODE",{},"Invalid property accessor parameter","Недопустимый параметр метода свойства");
            }
        };
        if(property.getter)check_accessor(*property.getter,false);
        if(property.setter)check_accessor(*property.setter,true);
        check_attributes(property);
    }
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

Value reflection_invoke(Values& values,Type owner,ReflectionMethod method,std::span<const Value> arguments,const ExecutionOptions& options) {
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
        const auto members=properties(values,target.target);
        if(method==M::get_property){
            if(std::holds_alternative<NilString>(arguments[1]))
                throw Error("E_ARGUMENT",values.location,"Property name cannot be null","Имя свойства не может быть nil");
            std::optional<std::size_t> found;
            for(std::size_t index=0;index<members.size();++index){
                const auto& member=members[index];
                if(from_utf8(member.display_name.empty() ? member.name : member.display_name)==string_text(arguments[1])){
                    if(found)throw Error("E_AMBIGUOUS",values.location,"More than one property matches the requested name",
                        "Указанному имени соответствует несколько свойств");
                    found=index;
                }
            }
            if(found)return property(values,owner,target,static_cast<std::int32_t>(*found));
            return values.zero(owner);
        }
        auto result=values.array(signature.result,0);
        for(std::size_t index=0;index<members.size();++index)
            result->items.push_back(values.cell(owner,property(values,owner,target,static_cast<std::int32_t>(index))));
        return result;
    }
    const auto& metadata=require_property(values,arguments[0]);
    const auto reflected=std::get<ReflectedType>(metadata.items[0]->value);
    const auto index=static_cast<std::size_t>(std::get<std::int32_t>(metadata.items[1]->value));
    if(!reflected_type_valid(values.program,reflected) || !compound(reflected.target))
        throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    const auto members=properties(values,reflected.target);
    if(index>=members.size())throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    const auto& member=members[index];
    const bool native_country=type_info(values.program,member.owner).name==country_class_name;
    switch(method){
    case M::name:return from_utf8(member.display_name.empty() ? member.name : member.display_name);
    case M::property_type:return ReflectedType{member.type};
    case M::declaring_type:return ReflectedType{member.owner};
    case M::reflected_type:return reflected;
    case M::can_read:return native_country || member.getter.has_value();
    case M::can_write:return member.setter.has_value();
    case M::to_string:{
        auto result=property_signature_type(values,member.type,options)+u" "+from_utf8(member.display_name.empty() ? member.name : member.display_name);
        if(!member.indices.empty()){
            result+=u" [";
            for(std::size_t index=0;index<member.indices.size();++index){
                if(index)result+=u", ";
                result+=property_signature_type(values,member.indices[index],options);
            }
            result+=u"]";
        }
        return result;
    }
    case M::get_value:case M::get_value_indexed:{
        if(!native_country && !member.getter)
            throw Error("E_ARGUMENT",values.location,"Property has no getter","У свойства нет метода чтения");
        if(member.static_member){
            const auto indices=index_arguments(values,member,method==M::get_value_indexed ? &arguments[2] : nullptr);
            return read_property(values,member,signature.result,indices);
        }
        const auto& boxed=std::get<std::shared_ptr<Aggregate>>(arguments[1]);
        if(!boxed || boxed->nil || boxed->items.empty())
            throw Error("E_TARGET",values.location,"A property requires an object instance","Для свойства требуется экземпляр объекта");
        const auto& input=*boxed->items.front();
        if(input.type!=member.owner && !class_reference_compatible(types,input.type,member.owner))
            throw Error("E_TARGET",values.location,"Object does not match the property declaring type",
                "Тип объекта не соответствует типу, объявляющему свойство");
        auto indices=index_arguments(values,member,method==M::get_value_indexed ? &arguments[2] : nullptr);
        if(native_country){
            const auto& instance=std::get<std::shared_ptr<Aggregate>>(input.value);
            const auto name=from_utf8(member.name);
            const auto field=std::ranges::find(property_names,name);
            if(field==property_names.end())throw runtime_error(RuntimeError::invalid_bytecode,values.location);
            return values.box_value(signature.result,member.type,instance->items[static_cast<std::size_t>(field-property_names.begin())]->value);
        }
        indices.insert(indices.begin(),input.value);
        const auto actual=std::get<std::shared_ptr<Aggregate>>(input.value)->type;
        return read_property(values,member,signature.result,indices,actual);
    }
    case M::attributes:case M::attributes_typed:case M::is_defined:{
        Type filter=Type::invalid;
        if(method!=M::attributes){
            const auto requested=descriptor(values,arguments[1]);
            const auto base=named_type(types,system_attribute_name);
            if(requested.implementation || (requested.target!=base && !class_reference_compatible(types,requested.target,base)))
                throw Error("E_ARGUMENT",values.location,"Attribute filter must derive from System.Attribute",
                    "Тип фильтра должен наследовать System.Attribute");
            filter=requested.target;
        }
        const auto includes=[&](const auto& attribute){return filter==Type::invalid || attribute.type==filter
            || class_reference_compatible(types,attribute.type,filter);};
        if(method==M::is_defined)return std::ranges::any_of(member.attributes,includes);
        auto result=values.array(signature.result,0);
        for(const auto& attribute:member.attributes){
            if(!includes(attribute))continue;
            auto entry=values.zero(attribute.type,true);
            std::get<std::shared_ptr<Aggregate>>(entry)->items[0]->value=attribute.arguments[0];
            const auto object=type_info(values.program,signature.result).element;
            result->items.push_back(values.cell(object,values.box_value(object,attribute.type,std::move(entry))));
        }
        return result;
    }
    default:throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    }
}
}

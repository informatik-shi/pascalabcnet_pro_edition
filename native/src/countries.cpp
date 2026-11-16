#include "countries.hpp"
#include "environment.hpp"
#include "file_reader.hpp"
#include "files.hpp"
#include "runtime_values.hpp"
#include "stdlib.hpp"

namespace abc {
namespace {
constexpr std::array country_parameters{Type::string,Type::string,Type::integer,Type::string};
constexpr std::array attribute_parameters{Type::integer};
constexpr std::array<std::string_view,4> property_names{"название","столица","население","континент"};

std::u16string database_path(FileContext& files,const ExecutionOptions& options,SourceLocation location) {
    constexpr auto relative=u"Databases/Страны.csv";
    const auto& executable=options.executable_name.empty() ? process_executable_name() : options.executable_name;
    if(!executable.empty()){
        const auto directory=files.resolve(executable,location).parent_path();
        const auto installed=(directory / "Files" / relative).u16string();
        if(file_exists(files,installed))return installed;
#ifdef __APPLE__
        if(directory.filename()=="MacOS"){
            const auto resource=(directory.parent_path() / "Resources" / "Files" / relative).u16string();
            if(file_exists(files,resource))return resource;
        }
#endif
    }
    if(file_exists(files,relative))return relative;
    throw Error("E_IO",location,"Country database Databases/Страны.csv was not found",
        "Не найден массив стран Databases\\Страны.csv");
}

Value construct(Values& values,Type type,std::span<const Value> arguments) {
    auto result=values.zero(type,true);
    auto& fields=std::get<std::shared_ptr<Aggregate>>(result)->items;
    for(std::size_t i=0;i<arguments.size();++i)values.write(fields[i],arguments[i]);
    return result;
}

Value load_countries(Values& values,FileContext& files,const ExecutionOptions& options,Type result) {
    const auto location=values.location;
    const auto path=database_path(files,options,location);
    const auto encoding=files.encoding(location);
    auto reader=std::make_shared<LineReader>(files.resolve(path,location),
        encoding ? *encoding : TextEncoding{TextEncodingKind::windows1251,false,false},location);
    files.prepare_line_open(reader,location);
    auto output=values.array(result,0);
    const auto country=type_info(values.program,result).element;
    while(auto line=reader->read_line(location,options)){
        std::array<Value,4> row;
        std::size_t count=0;
        std::u16string_view remaining=*line;
        while(!remaining.empty() && count<row.size()){
            const auto separator=remaining.find(u';');
            if(separator!=0)row[count++]=std::u16string(remaining.substr(0,separator));
            if(separator==remaining.npos)break;
            remaining.remove_prefix(separator+1);
        }
        // The source reads w[0], w[1] and w[2].ToInteger before accessing w[3].
        if(count<3)throw runtime_error(RuntimeError::index_bounds,location);
        row[2]=*stdlib::invoke_text(stdlib::Function::str_to_int,std::span<const Value>{row}.subspan(2,1),location);
        if(count<4)throw runtime_error(RuntimeError::index_bounds,location);
        output->items.push_back(values.cell(country,construct(values,country,row)));
    }
    return output;
}
}

TypeInfo countries_class(std::string_view name,Type parent) {
    TypeInfo result{std::string(name),CompoundKind::object_class,Type::invalid,{},parent};
    if(name==country_class_name){
        result.display_name="Countries.Country";
        constexpr std::array<std::string_view,4> field_names{"$nm","$cap","$inh","$cont"};
        for(std::size_t i=0;i<field_names.size();++i)
            result.fields.push_back({std::string(field_names[i]),country_parameters[i],false,MemberAccess::private_member});
    }else if(name==country_attribute_name){
        result.display_name="Countries.Att";
        result.fields.push_back({"num",Type::integer,false});
    }else{
        result.display_name="System.Attribute";
        result.abstract_class=true;
    }
    return result;
}

bool countries_class_shape(const std::vector<TypeInfo>& types,const TypeInfo& shape) {
    if(shape.kind!=CompoundKind::object_class)return false;
    if(shape.name!=country_class_name && shape.name!=country_attribute_name && shape.name!=system_attribute_name)return false;
    if(shape.name==country_attribute_name){
        if(!compound(shape.key) || static_cast<unsigned>(shape.key)-64U>=types.size())return false;
        const auto& parent=types[static_cast<unsigned>(shape.key)-64U];
        if(parent.name!=system_attribute_name || !countries_class_shape(types,parent))return false;
    }else if(shape.key!=Type::invalid)return false;
    const auto expected=countries_class(shape.name,shape.key);
    return shape.element==Type::invalid && shape.abstract_class==expected.abstract_class
        && shape.display_name==expected.display_name && shape.fields.size()==expected.fields.size()
        && std::ranges::equal(shape.fields,expected.fields,[](const auto& left,const auto& right){
            return left.name==right.name && left.type==right.type && left.global==right.global && left.access==right.access;
        });
}

bool countries_method_valid(const std::vector<TypeInfo>& types,Type owner,CountriesMethod method) {
    if(!compound(owner) || static_cast<unsigned>(owner)-64U>=types.size() || method>CountriesMethod::continent)return false;
    const auto& shape=types[static_cast<unsigned>(owner)-64U];
    if(method==CountriesMethod::load){
        if(shape.kind!=CompoundKind::array || shape.rank!=1 || !compound(shape.element)
            || static_cast<unsigned>(shape.element)-64U>=types.size())return false;
        const auto& country=types[static_cast<unsigned>(shape.element)-64U];
        return country.name==country_class_name && countries_class_shape(types,country);
    }
    return countries_class_shape(types,shape) && (shape.name==country_class_name
        || (shape.name==country_attribute_name && method==CountriesMethod::construct));
}

std::vector<Type> countries_parameters(Type owner,const TypeInfo& shape,CountriesMethod method) {
    if(method==CountriesMethod::load)return {};
    if(method!=CountriesMethod::construct)return {owner};
    if(shape.name==country_class_name)return {country_parameters.begin(),country_parameters.end()};
    return {attribute_parameters.begin(),attribute_parameters.end()};
}

Type countries_result(Type owner,CountriesMethod method) {
    if(method==CountriesMethod::construct || method==CountriesMethod::load)return owner;
    return country_parameters[static_cast<std::size_t>(method)-static_cast<std::size_t>(CountriesMethod::name)];
}

std::optional<CountriesMethod> country_property(std::string_view name) {
    const auto property=std::ranges::find(property_names,name);
    if(property==property_names.end())return {};
    return static_cast<CountriesMethod>(static_cast<std::size_t>(CountriesMethod::name)+(property-property_names.begin()));
}

Value countries_invoke(Values& values,FileContext& files,const ExecutionOptions& options,
    Type owner,CountriesMethod method,std::span<const Value> arguments) {
    if(method==CountriesMethod::construct)return construct(values,owner,arguments);
    if(method==CountriesMethod::load)return load_countries(values,files,options,owner);
    const auto& object=std::get<std::shared_ptr<Aggregate>>(arguments.front());
    if(!object || object->nil)throw runtime_error(RuntimeError::nil_reference,values.location);
    const auto index=static_cast<std::size_t>(method)-static_cast<std::size_t>(CountriesMethod::name);
    return object->items[index]->value;
}
}

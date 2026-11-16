#include "type_names.hpp"
#include "comparison.hpp"
#include "runtime_values.hpp"
#include "diagnostics.hpp"
#include "enumeration.hpp"
#include "sql_scalar.hpp"
#include "sql_decimal.hpp"
#include "sql_money.hpp"
#include "sql_date_time.hpp"
#include "sql_guid.hpp"
#include "sql_binary.hpp"
#include <new>

namespace abc {
namespace {
constexpr std::size_t text_limit=1'048'576;
constexpr std::u16string_view framework_identity=u", Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089";
struct Work {
    SourceLocation location;const ExecutionOptions& options;std::uint64_t used=0;
    void step(std::size_t count=1){
        if(count>options.instruction_limit-used)throw runtime_error(RuntimeError::instruction_limit,location);
        if(options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,location);
        used+=count;
    }
};
[[noreturn]] void unsupported(SourceLocation location){
    throw Error("E_UNSUPPORTED",location,"Metadata identity of this source-generated type is not implemented",
        "Имя метаданных этого сгенерированного типа ещё не перенесено");
}
std::u16string escaped_type_name(std::u16string_view value){
    std::u16string result;
    for(auto ch:value){if(ch==u'\\' || ch==u',' || ch==u'+' || ch==u'&' || ch==u'*' || ch==u'[' || ch==u']')result+=u'\\';result+=ch;}
    return result;
}
std::u16string source_assembly(const Program& program){
    // Assembly display names escape punctuation independently of type names.
    const auto name=from_utf8(program.module_name);std::u16string result;
    const char16_t quote=name.find(u'"')!=name.npos ? u'\'' : name.find(u'\'')!=name.npos || name.back()==u' ' ? u'"' : 0;
    if(quote)result+=quote;
    for(auto ch:name){
        if(ch==u'\t'){result+=u"\\t";continue;}if(ch==u'\n'){result+=u"\\n";continue;}if(ch==u'\r'){result+=u"\\r";continue;}
        if(ch==u'\\' || ch==u',' || ch==u'=' || ch==quote)result+=u'\\';result+=ch;
    }
    if(quote)result+=quote;
    return result+u", Version=1.0.0.0, Culture=neutral, PublicKeyToken=null";
}
struct Definition {
    std::u16string name,name_space=u"System",assembly=u"mscorlib",suffix;
    std::vector<Type> arguments;
    bool source=false;
};
void arity(Definition& result){
    if(!result.arguments.empty())result.name+=u"`"+from_utf8(std::to_string(result.arguments.size()));
}
void dictionary(Definition& result,const Program& program,Type owner){
    const auto& info=type_info(program,owner);
    const auto& base=dictionary_kind(info.kind) ? info : type_info(program,info.key);
    const bool sorted=ordered_dictionary_kind(base.kind);
    result.name=base.kind==CompoundKind::sorted_list ? u"SortedList`2" : sorted ? u"SortedDictionary`2" : u"Dictionary`2";result.name_space=u"System.Collections.Generic";
    if(sorted)result.assembly=u"System";
    result.arguments={base.key,type_info(program,base.element).fields[1].type};
    if(info.kind==CompoundKind::dictionary_keys)result.name+=base.kind==CompoundKind::sorted_list ? u"+KeyList" : u"+KeyCollection";
    if(info.kind==CompoundKind::dictionary_values)result.name+=base.kind==CompoundKind::sorted_list ? u"+ValueList" : u"+ValueCollection";
}
void collection_iterator(Definition& result,const Program& program,Type owner){
    const auto& info=type_info(program,owner);
    if((info.kind==CompoundKind::dictionary_keys || info.kind==CompoundKind::dictionary_values)
       && type_info(program,info.key).kind==CompoundKind::sorted_list){
        dictionary(result,program,info.key);
        result.name+=info.kind==CompoundKind::dictionary_keys ? u"+SortedListKeyEnumerator" : u"+SortedListValueEnumerator";
        return;
    }
    if(dictionary_kind(info.kind) || info.kind==CompoundKind::dictionary_keys || info.kind==CompoundKind::dictionary_values)dictionary(result,program,owner);
    else if(info.kind==CompoundKind::list || info.kind==CompoundKind::hash_set || info.kind==CompoundKind::sorted_set){
        result.name=info.kind==CompoundKind::list ? u"List`1" : info.kind==CompoundKind::hash_set ? u"HashSet`1" : u"SortedSet`1";
        result.name_space=u"System.Collections.Generic";result.arguments={info.element};
        if(info.kind==CompoundKind::hash_set)result.assembly=u"System.Core";
        if(info.kind==CompoundKind::sorted_set)result.assembly=u"System";
    }else unsupported({});
    result.name+=u"+Enumerator";
}
Definition definition(const Program& program,ReflectedType type,Work& work){
    Definition result;
    const auto& source_namespace=program.source_namespace.empty() ? program.module_name : program.source_namespace;
    while(compound(type.target) && type_info(program,type.target).kind==CompoundKind::array){
        work.step();const auto& info=type_info(program,type.target);
        result.suffix=(info.rank==2 ? u"[,]" : u"[]")+result.suffix;type.target=info.element;
    }
    if(type.implementation==1){result.name=u"RuntimeType";return result;}
    if(type.implementation>=2 && type.implementation<=13){
        constexpr std::u16string_view names[]={u"UTF8Encoding",u"UnicodeEncoding",u"UTF32Encoding",u"ASCIIEncoding",u"SBCSCodePageEncoding",u"Latin1Encoding",u"DBCSCodePageEncoding",u"EUCJPEncoding",u"GB18030Encoding",u"ISCIIEncoding",u"ISO2022Encoding",u"UTF7Encoding"};
        result.name=names[type.implementation-2];result.name_space=u"System.Text";return result;
    }
    if(type.implementation>=16 && type.implementation<=21){
        const auto element=type_info(program,type.target).element;result.name_space=u"System.Collections.Generic";
        if(type.implementation==20){result.name=u"ByteEqualityComparer";return result;}
        result.name=type.implementation==16 ? (default_comparable(program.types,element) && !enumeration(program,element) ? u"GenericComparer" : u"ObjectComparer")
            : type.implementation==17 ? u"ComparisonComparer" : type.implementation==18 ? u"GenericEqualityComparer"
            : type.implementation==19 ? u"ObjectEqualityComparer" : u"EnumEqualityComparer";
        result.arguments={element};arity(result);return result;
    }
    if(type.implementation>=32){
        const auto provider=static_cast<SequenceProvider>((type.implementation-32)/16);
        if(type_info(program,type.target).kind==CompoundKind::enumerator && compound(type.source)
            && (provider==SequenceProvider::list || provider==SequenceProvider::hash_set || provider==SequenceProvider::dictionary_entries
                || provider==SequenceProvider::dictionary_keys || provider==SequenceProvider::dictionary_values)){
            collection_iterator(result,program,type.source);return result;
        }
        unsupported(work.location);
    }
    switch(type.target){
    case Type::integer:result.name=u"Int32";break;case Type::int64:result.name=u"Int64";break;
    case Type::real:result.name=u"Double";break;case Type::single:result.name=u"Single";break;
    case Type::boolean:result.name=u"Boolean";break;case Type::character:result.name=u"Char";break;
    case Type::string:result.name=u"String";break;case Type::byte:result.name=u"Byte";break;
    case Type::shortint:result.name=u"SByte";break;case Type::word:result.name=u"UInt16";break;
    case Type::smallint:result.name=u"Int16";break;case Type::longword:result.name=u"UInt32";break;
    case Type::uint64:result.name=u"UInt64";break;case Type::unit:result.name=u"Void";break;
    case Type::type_descriptor:result.name=u"Type";break;
    case Type::big_integer:result.name=u"BigInteger";result.name_space=u"System.Numerics";result.assembly=u"System.Numerics";break;
    case Type::text_file:result.name=u"Text";result.name_space=u"PABCSystem";result.source=true;break;
    default:{
        const auto& info=type_info(program,type.target);
        if (complex_number_type(info)) {
            result.name=u"Complex";result.name_space=u"System.Numerics";result.assembly=u"System.Numerics";break;
        }
        if (info.native_record==NativeRecord::date_time || info.native_record==NativeRecord::time_span) {
            result.name=info.native_record==NativeRecord::date_time?u"DateTime":u"TimeSpan";
            result.name_space=u"System";break;
        }
        if (info.native_record==NativeRecord::decimal_value) {result.name=u"Decimal";result.name_space=u"System";break;}
        if (info.native_record==NativeRecord::guid) {result.name=u"Guid";result.name_space=u"System";break;}
        if (info.native_record==NativeRecord::date_time_offset) {result.name=u"DateTimeOffset";result.name_space=u"System";break;}
        if(sql_scalar_record(info)){
            result.name=sql_scalar_type(info.name)->framework_name;
            result.name_space=u"System.Data.SqlTypes";
            result.assembly=u"System.Data";
            break;
        }
        if(sql_decimal_record(info) || sql_money_record(info) || sql_date_time_record(info) || sql_guid_record(info) || sql_binary_record(info)){
            if(sql_decimal_record(info))result.name=u"SqlDecimal";
            else if(sql_money_record(info))result.name=u"SqlMoney";
            else if(sql_date_time_record(info))result.name=u"SqlDateTime";
            else if(sql_guid_record(info))result.name=u"SqlGuid";
            else result.name=u"SqlBinary";
            result.name_space=u"System.Data.SqlTypes";
            result.assembly=u"System.Data";
            break;
        }
        if(database_null_type(info)){result.name=u"DBNull";break;}
        if (info.kind==CompoundKind::enumeration && info.name=="system.midpointrounding") {
            result.name=u"MidpointRounding";result.name_space=u"System";break;
        }
        if (info.kind==CompoundKind::enumeration && (info.name=="system.datetimekind" || info.name=="system.dayofweek")) {
            result.name=info.name=="system.datetimekind"?u"DateTimeKind":u"DayOfWeek";
            result.name_space=u"System";break;
        }
        if(ordinal_range_type(info) || real_interval_type(info)){
            result.name=real_interval_type(info) ? u"RealRange" : info.element==Type::character ? u"CharRange" : u"IntRange";
            result.name_space=u"PABCSystem";result.source=true;break;
        }
        if(collection_enumerator_type(info) || dictionary_enumerator_type(info)){collection_iterator(result,program,info.key);break;}
        if(key_value_pair_type(info)){
            result.name=u"KeyValuePair";result.name_space=u"System.Collections.Generic";
            result.arguments={info.fields[0].type,info.fields[1].type};arity(result);break;
        }
        switch(info.kind){
        case CompoundKind::boxed_object:result.name=u"Object";break;
        case CompoundKind::record:case CompoundKind::object_class:case CompoundKind::enumeration:case CompoundKind::user_interface:{
            auto name=from_utf8(info.display_name.empty() ? info.name : info.display_name);
            if(name.empty() || name==u"record" || name.front()==u'$')unsupported(work.location);
            const auto dot=name.rfind(u'.');result.name=dot==name.npos ? name : name.substr(dot+1);
            result.name_space=dot==name.npos ? from_utf8(source_namespace) : name.substr(0,dot);result.source=true;break;
        }
        case CompoundKind::dictionary:case CompoundKind::sorted_dictionary:case CompoundKind::sorted_list:case CompoundKind::dictionary_keys:case CompoundKind::dictionary_values:dictionary(result,program,type.target);break;
        case CompoundKind::tuple:
            result.name=u"Tuple";for(const auto& field:info.fields)result.arguments.push_back(field.type);arity(result);break;
        case CompoundKind::callable:
            if(info.name.starts_with("$delegate:")){
                result.name=from_utf8(info.display_name);result.name_space=from_utf8(source_namespace);result.source=true;break;
            }
            if(info.name=="predicate2" || info.name=="predicate3"){
                result.name=info.name=="predicate2" ? u"Predicate2" : u"Predicate3";result.name_space=u"PABCSystem";result.source=true;
                for(const auto& parameter:info.parameters)result.arguments.push_back(parameter.type);arity(result);break;
            }
            for(const auto& parameter:info.parameters){if(parameter.by_reference)unsupported(work.location);result.arguments.push_back(parameter.type);}
            result.name=info.name=="predicate" ? u"Predicate" : info.element==Type::unit ? u"Action" : u"Func";
            if(info.name!="predicate" && info.element!=Type::unit)result.arguments.push_back(info.element);arity(result);break;
        case CompoundKind::encoding:result.name=u"Encoding";result.name_space=u"System.Text";break;
        case CompoundKind::string_builder:result.name=u"StringBuilder";result.name_space=u"System.Text";break;
        case CompoundKind::number_format:result.name=u"NumberFormatInfo";result.name_space=u"System.Globalization";break;
        case CompoundKind::ini_file:result.name=u"TIniFile";result.name_space=u"IniFile";result.source=true;break;
        case CompoundKind::school_address:result.name=u"Addr32";result.name_space=u"School";result.source=true;break;
        case CompoundKind::school_ip:result.name=u"CalcIP";result.name_space=u"School";result.source=true;break;
        case CompoundKind::empty_collection:result.name=u"EmptyCollection";result.name_space=u"PABCSystem";result.source=true;break;
        default:{
            result.name_space=u"System.Collections.Generic";result.arguments={info.element};
            switch(info.kind){
            case CompoundKind::list:result.name=u"List";break;
            case CompoundKind::hash_set:result.name=u"HashSet";result.assembly=u"System.Core";break;
            case CompoundKind::sorted_set:result.name=u"SortedSet";result.assembly=u"System";break;
            case CompoundKind::sequence:result.name=u"IEnumerable";break;
            case CompoundKind::enumerator:result.name=u"IEnumerator";break;
            case CompoundKind::collection_interface:result.name=u"ICollection";break;
            case CompoundKind::list_interface:result.name=u"IList";break;
            case CompoundKind::set_interface:result.name=u"ISet";result.assembly=u"System";break;
            case CompoundKind::dictionary_interface:result.name=u"IDictionary";result.arguments={info.key,type_info(program,info.element).fields[1].type};break;
            case CompoundKind::comparable_interface:result.name=u"IComparable";result.name_space=u"System";break;
            case CompoundKind::equatable_interface:result.name=u"IEquatable";result.name_space=u"System";break;
            case CompoundKind::comparer_interface:result.name=u"IComparer";break;
            case CompoundKind::comparer:result.name=u"Comparer";break;
            case CompoundKind::equality_comparer_interface:result.name=u"IEqualityComparer";break;
            case CompoundKind::equality_comparer:result.name=u"EqualityComparer";break;
            case CompoundKind::new_set:result.name=u"NewSet";result.name_space=u"PABCSystem";result.source=true;break;
            case CompoundKind::stack:result.name=u"Stack";result.source=true;break;
            case CompoundKind::queue:result.name=u"Queue";result.source=true;break;
            case CompoundKind::dynarray:result.name=u"DynArray";result.source=true;break;
            case CompoundKind::simple_set:result.name=u"SimpleSet";result.source=true;break;
            case CompoundKind::linked_list:result.name=u"LinkedList";result.source=true;break;
            case CompoundKind::linked_node:result.name=u"LinkedListNode";result.source=true;break;
            case CompoundKind::single_node:result.name=u"SingleNode";result.source=true;break;
            case CompoundKind::association:result.name=u"AssocArray";result.source=true;result.arguments={info.key,info.element};break;
            default:throw runtime_error(RuntimeError::invalid_bytecode,work.location);
            }
            if(result.source && info.kind!=CompoundKind::new_set)result.name_space=u"Collections";
            arity(result);break;
        }}break;
    }}
    return result;
}
}
bool module_name_valid(std::string_view name){
    if(name.empty() || name.size()>4096 || static_cast<unsigned char>(name.front())<=32 || name.find_first_of("/\\:")!=name.npos || name.find('\0')!=name.npos)return false;
    try{(void)from_utf8(name);return true;}catch(const std::invalid_argument&){return false;}
}
namespace {
Value render_type_property(const Program& program,const ReflectedType& type,TypeOperation property,
    SourceLocation location,const ExecutionOptions& options,bool assembly_arguments) try {
    if(!reflected_type_valid(program,type) || !type_operation_valid(program,Type::type_descriptor,property)
        || property==TypeOperation::value_name || property==TypeOperation::runtime_type)throw runtime_error(RuntimeError::invalid_bytecode,location);
    Work work{location,options};work.step();
    if(type.target==Type::invalid)throw runtime_error(RuntimeError::nil_reference,location);
    auto root=definition(program,type,work);
    if(property==TypeOperation::name_space){work.step(root.name_space.size());return root.name_space.empty() ? Value{NilString{}} : Value{std::move(root.name_space)};}
    if(property==TypeOperation::name){
        const auto nested=root.name.rfind(u'+');auto name=(nested==root.name.npos ? root.name : root.name.substr(nested+1))+root.suffix;
        work.step(name.size());return name;
    }
    struct Task{ReflectedType type{};std::u16string text;bool literal=false,qualified=false,embedded=false;};
    std::vector<Task> pending;std::u16string result;
    const auto push=[&](Task task){if(pending.size()>=options.allocation_limit || pending.size()>=text_limit)throw runtime_error(RuntimeError::allocation_limit,location);pending.push_back(std::move(task));};
    const auto text=[&](std::u16string value){push({{},std::move(value),true});};
    push({type,{},false,property==TypeOperation::assembly_qualified_name});
    while(!pending.empty()){
        work.step();auto task=std::move(pending.back());pending.pop_back();
        if(task.literal){if(task.text.size()>text_limit-result.size())throw runtime_error(RuntimeError::string_limit,location);work.step(task.text.size());result+=task.text;continue;}
        auto info=definition(program,task.type,work);
        if(task.qualified){
            auto assembly=info.source ? source_assembly(program) : info.assembly+std::u16string(framework_identity);
            if(task.embedded){std::u16string escaped;for(auto ch:assembly){if(ch==u']')escaped+=u'\\';escaped+=ch;}assembly=std::move(escaped);}
            text(u", "+assembly);
        }
        text(std::move(info.suffix));
        if(!info.arguments.empty()){
            text(assembly_arguments ? u"]]" : u"]");
            for(std::size_t i=info.arguments.size();i>0;--i){
                push({ReflectedType{info.arguments[i-1]}, {},false,assembly_arguments,assembly_arguments});
                if(i>1)text(assembly_arguments ? u"],[" : u",");
            }
            text(assembly_arguments ? u"[[" : u"[");
        }
        text((info.name_space.empty() ? u"" : escaped_type_name(info.name_space)+u".")+info.name);
    }
    return result;
}catch(const std::bad_alloc&){throw runtime_error(RuntimeError::allocation_limit,location);}
catch(const std::invalid_argument&){throw runtime_error(RuntimeError::invalid_bytecode,location);}
}
Value reflected_type_property(const Program& program,const ReflectedType& type,TypeOperation property,
    SourceLocation location,const ExecutionOptions& options){
    return render_type_property(program,type,property,location,options,true);
}
std::u16string reflected_type_text(const Program& program,const ReflectedType& type,SourceLocation location,
    const ExecutionOptions& options){
    return std::get<std::u16string>(render_type_property(program,type,TypeOperation::full_name,location,options,false));
}
}

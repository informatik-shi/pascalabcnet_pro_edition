#include "type_names.hpp"
#include "runtime_values.hpp"
#include "file_encoding.hpp"
#include <new>
namespace abc {
namespace {
constexpr std::uint32_t runtime_type_implementation=1,sequence_implementation=32;
constexpr std::size_t limit=1'048'576;
bool valid_target(const Program& program,Type type){return type==Type::unit || valid_value_type(program,type);}
struct Work {
    SourceLocation location;const ExecutionOptions& options;std::uint64_t used=0;
    void step(std::size_t n=1){
        if(n>options.instruction_limit-used)throw runtime_error(RuntimeError::instruction_limit,location);
        if(options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,location);used+=n;
    }
};
std::u16string named(const TypeInfo& type){return from_utf8(type.display_name.empty() ? type.name : type.display_name);}
std::u16string byref_name(const Program& program,Type type,Work& work){
    std::u16string suffix;
    while(compound(type) && type_info(program,type).kind==CompoundKind::array){
        work.step();const auto& info=type_info(program,type);suffix=(info.rank==2 ? u"[,]" : u"[]")+suffix;type=info.element;
        if(suffix.size()>limit)throw runtime_error(RuntimeError::string_limit,work.location);
    }
    std::u16string name;
    switch(type){
    case Type::integer:name=u"Int32";break;case Type::int64:name=u"Int64";break;case Type::real:name=u"Double";break;
    case Type::single:name=u"Single";break;case Type::boolean:name=u"Boolean";break;case Type::string:name=u"String";break;case Type::character:name=u"Char";break;
    case Type::byte:name=u"Byte";break;case Type::shortint:name=u"SByte";break;case Type::word:name=u"UInt16";break;
    case Type::smallint:name=u"Int16";break;case Type::longword:name=u"UInt32";break;case Type::uint64:name=u"UInt64";break;
    case Type::big_integer:name=u"BigInteger";break;case Type::text_file:name=u"Text";break;
    case Type::type_descriptor:name=u"Type";break;case Type::unit:name=u"Void";break;
    default:{
        const auto& info=type_info(program,type);std::size_t arity=1;
        if(ordinal_range_type(info)){name=info.element==Type::character ? u"CharRange" : u"IntRange";arity=0;}
        else if(real_interval_type(info)){name=u"RealRange";arity=0;}
        else if(key_value_pair_type(info)){name=u"KeyValuePair";arity=2;}
        else switch(info.kind){
        case CompoundKind::record:case CompoundKind::object_class:case CompoundKind::enumeration:name=named(info);arity=0;break;
        case CompoundKind::list:name=u"List";break;case CompoundKind::stack:name=u"Stack";break;case CompoundKind::queue:name=u"Queue";break;
        case CompoundKind::dynarray:name=u"DynArray";break;case CompoundKind::simple_set:name=u"SimpleSet";break;
        case CompoundKind::hash_set:name=u"HashSet";break;case CompoundKind::new_set:name=u"NewSet";break;
        case CompoundKind::dictionary:name=u"Dictionary";arity=2;break;
        case CompoundKind::dictionary_keys:name=u"KeyCollection";arity=2;break;
        case CompoundKind::dictionary_values:name=u"ValueCollection";arity=2;break;
        case CompoundKind::association:name=u"AssocArray";arity=2;break;case CompoundKind::linked_list:name=u"LinkedList";break;
        case CompoundKind::linked_node:name=u"LinkedListNode";break;case CompoundKind::single_node:name=u"SingleNode";break;
        case CompoundKind::sequence:name=u"IEnumerable";break;
        case CompoundKind::tuple:name=u"Tuple";arity=info.fields.size();break;
        case CompoundKind::callable:name=info.element==Type::unit ? u"Action" : u"Func";arity=info.parameters.size()+(info.element==Type::unit ? 0U : 1U);break;
        case CompoundKind::encoding:name=u"Encoding";arity=0;break;case CompoundKind::ini_file:name=u"TIniFile";arity=0;break;
        case CompoundKind::school_address:name=u"Addr32";arity=0;break;case CompoundKind::school_ip:name=u"CalcIP";arity=0;break;
        case CompoundKind::empty_collection:name=u"EmptyCollection";arity=0;break;
        default:throw runtime_error(RuntimeError::invalid_bytecode,work.location);
        }
        if(arity){name+=char16_t{96};name+=from_utf8(std::to_string(arity));}break;
    }}
    return name+suffix+u"&";
}
}
bool reflected_type_valid(const Program& program,const ReflectedType& type){
    if(type.target==Type::invalid)return !type.implementation && type.source==Type::invalid;
    if(!valid_target(program,type.target))return false;
    if(!type.implementation)return type.source==Type::invalid;
    if(type.implementation==runtime_type_implementation)return type.target==Type::type_descriptor && type.source==Type::invalid;
    if(type.implementation>=2 && type.implementation<=6)return compound(type.target) && type_info(program,type.target).kind==CompoundKind::encoding && type.source==Type::invalid;
    if(type.implementation<sequence_implementation || type.implementation>=sequence_implementation+(static_cast<unsigned>(SequenceProvider::dictionary_values)+1U)*16U)return false;
    const auto provider=static_cast<SequenceProvider>((type.implementation-sequence_implementation)/16U);
    const auto variant=(type.implementation-sequence_implementation)%16U;
    const auto last=provider==SequenceProvider::transform ? static_cast<unsigned>(SequenceTransform::skip_while)
        : provider==SequenceProvider::generated ? static_cast<unsigned>(GeneratorMethod::while_two)
        : provider==SequenceProvider::slice ? static_cast<unsigned>(SequenceSlice::default_if_empty) : 0U;
    return variant<=last && compound(type.target) && type_info(program,type.target).kind==CompoundKind::sequence
        && (type.source==Type::invalid || valid_value_type(program,type.source));
}
bool type_operation_valid(const Program& program,Type input,TypeOperation operation){
    return static_cast<unsigned>(operation)<=static_cast<unsigned>(TypeOperation::runtime_type) && valid_value_type(program,input);
}
ReflectedType reflected_value_type(const Program& program,const Value& value,Type input,SourceLocation location,const ExecutionOptions& options){
    Work work{location,options};work.step();
    if(!type_operation_valid(program,input,TypeOperation::runtime_type))throw runtime_error(RuntimeError::invalid_bytecode,location);
    if(std::holds_alternative<NilString>(value))return {};
    if(const auto* descriptor=std::get_if<ReflectedType>(&value)){
        if(!reflected_type_valid(program,*descriptor))throw runtime_error(RuntimeError::invalid_bytecode,location);
        return descriptor->target==Type::invalid ? ReflectedType{} : ReflectedType{Type::type_descriptor,runtime_type_implementation};
    }
    if(const auto* file=std::get_if<std::shared_ptr<TextFile>>(&value))return *file ? ReflectedType{Type::text_file} : ReflectedType{};
    if(const auto* set=std::get_if<NewSetValue>(&value))return {set->type};
    const auto* object=std::get_if<std::shared_ptr<Aggregate>>(&value);
    if(!object)return {input};
    auto current=*object;
    for(std::size_t depth=0;depth<256;++depth){
        work.step();if(!current || current->nil)return {};
        const auto& info=type_info(program,current->type);
        if(info.kind==CompoundKind::encoding){
            if(!current->encoding)throw runtime_error(RuntimeError::invalid_bytecode,location);
            const auto kind=current->encoding->kind;
            return {current->type,kind==TextEncodingKind::utf8 ? 2U : kind==TextEncodingKind::utf16_le || kind==TextEncodingKind::utf16_be ? 3U : kind==TextEncodingKind::utf32_le || kind==TextEncodingKind::utf32_be ? 4U : kind==TextEncodingKind::ascii ? 5U : 6U};
        }
        if(info.kind!=CompoundKind::sequence)return {current->type};
        if(!current->sequence)throw runtime_error(RuntimeError::invalid_bytecode,location);
        const auto& sequence=*current->sequence;
        if(sequence.provider==SequenceProvider::string){if(sequence.seed && std::holds_alternative<NilString>(sequence.seed->value))return {};return {Type::string};}
        if(sequence.provider==SequenceProvider::ordinal_range && sequence.source)return {sequence.source->type};
        if(collection_sequence(sequence.provider) || sequence.provider==SequenceProvider::hash_set || dictionary_provider(sequence.provider)){current=sequence.source;continue;}
        if(sequence.provider==SequenceProvider::new_set){
            if(!sequence.seed || !std::holds_alternative<NewSetValue>(sequence.seed->value))throw runtime_error(RuntimeError::invalid_bytecode,location);
            return {std::get<NewSetValue>(sequence.seed->value).type};
        }
        const auto variant=sequence.provider==SequenceProvider::transform ? static_cast<unsigned>(sequence.transform_method)
            : sequence.provider==SequenceProvider::generated ? static_cast<unsigned>(sequence.generator_method)
            : sequence.provider==SequenceProvider::slice ? static_cast<unsigned>(sequence.slice_method) : 0U;
        return {current->type,sequence_implementation+static_cast<unsigned>(sequence.provider)*16U+variant,sequence.source ? sequence.source->type : Type::invalid};
    }
    throw runtime_error(RuntimeError::allocation_limit,location);
}
std::u16string reflected_type_name(const Program& program,const ReflectedType& type,SourceLocation location,const ExecutionOptions& options) try {
    if(!reflected_type_valid(program,type))throw runtime_error(RuntimeError::invalid_bytecode,location);
    Work work{location,options};std::u16string result;
    struct Task{Type type=Type::invalid;std::u16string text{};bool literal=false,reference=false;};
    std::vector<Task> pending;
    const auto text=[&](std::u16string value){pending.push_back({Type::invalid,std::move(value),true});};
    const auto child=[&](Type value,bool reference=false){pending.push_back({value,{},false,reference});};
    if(type.target==Type::invalid)text(u"nil");
    else if(type.implementation==runtime_type_implementation)text(u"RuntimeType");
    else if(type.implementation>=2 && type.implementation<=6){
        constexpr std::u16string_view names[]={u"UTF8Encoding",u"UnicodeEncoding",u"UTF32Encoding",u"ASCIIEncoding",u"SBCSCodePageEncoding"};text(std::u16string(names[type.implementation-2]));
    }
    else if(type.implementation>=sequence_implementation){child(type_info(program,type.target).element);text(u"sequence of ");}
    else child(type.target);
    while(!pending.empty()){
        work.step();if(pending.size()>options.allocation_limit || pending.size()>limit)throw runtime_error(RuntimeError::allocation_limit,location);
        auto task=std::move(pending.back());pending.pop_back();
        if(task.literal){
            if(task.text.size()>limit-result.size())throw runtime_error(RuntimeError::string_limit,location);
            work.step(task.text.size());result+=task.text;continue;
        }
        if(task.reference){
            auto root=task.type;
            while(compound(root) && type_info(program,root).kind==CompoundKind::array){work.step();root=type_info(program,root).element;}
            std::u16string name;std::vector<Type> arguments;
            if(compound(root)){const auto& info=type_info(program,root);
                switch(info.kind){
                case CompoundKind::list:name=u"List";break;case CompoundKind::stack:name=u"Stack";break;case CompoundKind::queue:name=u"Queue";break;
                case CompoundKind::dynarray:name=u"DynArray";break;case CompoundKind::simple_set:name=u"SimpleSet";break;case CompoundKind::hash_set:name=u"HashSet";break;
                case CompoundKind::new_set:name=u"NewSet";break;case CompoundKind::linked_list:name=u"LinkedList";break;case CompoundKind::linked_node:name=u"LinkedListNode";break;
                case CompoundKind::single_node:name=u"SingleNode";break;case CompoundKind::sequence:name=u"IEnumerable";break;
                case CompoundKind::record:if(key_value_pair_type(info)){name=u"KeyValuePair";for(const auto& field:info.fields)arguments.push_back(field.type);}break;
                case CompoundKind::dictionary:name=u"Dictionary";arguments.push_back(info.key);arguments.push_back(type_info(program,info.element).fields[1].type);break;
                case CompoundKind::dictionary_keys:case CompoundKind::dictionary_values:{name=info.kind==CompoundKind::dictionary_keys ? u"KeyCollection" : u"ValueCollection";const auto& dictionary=type_info(program,info.key);arguments.push_back(dictionary.key);arguments.push_back(type_info(program,dictionary.element).fields[1].type);break;}
                case CompoundKind::association:name=u"AssocArray";arguments.push_back(info.key);break;
                case CompoundKind::tuple:name=u"Tuple";for(const auto& field:info.fields)arguments.push_back(field.type);break;
                case CompoundKind::callable:
                    name=info.element==Type::unit ? u"Action" : u"Func";for(const auto& parameter:info.parameters)arguments.push_back(parameter.type);
                    if(info.element!=Type::unit)arguments.push_back(info.element);if(arguments.empty())name.clear();break;
                default:break;
                }
                if(!name.empty() && info.kind!=CompoundKind::tuple && info.kind!=CompoundKind::callable && !key_value_pair_type(info) && info.kind!=CompoundKind::dictionary && info.kind!=CompoundKind::dictionary_keys && info.kind!=CompoundKind::dictionary_values)arguments.push_back(info.element);
            }
            if(!name.empty()){
                // Reflection forwards root generic arguments through byref/array wrappers.
                // The source removes the suffix after the arity tick, including [] and &.
                text(u">");for(std::size_t i=arguments.size();i>0;--i){child(arguments[i-1]);if(i>1)text(u", ");}text(name+u"<");
            }else text(byref_name(program,task.type,work));
            continue;
        }
        if(task.type==Type::unit){text(u"Void");continue;}
        if(task.type==Type::type_descriptor){text(u"Type");continue;}
        if(!compound(task.type)){text(from_utf8(type_name(task.type)));continue;}
        const auto& info=type_info(program,task.type);
        if(ordinal_range_type(info)){text(info.element==Type::character ? u"CharRange" : u"IntRange");continue;}
        if(real_interval_type(info)){text(u"RealRange");continue;}
        if(key_value_pair_type(info) || info.kind==CompoundKind::dictionary || info.kind==CompoundKind::dictionary_keys || info.kind==CompoundKind::dictionary_values){
            const auto& pair=key_value_pair_type(info) ? info : info.kind==CompoundKind::dictionary ? type_info(program,info.element) : type_info(program,type_info(program,info.key).element);
            const auto name=key_value_pair_type(info) ? u"KeyValuePair" : info.kind==CompoundKind::dictionary ? u"Dictionary" : info.kind==CompoundKind::dictionary_keys ? u"KeyCollection" : u"ValueCollection";
            text(u">");child(pair.fields[1].type);text(u", ");child(pair.fields[0].type);text(std::u16string(name)+u"<");continue;
        }
        if(info.kind==CompoundKind::record || info.kind==CompoundKind::object_class || info.kind==CompoundKind::enumeration){text(named(info));continue;}
        if(info.kind==CompoundKind::array){child(info.element);text(info.rank==2 ? u"array[,] of " : u"array of ");continue;}
        if(info.kind==CompoundKind::new_set){child(info.element);text(u"set of ");continue;}
        if(info.kind==CompoundKind::tuple){
            text(u")");for(std::size_t i=info.fields.size();i>0;--i){child(info.fields[i-1].type);if(i>1)text(u", ");}text(u"(");continue;
        }
        if(info.kind==CompoundKind::callable){
            if(info.element!=Type::unit){child(info.element);text(u": ");}
            if(!info.parameters.empty()){text(u")");for(std::size_t i=info.parameters.size();i>0;--i){child(info.parameters[i-1].type,info.parameters[i-1].by_reference);if(i>1)text(u", ");}text(u"(");}
            text(info.element==Type::unit ? u"procedure" : u"function");continue;
        }
        if(info.kind==CompoundKind::encoding){text(u"Encoding");continue;}
        if(info.kind==CompoundKind::ini_file){text(u"TIniFile");continue;}
        if(info.kind==CompoundKind::empty_collection){text(u"EmptyCollection");continue;}
        if(info.kind==CompoundKind::school_address){text(u"Addr32");continue;}
        if(info.kind==CompoundKind::school_ip){text(u"CalcIP");continue;}
        std::u16string name;
        switch(info.kind){
        case CompoundKind::list:name=u"List";break;case CompoundKind::stack:name=u"Stack";break;
        case CompoundKind::queue:name=u"Queue";break;case CompoundKind::dynarray:name=u"DynArray";break;
        case CompoundKind::simple_set:name=u"SimpleSet";break;case CompoundKind::association:name=u"AssocArray";break;
        case CompoundKind::linked_node:name=u"LinkedListNode";break;case CompoundKind::linked_list:name=u"LinkedList";break;
        case CompoundKind::single_node:name=u"SingleNode";break;case CompoundKind::hash_set:name=u"HashSet";break;
        case CompoundKind::sequence:name=u"IEnumerable";break;
        default:throw runtime_error(RuntimeError::invalid_bytecode,location);
        }
        text(u">");child(info.element);if(info.kind==CompoundKind::association){text(u", ");child(info.key);}text(name+u"<");
    }
    return result;
}catch(const std::bad_alloc&){throw runtime_error(RuntimeError::allocation_limit,location);}
catch(const std::invalid_argument&){throw runtime_error(RuntimeError::invalid_bytecode,location);}
}

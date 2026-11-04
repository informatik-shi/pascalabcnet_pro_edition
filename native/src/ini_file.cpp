// Native port of bin/Lib/IniFile.pas.
// Original copyright (c) Ivan Bondarev, Stanislav Mikhalkovich.
// GNU LGPL; see doc/license.txt and doc/copyright.txt.
#include "ini_file.hpp"
#include "comparable.hpp"
#include "runtime_values.hpp"
#include "files.hpp"
#include "stdlib.hpp"
#include <algorithm>

namespace abc {
struct IniEntry {std::u16string name;Value value;};
struct IniSection {std::u16string name;std::vector<IniEntry> entries;};
struct IniState {Value filename;std::vector<IniSection> sections;};
bool ini_signature(const Program& program,Type type,IniMethod method) {
    return compound(type) && static_cast<unsigned>(type)-64U<program.types.size()
        && program.types[static_cast<unsigned>(type)-64U].kind==CompoundKind::ini_file && method<=IniMethod::read_boolean;
}
std::vector<Type> ini_parameters(Type type,IniMethod method) {
    switch(method) {
    case IniMethod::construct:return {Type::string};
    case IniMethod::save:case IniMethod::file_name:return {type};
    case IniMethod::read_integer:case IniMethod::write_integer:return {type,Type::string,Type::string,Type::integer};
    case IniMethod::read_boolean:case IniMethod::write_boolean:return {type,Type::string,Type::string,Type::boolean};
    default:return {type,Type::string,Type::string,Type::string};
    }
}
Type ini_result(Type type,IniMethod method) {
    switch(method) {
    case IniMethod::construct:return type;
    case IniMethod::file_name:case IniMethod::read_string:return Type::string;
    case IniMethod::read_integer:return Type::integer;
    case IniMethod::read_boolean:return Type::boolean;
    default:return Type::unit;
    }
}
namespace {
constexpr std::u16string_view newline=
#ifdef _WIN32
    u"\r\n";
#else
    u"\n";
#endif
class IniCall {
    Values& values;FileContext& files;const ExecutionOptions& options;std::uint64_t work=0;
    SourceLocation location() const{return values.location;}
    void checkpoint(std::uint64_t amount=1) {
        if(amount>options.instruction_limit || work>options.instruction_limit-amount)throw runtime_error(RuntimeError::instruction_limit,location());
        work+=amount;if(options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,location());
    }
    void charge(std::size_t amount) {
        if(amount>values.remaining)throw runtime_error(RuntimeError::allocation_limit,location());values.remaining-=amount;
    }
    std::size_t size(const Value& value) const{return string_text(value).size();}
    template<class T> std::pair<std::size_t,bool> find(const std::vector<T>& entries,const Value& name) {
        if(std::holds_alternative<NilString>(name))throw runtime_error(RuntimeError::nil_reference,location());
        const auto& key=string_text(name);std::size_t first=0,last=entries.size();
        const auto order=[&](std::size_t at){const auto& other=entries[at].name;checkpoint((key.size()+other.size())/64+1);return compare_culture_strings(other,key,location(),options);};
        while(first<last){const auto middle=first+(last-first)/2;if(order(middle)<0)first=middle+1;else last=middle;}
        return {first,first<entries.size() && order(first)==0};
    }
    void put(std::vector<IniEntry>& entries,const Value& key,const Value& value) {
        const auto [at,found]=find(entries,key);
        if(found){charge(size(value)+1);entries[at].value=value;return;}
        checkpoint(entries.size()-at+1);charge(size(key)+size(value)+1);
        entries.insert(entries.begin()+static_cast<std::ptrdiff_t>(at),IniEntry{string_text(key),value});
    }
    IniSection& section(IniState& state,const Value& key) {
        const auto [at,found]=find(state.sections,key);if(found)return state.sections[at];
        checkpoint(state.sections.size()-at+1);charge(size(key)+1);
        return *state.sections.insert(state.sections.begin()+static_cast<std::ptrdiff_t>(at),IniSection{string_text(key),{}});
    }
    void parse(IniState& state,std::u16string line,Value& current) {
        auto text=std::u16string_view(line);
        while(!text.empty() && text.front()==u' ')text.remove_prefix(1);
        while(!text.empty() && text.back()==u' ')text.remove_suffix(1);
        checkpoint(text.size()/64+1);if(text.empty())return;
        if(text.front()==u'['){current=std::u16string(text.size()>2 ? text.substr(1,text.size()-2) : std::u16string_view{});(void)section(state,current);return;}
        if(string_text(current).empty())return;const auto equal=text.find(u'=');if(equal==text.npos)return;
        const Value key=std::u16string(text.substr(0,equal)),value=std::u16string(text.substr(equal+1));
        put(section(state,current).entries,key,value);
    }
    void write_line(TextFile& file,std::u16string text) {
        checkpoint(text.size()/64+1);text_write(file,text,location());text_write(file,newline,location());
    }
public:
    IniCall(Values& v,FileContext& f,const ExecutionOptions& o):values(v),files(f),options(o){}
    Value construct(Type type,const Value& filename) {
        checkpoint();charge(size(filename)+1);auto object=std::get<std::shared_ptr<Aggregate>>(values.zero(type,true));
        object->ini=std::make_shared<IniState>(IniState{filename,{}});auto file=std::make_shared<TextFile>();text_assign(files,*file,filename,location());
        if(!file_exists(files,string_text(filename)))text_open(files,file,1,location());
        else {
            try {
                text_open(files,file,0,location());Value current=std::u16string{};
                while(!text_eof(*file,false,false,location())){auto line=std::get<std::u16string>(text_read(*file,Type::string,location()));text_skip_line(*file,location());parse(*object->ini,std::move(line),current);}
            } catch(const Error& error) {
                if(error.code=="E_LIMIT" || error.code=="E_CANCELLED" || error.code=="E_BYTECODE" || error.code=="E_LOCALE")throw;
            }
        }
        // Source Close is outside the swallowed parse/open exceptions.
        file->close(location());return object;
    }
    void save(const IniState& state) {
        checkpoint();auto file=std::make_shared<TextFile>();text_assign(files,*file,state.filename,location());text_open(files,file,1,location());
        for(const auto& part:state.sections){
            write_line(*file,u"["+part.name+u"]");
            for(const auto& entry:part.entries)write_line(*file,entry.name+u"="+string_text(entry.value));
            write_line(*file,{});
        }
        file->close(location());
    }
    void write(IniState& state,const Value& group,const Value& key,const Value& value) {
        const auto [at,found]=find(state.sections,group);
        if(found){put(state.sections[at].entries,key,value);return;}
        // The source inserts into the new inner list before publishing the section.
        IniSection next{string_text(group),{}};put(next.entries,key,value);
        checkpoint(state.sections.size()-at+1);charge(size(group)+1);
        state.sections.insert(state.sections.begin()+static_cast<std::ptrdiff_t>(at),std::move(next));
    }
    Value read(const IniState& state,const Value& group,const Value& key,const Value& fallback) {
        const auto [at,found]=find(state.sections,group);if(!found)return fallback;
        const auto [entry,present]=find(state.sections[at].entries,key);return present ? state.sections[at].entries[entry].value : fallback;
    }
};
}
Value ini_invoke(Values& values,FileContext& files,Type type,IniMethod method,std::span<const Value> arguments,const ExecutionOptions& options) {
    if(!ini_signature(values.program,type,method) || arguments.size()!=ini_parameters(type,method).size())throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    try {
        IniCall call(values,files,options);if(method==IniMethod::construct)return call.construct(type,arguments[0]);
        const auto& object=std::get<std::shared_ptr<Aggregate>>(arguments[0]);
        if(!object || object->nil)throw runtime_error(RuntimeError::nil_reference,values.location);
        if(object->type!=type || !object->ini)throw runtime_error(RuntimeError::invalid_bytecode,values.location);
        auto& state=*object->ini;
        switch(method) {
        case IniMethod::file_name:return state.filename;
        case IniMethod::save:call.save(state);return false;
        case IniMethod::write_string:call.write(state,arguments[1],arguments[2],arguments[3]);return false;
        case IniMethod::write_integer:call.write(state,arguments[1],arguments[2],from_utf8(std::to_string(std::get<std::int32_t>(arguments[3]))));return false;
        case IniMethod::write_boolean:call.write(state,arguments[1],arguments[2],std::u16string(std::get<bool>(arguments[3]) ? u"True" : u"False"));return false;
        case IniMethod::read_string:return call.read(state,arguments[1],arguments[2],arguments[3]);
        case IniMethod::read_integer: {
            const auto text=call.read(state,arguments[1],arguments[2],from_utf8(std::to_string(std::get<std::int32_t>(arguments[3]))));
            return stdlib::try_parse_number(Type::integer,text,values.location).value_or(Value{std::int32_t{0}});
        }
        case IniMethod::read_boolean: {
            const auto text=call.read(state,arguments[1],arguments[2],std::u16string(std::get<bool>(arguments[3]) ? u"True" : u"False"));
            if(std::holds_alternative<NilString>(text))throw runtime_error(RuntimeError::nil_reference,values.location);
            auto result=string_text(text);for(auto& c:result)if(c>=u'a' && c<=u'z')c=static_cast<char16_t>(c-32);return result==u"TRUE";
        }
        default:throw runtime_error(RuntimeError::invalid_bytecode,values.location);
        }
    }catch(const std::bad_alloc&){throw runtime_error(RuntimeError::allocation_limit,values.location);}
     catch(const std::length_error&){throw runtime_error(RuntimeError::allocation_limit,values.location);}
}
} // namespace abc

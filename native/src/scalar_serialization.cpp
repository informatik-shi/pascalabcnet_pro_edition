#include "scalar_serialization.hpp"
#include "binary_codec.hpp"
#include "runtime_values.hpp"
#include <algorithm>
#include <array>

namespace abc {
namespace {
// MS-NRBF 2.1.2 and the scalar records emitted by PascalABC.NET.
enum class Record : std::uint8_t { header=0, system_class=4, string=6, end=11, primitive_array=15 };
struct Primitive {
    Type type;
    std::uint8_t code;
    std::u16string_view name;
};
constexpr std::array primitives{
    Primitive{Type::boolean,1,u"System.Boolean"},
    Primitive{Type::byte,2,u"System.Byte"},
    Primitive{Type::character,3,u"System.Char"},
    Primitive{Type::real,6,u"System.Double"},
    Primitive{Type::smallint,7,u"System.Int16"},
    Primitive{Type::integer,8,u"System.Int32"},
    Primitive{Type::int64,9,u"System.Int64"},
    Primitive{Type::shortint,10,u"System.SByte"},
    Primitive{Type::single,11,u"System.Single"},
    Primitive{Type::word,14,u"System.UInt16"},
    Primitive{Type::longword,15,u"System.UInt32"},
    Primitive{Type::uint64,16,u"System.UInt64"},
};

[[noreturn]] void invalid(SourceLocation location) {
    throw Error("E_FORMAT",location,"Invalid or unsupported serialized value",
        "Недопустимое или неподдерживаемое сериализованное значение");
}
void checkpoint(std::size_t index,SourceLocation location,const ExecutionOptions& options) {
    if (index>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,location);
    if ((index&255U)==0 && options.cancelled && options.cancelled())
        throw runtime_error(RuntimeError::cancelled,location);
}
}

bool nrbf_primitive_type(Type type) {
    return std::any_of(primitives.begin(),primitives.end(),[&](const Primitive& entry){return entry.type==type;});
}
bool nrbf_primitive_array(const std::vector<TypeInfo>& types,Type type) {
    if (!compound(type) || static_cast<unsigned>(type)-64U>=types.size()) return false;
    const auto& shape=types[static_cast<unsigned>(type)-64U];
    return shape.kind==CompoundKind::array && shape.rank==1 && nrbf_primitive_type(shape.element);
}

void serialize_value(BinaryDevice& stream,const Program& program,Type type,const Value& value,SourceLocation location,const ExecutionOptions& options) {
    const bool array=nrbf_primitive_array(program.types,type);
    const auto payload=array ? type_info(program,type).element : type;
    const auto primitive=std::find_if(primitives.begin(),primitives.end(),[&](const Primitive& entry){return entry.type==payload;});
    if ((type!=Type::string && primitive==primitives.end()) || std::holds_alternative<NilString>(value))
        invalid(location);
    BinaryCodec codec(stream,{TextEncodingKind::utf8,false,true});
    const auto byte=[&](std::uint8_t field){codec.write(Type::byte,field,location,options);};
    const auto integer=[&](std::int32_t field){codec.write(Type::integer,field,location,options);};

    byte(static_cast<std::uint8_t>(Record::header));
    integer(1);
    integer(-1);
    integer(1);
    integer(0);
    if (array) {
        const auto& data=std::get<std::shared_ptr<Aggregate>>(value);
        if (!data || data->nil || data->items.size()>static_cast<std::size_t>(INT32_MAX)) invalid(location);
        byte(static_cast<std::uint8_t>(Record::primitive_array));
        integer(1);
        integer(static_cast<std::int32_t>(data->items.size()));
        byte(primitive->code);
        if (payload==Type::character) {
            std::u16string characters;
            characters.reserve(data->items.size());
            for (std::size_t i=0;i<data->items.size();++i) {
                checkpoint(i,location,options);
                characters.push_back(std::get<char16_t>(data->items[i]->value));
            }
            stream.write(encode_text_bytes(characters,{TextEncodingKind::utf8,false,true},location),location);
        } else {
            for (std::size_t i=0;i<data->items.size();++i) {
                checkpoint(i,location,options);
                codec.write(payload,data->items[i]->value,location,options);
            }
        }
    } else if (type==Type::string) {
        byte(static_cast<std::uint8_t>(Record::string));
        integer(1);
    } else {
        byte(static_cast<std::uint8_t>(Record::system_class));
        integer(1);
        codec.write_string(primitive->name,location,options);
        integer(1);
        codec.write_string(u"m_value",location,options);
        byte(0);
        byte(primitive->code);
    }
    if (!array) codec.write(type,value,location,options);
    byte(static_cast<std::uint8_t>(Record::end));
}

SerializedValue deserialize_value(BinaryDevice& stream,Values& values,SourceLocation location,const ExecutionOptions& options) {
    BinaryCodec codec(stream,{TextEncodingKind::utf8,false,true});
    const auto byte=[&]{return std::get<std::uint8_t>(codec.read(Type::byte,location,options));};
    const auto integer=[&]{return std::get<std::int32_t>(codec.read(Type::integer,location,options));};

    if (static_cast<Record>(byte())!=Record::header) invalid(location);
    const auto root=integer();
    (void)integer();
    if (root<=0 || integer()!=1 || integer()!=0) invalid(location);
    const auto record=static_cast<Record>(byte());
    if (integer()!=root) invalid(location);

    if (record==Record::primitive_array) {
        const auto length=integer();
        const auto primitive_code=byte();
        const auto primitive=std::find_if(primitives.begin(),primitives.end(),[&](const Primitive& entry){return entry.code==primitive_code;});
        if (length<0 || primitive==primitives.end()) invalid(location);
        const auto size=static_cast<std::size_t>(length);
        if (size>values.remaining || size>1'048'576)
            throw runtime_error(RuntimeError::allocation_limit,location);
        if (size>options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,location);
        const auto array=std::find_if(values.program.types.begin(),values.program.types.end(),[&](const TypeInfo& info){
            return info.kind==CompoundKind::array && info.rank==1 && info.element==primitive->type;
        });
        if (array==values.program.types.end()) invalid(location);
        const auto type=static_cast<Type>(64+(array-values.program.types.begin()));
        auto data=values.array(type,size,true);
        if (primitive->type==Type::character) {
            TextDecoder decoder({TextEncodingKind::utf8,false,true},false);
            for (std::size_t i=0,bytes=0;i<size;) {
                checkpoint(bytes++,location,options);
                const auto next=static_cast<char>(byte());
                const auto text=decoder.decode(std::string_view(&next,1),false,location);
                if (text.size()>size-i) invalid(location);
                for (const auto character:text) values.write(data->items[i++],character);
            }
        } else {
            for (std::size_t i=0;i<size;++i) {
                checkpoint(i,location,options);
                values.write(data->items[i],codec.read(primitive->type,location,options));
            }
        }
        if (static_cast<Record>(byte())!=Record::end) invalid(location);
        return {type,std::move(data)};
    }

    auto type=Type::string;
    if (record==Record::system_class) {
        const auto name=codec.read_string(location,options);
        if (integer()!=1 || codec.read_string(location,options)!=u"m_value" || byte()!=0) invalid(location);
        const auto primitive_code=byte();
        const auto primitive=std::find_if(primitives.begin(),primitives.end(),[&](const Primitive& entry){
            return entry.code==primitive_code && entry.name==name;
        });
        if (primitive==primitives.end()) invalid(location);
        type=primitive->type;
    } else if (record!=Record::string) invalid(location);
    auto value=codec.read(type,location,options);
    if (static_cast<Record>(byte())!=Record::end) invalid(location);
    return {type,std::move(value)};
}
}

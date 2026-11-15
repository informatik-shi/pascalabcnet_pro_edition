#include "scalar_serialization.hpp"
#include "binary_codec.hpp"
#include <algorithm>
#include <array>

namespace abc {
namespace {
// MS-NRBF 2.1.2 and the scalar records emitted by PascalABC.NET.
enum class Record : std::uint8_t { header=0, system_class=4, string=6, end=11 };
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
    throw Error("E_FORMAT",location,"Invalid or unsupported serialized scalar",
        "Недопустимое или неподдерживаемое сериализованное скалярное значение");
}
}

void serialize_scalar(BinaryDevice& stream,Type type,const Value& value,SourceLocation location,const ExecutionOptions& options) {
    const auto primitive=std::find_if(primitives.begin(),primitives.end(),[&](const Primitive& entry){return entry.type==type;});
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
    if (type==Type::string) {
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
    codec.write(type,value,location,options);
    byte(static_cast<std::uint8_t>(Record::end));
}

SerializedScalar deserialize_scalar(BinaryDevice& stream,SourceLocation location,const ExecutionOptions& options) {
    BinaryCodec codec(stream,{TextEncodingKind::utf8,false,true});
    const auto byte=[&]{return std::get<std::uint8_t>(codec.read(Type::byte,location,options));};
    const auto integer=[&]{return std::get<std::int32_t>(codec.read(Type::integer,location,options));};

    if (static_cast<Record>(byte())!=Record::header) invalid(location);
    const auto root=integer();
    (void)integer();
    if (root<=0 || integer()!=1 || integer()!=0) invalid(location);
    const auto record=static_cast<Record>(byte());
    if (integer()!=root) invalid(location);

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

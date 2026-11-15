#include "scalar_serialization.hpp"
#include "binary_codec.hpp"
#include "calendar.hpp"
#include "decimal.hpp"
#include "runtime_values.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <map>
#include <optional>

namespace abc {
namespace {
// MS-NRBF 2.1.2 and the scalar records emitted by PascalABC.NET.
enum class Record : std::uint8_t {
    header=0, system_class=4, user_class=5, string=6, binary_array=7, member_reference=9, nil=10, end=11,
    library=12, nil_run_byte=13, nil_run=14, primitive_array=15, string_array=17
};
constexpr std::uint8_t rectangular_shape=2, primitive_member=0;
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
struct SerializedMember {
    std::u16string_view name;
    Type type;
    std::uint8_t code;
};
struct NativeLayout {
    NativeRecord tag;
    std::u16string_view name;
    std::span<const SerializedMember> members;
    bool numerics_library=false;
};
constexpr std::array decimal_members{
    SerializedMember{u"flags",Type::integer,8},
    SerializedMember{u"hi",Type::integer,8},
    SerializedMember{u"lo",Type::integer,8},
    SerializedMember{u"mid",Type::integer,8}
};
constexpr std::array date_members{
    SerializedMember{u"ticks",Type::int64,9},
    SerializedMember{u"dateData",Type::uint64,16}
};
constexpr std::array span_members{SerializedMember{u"_ticks",Type::int64,9}};
constexpr std::array complex_members{
    SerializedMember{u"m_real",Type::real,6},
    SerializedMember{u"m_imaginary",Type::real,6}
};
constexpr std::array native_layouts{
    NativeLayout{NativeRecord::decimal_value,u"System.Decimal",decimal_members},
    NativeLayout{NativeRecord::date_time,u"System.DateTime",date_members},
    NativeLayout{NativeRecord::time_span,u"System.TimeSpan",span_members},
    NativeLayout{NativeRecord::complex,u"System.Numerics.Complex",complex_members,true}
};
constexpr std::u16string_view numerics_assembly=
    u"System.Numerics, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089";
const NativeLayout* native_layout(const Program& program,Type type) {
    if (!compound(type)) return nullptr;
    const auto tag=type_info(program,type).native_record;
    for (const auto& layout:native_layouts) if (layout.tag==tag) return &layout;
    return nullptr;
}
Type registered_native_type(const Program& program,NativeRecord tag,SourceLocation location) {
    for (std::size_t i=0;i<program.types.size();++i)
        if (program.types[i].native_record==tag) return static_cast<Type>(64+i);
    invalid(location);
}
void write_native_value(BinaryCodec& codec,const NativeLayout& layout,const Value& value,
    SourceLocation location,const ExecutionOptions& options) {
    const auto byte=[&](std::uint8_t field){codec.write(Type::byte,field,location,options);};
    const auto integer=[&](std::int32_t field){codec.write(Type::integer,field,location,options);};
    if (layout.numerics_library) {
        byte(static_cast<std::uint8_t>(Record::library));
        integer(2);
        codec.write_string(numerics_assembly,location,options);
    }
    byte(static_cast<std::uint8_t>(layout.numerics_library?Record::user_class:Record::system_class));
    integer(1);
    codec.write_string(layout.name,location,options);
    integer(static_cast<std::int32_t>(layout.members.size()));
    for (const auto& member:layout.members) codec.write_string(member.name,location,options);
    for (std::size_t i=0;i<layout.members.size();++i) byte(primitive_member);
    for (const auto& member:layout.members) byte(member.code);
    if (layout.numerics_library) integer(2);
    if (layout.tag==NativeRecord::decimal_value) {
        const auto words=decimal_bits(value);
        for (const auto index:decimal_file_order) integer(std::bit_cast<std::int32_t>(words[index]));
    } else if (layout.tag==NativeRecord::date_time) {
        const auto raw=date_time_binary_bits(value);
        codec.write(Type::int64,static_cast<std::int64_t>(raw&date_time_ticks_mask),location,options);
        codec.write(Type::uint64,raw,location,options);
    } else {
        const auto& fields=std::get<std::shared_ptr<Aggregate>>(value)->items;
        for (std::size_t i=0;i<layout.members.size();++i)
            codec.write(layout.members[i].type,fields[i]->value,location,options);
    }
}
SerializedValue read_native_value(BinaryCodec& codec,Values& values,const NativeLayout& layout,
    std::optional<std::int32_t> library,SourceLocation location,const ExecutionOptions& options) {
    const auto byte=[&]{return std::get<std::uint8_t>(codec.read(Type::byte,location,options));};
    const auto integer=[&]{return std::get<std::int32_t>(codec.read(Type::integer,location,options));};
    if (library.has_value()!=layout.numerics_library || integer()!=static_cast<std::int32_t>(layout.members.size()))
        invalid(location);
    for (const auto& member:layout.members) if (codec.read_string(location,options)!=member.name) invalid(location);
    for (std::size_t i=0;i<layout.members.size();++i) if (byte()!=primitive_member) invalid(location);
    for (const auto& member:layout.members) if (byte()!=member.code) invalid(location);
    if (library && integer()!=*library) invalid(location);
    const auto type=registered_native_type(values.program,layout.tag,location);
    if (layout.tag==NativeRecord::decimal_value) {
        std::array<std::uint32_t,4> words{};
        for (const auto index:decimal_file_order) words[index]=std::bit_cast<std::uint32_t>(integer());
        return {type,decimal_from_bits(values,type,words)};
    }
    if (layout.tag==NativeRecord::date_time) {
        (void)codec.read(Type::int64,location,options);
        const auto data=std::get<std::uint64_t>(codec.read(Type::uint64,location,options));
        return {type,date_time_from_binary_bits(values,type,data)};
    }
    auto result=std::get<std::shared_ptr<Aggregate>>(values.zero(type,false,true));
    for (std::size_t i=0;i<layout.members.size();++i)
        result->items[i]->value=codec.read(layout.members[i].type,location,options);
    return {type,std::move(result)};
}
void checkpoint(std::size_t index,SourceLocation location,const ExecutionOptions& options) {
    if (index>=options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,location);
    if ((index&255U)==0 && options.cancelled && options.cancelled())
        throw runtime_error(RuntimeError::cancelled,location);
}
void write_strings(BinaryCodec& codec,const Aggregate& array,SourceLocation location,const ExecutionOptions& options) {
    const auto record=[&](Record code){codec.write(Type::byte,static_cast<std::uint8_t>(code),location,options);};
    const auto integer=[&](std::int32_t field){codec.write(Type::integer,field,location,options);};
    std::int32_t next_id=2;
    for (std::size_t i=0;i<array.items.size();) {
        checkpoint(i,location,options);
        if (!std::holds_alternative<NilString>(array.items[i]->value)) {
            record(Record::string);
            integer(next_id++);
            codec.write_string(string_text(array.items[i++]->value),location,options);
            continue;
        }
        const auto first=i;
        while (i<array.items.size() && std::holds_alternative<NilString>(array.items[i]->value)) {
            checkpoint(i,location,options);
            ++i;
        }
        const auto count=i-first;
        if (count==1) {
            record(Record::nil);
        } else if (count<=255) {
            record(Record::nil_run_byte);
            codec.write(Type::byte,static_cast<std::uint8_t>(count),location,options);
        } else {
            record(Record::nil_run);
            integer(static_cast<std::int32_t>(count));
        }
    }
}
void read_strings(BinaryCodec& codec,Aggregate& array,Values& values,std::int32_t root,SourceLocation location,const ExecutionOptions& options) {
    const auto byte=[&]{return std::get<std::uint8_t>(codec.read(Type::byte,location,options));};
    const auto integer=[&]{return std::get<std::int32_t>(codec.read(Type::integer,location,options));};
    const auto charge=[&](std::size_t units){
        if (units>values.remaining) throw runtime_error(RuntimeError::allocation_limit,location);
        values.remaining-=units;
    };
    std::map<std::int32_t,std::size_t> definitions;
    std::vector<std::pair<std::size_t,std::int32_t>> references;
    for (std::size_t i=0;i<array.items.size();) {
        checkpoint(i,location,options);
        switch (static_cast<Record>(byte())) {
        case Record::string: {
            const auto id=integer();
            if (id<=0 || id==root || !definitions.emplace(id,i).second) invalid(location);
            auto text=codec.read_string(location,options);
            charge(text.size());
            array.items[i++]->value=std::move(text);
            break;
        }
        case Record::member_reference: {
            const auto id=integer();
            if (id<=0 || id==root) invalid(location);
            references.emplace_back(i++,id);
            break;
        }
        case Record::nil:
            ++i;
            break;
        case Record::nil_run_byte: {
            const auto count=byte();
            if (count==0 || count>array.items.size()-i) invalid(location);
            i+=count;
            break;
        }
        case Record::nil_run: {
            const auto count=integer();
            if (count<=0 || static_cast<std::size_t>(count)>array.items.size()-i) invalid(location);
            i+=static_cast<std::size_t>(count);
            break;
        }
        default: invalid(location);
        }
    }
    for (const auto& [index,id]:references) {
        checkpoint(index,location,options);
        const auto known=definitions.find(id);
        if (known==definitions.end()) invalid(location);
        const auto& value=array.items[known->second]->value;
        charge(string_text(value).size());
        array.items[index]->value=value;
    }
}
}

bool nrbf_primitive_type(Type type) {
    return std::any_of(primitives.begin(),primitives.end(),[&](const Primitive& entry){return entry.type==type;});
}
bool nrbf_array_type(const std::vector<TypeInfo>& types,Type type) {
    if (!compound(type) || static_cast<unsigned>(type)-64U>=types.size()) return false;
    const auto& shape=types[static_cast<unsigned>(type)-64U];
    return shape.kind==CompoundKind::array
        && ((shape.rank==1 && shape.element==Type::string)
            || ((shape.rank==1 || shape.rank==2) && nrbf_primitive_type(shape.element)));
}

void serialize_value(BinaryDevice& stream,const Program& program,Type type,const Value& value,SourceLocation location,const ExecutionOptions& options) {
    const bool array=nrbf_array_type(program.types,type);
    const auto* layout=native_layout(program,type);
    const auto payload=array ? type_info(program,type).element : type;
    const bool matrix=array && type_info(program,type).rank==2;
    const auto primitive=std::find_if(primitives.begin(),primitives.end(),[&](const Primitive& entry){return entry.type==payload;});
    if ((!layout && payload!=Type::string && primitive==primitives.end()) || std::holds_alternative<NilString>(value))
        invalid(location);
    BinaryCodec codec(stream,{TextEncodingKind::utf8,false,true});
    const auto byte=[&](std::uint8_t field){codec.write(Type::byte,field,location,options);};
    const auto integer=[&](std::int32_t field){codec.write(Type::integer,field,location,options);};

    byte(static_cast<std::uint8_t>(Record::header));
    integer(1);
    integer(-1);
    integer(1);
    integer(0);
    if (layout) {
        write_native_value(codec,*layout,value,location,options);
    } else if (array) {
        const auto& data=std::get<std::shared_ptr<Aggregate>>(value);
        if (!data || data->nil || data->items.size()>static_cast<std::size_t>(INT32_MAX)) invalid(location);
        const auto record=matrix ? Record::binary_array : payload==Type::string ? Record::string_array : Record::primitive_array;
        byte(static_cast<std::uint8_t>(record));
        integer(1);
        if (matrix) {
            const auto rows=data->rows, columns=data->columns;
            if (rows>static_cast<std::size_t>(INT32_MAX) || columns>static_cast<std::size_t>(INT32_MAX)
                || (rows && columns>data->items.size()/rows) || rows*columns!=data->items.size())
                invalid(location);
            byte(rectangular_shape);
            integer(2);
            integer(static_cast<std::int32_t>(rows));
            integer(static_cast<std::int32_t>(columns));
            byte(primitive_member);
        } else {
            integer(static_cast<std::int32_t>(data->items.size()));
        }
        if (payload!=Type::string) byte(primitive->code);
        if (payload==Type::string) {
            write_strings(codec,*data,location,options);
        } else if (payload==Type::character && !matrix) {
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
        byte(primitive_member);
        byte(primitive->code);
    }
    if (!array && !layout) codec.write(type,value,location,options);
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
    auto record=static_cast<Record>(byte());
    std::optional<std::int32_t> library;
    if (record==Record::library) {
        library=integer();
        if (*library<=0 || codec.read_string(location,options)!=numerics_assembly) invalid(location);
        record=static_cast<Record>(byte());
    }
    if (library.has_value()!=(record==Record::user_class)) invalid(location);
    if (integer()!=root) invalid(location);

    const bool matrix=record==Record::binary_array;
    if (record==Record::primitive_array || record==Record::string_array || matrix) {
        std::int32_t rows=1, columns;
        if (matrix) {
            if (byte()!=rectangular_shape || integer()!=2) invalid(location);
            rows=integer();
            columns=integer();
            if (byte()!=primitive_member) invalid(location);
        } else {
            columns=integer();
        }
        auto element=Type::string;
        if (record!=Record::string_array) {
            const auto primitive_code=byte();
            const auto primitive=std::find_if(primitives.begin(),primitives.end(),[&](const Primitive& entry){return entry.code==primitive_code;});
            if (primitive==primitives.end()) invalid(location);
            element=primitive->type;
        }
        if (rows<0 || columns<0) invalid(location);
        const auto count=static_cast<std::uint64_t>(rows)*static_cast<std::uint64_t>(columns);
        if (count>values.remaining || count>1'048'576)
            throw runtime_error(RuntimeError::allocation_limit,location);
        if (count>options.instruction_limit) throw runtime_error(RuntimeError::instruction_limit,location);
        const auto size=static_cast<std::size_t>(count);
        const auto rank=matrix ? 2U : 1U;
        const auto array=std::find_if(values.program.types.begin(),values.program.types.end(),[&](const TypeInfo& info){
            return info.kind==CompoundKind::array && info.rank==rank && info.element==element;
        });
        if (array==values.program.types.end()) invalid(location);
        const auto type=static_cast<Type>(64+(array-values.program.types.begin()));
        auto data=matrix ? values.matrix(type,rows,columns,true) : values.array(type,size,true);
        if (element==Type::string) {
            read_strings(codec,*data,values,root,location,options);
        } else if (element==Type::character && !matrix) {
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
                values.write(data->items[i],codec.read(element,location,options));
            }
        }
        if (static_cast<Record>(byte())!=Record::end) invalid(location);
        return {type,std::move(data)};
    }

    auto type=Type::string;
    if (record==Record::system_class || record==Record::user_class) {
        const auto name=codec.read_string(location,options);
        const auto layout=std::find_if(native_layouts.begin(),native_layouts.end(),[&](const auto& entry){return entry.name==name;});
        if (layout!=native_layouts.end()) {
            auto value=read_native_value(codec,values,*layout,library,location,options);
            if (static_cast<Record>(byte())!=Record::end) invalid(location);
            return value;
        }
        if (record==Record::user_class) invalid(location);
        if (integer()!=1 || codec.read_string(location,options)!=u"m_value" || byte()!=primitive_member) invalid(location);
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

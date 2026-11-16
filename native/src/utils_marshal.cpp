#include "utils_marshal.hpp"
#include "calendar.hpp"
#include "cp1251.hpp"
#include "decimal.hpp"
#include "guid.hpp"
#include "integer_values.hpp"
#include "runtime_values.hpp"
#include <algorithm>
#include <bit>
#include <map>
#include <span>

namespace abc {
namespace {
struct Layout {
    std::size_t size;
    std::size_t alignment;
    std::vector<std::size_t> offsets;
};

std::size_t scalar_width(Type type) {
    switch(type){
    case Type::character:case Type::byte:case Type::shortint:return 1;
    case Type::smallint:case Type::word:return 2;
    case Type::integer:case Type::longword:case Type::single:case Type::boolean:return 4;
    case Type::int64:case Type::uint64:case Type::real:return 8;
    default:return 0;
    }
}

class RecordMarshaller {
    Values& values_;
    const ExecutionOptions& options_;
    std::map<Type,Layout> layouts_;
    std::uint64_t work_=0;

    [[noreturn]] void unsupported() const {
        throw Error("E_TYPE",values_.location,"RecordToByteArray requires a value with a supported unmanaged layout",
            "RecordToByteArray требует значение с поддерживаемой структурой в неуправляемой памяти");
    }
    void check_work(unsigned depth) {
        if(depth>64 || work_>=options_.instruction_limit)
            throw runtime_error(RuntimeError::instruction_limit,values_.location);
        if((work_++&255U)==0 && options_.cancelled && options_.cancelled())
            throw runtime_error(RuntimeError::cancelled,values_.location);
    }
    std::size_t checked_add(std::size_t offset,std::size_t size) const {
        constexpr auto bound=static_cast<std::size_t>(INT32_MAX);
        if(size>bound-offset)throw runtime_error(RuntimeError::allocation_limit,values_.location);
        return offset+size;
    }
    std::size_t align(std::size_t offset,std::size_t boundary) const {
        return checked_add(offset,(boundary-offset%boundary)%boundary);
    }
    const Layout& layout(Type type,unsigned depth) {
        check_work(depth);
        if(const auto found=layouts_.find(type);found!=layouts_.end())return found->second;
        const auto width=scalar_width(type);
        if(width)return layouts_.emplace(type,Layout{width,width,{}}).first->second;
        if(!compound(type))unsupported();
        const auto& shape=type_info(values_.program,type);
        if(shape.kind==CompoundKind::enumeration)
            return layouts_.emplace(type,Layout{4,4,{}}).first->second;
        if(shape.kind!=CompoundKind::record)unsupported();
        if(decimal_type(shape))return layouts_.emplace(type,Layout{16,8,{}}).first->second;
        if(guid_record_type(shape))return layouts_.emplace(type,Layout{16,4,{}}).first->second;
        if(shape.native_record==NativeRecord::date_time)return layouts_.emplace(type,Layout{8,8,{}}).first->second;
        if(shape.native_record!=NativeRecord::none && shape.native_record!=NativeRecord::time_span
            && !complex_number_type(shape))unsupported();
        Layout result{0,1,{}};
        for(const auto& field:shape.fields){
            const auto& part=layout(field.type,depth+1);
            const auto offset=align(result.size,part.alignment);
            result.offsets.push_back(offset);
            result.size=checked_add(offset,part.size);
            result.alignment=std::max(result.alignment,part.alignment);
        }
        result.size=align(std::max<std::size_t>(1,result.size),result.alignment);
        return layouts_.emplace(type,std::move(result)).first->second;
    }
    std::uint64_t scalar_bits(Type type,const Value& value) const {
        switch(type){
        case Type::single:return std::bit_cast<std::uint32_t>(std::get<float>(value));
        case Type::real:return std::bit_cast<std::uint64_t>(std::get<double>(value));
        case Type::character:return windows1251_ordinal(std::get<char16_t>(value));
        case Type::uint64:return std::get<std::uint64_t>(value);
        default:return static_cast<std::uint64_t>(integral_number(value,values_.location));
        }
    }
    static void write_bits(std::span<std::uint8_t> output,std::uint64_t bits) {
        for(auto& byte:output){
            byte=static_cast<std::uint8_t>(bits);
            bits>>=8;
        }
    }
    void write(std::span<std::uint8_t> output,Type type,const Value& value,unsigned depth) {
        check_work(depth);
        if(scalar_width(type)){write_bits(output,scalar_bits(type,value));return;}
        const auto& shape=type_info(values_.program,type);
        if(shape.kind==CompoundKind::enumeration){
            write_bits(output,static_cast<std::uint32_t>(std::get<EnumValue>(value).value));
            return;
        }
        if(decimal_type(shape)){
            const auto words=decimal_bits(value);
            for(std::size_t i=0;i<decimal_file_order.size();++i)
                write_bits(output.subspan(4*i,4),words[decimal_file_order[i]]);
            return;
        }
        if(shape.native_record==NativeRecord::date_time){
            write_bits(output,std::bit_cast<std::uint64_t>(calendar_ole_date(value,values_.location)));
            return;
        }
        if(guid_record_type(shape)){
            const auto bytes=guid_bytes(value);
            std::copy(bytes.begin(),bytes.end(),output.begin());
            return;
        }
        const auto& record=*std::get<std::shared_ptr<Aggregate>>(value);
        const auto& structure=layouts_.at(type);
        for(std::size_t i=0;i<shape.fields.size();++i){
            const auto& field=*record.items[i];
            const auto& part=layouts_.at(field.type);
            write(output.subspan(structure.offsets[i],part.size),field.type,field.value,depth+1);
        }
    }
public:
    RecordMarshaller(Values& values,const ExecutionOptions& options):values_(values),options_(options){}

    Value convert(Type result,const Value& object) {
        const auto& boxed=*std::get<std::shared_ptr<Aggregate>>(object);
        if(boxed.nil)throw runtime_error(RuntimeError::nil_reference,values_.location);
        if(boxed.items.size()!=1)throw runtime_error(RuntimeError::invalid_bytecode,values_.location);
        const auto& source=*boxed.items.front();
        if(compound(source.type)){
            const auto& shape=type_info(values_.program,source.type);
            if(shape.kind==CompoundKind::enumeration || shape.native_record==NativeRecord::date_time)unsupported();
        }
        const auto size=layout(source.type,0).size;
        if(size>values_.remaining)throw runtime_error(RuntimeError::allocation_limit,values_.location);
        std::vector<std::uint8_t> bytes(size);
        write(bytes,source.type,source.value,0);
        auto output=values_.array(result,size);
        for(std::size_t i=0;i<size;++i)output->items[i]->value=bytes[i];
        return output;
    }
};
} // namespace

Value utils_record_to_bytes(Values& values,Type result,const Value& object,const ExecutionOptions& options) {
    return RecordMarshaller(values,options).convert(result,object);
}
} // namespace abc

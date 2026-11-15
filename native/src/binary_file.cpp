#include "binary_file.hpp"
#include "decimal.hpp"
#include "runtime_values.hpp"
#include "files.hpp"
#include "file_reader.hpp"
#include "file_writer.hpp"
#include "encoding.hpp"
#include "encoding_metadata.hpp"
#include "type_names.hpp"
#include "fixed_array.hpp"
#include "integer_values.hpp"
#include "scalar_serialization.hpp"
#include "single.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
namespace abc {
namespace {
const TypeInfo& info(const std::vector<TypeInfo>& types,Type t){return types.at(static_cast<unsigned>(t)-64U);}
Type encoding_type(const std::vector<TypeInfo>& types){for(std::size_t n=0;n<types.size();++n)if(types[n].kind==CompoundKind::encoding)return static_cast<Type>(64+n);return Type::invalid;}
BinaryMethod method(std::size_t operand){return static_cast<BinaryMethod>(operand&255U);}
Type payload(std::size_t operand){return static_cast<Type>(operand>>8U);}
bool same_path(const std::filesystem::path& a,const std::filesystem::path& b){if(a==b)return true;std::error_code e;return std::filesystem::equivalent(a,b,e)&&!e;}
void assigned(const BinaryFile& file,SourceLocation location){if(file.filename.empty())throw runtime_error(RuntimeError::binary_unassigned,location);}
void opened(const BinaryFile& file,SourceLocation location){assigned(file,location);if(!file.is_open())throw runtime_error(RuntimeError::binary_closed,location);}
BinaryFile& file_value(const Value& value,SourceLocation location){const auto object=std::get<std::shared_ptr<Aggregate>>(value);if(!object||object->nil)throw runtime_error(RuntimeError::nil_reference,location);if(!object->binary)throw runtime_error(RuntimeError::invalid_bytecode,location);return *object->binary;}
void assign(FileContext& files,BinaryFile& file,const Value& name,SourceLocation location){if(std::holds_alternative<NilString>(name))throw runtime_error(RuntimeError::nil_reference,location);file.filename=files.resolve(string_text(name),location);}
void open(FileContext& files,const std::shared_ptr<BinaryFile>& file,bool create,const TextEncoding& encoding,SourceLocation location){
    assigned(*file,location);
    if(file->element!=Type::invalid&&!std::get<bool>(encoding_metadata_value(static_cast<std::uint16_t>(encoding.kind),EncodingMethod::is_single_byte,0,location)))throw runtime_error(RuntimeError::typed_file_encoding,location);
    if(file->is_open()){file->stream->seek(0,location);if(create)file->stream->truncate(location);return;}
    files.prepare_binary_open(file,location);auto stream=std::make_unique<BinaryStream>(file->filename,create,location);
    auto codec=std::make_unique<BinaryCodec>(*stream,encoding,file->element!=Type::invalid);file->stream=std::move(stream);file->codec=std::move(codec);
}
Value read_value(Values& values,BinaryFile& file,Type type,const ExecutionOptions& options,unsigned depth){
    if(depth>64)throw runtime_error(RuntimeError::sequence_depth,values.location);
    if(!compound(type))return file.codec->read(type,values.location,options);
    const auto& shape=type_info(values.program,type);
    if(shape.kind==CompoundKind::enumeration)return EnumValue{type,std::get<std::int32_t>(file.codec->read(Type::integer,values.location,options))};
    if (decimal_type(shape)) {
        std::array<std::uint32_t,4> words{};
        for (const auto index:decimal_file_order)
            words[index]=std::get<std::uint32_t>(file.codec->read(Type::longword,values.location,options));
        return decimal_from_bits(values,type,words);
    }
    if(shape.kind==CompoundKind::fixed_array){auto result=fixed_array_construct(values,type);const auto wrapper=std::get<std::shared_ptr<Aggregate>>(result),data=std::get<std::shared_ptr<Aggregate>>(wrapper->items[0]->value);for(const auto& item:data->items)values.write(item,read_value(values,file,shape.element,options,depth+1));return result;}
    auto result=std::get<std::shared_ptr<Aggregate>>(values.zero(type,false,true));
    for(std::size_t n=0;n<shape.fields.size();++n)values.write(result->items[n],read_value(values,file,shape.fields[n].type,options,depth+1));return result;
}
void write_value(Values& values,BinaryFile& file,Type type,const Value& value,const ExecutionOptions& options,unsigned depth){
    if(depth>64)throw runtime_error(RuntimeError::sequence_depth,values.location);
    if(!compound(type)){file.codec->write(type,value,values.location,options);return;}
    const auto& shape=type_info(values.program,type);
    if(shape.kind==CompoundKind::enumeration){file.codec->write(Type::integer,std::get<EnumValue>(value).value,values.location,options);return;}
    const auto data=std::get<std::shared_ptr<Aggregate>>(value);if(!data||data->nil)throw runtime_error(RuntimeError::nil_reference,values.location);
    if (decimal_type(shape)) {
        const auto words=decimal_bits(value);
        for (const auto index:decimal_file_order) file.codec->write(Type::longword,words[index],values.location,options);
        return;
    }
    if(shape.kind==CompoundKind::fixed_array){const auto storage=std::get<std::shared_ptr<Aggregate>>(data->items[0]->value);for(const auto& item:storage->items)write_value(values,file,shape.element,item->value,options,depth+1);return;}
    std::uint64_t work=0;for(std::size_t n=0;n<data->items.size();++n){if(work++>=options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,values.location);if((n&255U)==0&&options.cancelled&&options.cancelled())throw runtime_error(RuntimeError::cancelled,values.location);
        const auto element=shape.kind==CompoundKind::array?shape.element:shape.fields.at(n).type;write_value(values,file,element,data->items[n]->value,options,depth+1);}
}
ReflectedType element_type(const BinaryFile& file) {
    return file.element_override.value_or(ReflectedType{file.element});
}
Type selected_element(const Program& program,const BinaryFile& file,SourceLocation location) {
    const auto type=element_type(file).target;
    if (type==Type::invalid) throw runtime_error(RuntimeError::nil_reference,location);
    if (!binary_value_supported(program.types,type,false))
        throw Error("E_UNSUPPORTED",location,"Binary layout of the selected ElementType is not supported",
            "Двоичный формат выбранного ElementType ещё не поддерживается");
    return type;
}
Error element_cast_error(SourceLocation location) {
    return Error("E_CAST",location,"The selected ElementType is incompatible with the file data value",
        "Выбранный ElementType несовместим со значением элемента файла");
}
bool element_enum(const Program& program,Type type) {
    return compound(type) && type_info(program,type).kind==CompoundKind::enumeration;
}
Value convert_element_scalar(Type type,const Value& value,SourceLocation location) {
    const auto source=std::get_if<EnumValue>(&value);
    const Value number=source?Value{source->value}:value;
    if (type==Type::boolean || type==Type::single || type==Type::real) {
        return std::visit([&](const auto& v)->Value {
            using Number=std::decay_t<decltype(v)>;
            if constexpr (std::is_arithmetic_v<Number> && !std::is_same_v<Number,char16_t>) {
                if (type==Type::boolean) return v!=0;
                if (type==Type::single) return static_cast<float>(v);
                return static_cast<double>(v);
            } else throw element_cast_error(location);
        },number);
    }
    if (type==Type::character) {
        if (!integer_type(value_type(number))) throw element_cast_error(location);
        return static_cast<char16_t>(std::get<std::uint16_t>(
            checked_small_integer(Type::word,integral_number(number,location,"char"),location)));
    }
    if (!integer_type(type)) throw element_cast_error(location);
    if (const auto from=value_type(number); from==Type::single || from==Type::real) {
        const auto real=from==Type::real?std::get<double>(number):static_cast<double>(std::get<float>(number));
        double integral=0;
        const auto fraction=std::modf(real,&integral);
        if (std::abs(fraction)>0.5 || (std::abs(fraction)==0.5 && std::fmod(integral,2.0)!=0))
            integral+=std::copysign(1.0,fraction);
        return floating_integer_cast(type,Value{integral},location);
    }
    if (!integer_type(value_type(number)) && value_type(number)!=Type::boolean && value_type(number)!=Type::character)
        throw element_cast_error(location);
    if (wide_unsigned(type)) return checked_unsigned_integer(type,number,location);
    if (small_integer(type)) return checked_small_integer(type,integral_number(number,location,type_name(type)),location);
    return checked_signed_integer(type,number,location);
}
Value read_element(Values& values,BinaryFile& file,Type declared,const ExecutionOptions& options) {
    if (!file.element_override) return read_value(values,file,declared,options,0);
    const auto actual=selected_element(values.program,file,values.location);
    auto result=read_value(values,file,actual,options,0);
    if (actual==declared) return result;
    if (const auto* enumerated=std::get_if<EnumValue>(&result)) {
        if (declared==Type::integer) return enumerated->value;
        if (element_enum(values.program,declared)) return EnumValue{declared,enumerated->value};
    }
    if (actual==Type::integer && element_enum(values.program,declared))
        return EnumValue{declared,std::get<std::int32_t>(result)};
    throw element_cast_error(values.location);
}
void write_element(Values& values,BinaryFile& file,Type type,const Value& value,const ExecutionOptions& options) {
    if (!file.element_override || (compound(type) && type_info(values.program,type).kind==CompoundKind::array)) {
        write_value(values,file,type,value,options,0);
        return;
    }
    const auto actual=selected_element(values.program,file,values.location);
    if (actual==type) {
        write_value(values,file,type,value,options,0);
        return;
    }
    const auto location=values.location;
    Value converted=element_enum(values.program,actual)
        ? Value{EnumValue{actual,std::get<std::int32_t>(convert_element_scalar(Type::integer,value,location))}}
        : convert_element_scalar(actual,value,location);
    write_value(values,file,actual,converted,options,0);
}
}
bool binary_value_supported(const std::vector<TypeInfo>& types,Type type,bool strings,bool arrays){
    const auto walk=[&](auto&& self,Type t,unsigned depth)->bool{if(depth>64)return false;
        if(t==Type::string)return strings;if(t==Type::big_integer)return false;if(scalar(t))return true;
        if(!compound(t)||static_cast<unsigned>(t)-64U>=types.size())return false;const auto& shape=info(types,t);
        if(shape.native_record==NativeRecord::date_time)return false;
        if(shape.kind==CompoundKind::enumeration)return true;
        if(shape.kind==CompoundKind::fixed_array)return self(self,shape.element,depth+1);
        if(shape.kind==CompoundKind::array)return arrays&&self(self,shape.element,depth+1);
        if(shape.kind!=CompoundKind::record)return false;
        return std::all_of(shape.fields.begin(),shape.fields.end(),[&](const auto& f){return self(self,f.type,depth+1);});};
    return walk(walk,type,0);
}
std::int64_t binary_element_size(const Program& program,Type type,SourceLocation location){
    const auto walk=[&](auto&& self,Type t,unsigned depth)->std::int64_t{if(depth>64)throw runtime_error(RuntimeError::sequence_depth,location);
        if(t==Type::boolean||t==Type::byte||t==Type::shortint||t==Type::character)return 1;
        if(t==Type::word||t==Type::smallint)return 2;
        if(t==Type::integer||t==Type::longword||t==Type::single)return 4;
        if(t==Type::int64||t==Type::uint64||t==Type::real)return 8;
        if(short_string(t))return short_string_capacity(t)+1;
        const auto& shape=type_info(program,t);if(shape.kind==CompoundKind::enumeration)return 4;
        if(shape.kind==CompoundKind::fixed_array){const auto& s=fixed_array_shape(program,t);const auto count=static_cast<std::int64_t>(s.upper)-s.lower+1;const auto bytes=self(self,shape.element,depth+1);if(bytes&&count>INT64_MAX/bytes)throw runtime_error(RuntimeError::allocation_limit,location);return count*bytes;}std::int64_t size=0;
        for(const auto& field:shape.fields){const auto n=self(self,field.type,depth+1);if(n>std::numeric_limits<std::int64_t>::max()-size)throw runtime_error(RuntimeError::allocation_limit,location);size+=n;}return size;};
    return walk(walk,type,0);
}
void binary_element_probe(Values& values,Type type){
    if(!compound(type))return;const auto& shape=type_info(values.program,type);
    if(shape.kind==CompoundKind::fixed_array){(void)fixed_array_construct(values,type);binary_element_probe(values,shape.element);}
    else if(shape.kind==CompoundKind::record)for(const auto& field:shape.fields)binary_element_probe(values,field.type);
}
std::vector<Type> binary_parameters(const std::vector<TypeInfo>& types,Type owner,std::size_t operand){
    const auto code=method(operand);const auto& shape=info(types,owner);const auto encoding=encoding_type(types);
    switch(code){case BinaryMethod::assign:case BinaryMethod::rename:case BinaryMethod::reset_named:case BinaryMethod::rewrite_named:return {owner,Type::string};
    case BinaryMethod::reset_encoding:case BinaryMethod::rewrite_encoding:return {owner,encoding};
    case BinaryMethod::reset_named_encoding:case BinaryMethod::rewrite_named_encoding:return {owner,Type::string,encoding};
    case BinaryMethod::seek:return {owner,Type::int64};case BinaryMethod::read_bytes:return {owner,Type::integer};
    case BinaryMethod::set_element_type:return {owner,Type::type_descriptor};
    case BinaryMethod::write_bytes:return {owner,shape.key};case BinaryMethod::write_one:case BinaryMethod::write_many:return {owner,payload(operand)};
    case BinaryMethod::writeln_data:return {owner,payload(operand)};
    case BinaryMethod::serialize:return {owner,payload(operand)};
    case BinaryMethod::serialize_named:return {Type::string,payload(operand)};
    case BinaryMethod::deserialize_named:return {Type::string};
    default:return {owner};}
}
Type binary_result(const std::vector<TypeInfo>& types,Type owner,std::size_t operand){
    switch(method(operand)){case BinaryMethod::eof:return Type::boolean;case BinaryMethod::position:case BinaryMethod::size:return Type::int64;
    case BinaryMethod::element_type:return Type::type_descriptor;
    case BinaryMethod::name:case BinaryMethod::full_name:case BinaryMethod::to_string:return Type::string;
    case BinaryMethod::read_data:case BinaryMethod::deserialize:case BinaryMethod::deserialize_named:return payload(operand);
    case BinaryMethod::read_bytes:return info(types,owner).key;
    default:return Type::unit;}
}
bool binary_signature_valid(const Program& program,Type owner,std::size_t operand){
    if(!compound(owner)||static_cast<unsigned>(owner)-64U>=program.types.size())return false;const auto& shape=type_info(program,owner);
    if(shape.kind!=CompoundKind::binary_file||method(operand)>BinaryMethod::set_element_type)return false;
    const auto code=method(operand);const auto t=payload(operand);
    if (code==BinaryMethod::element_type || code==BinaryMethod::set_element_type)
        return shape.element!=Type::invalid && t==Type::integer;
    if (code>=BinaryMethod::serialize)
        return shape.element==Type::invalid && compound(t) && valid_value_type(program,t)
            && type_info(program,t).kind==CompoundKind::boxed_object;
    if(code==BinaryMethod::read_data)return binary_value_supported(program.types,t,shape.element==Type::invalid)&& (shape.element==Type::invalid||shape.element==t);
    const auto writable=[&](Type value){return binary_value_supported(program.types,value,true,true)&&(shape.element==Type::invalid||value==shape.element||(compound(value)&&type_info(program,value).kind==CompoundKind::array));};
    if(code==BinaryMethod::write_one)return writable(t);
    if(code==BinaryMethod::write_many){const auto pack=[&](auto&& self,Type item,unsigned depth)->bool{if(depth>64||!compound(item)||static_cast<unsigned>(item)-64U>=program.types.size())return false;const auto& tuple=type_info(program,item);return tuple.kind==CompoundKind::tuple&&std::all_of(tuple.fields.begin(),tuple.fields.end(),[&](const auto& f){return writable(f.type)||(compound(f.type)&&self(self,f.type,depth+1));});};return pack(pack,t,0);}
    if(code==BinaryMethod::writeln_data)return valid_value_type(program,t);
    if((code==BinaryMethod::read_bytes||code==BinaryMethod::write_bytes)&&shape.element!=Type::invalid)return false;
    return t==Type::integer;
}
void FileContext::check_binary_conflict(const std::filesystem::path& path,SourceLocation location) const{for(const auto& weak:opened_->binaries)if(const auto file=weak.lock();file&&file->is_open()&&same_path(path,file->stream->path()))throw runtime_error(RuntimeError::file_io,location);}
void FileContext::prepare_binary_open(const std::shared_ptr<BinaryFile>& file,SourceLocation location){
    std::erase_if(opened_->texts,[](const auto& p){const auto f=p.lock();return !f||!f->is_open();});
    std::erase_if(opened_->readers,[](const auto& p){const auto f=p.lock();return !f||!f->is_open();});
    std::erase_if(opened_->binaries,[](const auto& p){const auto f=p.lock();return !f||!f->is_open();});
    check_binary_conflict(file->filename,location);
    for(const auto& weak:opened_->texts)if(const auto text=weak.lock();text&&((text->reader&&text->reader->is_open()&&same_path(text->reader->path(),file->filename))||(text->writer&&text->writer->is_open()&&same_path(text->writer->path(),file->filename))))throw runtime_error(RuntimeError::file_io,location);
    for(const auto& weak:opened_->readers)if(const auto reader=weak.lock();reader&&same_path(reader->path(),file->filename))throw runtime_error(RuntimeError::file_io,location);
    if(opened_->texts.size()+opened_->readers.size()+opened_->binaries.size()>=64)throw runtime_error(RuntimeError::binary_open_limit,location);
    opened_->binaries.push_back(file);
}
Value binary_invoke(Values& values,FileContext& files,Type owner,std::size_t operand,std::span<const Value> args){
    const auto code=method(operand);const auto location=values.location;if(code==BinaryMethod::writeln||code==BinaryMethod::writeln_data)throw runtime_error(RuntimeError::binary_writeln,location);
    const ExecutionOptions defaults;const auto& options=files.execution_options?*files.execution_options:defaults;
    if (code>=BinaryMethod::serialize && code<=BinaryMethod::deserialize_named) {
        const bool writing=code==BinaryMethod::serialize || code==BinaryMethod::serialize_named;
        std::shared_ptr<BinaryFile> temporary;
        BinaryFile* target=nullptr;
        if (code==BinaryMethod::serialize_named || code==BinaryMethod::deserialize_named) {
            temporary=std::make_shared<BinaryFile>();
            assign(files,*temporary,args.front(),location);
            open(files,temporary,writing,{TextEncodingKind::utf8,false,true},location);
            target=temporary.get();
        } else {
            target=&file_value(args.front(),location);
            opened(*target,location);
        }
        if (writing) {
            const auto& object=std::get<std::shared_ptr<Aggregate>>(args.back());
            if (object->nil || object->items.size()!=1)
                throw Error("E_FORMAT",location,"Serialization requires a supported boxed value",
                    "Для сериализации требуется поддерживаемое значение в object");
            const auto& item=*object->items.front();
            serialize_value(*target->stream,values.program,item.type,item.value,location,options);
            return false;
        }
        auto stored=deserialize_value(*target->stream,values,location,options);
        return values.box_value(payload(operand),stored.type,std::move(stored.value));
    }
    auto& file=file_value(args[0],location);
    if(code==BinaryMethod::element_type)return element_type(file);
    if(code==BinaryMethod::set_element_type){file.element_override=std::get<ReflectedType>(args[1]);return false;}
    if(code==BinaryMethod::assign){assign(files,file,args[1],location);return false;}
    if(code>=BinaryMethod::reset&&code<=BinaryMethod::rewrite_named_encoding){
        const auto n=static_cast<unsigned>(code)-static_cast<unsigned>(BinaryMethod::reset);const bool create=(n&1U)!=0,named=(n==2||n==3||n==6||n==7),explicit_encoding=n>=4;
        if(named)assign(files,file,args[1],location);assigned(file,location);
        const auto selected=explicit_encoding?&encoding_value(args.back(),location):files.encoding(location);
        open(files,std::get<std::shared_ptr<Aggregate>>(args[0])->binary,create,selected?*selected:TextEncoding{},location);return false;
    }
    if(code==BinaryMethod::to_string){
        if(file.element==Type::invalid)return std::u16string(u"file");
        auto name=std::u16string(u"file of ");
        const auto element=element_type(file);
        if(element.target!=Type::invalid)
            name+=std::get<std::u16string>(reflected_type_property(values.program,element,TypeOperation::full_name,location,options));
        return name;
    }
    if(code==BinaryMethod::name||code==BinaryMethod::full_name){if(file.filename.empty())throw runtime_error(RuntimeError::nil_reference,location);return code==BinaryMethod::name?file.filename.filename().u16string():file.filename.u16string();}
    assigned(file,location);
    if(code==BinaryMethod::erase){if(!delete_file(files,file.filename.u16string())&&file_exists(files,file.filename.u16string()))throw runtime_error(RuntimeError::file_erase,location);return false;}
    if(code==BinaryMethod::rename){if(!files.rename(file.filename.u16string(),string_text(args[1]),false))throw runtime_error(RuntimeError::file_rename,location);return false;}
    opened(file,location);
    switch(code){case BinaryMethod::close:file.codec.reset();file.stream->close();file.stream.reset();return false;
    case BinaryMethod::write_empty:return false;
    case BinaryMethod::truncate:file.stream->truncate(location);return false;
    case BinaryMethod::eof:return file.stream->position(location)==file.stream->length(location);
    case BinaryMethod::size:{const auto length=file.stream->length(location);if(!file.element_size)throw runtime_error(RuntimeError::division_zero,location);if(length%file.element_size)throw runtime_error(RuntimeError::typed_file_size,location);return length/file.element_size;}
    case BinaryMethod::position:if(!file.element_size)throw runtime_error(RuntimeError::division_zero,location);return file.stream->position(location)/file.element_size;
    case BinaryMethod::seek:{const auto index=std::get<std::int64_t>(args[1]);const auto product=static_cast<std::uint64_t>(index)*static_cast<std::uint64_t>(file.element_size);file.stream->seek(std::bit_cast<std::int64_t>(product),location);return false;}
    case BinaryMethod::read_data:return read_element(values,file,payload(operand),options);
    case BinaryMethod::write_one:write_element(values,file,payload(operand),args[1],options);return false;
    case BinaryMethod::write_many:{const auto pack=std::get<std::shared_ptr<Aggregate>>(args[1]);const auto flatten=[&](auto&& self,Type t,const std::shared_ptr<Aggregate>& a,unsigned depth)->void{if(depth>64)throw runtime_error(RuntimeError::sequence_depth,location);const auto& shape=type_info(values.program,t);for(std::size_t n=0;n<shape.fields.size();++n){const auto field=shape.fields[n].type;if(compound(field)&&type_info(values.program,field).kind==CompoundKind::tuple)self(self,field,std::get<std::shared_ptr<Aggregate>>(a->items[n]->value),depth+1);else write_element(values,file,field,a->items[n]->value,options);}};flatten(flatten,payload(operand),pack,0);return false;}
    case BinaryMethod::read_bytes:{const auto count=std::get<std::int32_t>(args[1]);if(count<0)throw runtime_error(RuntimeError::binary_buffer_count,location);if(static_cast<std::uint32_t>(count)>values.remaining)throw runtime_error(RuntimeError::allocation_limit,location);std::string data(static_cast<std::size_t>(count),'\0');std::size_t done=0;while(done<data.size()){const auto n=file.stream->read(std::span<char>(data).subspan(done),location);if(!n)break;done+=n;}auto result=values.array(type_info(values.program,owner).key,done);for(std::size_t n=0;n<done;++n)result->items[n]->value=static_cast<std::uint8_t>(data[n]);return result;}
    case BinaryMethod::write_bytes:{const auto data=std::get<std::shared_ptr<Aggregate>>(args[1]);if(!data||data->nil)throw runtime_error(RuntimeError::nil_reference,location);std::string bytes;bytes.reserve(data->items.size());for(const auto& item:data->items)bytes+=static_cast<char>(std::get<std::uint8_t>(item->value));file.stream->write(bytes,location);return false;}
    case BinaryMethod::writeln:throw runtime_error(RuntimeError::binary_writeln,location);
    default:throw runtime_error(RuntimeError::invalid_bytecode,location);}
}
}

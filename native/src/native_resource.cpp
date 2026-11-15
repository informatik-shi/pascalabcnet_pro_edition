#include "native_resource.hpp"
#include "binary_file.hpp"
#include "runtime_values.hpp"
#include "fixed_array.hpp"
#include "string_builder.hpp"
#include "number_format.hpp"
#include "complex_number.hpp"
namespace abc {
namespace {
bool resource_kind(const Program& program,Type type,CompoundKind kind) {
    return compound(type) && static_cast<unsigned>(type)-64U<program.types.size()
        && type_info(program,type).kind==kind;
}
bool complex_resource(const Program& program,Type type) {
    return resource_kind(program,type,CompoundKind::record) && complex_number_type(type_info(program,type));
}
}
bool native_resource_signature_valid(const Program& program,Type owner,std::size_t method) {
    if (pascal_copy_type(program.types,owner)) return fixed_signature_valid(program,owner,method);
    if (resource_kind(program,owner,CompoundKind::binary_file)) return binary_signature_valid(program,owner,method);
    if (resource_kind(program,owner,CompoundKind::string_builder)) return string_builder_signature(program,owner,method);
    if (resource_kind(program,owner,CompoundKind::number_format)) return number_format_signature(program,owner,method);
    if (complex_resource(program,owner)) return complex_signature(program,owner,method);
    return method<=static_cast<std::size_t>(EncodingMethod::max_char_count)
        && encoding_signature_valid(program,owner,static_cast<EncodingMethod>(method));
}
std::vector<Type> native_resource_parameters(const Program& program,Type owner,std::size_t method) {
    if (pascal_copy_type(program.types,owner)) return fixed_parameters(program.types,program.fixed_arrays,owner,method);
    if (resource_kind(program,owner,CompoundKind::binary_file)) return binary_parameters(program.types,owner,method);
    if (resource_kind(program,owner,CompoundKind::string_builder)) {
        const auto call=static_cast<StringBuilderMethod>(method&255U);
        const auto provider=call==StringBuilderMethod::append_format_provider?known_number_format_type(program):Type::invalid;
        return string_builder_parameters(owner,call,static_cast<Type>(method>>8U),provider);
    }
    if (resource_kind(program,owner,CompoundKind::number_format))
        return number_format_parameters(owner,type_info(program,owner).key,method);
    if (complex_resource(program,owner)) return complex_parameters(owner,method);
    return encoding_parameters(program,owner,static_cast<EncodingMethod>(method));
}
Type native_resource_result(const Program& program,Type owner,std::size_t method) {
    if (pascal_copy_type(program.types,owner)) return fixed_result(program.types,owner,method);
    if (resource_kind(program,owner,CompoundKind::binary_file)) return binary_result(program.types,owner,method);
    if (resource_kind(program,owner,CompoundKind::string_builder))
        return string_builder_result(owner,static_cast<StringBuilderMethod>(method&255U));
    if (resource_kind(program,owner,CompoundKind::number_format))
        return number_format_result(owner,type_info(program,owner).key,method);
    if (complex_resource(program,owner)) return complex_result(owner,method);
    return encoding_result(program,owner,static_cast<EncodingMethod>(method));
}
Value native_resource_invoke(EncodingContext& enc,Values& values,FileContext& files,Type owner,std::size_t method,std::span<const Value> args) {
    if (pascal_copy_type(values.program.types,owner)) return fixed_invoke(values,owner,method,args);
    if (resource_kind(values.program,owner,CompoundKind::binary_file)) return binary_invoke(values,files,owner,method,args);
    if (resource_kind(values.program,owner,CompoundKind::string_builder))
        return string_builder_invoke(values,owner,static_cast<StringBuilderMethod>(method&255U),args);
    if (resource_kind(values.program,owner,CompoundKind::number_format))
        return number_format_invoke(values,owner,method,args);
    if (complex_resource(values.program,owner)) return complex_invoke(values,owner,method,args);
    return enc.invoke(values,files,owner,static_cast<EncodingMethod>(method),args);
}
}

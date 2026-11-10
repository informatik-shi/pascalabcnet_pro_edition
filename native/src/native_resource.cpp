#include "native_resource.hpp"
#include "binary_file.hpp"
#include "runtime_values.hpp"
#include "fixed_array.hpp"
namespace abc {
namespace {bool binary(const Program& p,Type t){return compound(t)&&static_cast<unsigned>(t)-64U<p.types.size()&&type_info(p,t).kind==CompoundKind::binary_file;}}
bool native_resource_signature_valid(const Program& p,Type owner,std::size_t method){if(pascal_copy_type(p.types,owner))return fixed_signature_valid(p,owner,method);return binary(p,owner)?binary_signature_valid(p,owner,method):method<=static_cast<std::size_t>(EncodingMethod::max_char_count)&&encoding_signature_valid(p,owner,static_cast<EncodingMethod>(method));}
std::vector<Type> native_resource_parameters(const Program& p,Type owner,std::size_t method){if(pascal_copy_type(p.types,owner))return fixed_parameters(p.types,p.fixed_arrays,owner,method);return binary(p,owner)?binary_parameters(p.types,owner,method):encoding_parameters(p,owner,static_cast<EncodingMethod>(method));}
Type native_resource_result(const Program& p,Type owner,std::size_t method){if(pascal_copy_type(p.types,owner))return fixed_result(p.types,owner,method);return binary(p,owner)?binary_result(p.types,owner,method):encoding_result(p,owner,static_cast<EncodingMethod>(method));}
Value native_resource_invoke(EncodingContext& enc,Values& values,FileContext& files,Type owner,std::size_t method,std::span<const Value> args){if(pascal_copy_type(values.program.types,owner))return fixed_invoke(values,owner,method,args);return binary(values.program,owner)?binary_invoke(values,files,owner,method,args):enc.invoke(values,files,owner,static_cast<EncodingMethod>(method),args);}
}

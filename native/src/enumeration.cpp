#include "enumeration.hpp"
#include "diagnostics.hpp"
#include <algorithm>
#include <bit>

namespace abc {
bool enumeration(const std::vector<TypeInfo>& types,Type type) {
    return compound(type) && static_cast<unsigned>(type)-64U<types.size()
        && types[static_cast<unsigned>(type)-64U].kind==CompoundKind::enumeration;
}
bool enumeration(const Program& program,Type type){return enumeration(program.types,type);}
std::vector<Type> enum_parameters(Type owner,EnumMethod method,Type other) {
    switch(method) {
    case EnumMethod::construct:return {Type::integer};
    case EnumMethod::low:case EnumMethod::high:return {};
    case EnumMethod::has_flag:return {owner,other==Type::invalid ? owner : other};
    case EnumMethod::has_flag_nil:return {owner};
    case EnumMethod::format:return {owner,Type::string};
    case EnumMethod::bit_and:case EnumMethod::bit_or:case EnumMethod::bit_xor:return {owner,owner};
    default:return {owner};
    }
}
Type enum_result(Type owner,EnumMethod method) {
    if(method==EnumMethod::ordinal)return Type::integer;
    if(method==EnumMethod::has_flag || method==EnumMethod::has_flag_nil)return Type::boolean;
    if(method==EnumMethod::to_string || method==EnumMethod::format)return Type::string;
    return owner;
}
EnumValue enum_advance(EnumValue value,std::int64_t step,SourceLocation location) {
    if(step>std::int64_t(INT32_MAX)-value.value || step<std::int64_t(INT32_MIN)-value.value)
        throw runtime_error(RuntimeError::integer_overflow,location);
    value.value=static_cast<std::int32_t>(value.value+step);return value;
}
std::u16string enum_text(const Program& program,EnumValue value,SourceLocation location,bool force_flags) {
    if(!enumeration(program,value.type))throw runtime_error(RuntimeError::invalid_bytecode,location);
    const auto& info=program.types[static_cast<unsigned>(value.type)-64U];
    for(const auto& member:info.enum_members)if(member.value==value.value)return from_utf8(member.text);
    if((info.enum_flags || force_flags) && value.value) {
        std::vector<const EnumMember*> members;members.reserve(info.enum_members.size());
        for(const auto& member:info.enum_members)if(member.value)members.push_back(&member);
        std::stable_sort(members.begin(),members.end(),[](const auto* a,const auto* b){return std::bit_cast<std::uint32_t>(a->value)>std::bit_cast<std::uint32_t>(b->value);});
        auto remaining=std::bit_cast<std::uint32_t>(value.value);std::vector<const EnumMember*> selected;
        for(const auto* member:members) {
            const auto bits=std::bit_cast<std::uint32_t>(member->value);
            if((remaining&bits)==bits){remaining&=~bits;selected.push_back(member);}
        }
        if(!remaining && !selected.empty()) {
            std::u16string result;
            for(auto it=selected.rbegin();it!=selected.rend();++it){if(!result.empty())result+=u", ";result+=from_utf8((*it)->text);}
            return result;
        }
    }
    return from_utf8(std::to_string(value.value));
}
Value enum_invoke(const Program& program,Type owner,EnumMethod method,std::span<const Value> arguments,SourceLocation location) {
    if(!enumeration(program,owner) || method>EnumMethod::has_flag_nil || arguments.size()!=enum_parameters(owner,method).size())
        throw runtime_error(RuntimeError::invalid_bytecode,location);
    if(method==EnumMethod::construct)return EnumValue{owner,std::get<std::int32_t>(arguments[0])};
    if(method==EnumMethod::low || method==EnumMethod::low_value)return EnumValue{owner,0};
    if(method==EnumMethod::high || method==EnumMethod::high_value)
        return EnumValue{owner,static_cast<std::int32_t>(program.types[static_cast<unsigned>(owner)-64U].enum_members.size()-1)};
    const auto value=std::get<EnumValue>(arguments[0]);
    if(method==EnumMethod::has_flag_nil)throw runtime_error(RuntimeError::nil_reference,location);
    char16_t format=u'g';
    if(method==EnumMethod::format) {
        const auto& requested=string_text(arguments[1]);
        if(requested.size()>1)throw runtime_error(RuntimeError::enum_format,location);
        if(!requested.empty())format=static_cast<char16_t>(requested[0]|0x20);
        if(format!=u'g' && format!=u'd' && format!=u'x' && format!=u'f')throw runtime_error(RuntimeError::enum_format,location);
    }
    if(method==EnumMethod::to_string || method==EnumMethod::format) {
        if(format==u'd')return from_utf8(std::to_string(value.value));
        if(format==u'x') {
            constexpr char16_t digits[]=u"0123456789ABCDEF";auto bits=std::bit_cast<std::uint32_t>(value.value);
            std::u16string result(8,u'0');for(std::size_t i=8;i>0;--i){result[i-1]=digits[bits&15U];bits>>=4U;}return result;
        }
        return enum_text(program,value,location,format==u'f');
    }

    switch(method) {
    case EnumMethod::ordinal:return value.value;
    case EnumMethod::successor:return enum_advance(value,1,location);
    case EnumMethod::predecessor:return enum_advance(value,-1,location);
    default: {
        const auto flag=std::get<EnumValue>(arguments[1]);
        if(flag.type!=owner)throw runtime_error(RuntimeError::enum_flag_type,location);
        const auto a=std::bit_cast<std::uint32_t>(value.value),b=std::bit_cast<std::uint32_t>(flag.value);
        if(method==EnumMethod::has_flag)return (a&b)==b;
        const auto result=method==EnumMethod::bit_and ? a&b : method==EnumMethod::bit_or ? a|b : a^b;
        return EnumValue{owner,std::bit_cast<std::int32_t>(result)};
    }
    }
}
}

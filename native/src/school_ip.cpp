#include "school_ip.hpp"
#include "sequences.hpp"
#include "runtime_values.hpp"
#include "diagnostics.hpp"
#include <algorithm>
#include <limits>

namespace abc {
namespace {
void check(const std::shared_ptr<Aggregate>& object,SourceLocation location) {
    if (!object || object->nil) throw runtime_error(RuntimeError::nil_reference,location);
}
std::u16string decimal(std::uint32_t number) { const auto bytes=std::to_string(number); return {bytes.begin(),bytes.end()}; }
std::u16string binary(std::uint32_t number) {
    std::u16string result(32,u'0');
    for (unsigned i=0;i<32;++i) if ((number>>i)&1U) result[31-i]=u'1';
    return result;
}
std::u16string dotted(std::uint32_t number) {
    std::u16string result;
    for (int i=3;i>=0;--i) { if (i!=3) result+=u'.'; result+=decimal((number>>(i*8))&255U); }
    return result;
}
std::uint32_t parse(const Value& value,bool decimal_only,SourceLocation location) {
    if (std::holds_alternative<NilString>(value)) throw runtime_error(RuntimeError::nil_reference,location);
    const auto& text=string_text(value); std::uint32_t number=0;
    if (text.find(u'.')==std::u16string::npos && !decimal_only) {
        if (text.size()!=32) throw runtime_error(RuntimeError::school_ip_address,location);
        for (const auto c:text) { if (c!=u'0' && c!=u'1') throw runtime_error(RuntimeError::school_ip_address,location); number=(number<<1)|(c-u'0'); }
        return number;
    }
    if (text.size()<7 || text.size()>15) throw runtime_error(RuntimeError::school_ip_address,location);
    std::size_t position=0;
    for (unsigned octet=0;octet<4;++octet) {
        unsigned part=0,digits=0;
        while (position<text.size() && text[position]>=u'0' && text[position]<=u'9') {
            if (++digits>3) throw runtime_error(RuntimeError::school_ip_address,location);
            part=part*10+(text[position++]-u'0');
        }
        if (!digits || part>255 || (octet!=3 && (position==text.size() || text[position++]!=u'.')))
            throw runtime_error(RuntimeError::school_ip_address,location);
        number=(number<<8)|part;
    }
    if (position!=text.size()) throw runtime_error(RuntimeError::school_ip_address,location);
    return number;
}
std::shared_ptr<Aggregate> empty(Values& values,Type type) {
    auto object=std::make_shared<Aggregate>(); object->type=type;
    for (const auto& field:type_info(values.program,type).fields) object->items.push_back(values.cell(field.type,values.zero(field.type)));
    return object;
}
void set(const std::shared_ptr<Aggregate>& object,std::uint32_t number,std::u16string text) {
    object->items[0]->value=std::move(text); object->items[1]->value=binary(number); object->items[2]->value=number;
}
std::shared_ptr<Aggregate> address(Values& values,Type type,std::uint32_t number) {
    auto result=empty(values,type); set(result,number,dotted(number)); return result;
}
std::shared_ptr<Aggregate> address(Values& values,Type type,const Value& text) {
    const auto number=parse(text,false,values.location); auto result=empty(values,type);
    set(result,number,string_text(text).find(u'.')!=std::u16string::npos ? string_text(text) : dotted(number)); return result;
}
std::uint32_t number(const std::shared_ptr<Aggregate>& object,SourceLocation location) { check(object,location); return std::get<std::uint32_t>(object->items[2]->value); }
std::u16string format(std::u16string_view bits,SourceLocation location) {
    if (bits.size()<32) throw runtime_error(RuntimeError::index_bounds,location);
    std::u16string result(bits.substr(0,8));
    for (unsigned i=1;i<4;++i) { result+=u'.'; result+=bits.substr(i*8,8); }
    return result;
}
std::uint32_t suffix(std::int32_t count,SourceLocation location) {
    if (count<1 || count>31) throw runtime_error(RuntimeError::school_ip_suffix,location);
    return UINT32_MAX<<(32-static_cast<unsigned>(count));
}
std::shared_ptr<Aggregate> child(const std::shared_ptr<Aggregate>& object,std::size_t index) { return std::get<std::shared_ptr<Aggregate>>(object->items[index]->value); }
}
std::u16string school_ip_text(const Program& program,const std::shared_ptr<Aggregate>& object,SourceLocation location) {
    check(object,location);
    if (type_info(program,object->type).kind==CompoundKind::school_address) {
        auto result=string_text(object->items[0]->value)+u","; if (result.size()<15) result.resize(15,u' ');
        result+=format(string_text(object->items[1]->value),location); return result;
    }
    const auto mask=std::get<std::uint8_t>(object->items[2]->value);
    const auto fine=[&](std::size_t index) {
        const auto item=child(object,index); check(item,location);
        auto result=format(string_text(item->items[1]->value),location);
        if (mask) result.insert(mask+mask/8,u"  "); return result;
    };
    const auto line=[&](std::u16string_view label,std::size_t index) {
        const auto item=child(object,index); check(item,location);
        auto value=string_text(item->items[0]->value); if (value.size()<15) value.resize(15,u' ');
        return std::u16string(label)+value+u" | "+fine(index);
    };
    const auto newline=
#ifdef _WIN32
        u"\r\n";
#else
        u"\n";
#endif
    auto result=line(u"IP адрес (Address)                  | ",0)+newline;
    result+=u"Префикс маски подсети (Bitmask)     | /"+decimal(mask)+newline;
    result+=line(u"Маска подсети (Netmask)             | ",1)+newline;
    result+=line(u"Маска хостов (Wildcard)             | ",4)+newline;
    result+=line(u"IP адрес сети (Network)             | ",5)+newline;
    result+=line(u"Широковещательный адрес (Broadcast) | ",6)+newline;
    const auto hosts=std::get<std::int32_t>(object->items[3]->value);
    result+=u"Доступно адресов для хостов (Hosts) | "+decimal(static_cast<std::uint32_t>(hosts))+newline;
    result+=line(u"Адрес первого хоста (Hostmin)       | ",7)+newline;
    result+=line(u"Адрес последнего хоста (Hostmax)    | ",8)+newline;
    return result;
}
void school_ip_sequence_start(SequenceState& state,SourceLocation location) {
    if (state.provider!=SequenceProvider::school_ip_hosts && state.provider!=SequenceProvider::school_ip_binary) throw runtime_error(RuntimeError::invalid_bytecode,location);
    const auto hosts=std::get<std::int32_t>(state.source->items[3]->value);
    if (hosts<=0) return;
    if (state.provider==SequenceProvider::school_ip_hosts) { state.first=1; state.count=static_cast<std::uint32_t>(hosts); }
    else {
        state.first=number(child(state.source,5),location);
        const auto finish=number(child(state.source,6),location);
        if (state.first<=finish) state.count=finish-state.first+1;
    }
}
Value school_ip_sequence_value(Values& values,Type element,const SequenceState& state,std::int64_t index) {
    if (state.provider==SequenceProvider::school_ip_binary) return binary(static_cast<std::uint32_t>(state.first+static_cast<std::uint64_t>(index)));
    const auto n=std::uint64_t(number(child(state.source,5),values.location))+state.first+static_cast<std::uint64_t>(index);
    if (n>UINT32_MAX) throw runtime_error(RuntimeError::integer_overflow,values.location);
    return address(values,element,static_cast<std::uint32_t>(n));
}
Value invoke_school_ip(Values& values,Type owner,SchoolIPMethod method,Type generated,std::span<const Value> arguments,const ExecutionOptions&) {
    const auto location=values.location;
    if (method==SchoolIPMethod::address_empty || method==SchoolIPMethod::ip_empty) return empty(values,owner);
    if (method==SchoolIPMethod::address_text) return address(values,owner,arguments[0]);
    if (method==SchoolIPMethod::address_suffix) return address(values,owner,suffix(std::get<std::int32_t>(arguments[0]),location));
    if (method==SchoolIPMethod::ip_text || method==SchoolIPMethod::ip_suffix) {
        const auto addr=type_info(values.program,owner).fields[0].type;
        std::shared_ptr<Aggregate> mask;
        if (method==SchoolIPMethod::ip_suffix) (void)suffix(std::get<std::uint8_t>(arguments[1]),location);
        auto ip=address(values,addr,arguments[0]);
        mask=method==SchoolIPMethod::ip_text ? address(values,addr,arguments[1]) : address(values,addr,suffix(std::get<std::uint8_t>(arguments[1]),location));
        const auto bits=number(mask,location); unsigned prefix=0;
        while (prefix<32 && (bits&(std::uint32_t{1}<<(31-prefix)))) ++prefix;
        if (prefix<1 || prefix>31 || bits!=suffix(static_cast<std::int32_t>(prefix),location)) throw runtime_error(RuntimeError::school_ip_mask,location);
        // Source converts 2^(32-prefix) to integer before subtracting two.
        if (prefix==1) throw runtime_error(RuntimeError::integer_overflow,location);
        const auto hosts=(std::uint32_t{1}<<(32-prefix))-2;
        const auto network=number(ip,location)&bits,wildcard=~bits;
        auto result=empty(values,owner);
        result->items[0]->value=ip; result->items[1]->value=mask; result->items[2]->value=static_cast<std::uint8_t>(prefix);
        result->items[3]->value=static_cast<std::int32_t>(hosts);
        result->items[4]->value=address(values,addr,wildcard); result->items[5]->value=address(values,addr,network);
        result->items[6]->value=address(values,addr,network|wildcard); result->items[7]->value=address(values,addr,network+1);
        result->items[8]->value=address(values,addr,network+hosts); return result;
    }
    const auto receiver=std::get<std::shared_ptr<Aggregate>>(arguments[0]); check(receiver,location);
    if (method>=SchoolIPMethod::ip_address && method<=SchoolIPMethod::ip_hostmax)
        return receiver->items[static_cast<std::size_t>(method)-static_cast<std::size_t>(SchoolIPMethod::ip_address)]->value;
    if (method==SchoolIPMethod::address_value) return receiver->items[0]->value;
    if (method==SchoolIPMethod::address_binary) return receiver->items[1]->value;
    if (method==SchoolIPMethod::address_number) return receiver->items[2]->value;
    if (method==SchoolIPMethod::address_set) { const auto n=parse(arguments[1],true,location); set(receiver,n,string_text(arguments[1])); return std::int32_t{0}; }
    if (method==SchoolIPMethod::address_string || method==SchoolIPMethod::ip_string) return school_ip_text(values.program,receiver,location);
    if (method==SchoolIPMethod::address_format) {
        if (std::holds_alternative<NilString>(arguments[1])) throw runtime_error(RuntimeError::nil_reference,location);
        return format(string_text(arguments[1]),location);
    }
    if (school_ip_generator(method)) {
        return sequence_create(values,generated,receiver,method==SchoolIPMethod::ip_generate ? SequenceProvider::school_ip_hosts : SequenceProvider::school_ip_binary);
    }
    const auto a=number(receiver,location);
    if (method==SchoolIPMethod::address_not) return address(values,owner,~a);
    if (method==SchoolIPMethod::address_add) {
        const auto b=std::get<std::uint32_t>(arguments[1]);
        if (b>UINT32_MAX-a) throw runtime_error(RuntimeError::integer_overflow,location);
        return address(values,owner,a+b);
    }
    const auto b=number(std::get<std::shared_ptr<Aggregate>>(arguments[1]),location);
    return address(values,owner,method==SchoolIPMethod::address_and ? a&b : a|b);
}
} // namespace abc

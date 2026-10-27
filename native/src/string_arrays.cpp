#include "string_arrays.hpp"
#include "runtime_values.hpp"
#include "stdlib.hpp"
#include <array>
#include <bitset>

namespace abc {
std::shared_ptr<Aggregate> make_string_array(Values& values, Type type, StringArray method,
    const Value& source, const Value& parameter, const ExecutionOptions& options) {
    const auto location=values.location;
    const auto check=[&](std::size_t index) {
        if ((index&255U)==0 && options.cancelled && options.cancelled()) throw runtime_error(RuntimeError::cancelled,location);
    };
    if (method==StringArray::integers_count) {
        const auto count=std::get<std::int32_t>(parameter);
        if (count<0) throw runtime_error(RuntimeError::array_length,location);
        auto result=values.array(type,static_cast<std::size_t>(count));
        std::int32_t position=1;
        for (std::size_t i=0;i<result->items.size();++i) {
            check(i);
            const auto [number,next]=stdlib::read_string_integer(source,position,location,options);
            values.write(result->items[i],number); position=next;
        }
        return result;
    }
    if (method==StringArray::reals_separator && string_text(parameter).empty()) throw runtime_error(RuntimeError::decimal_separator,location);
    if (method==StringArray::words && std::holds_alternative<NilString>(parameter)) throw runtime_error(RuntimeError::nil_reference,location);
    if (std::holds_alternative<NilString>(source)) throw runtime_error(RuntimeError::nil_reference,location);
    const auto& text=string_text(source);
    auto result=values.array(type,0);
    const auto element=string_array_element(method);
    const auto add=[&](Value value) { result->items.push_back(values.cell(element,std::move(value))); };
    if (method==StringArray::integers) {
        std::int32_t position=1;
        while (static_cast<std::size_t>(position)<=text.size()) {
            const auto [number,next]=stdlib::read_string_integer(source,position,location,options);
            add(number);
            position=stdlib::read_string_word(source,next,location,options,true).second;
        }
        return result;
    }
    std::bitset<65536> separators;
    bool whitespace=false;
    if (method==StringArray::words) {
        const auto& delimiters=string_text(parameter); whitespace=delimiters.empty();
        for (std::size_t i=0;i<delimiters.size();++i) { check(i); separators.set(delimiters[i]); }
    } else if (method==StringArray::words_array) {
        const auto& delimiters=std::get<std::shared_ptr<Aggregate>>(parameter);
        whitespace=delimiters->nil || delimiters->items.empty();
        if (!delimiters->nil) for (std::size_t i=0;i<delimiters->items.size();++i) {
            check(i); separators.set(std::get<char16_t>(delimiters->items[i]->value));
        }
    } else if (method==StringArray::reals || method==StringArray::reals_separator || method==StringArray::integers64) {
        for (auto c:std::u16string_view{u" \t\r\n"}) separators.set(c);
    }
    const auto part=[&](std::size_t start,std::size_t end) {
        if (method!=StringArray::lines && start==end) return;
        auto word=text.substr(start,end-start);
        if (element==Type::real) {
            const std::array<Value,2> arguments{std::move(word),parameter};
            add(*stdlib::invoke_text(stdlib::Function::str_to_real,
                std::span<const Value>(arguments.data(),method==StringArray::reals_separator ? 2U : 1U),location));
        } else if (element==Type::int64) {
            const std::array<Value,1> arguments{std::move(word)};
            add(*stdlib::invoke_text(stdlib::Function::str_to_int64,arguments,location));
        } else add(std::move(word));
    };
    std::size_t start=0,index=0;
    while (index<text.size()) {
        check(index);
        const auto c=text[index];
        const bool split=method==StringArray::lines ? c==u'\r' || c==u'\n'
            : whitespace ? stdlib::unicode_whitespace(c) : separators.test(c);
        if (!split) { ++index; continue; }
        part(start,index);
        index+=method==StringArray::lines && c==u'\r' && index+1<text.size() && text[index+1]==u'\n' ? 2 : 1;
        start=index;
    }
    part(start,text.size());
    return result;
}
} // namespace abc

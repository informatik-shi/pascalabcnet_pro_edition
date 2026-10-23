#include "abc/core.hpp"
#include "number.hpp"
#include "diagnostics.hpp"
#include "stdlib.hpp"
#include "runtime_values.hpp"
#include "files.hpp"
#include "linked.hpp"
#include "text_file.hpp"

#include <charconv>
#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>

namespace abc {
namespace {

[[noreturn]] void trap(const Instruction& instruction, RuntimeError id, std::string_view context = {}) {
    throw runtime_error(id, instruction.location, context);
}

template<class T> T integer_operation(T a, T b, const Instruction& instruction) {
    constexpr auto min = std::numeric_limits<T>::min();
    constexpr auto max = std::numeric_limits<T>::max();
    const auto overflow = [&]() -> T { trap(instruction, RuntimeError::integer_overflow); };
    switch (instruction.op) {
    case Op::add:
        if ((b > 0 && a > max - b) || (b < 0 && a < min - b)) return overflow();
        return static_cast<T>(a + b);
    case Op::subtract:
        if ((b > 0 && a < min + b) || (b < 0 && a > max + b)) return overflow();
        return static_cast<T>(a - b);
    case Op::multiply: {
        using U = std::make_unsigned_t<T>;
        const auto magnitude = [](T value) -> U {
            return value < 0 ? static_cast<U>(-(value + 1)) + U{1} : static_cast<U>(value);
        };
        const bool negative = (a < 0) != (b < 0);
        const U limit = negative ? magnitude(min) : static_cast<U>(max);
        const U left = magnitude(a), right = magnitude(b);
        if (right != 0 && left > limit / right) return overflow();
        const U product = left * right;
        if (negative && product == magnitude(min)) return min;
        return negative ? static_cast<T>(-static_cast<T>(product)) : static_cast<T>(product);
    }
    case Op::divide_int: case Op::modulo:
        if (b == 0) trap(instruction, RuntimeError::division_zero);
        if (a == min && b == -1) return instruction.op == Op::modulo ? T{0} : overflow();
        return instruction.op == Op::modulo ? static_cast<T>(a % b) : static_cast<T>(a / b);
    default: trap(instruction, RuntimeError::invalid_bytecode);
    }
}

std::string format(const Value& value, const Instruction& instruction) {
    if (std::holds_alternative<NilString>(value)) return "nil";
    if (const auto* text = std::get_if<std::u16string>(&value)) return to_utf8(*text, true);
    if (const auto* boolean = std::get_if<bool>(&value)) return *boolean ? "True" : "False";
    char buffer[128];
    std::to_chars_result result;
    if (const auto* integer = std::get_if<std::int32_t>(&value)) result = std::to_chars(buffer, buffer + sizeof buffer, *integer);
    else if (const auto* wide = std::get_if<std::int64_t>(&value)) result = std::to_chars(buffer, buffer + sizeof buffer, *wide);
    else result = std::to_chars(buffer, buffer + sizeof buffer, std::get<double>(value));
    if (result.ec != std::errc{}) trap(instruction, RuntimeError::number_format);
    return {buffer, result.ptr};
}
std::u16string file_text(const Value& value, const Instruction& instruction) {
    if (std::holds_alternative<NilString>(value)) return u"nil";
    return value_type(value)==Type::string ? string_text(value) : from_utf8(format(value,instruction));
}
constexpr std::u16string_view file_newline=
#ifdef _WIN32
    u"\r\n";
#else
    u"\n";
#endif

void append_collection_text(std::u16string& text, const Value& value, const Instruction& instruction, bool concatenate = false) {
    if (!concatenate && std::holds_alternative<NilString>(value)) trap(instruction,RuntimeError::nil_reference);
    auto part = value_type(value) == Type::string ? string_text(value) : from_utf8(format(value,instruction));
    if (part.size() >= 1'048'576 || text.size() > 1'048'576 - part.size() - 1) trap(instruction,RuntimeError::string_limit);
    text += part; text += u' ';
}

Value read_line(std::istream& input, const Instruction& instruction) {
    constexpr std::size_t maximum_line = 1'048'576;
    std::string line;
    bool received = false;
    char c;
    while (input.get(c)) {
        received = true;
        if (c == '\n') break;
        if (line.size() == maximum_line) trap(instruction, RuntimeError::input_limit);
        line += c;
    }
    if (input.bad()) trap(instruction, RuntimeError::input_stream);
    if (!received) trap(instruction, RuntimeError::input_end);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (instruction.type == Type::string) {
        try { return from_utf8(line); }
        catch (const std::invalid_argument&) { trap(instruction, RuntimeError::input_encoding); }
    }
    const auto first = line.find_first_not_of(" \t");
    const auto last = line.find_last_not_of(" \t");
    if (first == std::string::npos) trap(instruction, RuntimeError::input_empty);
    auto text = std::string_view(line).substr(first, last - first + 1);
    if (instruction.type == Type::boolean) {
        std::string normalized(text);
        for (auto& ch : normalized) if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch + 32);
        if (normalized == "true") return true;
        if (normalized == "false") return false;
        trap(instruction, RuntimeError::input_boolean);
    }
    if (text.starts_with('+')) text.remove_prefix(1);
    const auto parse = [&]<class T>() -> Value {
        if constexpr (std::is_floating_point_v<T>) {
            double value = 0;
            if (!parse_real(text, value)) trap(instruction, RuntimeError::input_real);
            return value;
        } else {
            T value{};
            const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
            if (result.ec != std::errc{} || result.ptr != text.data() + text.size())
                trap(instruction, RuntimeError::input_range, type_name(instruction.type));
            return value;
        }
    };
    switch (instruction.type) {
    case Type::integer: return parse.operator()<std::int32_t>();
    case Type::int64: return parse.operator()<std::int64_t>();
    case Type::real: return parse.operator()<double>();
    default: trap(instruction, RuntimeError::invalid_bytecode);
    }
}

} // namespace

void execute(const Program& program, std::istream& input, std::ostream& output, const ExecutionOptions& options) {
    try { verify(program); }
    catch (const Error& error) {
        throw Error(error.code, error.location, error.what(), "Некорректный байт-код: выполнение невозможно");
    }
    Values values{program,options.allocation_limit};
    LinkedRuntime linked{values,{}};
    FileContext files;
    std::vector<std::shared_ptr<Cell>> globals(program.locals.size());
    for (std::size_t i = 0; i < program.locals.size(); ++i) if (program.locals[i].global)
        globals[i] = values.cell(program.locals[i].type,values.zero(program.locals[i].type));
    struct Frame {
        const std::vector<Instruction>* code;
        const Routine* routine;
        std::vector<std::shared_ptr<Cell>> locals;
        std::size_t pc = 0, base = 0;
    };
    std::vector<Frame> frames{{&program.code,nullptr,globals,0,0}};
    std::vector<Value> stack;
    std::uint64_t steps = 0;
    std::size_t output_bytes = 0;
    auto write = [&](std::string_view text, const Instruction& instruction) {
        if (text.size() > options.output_limit - output_bytes)
            trap(instruction, RuntimeError::output_limit);
        output.write(text.data(), static_cast<std::streamsize>(text.size()));
        output.flush();
        if (!output) trap(instruction, RuntimeError::output_stream);
        output_bytes += text.size();
    };
    auto pop = [&]() { auto value = std::move(stack.back()); stack.pop_back(); return value; };
    while (!frames.empty()) {
        auto& frame = frames.back(); auto& locals = frame.locals;
        const auto& instruction = (*frame.code)[frame.pc++]; values.location = instruction.location;
        if (steps++ >= options.instruction_limit) trap(instruction, RuntimeError::instruction_limit);
        if (options.cancelled && options.cancelled()) trap(instruction, RuntimeError::cancelled);
        switch (instruction.op) {
        case Op::linked_call: {
            const auto method = static_cast<LinkedMethod>(instruction.operand);
            const auto& info = type_info(program,instruction.type);
            const auto parameters = linked_parameters(instruction.type,info,method);
            std::vector<Value> arguments(parameters.size());
            for (std::size_t i=arguments.size();i>0;--i) arguments[i-1] = pop();
            if (method == LinkedMethod::construct) {
                if (info.kind == CompoundKind::single_node) stack.emplace_back(linked.node(instruction.type,arguments[0],{},std::get<std::shared_ptr<Aggregate>>(arguments[1])));
                else stack.emplace_back(linked.node(instruction.type,arguments[0],std::get<std::shared_ptr<Aggregate>>(arguments[1]),std::get<std::shared_ptr<Aggregate>>(arguments[2])));
                break;
            }
            const auto target = linked.require(std::get<std::shared_ptr<Aggregate>>(pop()));
            auto reference = [&](std::shared_ptr<Aggregate> node, Type type) {
                if (node) stack.emplace_back(std::move(node)); else stack.push_back(values.zero(type));
            };
            switch (method) {
            case LinkedMethod::first: reference(target->links->prev.lock(),info.key); break;
            case LinkedMethod::last: reference(target->links->next.lock(),info.key); break;
            case LinkedMethod::prev: reference(target->links->prev.lock(),instruction.type); break;
            case LinkedMethod::next: reference(target->links->next.lock(),instruction.type); break;
            case LinkedMethod::set_next: {
                auto next = std::get<std::shared_ptr<Aggregate>>(arguments[0]);
                if (next->nil) target->links->next.reset(); else target->links->next=next;
                break;
            }
            case LinkedMethod::value: stack.push_back(values.copy(target->items[0]->value)); break;
            case LinkedMethod::set_value: values.write(target->items[0],arguments[0]); break;
            case LinkedMethod::add_first: case LinkedMethod::add_last: linked.add(target,arguments[0],method==LinkedMethod::add_first); break;
            case LinkedMethod::remove_first: case LinkedMethod::remove_last: linked.remove_end(target,method==LinkedMethod::remove_first); break;
            case LinkedMethod::add_before: case LinkedMethod::add_after:
                linked.insert(target,std::get<std::shared_ptr<Aggregate>>(arguments[0]),arguments[1],method==LinkedMethod::add_before); break;
            case LinkedMethod::remove: linked.remove(target,std::get<std::shared_ptr<Aggregate>>(arguments[0])); break;
            case LinkedMethod::to_string: case LinkedMethod::print: case LinkedMethod::println: {
                std::u16string text;
                for (auto node=target->links->prev.lock();node;node=node->links->next.lock()) {
                    if (steps++ >= options.instruction_limit) trap(instruction,RuntimeError::instruction_limit);
                    if (options.cancelled && options.cancelled()) trap(instruction,RuntimeError::cancelled);
                    // Original Collections.LinkedList.ToString replaces the result on each node.
                    text.clear(); append_collection_text(text,node->items[0]->value,instruction);
                }
                if (method==LinkedMethod::to_string) stack.emplace_back(std::move(text));
                else { write(to_utf8(text,true),instruction); if (method==LinkedMethod::println) write("\n",instruction); }
                break;
            }
            case LinkedMethod::construct: break;
            }
            break;
        }
        case Op::association_call: {
            const auto method = static_cast<AssociationMethod>(instruction.operand);
            const auto& info = type_info(program,instruction.type);
            const auto parameters = association_parameters(info,method);
            std::vector<Value> arguments(parameters.size());
            for (std::size_t i=arguments.size();i>0;--i) arguments[i-1] = pop();
            const auto target = std::get<std::shared_ptr<Aggregate>>(pop());
            if (target->nil) trap(instruction,RuntimeError::nil_reference);
            if (method == AssociationMethod::get || method == AssociationMethod::set) {
                auto found = target->items.size();
                for (std::size_t i=0;i<target->items.size();i+=2) if (values.equal(target->items[i]->value,arguments[0])) { found = i; break; }
                if (method == AssociationMethod::get) {
                    stack.push_back(found == target->items.size() ? values.zero(info.element,false,true) : values.copy(target->items[found+1]->value));
                } else if (found != target->items.size()) values.write(target->items[found+1],arguments[1]);
                else {
                    if (values.remaining < 2 || target->items.size() > target->items.max_size()-2) trap(instruction,RuntimeError::allocation_limit);
                    auto key = values.cell(info.key,values.copy(arguments[0]));
                    auto value = values.cell(info.element,values.copy(arguments[1]));
                    const auto next = target->items.size()+2;
                    if (next > target->items.capacity()) {
                        const auto capacity = target->items.capacity();
                        target->items.reserve(capacity <= target->items.max_size()/2 ? std::max(next,capacity*2) : next);
                    }
                    target->items.push_back(std::move(key)); target->items.push_back(std::move(value));
                }
            } else {
                std::u16string text;
                for (std::size_t i=0;i<target->items.size();i+=2) {
                    append_collection_text(text,target->items[i]->value,instruction);
                    append_collection_text(text,target->items[i+1]->value,instruction);
                    // Original AssocArray uses CRLF, including on non-Windows hosts.
                    text.back() = u'\r';
                    if (text.size() == 1'048'576) trap(instruction,RuntimeError::string_limit);
                    text += u'\n';
                }
                if (method == AssociationMethod::to_string) stack.emplace_back(std::move(text));
                else write(to_utf8(text,true),instruction); // Println also adds no extra newline.
            }
            break;
        }
        case Op::buffer_call: {
            const auto method = static_cast<BufferMethod>(instruction.operand);
            const auto& info = type_info(program,instruction.type);
            const auto parameters = buffer_parameters(info.kind,info.element,method);
            std::vector<Value> arguments(parameters.size());
            for (std::size_t i=arguments.size();i>0;--i) arguments[i-1] = pop();
            if (method == BufferMethod::construct) {
                stack.emplace_back(values.buffer(instruction.type,std::get<std::int32_t>(arguments[0]))); break;
            }
            const auto target = std::get<std::shared_ptr<Aggregate>>(pop());
            if (target->nil) trap(instruction,RuntimeError::nil_reference);
            auto integer = [&](std::size_t i) { return std::get<std::int32_t>(arguments[i]); };
            auto find = [&]() {
                for (std::size_t i=0;i<target->count;++i) if (values.equal(target->items[i]->value,arguments[0])) return static_cast<std::int32_t>(i);
                return std::int32_t{-1};
            };
            auto index = [&]() {
                const auto i = integer(0);
                if (i < 0 || static_cast<std::size_t>(i) >= target->count) trap(instruction,RuntimeError::index_bounds);
                return static_cast<std::size_t>(i);
            };
            auto grow = [&]() {
                if (target->count >= INT32_MAX) trap(instruction,RuntimeError::integer_overflow);
                values.resize_buffer(target,static_cast<std::int32_t>(target->count)+1);
            };
            switch (method) {
            case BufferMethod::add:
                if (info.kind == CompoundKind::dynarray || find() == -1) { grow(); values.write(target->items[target->count-1],arguments[0]); }
                break;
            case BufferMethod::insert: {
                const auto pos = index(); grow();
                for (std::size_t i=target->count-1;i>pos;--i) values.write(target->items[i],target->items[i-1]->value);
                values.write(target->items[pos],arguments[1]); break;
            }
            case BufferMethod::remove: {
                const auto pos = info.kind == CompoundKind::simple_set ? find() : static_cast<std::int32_t>(index());
                if (pos >= 0) {
                    for (std::size_t i=static_cast<std::size_t>(pos);i+1<target->count;++i) values.write(target->items[i],target->items[i+1]->value);
                    --target->count;
                }
                break;
            }
            case BufferMethod::find: stack.emplace_back(find()); break;
            case BufferMethod::contains: stack.emplace_back(find() >= 0); break;
            case BufferMethod::reserve: values.reserve_buffer(target,integer(0)); break;
            case BufferMethod::resize: values.resize_buffer(target,integer(0)); break;
            case BufferMethod::count: stack.emplace_back(static_cast<std::int32_t>(target->count)); break;
            case BufferMethod::capacity: stack.emplace_back(static_cast<std::int32_t>(target->items.size())); break;
            case BufferMethod::get: stack.push_back(values.copy(target->items[index()]->value)); break;
            case BufferMethod::set: values.write(target->items[index()],arguments[1]); break;
            case BufferMethod::to_string: case BufferMethod::print: case BufferMethod::println: {
                std::u16string text;
                for (std::size_t i=0;i<target->count;++i) append_collection_text(text,target->items[i]->value,instruction);
                if (method == BufferMethod::to_string) stack.emplace_back(std::move(text));
                else { write(to_utf8(text,true),instruction); if (method == BufferMethod::println) write("\n",instruction); }
                break;
            }
            case BufferMethod::construct: break;
            }
            break;
        }
        case Op::endpoint_call: {
            const auto method = static_cast<EndpointMethod>(instruction.operand);
            std::optional<Value> argument;
            if (method == EndpointMethod::add) argument = pop();
            auto collection = std::get<std::shared_ptr<Aggregate>>(pop());
            if (collection->nil) trap(instruction,RuntimeError::nil_reference);
            auto& items = *collection->ends;
            const bool lifo = type_info(program,instruction.type).kind == CompoundKind::stack;
            if (method == EndpointMethod::add) {
                items.push_back(values.cell(type_info(program,instruction.type).element,values.copy(*argument)));
            } else if (method == EndpointMethod::is_empty) stack.emplace_back(items.empty());
            else if (method == EndpointMethod::top || method == EndpointMethod::remove) {
                if (items.empty()) trap(instruction,lifo ? RuntimeError::stack_empty : RuntimeError::queue_empty);
                auto result = values.copy((lifo ? items.back() : items.front())->value);
                if (method == EndpointMethod::remove) {
                    if (lifo) items.pop_back(); else items.pop_front();
                }
                stack.push_back(std::move(result));
            } else {
                std::u16string text;
                auto append = [&](const std::shared_ptr<Cell>& item) {
                    append_collection_text(text,item->value,instruction,!lifo);
                };
                if (lifo) for (auto item = items.rbegin(); item != items.rend(); ++item) append(*item);
                else for (const auto& item : items) append(item);
                if (method == EndpointMethod::to_string) stack.emplace_back(std::move(text));
                else {
                    write(to_utf8(text,true),instruction);
                    // Collections.Stack.Print also writes a newline in the original unit.
                    if (lifo || method == EndpointMethod::println) write("\n",instruction);
                }
            }
            break;
        }
        case Op::read_value: stack.push_back(read_line(input,instruction)); break;
        case Op::file_print: {
            const auto value=pop(); auto& file=text_file(stack.back(),instruction.location);
            text_write(file,file_text(value,instruction),instruction.location); break;
        }
        case Op::file_output: {
            std::vector<Value> arguments(instruction.operand/4);
            for (auto value=arguments.rbegin();value!=arguments.rend();++value) *value=pop();
            const auto receiver=pop();
            const auto method=static_cast<TextOutput>(instruction.operand%4);
            auto& file=text_file(receiver,instruction.location);
            // Empty Write and Print do not require an assigned or open file.
            if (arguments.empty() && (method==TextOutput::write || method==TextOutput::print)) break;
            for (std::size_t n=0;n<arguments.size();++n) {
                text_write(file,file_text(arguments[n],instruction),instruction.location);
                if (method==TextOutput::print || (method==TextOutput::println && n+1<arguments.size()))
                    text_write(file,u" ",instruction.location);
            }
            if (method==TextOutput::writeln || method==TextOutput::println) text_write(file,file_newline,instruction.location);
            break;
        }
        case Op::file_read: {
            const auto target=std::get<std::shared_ptr<Cell>>(pop());
            values.write(target,text_read(text_file(stack.back(),instruction.location),instruction.type,instruction.location)); break;
        }
        case Op::file_newline:
            text_write(text_file(stack.back(),instruction.location),file_newline,instruction.location); break;
        case Op::file_skip_line: text_skip_line(text_file(stack.back(),instruction.location),instruction.location); break;
        case Op::increment: {
            const auto step=pop(); auto target=std::get<std::shared_ptr<Cell>>(pop()); auto operation=instruction;
            operation.op=instruction.operand ? Op::subtract : Op::add;
            if (instruction.type==Type::integer) target->value=integer_operation(std::get<std::int32_t>(target->value),std::get<std::int32_t>(step),operation);
            else target->value=integer_operation(std::get<std::int64_t>(target->value),std::get<std::int64_t>(step),operation);
            break;
        }
        case Op::mutate_string: {
            const auto& signature=stdlib::signatures()[instruction.operand];
            const std::size_t ref=signature.function==stdlib::Function::insert_string ? 1 : 0;
            std::vector<Value> arguments(signature.parameters.size());
            for (std::size_t i=arguments.size();i>0;--i) arguments[i-1]=pop();
            auto target=std::get<std::shared_ptr<Cell>>(arguments[ref]); arguments[ref]=target->value;
            const auto result=stdlib::invoke(instruction.operand,arguments,instruction.location,files); values.write(target,result); break;
        }
        case Op::read_lines: {
            const auto path=pop(); const auto text=read_text_file(files,string_text(path),instruction.location);
            auto result=values.array(instruction.type,0); std::size_t start=0;
            for (std::size_t i=0;i<text.size();++i) if (text[i]==u'\r' || text[i]==u'\n') {
                result->items.push_back(values.cell(Type::string,std::u16string(text.substr(start,i-start))));
                if (text[i]==u'\r' && i+1<text.size() && text[i+1]==u'\n') ++i;
                start=i+1;
            }
            if (start<text.size()) result->items.push_back(values.cell(Type::string,std::u16string(text.substr(start))));
            stack.emplace_back(result); break;
        }
        case Op::write_lines: {
            const auto lines=std::get<std::shared_ptr<Aggregate>>(pop()); const auto path=pop();
            if (lines->nil) trap(instruction,RuntimeError::nil_reference);
            std::u16string text;
            for (const auto& line:lines->items) {
                const auto& part=string_text(line->value);
#ifdef _WIN32
                const auto newline=u"\r\n";
#else
                const auto newline=u"\n";
#endif
                if (part.size()>1'048'576 || text.size()>1'048'576-part.size()) trap(instruction,RuntimeError::file_limit);
                text+=part; text+=newline;
                if (text.size()>1'048'576) trap(instruction,RuntimeError::file_limit);
            }
            write_text_file(files,string_text(path),text,instruction.location); break;
        }
        case Op::array_copy: {
            const auto original=std::get<std::shared_ptr<Aggregate>>(pop());
            if (original->nil) { stack.emplace_back(original); break; }
            auto result=values.array(instruction.type,original->items.size());
            for (std::size_t i=0;i<result->items.size();++i) values.write(result->items[i],original->items[i]->value);
            stack.emplace_back(result); break;
        }
        case Op::array_high: {
            auto array=std::get<std::shared_ptr<Aggregate>>(pop()); stack.emplace_back(static_cast<std::int32_t>(array->items.size())-1); break;
        }
        case Op::sequence_sort: {
            auto sequence=std::get<std::shared_ptr<Aggregate>>(pop());
            if (sequence->nil) trap(instruction,RuntimeError::nil_reference);
            std::vector<Value> items;
            for (const auto& item:sequence->items) items.push_back(item->value);
            std::sort(items.begin(),items.end(),[&](const Value& a,const Value& b){
                if (value_type(a)==Type::string) return instruction.operand ? compare_strings(a,b)>0 : compare_strings(a,b)<0;
                return instruction.operand ? a>b : a<b;
            });
            for (std::size_t i=0;i<items.size();++i) values.write(sequence->items[i],items[i]); break;
        }
        case Op::sequence_fill: {
            const auto value=pop(); auto sequence=std::get<std::shared_ptr<Aggregate>>(pop());
            if (sequence->nil) trap(instruction,RuntimeError::nil_reference);
            for (const auto& item:sequence->items) values.write(item,value); break;
        }
        case Op::make_default: stack.push_back(values.zero(instruction.type,instruction.operand!=0)); break;
        case Op::load_global: stack.push_back(values.copy(globals[instruction.operand]->value)); break;
        case Op::store_global: values.write(globals[instruction.operand],pop()); break;
        case Op::ref_local: stack.emplace_back(locals[instruction.operand]); break;
        case Op::ref_global: stack.emplace_back(globals[instruction.operand]); break;
        case Op::store_ref: {
            auto value = pop(); auto target = std::get<std::shared_ptr<Cell>>(pop()); values.write(target,value); break;
        }
        case Op::drop: (void)pop(); break;
        case Op::get_index: case Op::ref_index: {
            const auto index = std::get<std::int32_t>(pop()); auto array = std::get<std::shared_ptr<Aggregate>>(pop());
            if (array->nil) trap(instruction,RuntimeError::nil_reference);
            if (index < 0 || static_cast<std::size_t>(index) >= array->items.size()) trap(instruction,RuntimeError::index_bounds);
            auto cell = array->items[static_cast<std::size_t>(index)];
            if (instruction.op == Op::ref_index) stack.emplace_back(cell); else stack.push_back(values.copy(cell->value));
            break;
        }
        case Op::get_field: {
            auto record = std::get<std::shared_ptr<Aggregate>>(pop()); stack.push_back(values.copy(record->items[instruction.operand]->value)); break;
        }
        case Op::ref_field: {
            auto record = std::get<std::shared_ptr<Aggregate>>(std::get<std::shared_ptr<Cell>>(pop())->value);
            stack.emplace_back(record->items[instruction.operand]); break;
        }
        case Op::length: {
            auto array = std::get<std::shared_ptr<Aggregate>>(pop());
            if (array->nil && type_info(program,instruction.type).kind==CompoundKind::list) trap(instruction,RuntimeError::nil_reference);
            stack.emplace_back(static_cast<std::int32_t>(array->items.size())); break;
        }
        case Op::array_new: case Op::array_literal: {
            const auto count = instruction.op == Op::array_literal ? static_cast<std::int32_t>(instruction.operand) : std::get<std::int32_t>(pop());
            if (count < 0) trap(instruction,RuntimeError::array_length);
            auto array = values.array(instruction.type,static_cast<std::size_t>(count));
            if (instruction.op == Op::array_literal)
                for (auto n = array->items.size(); n > 0; --n) values.write(array->items[n-1],pop());
            stack.emplace_back(array); break;
        }
        case Op::resize_array: {
            const auto count = std::get<std::int32_t>(pop()); auto target = std::get<std::shared_ptr<Cell>>(pop());
            if (count < 0) trap(instruction,RuntimeError::array_length);
            const auto original = std::get<std::shared_ptr<Aggregate>>(target->value);
            if (static_cast<std::size_t>(count) == original->items.size() && !original->nil) break;
            auto result = values.array(instruction.type,static_cast<std::size_t>(count));
            for (std::size_t i = 0; i < std::min(result->items.size(),original->items.size()); ++i) values.write(result->items[i],original->items[i]->value);
            target->value = result; break;
        }
        case Op::list_add: {
            auto value = pop(); auto list = std::get<std::shared_ptr<Aggregate>>(pop());
            if (list->nil) trap(instruction,RuntimeError::nil_reference);
            list->items.push_back(values.cell(type_info(program,instruction.type).element,values.copy(value))); break;
        }
        case Op::list_clear: {
            auto list=std::get<std::shared_ptr<Aggregate>>(pop()); if (list->nil) trap(instruction,RuntimeError::nil_reference);
            list->items.clear(); break;
        }
        case Op::list_remove: {
            const auto index = std::get<std::int32_t>(pop()); auto list = std::get<std::shared_ptr<Aggregate>>(pop());
            if (list->nil) trap(instruction,RuntimeError::nil_reference);
            if (index < 0 || static_cast<std::size_t>(index) >= list->items.size()) trap(instruction,RuntimeError::index_bounds);
            list->items.erase(list->items.begin()+index); break;
        }
        case Op::list_contains: {
            auto value = pop(); auto list = std::get<std::shared_ptr<Aggregate>>(pop());
            if (list->nil) trap(instruction,RuntimeError::nil_reference);
            bool found = false;
            for (const auto& item : list->items) if (values.equal(item->value,value)) { found = true; break; }
            stack.emplace_back(found); break;
        }
        case Op::call_user: {
            if (frames.size() > options.call_limit) trap(instruction,RuntimeError::call_limit);
            const auto& routine = program.routines[instruction.operand];
            Frame next{&routine.code,&routine,std::vector<std::shared_ptr<Cell>>(program.locals.size()),0,stack.size()-routine.parameters.size()};
            for (auto index : routine.locals) next.locals[index] = values.cell(program.locals[index].type,values.zero(program.locals[index].type));
            for (std::size_t i = routine.parameters.size(); i > 0; --i) {
                const auto& parameter = routine.parameters[i-1]; auto value = pop();
                if (parameter.by_reference) next.locals[parameter.local] = std::get<std::shared_ptr<Cell>>(value);
                else values.write(next.locals[parameter.local],value);
            }
            frames.push_back(std::move(next)); break;
        }
        case Op::return_user: {
            std::optional<Value> result;
            if (frame.routine->result != Type::unit) result = values.copy(frame.locals[frame.routine->result_local]->value);
            stack.resize(frame.base); frames.pop_back();
            if (result) stack.push_back(std::move(*result)); break;
        }
        case Op::call_builtin: {
            const auto count = stdlib::signatures()[instruction.operand].parameters.size();
            const auto start = stack.size() - count;
            auto value = stdlib::invoke(instruction.operand, std::span<const Value>(stack).subspan(start), instruction.location,files);
            stack.resize(start);
            if (instruction.type != Type::unit) stack.push_back(std::move(value));
            break;
        }
        case Op::push: stack.push_back(values.copy(program.constants[instruction.operand])); break;
        case Op::load: stack.push_back(values.copy(locals[instruction.operand]->value)); break;
        case Op::store: values.write(locals[instruction.operand],pop()); break;
        case Op::convert: {
            const auto value = pop();
            if (value_type(value) == instruction.type) stack.push_back(value);
            else if (instruction.type == Type::int64) stack.emplace_back(static_cast<std::int64_t>(std::get<std::int32_t>(value)));
            else if (instruction.type == Type::real) {
                if (value_type(value) == Type::integer) stack.emplace_back(static_cast<double>(std::get<std::int32_t>(value)));
                else if (value_type(value) == Type::int64) stack.emplace_back(static_cast<double>(std::get<std::int64_t>(value)));
                else stack.push_back(value);
            } else stack.push_back(value);
            break;
        }
        case Op::negate: {
            auto value = pop();
            if (instruction.type == Type::integer) {
                const auto n = std::get<std::int32_t>(value);
                if (n == std::numeric_limits<std::int32_t>::min()) trap(instruction, RuntimeError::integer_overflow);
                stack.emplace_back(-n);
            } else if (instruction.type == Type::int64) {
                const auto n = std::get<std::int64_t>(value);
                if (n == std::numeric_limits<std::int64_t>::min()) trap(instruction, RuntimeError::integer_overflow);
                stack.emplace_back(-n);
            } else stack.emplace_back(-std::get<double>(value));
            break;
        }
        case Op::logical_not: stack.emplace_back(!std::get<bool>(pop())); break;
        case Op::add: case Op::subtract: case Op::multiply:
        case Op::divide_real: case Op::divide_int: case Op::modulo: {
            auto right = pop(), left = pop();
            if (instruction.type == Type::integer) stack.emplace_back(integer_operation(
                std::get<std::int32_t>(left), std::get<std::int32_t>(right), instruction));
            else if (instruction.type == Type::int64) stack.emplace_back(integer_operation(
                std::get<std::int64_t>(left), std::get<std::int64_t>(right), instruction));
            else if (instruction.type == Type::string) {
                constexpr std::size_t string_limit = 1'048'576;
                auto a = string_text(left);
                const auto& b = string_text(right);
                if (b.size() > string_limit || a.size() > string_limit - b.size())
                    trap(instruction, RuntimeError::string_limit);
                a += b;
                stack.emplace_back(std::move(a));
            } else {
                const auto a = std::get<double>(left), b = std::get<double>(right);
                double result = 0;
                switch (instruction.op) {
                case Op::add: result = a + b; break;
                case Op::subtract: result = a - b; break;
                case Op::multiply: result = a * b; break;
                default:
                    if (b == 0) trap(instruction, RuntimeError::division_zero);
                    result = a / b;
                }
                if (!std::isfinite(result)) trap(instruction, RuntimeError::real_overflow);
                stack.emplace_back(result);
            }
            break;
        }
        case Op::equal: case Op::not_equal: case Op::less: case Op::less_equal: case Op::greater: case Op::greater_equal: {
            const auto right = pop(), left = pop();
            bool result = false;
            if (instruction.type==Type::string) {
                const auto order=compare_strings(left,right);
                switch (instruction.op) {
                case Op::equal: result=order==0; break;
                case Op::not_equal: result=order!=0; break;
                case Op::less: result=order<0; break;
                case Op::less_equal: result=order<=0; break;
                case Op::greater: result=order>0; break;
                default: result=order>=0;
                }
                stack.emplace_back(result); break;
            }
            switch (instruction.op) {
            case Op::equal: result = values.equal(left,right); break;
            case Op::not_equal: result = !values.equal(left,right); break;
            case Op::less: result = left < right; break;
            case Op::less_equal: result = left <= right; break;
            case Op::greater: result = left > right; break;
            default: result = left >= right;
            }
            stack.emplace_back(result);
            break;
        }
        case Op::jump: frame.pc = instruction.operand; break;
        case Op::jump_false: if (!std::get<bool>(pop())) frame.pc = instruction.operand; break;
        case Op::print: write(format(pop(), instruction), instruction); break;
        case Op::newline: write("\n", instruction); break;
        case Op::read_line: (program.locals[instruction.operand].global ? globals : locals)[instruction.operand]->value = read_line(input, instruction); break;
        case Op::halt: return;
        }
    }
}

} // namespace abc

#include "abc/core.hpp"
#include "number.hpp"
#include "diagnostics.hpp"
#include "stdlib.hpp"
#include "runtime_values.hpp"
#include "files.hpp"
#include "linked.hpp"
#include "text_file.hpp"
#include "integer_values.hpp"
#include "random.hpp"
#include "console_input.hpp"
#include "console_output.hpp"
#include "formatting.hpp"

#include <charconv>
#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>
#include <array>
#include <bit>

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

std::uint64_t unsigned_operation(std::uint64_t a, std::uint64_t b, const Instruction& instruction) {
    switch (instruction.op) {
    case Op::add:
        if (a>UINT64_MAX-b) trap(instruction,RuntimeError::integer_overflow);
        return a+b;
    case Op::subtract:
        if (a<b) trap(instruction,RuntimeError::integer_overflow);
        return a-b;
    case Op::multiply:
        if (b && a>UINT64_MAX/b) trap(instruction,RuntimeError::integer_overflow);
        return a*b;
    case Op::divide_int: case Op::modulo:
        if (!b) trap(instruction,RuntimeError::division_zero);
        return instruction.op==Op::modulo ? a%b : a/b;
    default: trap(instruction,RuntimeError::invalid_bytecode);
    }
}

std::size_t random_signature(RandomArrayMethod method) {
    static const auto identifiers=[] {
        std::array<std::size_t,4> result{}; const auto catalog=stdlib::signatures();
        for (std::size_t i=0;i<result.size();++i) {
            const auto mode=static_cast<RandomArrayMethod>(i);
            const auto name=mode==RandomArrayMethod::rounded_real ? "randomreal" : "random";
            const auto parameters=mode==RandomArrayMethod::scaled_real ? std::vector<Type>{} : random_array_parameters(mode);
            const auto found=std::find_if(catalog.begin(),catalog.end(),[&](const auto& signature) {
                return signature.unit.empty() && signature.name==name && signature.parameters==parameters;
            });
            if (found==catalog.end()) throw runtime_error(RuntimeError::invalid_bytecode,{});
            result[i]=static_cast<std::size_t>(found-catalog.begin());
        }
        return result;
    }();
    return identifiers[static_cast<std::size_t>(method)];
}

std::string format(const Value& value, const Instruction& instruction) {
    if (std::holds_alternative<NilString>(value)) return "nil";
    if (const auto* text = std::get_if<std::u16string>(&value)) return to_utf8(*text, true);
    if (const auto* ch=std::get_if<char16_t>(&value)) return to_utf8(std::u16string(1,*ch),true);
    if (const auto* boolean = std::get_if<bool>(&value)) return *boolean ? "True" : "False";
    char buffer[128];
    std::to_chars_result result;
    if (const auto* integer = std::get_if<std::int32_t>(&value)) result = std::to_chars(buffer, buffer + sizeof buffer, *integer);
    else if (const auto* wide = std::get_if<std::int64_t>(&value)) result = std::to_chars(buffer, buffer + sizeof buffer, *wide);
    else if (const auto* word = std::get_if<std::uint16_t>(&value)) result = std::to_chars(buffer, buffer + sizeof buffer, *word);
    else if (const auto* byte_value = std::get_if<std::uint8_t>(&value)) result = std::to_chars(buffer, buffer + sizeof buffer, *byte_value);
    else if (const auto* short_value = std::get_if<std::int8_t>(&value)) result = std::to_chars(buffer, buffer + sizeof buffer, *short_value);
    else if (const auto* small_value = std::get_if<std::int16_t>(&value)) result = std::to_chars(buffer, buffer + sizeof buffer, *small_value);
    else if (const auto* unsigned32 = std::get_if<std::uint32_t>(&value)) result = std::to_chars(buffer, buffer + sizeof buffer, *unsigned32);
    else if (const auto* unsigned64 = std::get_if<std::uint64_t>(&value)) result = std::to_chars(buffer, buffer + sizeof buffer, *unsigned64);
    else result = std::to_chars(buffer, buffer + sizeof buffer, std::get<double>(value));
    if (result.ec != std::errc{}) trap(instruction, RuntimeError::number_format);
    return {buffer, result.ptr};
}
std::u16string scalar_text(const Value& value, const Instruction& instruction) {
    if (std::holds_alternative<NilString>(value)) return u"nil";
    if (const auto* ch=std::get_if<char16_t>(&value)) return std::u16string(1,*ch);
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
    auto part = value_type(value) == Type::string ? string_text(value)
        : value_type(value)==Type::character ? std::u16string(1,std::get<char16_t>(value)) : from_utf8(format(value,instruction));
    if (part.size() >= 1'048'576 || text.size() > 1'048'576 - part.size() - 1) trap(instruction,RuntimeError::string_limit);
    text += part; text += u' ';
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
    RandomContext random;
    ConsoleInput console(input,options);
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
    ConsoleOutput console_output(output,options);
    auto write = [&](std::u16string_view text, const Instruction& instruction) {
        console_output.write(text,instruction.location);
    };
    auto pop = [&]() { auto value = std::move(stack.back()); stack.pop_back(); return value; };
    const Value default_delimiter=std::u16string{u" "};
    auto delimiter_value=[&]()->const Value& { return program.print_delimiter ? globals[*program.print_delimiter]->value : default_delimiter; };
    stdlib::Context services{files,random,console,[&](std::u16string_view text,SourceLocation location) {
        Instruction instruction; instruction.location=location;
        write(text,instruction);
    },delimiter_value};
    while (!frames.empty()) {
        auto& frame = frames.back(); auto& locals = frame.locals;
        const auto& instruction = (*frame.code)[frame.pc++]; values.location = instruction.location;
        if (steps++ >= options.instruction_limit) trap(instruction, RuntimeError::instruction_limit);
        if (options.cancelled && options.cancelled()) trap(instruction, RuntimeError::cancelled);
        switch (instruction.op) {
        case Op::string_read: {
            const auto original=static_cast<StringRead>(instruction.operand); const auto method=string_read_base(original);
            const bool member=string_read_member(original);
            const auto result=string_read_try(method) ? std::get<std::shared_ptr<Cell>>(pop()) : std::shared_ptr<Cell>{};
            const auto cursor=std::get<std::shared_ptr<Cell>>(pop()); const auto source=pop();
            const auto start=std::get<std::int32_t>(cursor->value);
            if (member && start==INT32_MAX) trap(instruction,RuntimeError::integer_overflow);
            const auto initial=member ? start+1 : start;
            if (method==StringRead::integer) {
                const auto [number,position]=stdlib::read_string_integer(source,initial,instruction.location,options);
                values.write(cursor,member ? position-1 : position); stack.emplace_back(number); break;
            }
            auto [word,position]=stdlib::read_string_word(source,initial,instruction.location,options,method==StringRead::is_empty);
            if (!member && position!=start) values.write(cursor,position);
            if (method==StringRead::is_empty) { stack.emplace_back(static_cast<std::size_t>(position)>string_text(source).size()); break; }
            if (method==StringRead::word) {
                if (member) values.write(cursor,position-1);
                stack.emplace_back(std::move(word)); break;
            }
            if (method==StringRead::real) {
                const std::array<Value,1> arguments{std::move(word)};
                const auto number=*stdlib::invoke_text(stdlib::Function::str_to_real,arguments,instruction.location);
                if (member) values.write(cursor,position-1);
                stack.emplace_back(number); break;
            }
            const auto parsed=stdlib::try_parse_number(string_read_number(method),Value{std::move(word)},instruction.location);
            if (parsed) values.write(result,*parsed); else if (method==StringRead::try_real) values.write(result,0.0);
            if (!parsed) values.write(cursor,start);
            stack.emplace_back(parsed.has_value()); break;
        }
        case Op::convert_string: {
            const auto target=std::get<std::shared_ptr<Cell>>(pop()); const auto source=pop();
            values.write(target,instruction.type==Type::string ? source : Value{format_field(source,0,std::nullopt,instruction.location)}); break;
        }
        case Op::try_parse: case Op::val: {
            const auto code=instruction.op==Op::val ? std::get<std::shared_ptr<Cell>>(pop()) : std::shared_ptr<Cell>{};
            const auto target=std::get<std::shared_ptr<Cell>>(pop()); const auto source=pop();
            const auto parsed=stdlib::try_parse_number(instruction.type,source,instruction.location);
            if (parsed) values.write(target,*parsed);
            else if (instruction.type!=Type::integer) values.write(target,values.zero(instruction.type));
            if (code) values.write(code,std::int32_t{parsed ? 0 : 1}); else stack.emplace_back(parsed.has_value());
            break;
        }
        case Op::try_read: {
            const auto message=pop(); const auto target=std::get<std::shared_ptr<Cell>>(pop());
            try {
                if (std::holds_alternative<NilString>(message) || !string_text(message).empty()) {
                    write(scalar_text(message,instruction),instruction);
                    const auto& delimiter=delimiter_value();
                    if (std::holds_alternative<NilString>(delimiter) || !string_text(delimiter).empty())
                        write(scalar_text(delimiter,instruction),instruction);
                }
                values.write(target,console.read(instruction.type,instruction.location));
                stack.emplace_back(true);
            } catch (const Error& error) {
                // Resource limits, cancellation and VM faults remain uncatchable by language helpers.
                if (error.code!="E_INPUT" && error.code!="E_OUTPUT") throw;
                stack.emplace_back(false);
            }
            break;
        }
        case Op::object_text: {
            const auto width=instruction.operand ? std::get<std::int32_t>(pop()) : 0;
            const auto value=pop();
            stack.emplace_back(pad_field(object_text(program,value,instruction.type,instruction.location,options),width,instruction.location));
            break;
        }
        case Op::write_matrix: {
            const auto digits=instruction.operand ? std::optional<std::int32_t>{std::get<std::int32_t>(pop())} : std::nullopt;
            const auto width=std::get<std::int32_t>(pop()); const auto matrix=std::get<std::shared_ptr<Aggregate>>(pop());
            if (matrix->nil) trap(instruction,RuntimeError::nil_reference);
            for (std::size_t r=0;r<matrix->rows;++r) {
                if ((r&255U)==0 && options.cancelled && options.cancelled()) trap(instruction,RuntimeError::cancelled);
                for (std::size_t c=0;c<matrix->columns;++c) {
                    if ((c&255U)==0 && options.cancelled && options.cancelled()) trap(instruction,RuntimeError::cancelled);
                    const auto& value=matrix->items[r*matrix->columns+c]->value;
                    write(digits ? format_field(value,width,digits,instruction.location)
                        : pad_field(object_text(program,value,type_info(program,instruction.type).element,instruction.location,options),width,instruction.location),instruction);
                    write(u" ",instruction);
                }
                write(u"\n",instruction);
            }
            break;
        }
        case Op::array_query: {
            const auto query=static_cast<ArrayQuery>(instruction.operand);
            const auto dimension=query==ArrayQuery::rank ? 0 : std::get<std::int32_t>(pop());
            const auto array=std::get<std::shared_ptr<Aggregate>>(pop());
            if (array->nil) {
                if (query!=ArrayQuery::length_or_zero) trap(instruction,RuntimeError::nil_reference);
                stack.emplace_back(std::int32_t{0}); break;
            }
            const auto rank=type_info(program,instruction.type).rank;
            if (query==ArrayQuery::rank) { stack.emplace_back(static_cast<std::int32_t>(rank)); break; }
            if (dimension<0 || static_cast<unsigned>(dimension)>=rank) trap(instruction,RuntimeError::index_bounds);
            const auto size=rank==1 ? array->items.size() : dimension==0 ? array->rows : array->columns;
            stack.emplace_back(query==ArrayQuery::lower_bound ? std::int32_t{0} : static_cast<std::int32_t>(size)-(query==ArrayQuery::upper_bound ? 1 : 0));
            break;
        }
        case Op::console_output: {
            std::vector<Value> arguments(instruction.operand/2);
            for (auto i=arguments.size();i>0;--i) arguments[i-1]=pop();
            for (const auto& value:arguments) write(compound(value_type(value))
                ? object_text(program,value,value_type(value),instruction.location,options) : scalar_text(value,instruction),instruction);
            if (instruction.operand&1U) write(u"\n",instruction);
            break;
        }
        case Op::read_console: {
            std::vector<std::shared_ptr<Cell>> targets(instruction.operand/2);
            for (auto i=targets.size();i>0;--i) targets[i-1]=std::get<std::shared_ptr<Cell>>(pop());
            for (const auto& target:targets) values.write(target,console.read(target->type,instruction.location));
            if (instruction.operand&1U) console.skip_line(instruction.location);
            break;
        }
        case Op::read_array: {
            const auto target=std::get<std::shared_ptr<Aggregate>>(pop());
            if (target->nil) trap(instruction,RuntimeError::nil_reference);
            for (const auto& cell:target->items) values.write(cell,console.read(cell->type,instruction.location));
            console.skip_line(instruction.location); break;
        }
        case Op::bit_not: {
            const auto value=pop();
            stack.push_back(std::visit([&](const auto& n)->Value {
                using T=std::decay_t<decltype(n)>;
                if constexpr (std::is_integral_v<T> && !std::is_same_v<T,bool> && !std::is_same_v<T,char16_t>) {
                    using U=std::make_unsigned_t<T>;
                    return std::bit_cast<T>(static_cast<U>(~std::bit_cast<U>(n)));
                } else trap(instruction,RuntimeError::invalid_bytecode);
            },value));
            break;
        }
        case Op::bit_and: case Op::bit_or: case Op::bit_xor: {
            const auto right=pop(), left=pop();
            stack.push_back(std::visit([&](const auto& a)->Value {
                using T=std::decay_t<decltype(a)>;
                if constexpr (std::is_same_v<T,bool>) return a!=std::get<bool>(right);
                else if constexpr (std::is_integral_v<T> && !std::is_same_v<T,char16_t>) {
                    using U=std::make_unsigned_t<T>;
                    const auto x=std::bit_cast<U>(a), y=std::bit_cast<U>(std::get<T>(right));
                    const auto bits=instruction.op==Op::bit_and ? x&y : instruction.op==Op::bit_or ? x|y : x^y;
                    return std::bit_cast<T>(static_cast<U>(bits));
                } else trap(instruction,RuntimeError::invalid_bytecode);
            },left));
            break;
        }
        case Op::shift_left: case Op::shift_right: {
            const auto count=std::get<std::int32_t>(pop()); const auto value=pop();
            stack.push_back(std::visit([&](const auto& n)->Value {
                using T=std::decay_t<decltype(n)>;
                if constexpr (std::is_same_v<T,std::int32_t> || std::is_same_v<T,std::int64_t>
                    || std::is_same_v<T,std::uint32_t> || std::is_same_v<T,std::uint64_t>) {
                    using U=std::make_unsigned_t<T>;
                    constexpr unsigned width=sizeof(T)*8U;
                    const auto shift=static_cast<std::uint32_t>(count)&(width-1U);
                    const auto bits=std::bit_cast<U>(n);
                    U result=instruction.op==Op::shift_left ? static_cast<U>(bits<<shift) : static_cast<U>(bits>>shift);
                    if constexpr (std::is_signed_v<T>)
                        if (instruction.op==Op::shift_right && n<0 && shift) result|=static_cast<U>(~U{0}<<(width-shift));
                    return std::bit_cast<T>(result);
                } else trap(instruction,RuntimeError::invalid_bytecode);
            },value));
            break;
        }
        case Op::write_array: {
            const auto delimiter=pop(); const auto sequence=std::get<std::shared_ptr<Aggregate>>(pop());
            if (sequence->nil && (instruction.operand&2U)) trap(instruction,RuntimeError::nil_reference);
            if (type_info(program,instruction.type).rank==2) {
                for (std::size_t r=0;r<sequence->rows;++r) {
                    if ((r&255U)==0 && options.cancelled && options.cancelled()) trap(instruction,RuntimeError::cancelled);
                    for (std::size_t c=0;c<sequence->columns;++c) {
                        if ((c&255U)==0 && options.cancelled && options.cancelled()) trap(instruction,RuntimeError::cancelled);
                        write(object_text(program,sequence->items[r*sequence->columns+c]->value,type_info(program,instruction.type).element,instruction.location,options),instruction);
                        write(scalar_text(delimiter,instruction),instruction);
                    }
                    write(u"\n",instruction);
                }
                if (instruction.operand&1U) write(u"\n",instruction);
                break;
            }
            for (std::size_t i=0;i<sequence->items.size();++i) {
                if ((i&255U)==0 && options.cancelled && options.cancelled()) trap(instruction,RuntimeError::cancelled);
                if (i) write(scalar_text(delimiter,instruction),instruction);
                write(object_text(program,sequence->items[i]->value,type_info(program,instruction.type).element,instruction.location,options),instruction);
            }
            if (instruction.operand&1U) write(u"\n",instruction);
            break;
        }
        case Op::swap_values: {
            const auto right=std::get<std::shared_ptr<Cell>>(pop()), left=std::get<std::shared_ptr<Cell>>(pop());
            const auto saved=values.copy(left->value);
            values.write(left,right->value); values.write(right,saved); break;
        }
        case Op::sequence_mutate: {
            const auto method=static_cast<SequenceMutation>(instruction.operand);
            const auto second=mutation_indices(method) ? std::int64_t(std::get<std::int32_t>(pop())) : 0;
            const auto first=mutation_indices(method) ? std::int64_t(std::get<std::int32_t>(pop())) : 0;
            const auto receiver=pop();
            std::shared_ptr<Cell> text_cell;
            std::shared_ptr<Aggregate> sequence;
            std::size_t size;
            if (instruction.type==Type::string) {
                text_cell=std::get<std::shared_ptr<Cell>>(receiver);
                if (std::holds_alternative<NilString>(text_cell->value)) trap(instruction,RuntimeError::nil_reference);
                size=string_text(text_cell->value).size();
            } else {
                sequence=std::get<std::shared_ptr<Aggregate>>(receiver);
                if (sequence->nil) trap(instruction,RuntimeError::nil_reference);
                size=sequence->items.size();
            }
            std::size_t start=0, count=size;
            if (method==SequenceMutation::reverse_range) {
                const auto index=first-(instruction.type==Type::string ? 1 : 0);
                if (index<0 || second<0 || static_cast<std::uint64_t>(index)>size || static_cast<std::uint64_t>(second)>size-static_cast<std::size_t>(index))
                    trap(instruction,instruction.type==Type::string ? RuntimeError::string_slice : RuntimeError::index_bounds);
                start=static_cast<std::size_t>(index); count=static_cast<std::size_t>(second);
            }
            auto exchange=[&](std::size_t a,std::size_t b) {
                const auto saved=values.copy(sequence->items[a]->value);
                values.write(sequence->items[a],sequence->items[b]->value); values.write(sequence->items[b],saved);
            };
            if (method==SequenceMutation::swap_indices) {
                if (first<0 || second<0 || static_cast<std::uint64_t>(first)>=size || static_cast<std::uint64_t>(second)>=size)
                    trap(instruction,RuntimeError::index_bounds);
                exchange(static_cast<std::size_t>(first),static_cast<std::size_t>(second));
            } else if (method==SequenceMutation::shuffle || method==SequenceMutation::shuffle_result) {
                for (auto i=size;i>1;--i) {
                    if ((i&255U)==0 && options.cancelled && options.cancelled()) trap(instruction,RuntimeError::cancelled);
                    exchange(i-1,static_cast<std::size_t>(random.range(0,static_cast<std::int32_t>(i),instruction.location)));
                }
            } else if (text_cell) {
                auto text=string_text(text_cell->value);
                std::reverse(text.begin()+static_cast<std::ptrdiff_t>(start),text.begin()+static_cast<std::ptrdiff_t>(start+count));
                values.write(text_cell,text);
            } else {
                if (type_info(program,instruction.type).kind==CompoundKind::list) ++sequence->version;
                for (std::size_t i=0;i<count/2;++i) {
                    if ((i&255U)==0 && options.cancelled && options.cancelled()) trap(instruction,RuntimeError::cancelled);
                    exchange(start+i,start+count-i-1);
                }
            }
            if (mutation_result(method)) stack.push_back(receiver);
            break;
        }
        case Op::random_array: case Op::fill_random_array: {
            const auto method=static_cast<RandomArrayMethod>(instruction.operand);
            const auto count=random_array_parameters(method).size();
            std::array<Value,3> parameters;
            for (auto i=count;i>0;--i) parameters[i-1]=pop();
            std::shared_ptr<Aggregate> target;
            if (instruction.op==Op::random_array) {
                const auto length=std::get<std::int32_t>(pop());
                if (type_info(program,instruction.type).rank==2) target=values.matrix(instruction.type,std::get<std::int32_t>(pop()),length);
                else {
                    if (length<0) trap(instruction,RuntimeError::array_length);
                    target=values.array(instruction.type,static_cast<std::size_t>(length));
                }
            } else {
                target=std::get<std::shared_ptr<Aggregate>>(pop());
                if (target->nil) trap(instruction,RuntimeError::nil_reference);
            }
            const auto id=random_signature(method);
            std::size_t sampled=0;
            for (const auto& cell:target->items) {
                if ((sampled++&255U)==0 && options.cancelled && options.cancelled()) trap(instruction,RuntimeError::cancelled);
                Value value;
                if (method==RandomArrayMethod::scaled_real) {
                    const auto sample=stdlib::invoke(id,{},instruction.location,services);
                    const auto number=std::get<double>(sample)*(std::get<double>(parameters[1])-std::get<double>(parameters[0]));
                    if (std::isnan(number)) trap(instruction,RuntimeError::math_domain);
                    if (!std::isfinite(number)) trap(instruction,RuntimeError::real_overflow);
                    value=number;
                } else value=stdlib::invoke(id,std::span<const Value>(parameters.data(),count),instruction.location,services);
                values.write(cell,value);
            }
            if (instruction.op==Op::random_array) stack.emplace_back(target);
            break;
        }
        case Op::set_character: {
            const auto ch=std::get<char16_t>(pop()); const auto index=std::int64_t(std::get<std::int32_t>(pop()))-1;
            const auto target=std::get<std::shared_ptr<Cell>>(pop());
            if (std::holds_alternative<NilString>(target->value)) trap(instruction,RuntimeError::nil_reference);
            auto text=string_text(target->value);
            if (index<0 || static_cast<std::size_t>(index)>=text.size()) trap(instruction,RuntimeError::index_bounds);
            text[static_cast<std::size_t>(index)]=ch; values.write(target,std::move(text)); break;
        }
        case Op::iteration_version: case Op::iteration_check: {
            const auto expected=instruction.op==Op::iteration_check ? std::get<std::int64_t>(pop()) : 0;
            const auto value=pop();
            if (instruction.type==Type::string) {
                if (std::holds_alternative<NilString>(value)) trap(instruction,RuntimeError::nil_reference);
                if (instruction.op==Op::iteration_version) stack.emplace_back(std::int64_t{0});
                break;
            }
            const auto sequence=std::get<std::shared_ptr<Aggregate>>(value);
            if (sequence->nil) trap(instruction,RuntimeError::nil_reference);
            const auto version=static_cast<std::int64_t>(sequence->version);
            if (instruction.op==Op::iteration_version) stack.emplace_back(version);
            else if (type_info(program,instruction.type).kind==CompoundKind::list && version!=expected)
                trap(instruction,RuntimeError::collection_changed);
            break;
        }
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
                else { write(text,instruction); if (method==LinkedMethod::println) write(u"\n",instruction); }
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
                else write(text,instruction); // Println also adds no extra newline.
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
                else { write(text,instruction); if (method == BufferMethod::println) write(u"\n",instruction); }
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
                    write(text,instruction);
                    // Collections.Stack.Print also writes a newline in the original unit.
                    if (lifo || method == EndpointMethod::println) write(u"\n",instruction);
                }
            }
            break;
        }
        case Op::read_value: stack.push_back(console.read(instruction.type,instruction.location)); break;
        case Op::file_print: {
            const auto value=pop(); auto& file=text_file(stack.back(),instruction.location);
            text_write(file,scalar_text(value,instruction),instruction.location); break;
        }
        case Op::print_output: case Op::file_output: {
            std::vector<Value> arguments(instruction.operand/4);
            for (auto value=arguments.rbegin();value!=arguments.rend();++value) *value=pop();
            const bool file_output=instruction.op==Op::file_output;
            const auto receiver=file_output ? pop() : Value{std::int32_t{0}};
            const auto method=static_cast<TextOutput>(instruction.operand%4);
            auto* file=file_output ? &text_file(receiver,instruction.location) : nullptr;
            const auto emit_text=[&](std::u16string_view text) {
                if (file) text_write(*file,text,instruction.location); else write(text,instruction);
            };
            // Empty Write and Print do not require an assigned or open file.
            if (arguments.empty() && (method==TextOutput::write || method==TextOutput::print)) break;
            for (std::size_t n=0;n<arguments.size();++n) {
                emit_text(compound(value_type(arguments[n]))
                    ? object_text(program,arguments[n],value_type(arguments[n]),instruction.location,options) : scalar_text(arguments[n],instruction));
                if (method==TextOutput::print || (method==TextOutput::println && n+1<arguments.size())) {
                    const auto& delimiter=delimiter_value();
                    if (std::holds_alternative<NilString>(delimiter) || !string_text(delimiter).empty())
                        emit_text(scalar_text(delimiter,instruction));
                }
            }
            if (method==TextOutput::writeln || method==TextOutput::println) emit_text(file ? file_newline : u"\n");
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
            if (instruction.type==Type::boolean) {
                const auto target=std::get<std::shared_ptr<Cell>>(pop()); values.write(target,!std::get<bool>(target->value)); break;
            }
            const auto step=pop(); auto target=std::get<std::shared_ptr<Cell>>(pop()); auto operation=instruction;
            operation.op=instruction.operand ? Op::subtract : Op::add;
            if (wide_unsigned(instruction.type)) values.write(target,shifted_unsigned(instruction.type,target->value,integral_number(step,instruction.location),instruction.operand!=0,instruction.location));
            else if (instruction.type==Type::integer) values.write(target,integer_operation(std::get<std::int32_t>(target->value),std::get<std::int32_t>(step),operation));
            else if (instruction.type==Type::int64) values.write(target,integer_operation(std::get<std::int64_t>(target->value),std::get<std::int64_t>(step),operation));
            else {
                const auto old=integral_number(target->value,instruction.location);
                const auto amount=std::int64_t(std::get<std::int32_t>(step));
                const auto next=instruction.operand ? old-amount : old+amount;
                const auto [minimum,maximum]=instruction.type==Type::character ? std::pair<std::int64_t,std::int64_t>{0,65535} : small_integer_bounds(instruction.type);
                if (next<minimum || next>maximum) trap(instruction,RuntimeError::integer_overflow);
                values.write(target,instruction.type==Type::character ? Value{static_cast<char16_t>(next)} : checked_small_integer(instruction.type,next,instruction.location));
            }
            break;
        }
        case Op::mutate_string: {
            const auto& signature=stdlib::signatures()[instruction.operand];
            const std::size_t ref=signature.function==stdlib::Function::insert_string ? 1 : 0;
            std::vector<Value> arguments(signature.parameters.size());
            for (std::size_t i=arguments.size();i>0;--i) arguments[i-1]=pop();
            auto target=std::get<std::shared_ptr<Cell>>(arguments[ref]); arguments[ref]=target->value;
            const auto result=stdlib::invoke(instruction.operand,arguments,instruction.location,services); values.write(target,result); break;
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
            result->rows=original->rows; result->columns=original->columns;
            for (std::size_t i=0;i<result->items.size();++i) values.write(result->items[i],original->items[i]->value);
            stack.emplace_back(result); break;
        }
        case Op::array_high: {
            auto array=std::get<std::shared_ptr<Aggregate>>(pop());
            stack.emplace_back(static_cast<std::int32_t>(type_info(program,instruction.type).rank==2 ? array->rows : array->items.size())-1); break;
        }
        case Op::sequence_sort: {
            auto sequence=std::get<std::shared_ptr<Aggregate>>(pop());
            if (sequence->nil) trap(instruction,RuntimeError::nil_reference);
            if (type_info(program,instruction.type).kind==CompoundKind::list) ++sequence->version;
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
            const auto last=std::int64_t(std::get<std::int32_t>(pop()));
            const bool matrix=instruction.type!=Type::string && type_info(program,instruction.type).rank==2 && !instruction.operand;
            const auto first=matrix ? std::int64_t(std::get<std::int32_t>(pop())) : last;
            const auto value=pop();
            if (instruction.type==Type::string) {
                if (std::holds_alternative<NilString>(value)) trap(instruction,RuntimeError::nil_reference);
                const auto& text=string_text(value); const auto offset=first-(instruction.operand ? 0 : 1);
                if (offset<0 || static_cast<std::size_t>(offset)>=text.size()) trap(instruction,RuntimeError::index_bounds);
                stack.emplace_back(text[static_cast<std::size_t>(offset)]); break;
            }
            auto array = std::get<std::shared_ptr<Aggregate>>(value);
            if (array->nil) trap(instruction,RuntimeError::nil_reference);
            std::int64_t index=first;
            if (matrix) {
                if (first<0 || last<0 || static_cast<std::uint64_t>(first)>=array->rows || static_cast<std::uint64_t>(last)>=array->columns)
                    trap(instruction,RuntimeError::index_bounds);
                index=static_cast<std::int64_t>(static_cast<std::size_t>(first)*array->columns+static_cast<std::size_t>(last));
            }
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
            const auto value=pop();
            if (instruction.type==Type::string) { stack.emplace_back(static_cast<std::int32_t>(string_text(value).size())); break; }
            auto array = std::get<std::shared_ptr<Aggregate>>(value);
            if (array->nil && (instruction.operand || type_info(program,instruction.type).kind==CompoundKind::list)) trap(instruction,RuntimeError::nil_reference);
            stack.emplace_back(static_cast<std::int32_t>(array->items.size())); break;
        }
        case Op::array_new: case Op::array_literal: {
            if (type_info(program,instruction.type).rank==2) {
                const auto columns=std::get<std::int32_t>(pop()), rows=std::get<std::int32_t>(pop());
                stack.emplace_back(values.matrix(instruction.type,rows,columns)); break;
            }
            const auto count = instruction.op == Op::array_literal ? static_cast<std::int32_t>(instruction.operand) : std::get<std::int32_t>(pop());
            if (count < 0) trap(instruction,RuntimeError::array_length);
            auto array = values.array(instruction.type,static_cast<std::size_t>(count));
            if (instruction.op == Op::array_literal)
                for (auto n = array->items.size(); n > 0; --n) values.write(array->items[n-1],pop());
            stack.emplace_back(array); break;
        }
        case Op::resize_array: {
            if (type_info(program,instruction.type).rank==2) {
                const auto columns=std::get<std::int32_t>(pop()), rows=std::get<std::int32_t>(pop());
                const auto original=std::get<std::shared_ptr<Aggregate>>(pop());
                const auto target=std::get<std::shared_ptr<Cell>>(pop());
                auto result=values.matrix(instruction.type,rows,columns);
                std::size_t copied=0;
                for (std::size_t r=0;r<std::min(result->rows,original->rows);++r)
                    for (std::size_t c=0;c<std::min(result->columns,original->columns);++c) {
                        if ((copied++&255U)==0 && options.cancelled && options.cancelled()) trap(instruction,RuntimeError::cancelled);
                        values.write(result->items[r*result->columns+c],original->items[r*original->columns+c]->value);
                    }
                values.write(target,result); break;
            }
            const auto count = std::get<std::int32_t>(pop()); auto target = std::get<std::shared_ptr<Cell>>(pop());
            if (count < 0) trap(instruction,RuntimeError::array_length);
            const auto original = std::get<std::shared_ptr<Aggregate>>(target->value);
            if (static_cast<std::size_t>(count) == original->items.size() && !original->nil) break;
            auto result = values.array(instruction.type,static_cast<std::size_t>(count));
            for (std::size_t i = 0; i < std::min(result->items.size(),original->items.size()); ++i) values.write(result->items[i],original->items[i]->value);
            values.write(target,result); break;
        }
        case Op::list_add: {
            auto value = pop(); auto list = std::get<std::shared_ptr<Aggregate>>(pop());
            if (list->nil) trap(instruction,RuntimeError::nil_reference);
            auto item=values.cell(type_info(program,instruction.type).element,values.copy(value)); values.own_list_cell(item,list);
            list->items.push_back(std::move(item)); ++list->version; break;
        }
        case Op::list_clear: {
            auto list=std::get<std::shared_ptr<Aggregate>>(pop()); if (list->nil) trap(instruction,RuntimeError::nil_reference);
            list->items.clear(); ++list->version; break;
        }
        case Op::list_remove: {
            const auto index = std::get<std::int32_t>(pop()); auto list = std::get<std::shared_ptr<Aggregate>>(pop());
            if (list->nil) trap(instruction,RuntimeError::nil_reference);
            if (index < 0 || static_cast<std::size_t>(index) >= list->items.size()) trap(instruction,RuntimeError::index_bounds);
            list->items.erase(list->items.begin()+index); ++list->version; break;
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
            auto value = stdlib::invoke(instruction.operand, std::span<const Value>(stack).subspan(start), instruction.location,services);
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
            else if (instruction.type==Type::string) stack.emplace_back(std::u16string(1,std::get<char16_t>(value)));
            else if (wide_unsigned(instruction.type)) stack.push_back(checked_unsigned_integer(instruction.type,value,instruction.location));
            else if (wide_unsigned(value_type(value))) {
                if (instruction.type==Type::real) stack.emplace_back(static_cast<double>(unsigned_number(value,instruction.location)));
                else stack.push_back(checked_signed_integer(instruction.type,value,instruction.location));
            }
            else if (small_integer(value_type(value))) {
                const auto n=integral_number(value,instruction.location);
                if (instruction.type==Type::integer) stack.emplace_back(std::int32_t(n));
                else if (instruction.type==Type::int64) stack.emplace_back(std::int64_t(n));
                else if (small_integer(instruction.type)) stack.push_back(checked_small_integer(instruction.type,n,instruction.location));
                else stack.emplace_back(double(n));
            }
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
            } else if (instruction.type==Type::uint64) {
                const auto n=std::get<std::uint64_t>(value);
                if (n) trap(instruction,RuntimeError::integer_overflow);
                stack.emplace_back(std::uint64_t{0});
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
            else if (instruction.type==Type::uint64) stack.emplace_back(unsigned_operation(
                std::get<std::uint64_t>(left),std::get<std::uint64_t>(right),instruction));
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
        case Op::print: write(scalar_text(pop(), instruction), instruction); break;
        case Op::newline: write(u"\n", instruction); break;
        case Op::read_line: values.write((program.locals[instruction.operand].global ? globals : locals)[instruction.operand],console.read(instruction.type,instruction.location)); console.skip_line(instruction.location); break;
        case Op::halt: console_output.finish(); return;
        }
    }
    console_output.finish();
}

} // namespace abc

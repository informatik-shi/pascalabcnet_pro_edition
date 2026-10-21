#pragma once
#include "abc/core.hpp"
#include "diagnostics.hpp"

namespace abc {
inline const TypeInfo& type_info(const Program& program, Type type) {
    const auto index = static_cast<unsigned>(type) - 64U;
    if (!compound(type) || index >= program.types.size()) throw Error("E_BYTECODE", {}, "Invalid compound type");
    return program.types[index];
}
inline bool valid_value_type(const Program& program, Type type) {
    return numeric(type) || type == Type::boolean || type == Type::string
        || (compound(type) && static_cast<unsigned>(type) - 64U < program.types.size());
}
struct Values {
    const Program& program;
    std::size_t remaining;
    SourceLocation location{};
    std::shared_ptr<Cell> cell(Type type, Value value) {
        if (!remaining) throw runtime_error(RuntimeError::allocation_limit, location);
        --remaining;
        return std::make_shared<Cell>(Cell{type, std::move(value)});
    }
    Value zero(Type type, bool constructed = false) {
        switch (type) {
        case Type::integer: return std::int32_t{0};
        case Type::int64: return std::int64_t{0};
        case Type::real: return 0.0;
        case Type::boolean: return false;
        case Type::string: return std::u16string{};
        default: {
            const auto& info = type_info(program, type);
            auto aggregate = std::make_shared<Aggregate>(); aggregate->type = type;
            aggregate->nil = info.kind != CompoundKind::record && !constructed;
            if (endpoint_collection(info.kind) && constructed)
                aggregate->ends = std::make_unique<std::deque<std::shared_ptr<Cell>>>();
            if (info.kind == CompoundKind::record)
                for (const auto& field : info.fields) aggregate->items.push_back(cell(field.type, zero(field.type)));
            return aggregate;
        }
        }
    }
    Value copy(const Value& value) {
        if (const auto* aggregate = std::get_if<std::shared_ptr<Aggregate>>(&value);
            aggregate && type_info(program, (*aggregate)->type).kind == CompoundKind::record) {
            auto result = std::make_shared<Aggregate>(); result->type = (*aggregate)->type;
            for (const auto& field : (*aggregate)->items) result->items.push_back(cell(field->type, copy(field->value)));
            return result;
        }
        return value;
    }
    void write(const std::shared_ptr<Cell>& target, const Value& value) {
        if (compound(target->type) && type_info(program,target->type).kind == CompoundKind::record) {
            const auto original = std::get<std::shared_ptr<Aggregate>>(target->value);
            const auto source = std::get<std::shared_ptr<Aggregate>>(value);
            if (original == source) return;
            for (std::size_t i = 0; i < original->items.size(); ++i) write(original->items[i],source->items[i]->value);
        } else target->value = value;
    }
    bool equal(const Value& left, const Value& right) const {
        if (const auto* a = std::get_if<std::shared_ptr<Aggregate>>(&left)) {
            const auto& b = std::get<std::shared_ptr<Aggregate>>(right);
            if (type_info(program,(*a)->type).kind != CompoundKind::record) return ((*a)->nil && b->nil) || *a == b;
            for (std::size_t i = 0; i < (*a)->items.size(); ++i)
                if (!equal((*a)->items[i]->value,b->items[i]->value)) return false;
            return true;
        }
        return left == right;
    }
    std::shared_ptr<Aggregate> array(Type type, std::size_t size) {
        if (size > remaining) throw runtime_error(RuntimeError::allocation_limit, location);
        auto result = std::make_shared<Aggregate>(); result->type = type;
        const auto element = type_info(program, type).element;
        for (std::size_t i = 0; i < size; ++i) result->items.push_back(cell(element, zero(element)));
        return result;
    }
};
} // namespace abc

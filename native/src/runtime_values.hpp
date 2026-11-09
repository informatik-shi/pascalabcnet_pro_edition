#pragma once
#include "abc/core.hpp"
#include "diagnostics.hpp"
#include "hash_set.hpp"
#include "text_file.hpp"
#include "record_interface.hpp"
#include "cell_references.hpp"
#include <span>
#include <functional>
#include <algorithm>
#include <new>
#include <stdexcept>
#include <map>

namespace abc {
inline const TypeInfo& type_info(const Program& program, Type type) {
    const auto index = static_cast<unsigned>(type) - 64U;
    if (!compound(type) || index >= program.types.size()) throw Error("E_BYTECODE", {}, "Invalid compound type");
    return program.types[index];
}
inline bool valid_value_type(const Program& program, Type type) {
    return scalar(type) || type==Type::text_file || type==Type::type_descriptor
        || (compound(type) && static_cast<unsigned>(type) - 64U < program.types.size());
}
// Execution-scoped tracking breaks reference cycles without allocating during cleanup.
struct ReferenceObjects {
    struct Entry { std::weak_ptr<Aggregate> object;std::shared_ptr<Aggregate> retained; };
    std::vector<Entry> entries;
    void observe(const std::shared_ptr<Aggregate>& object) { entries.push_back({object,{}}); }
    ~ReferenceObjects() {
        for (auto& entry:entries) entry.retained=entry.object.lock();
        for (auto& entry:entries) if (entry.retained) {entry.retained->enumerator.reset();entry.retained->list_storage.reset();entry.retained->hash_set.reset();entry.retained->items.clear();}
    }
};
struct Values {
    const Program& program;
    std::size_t remaining;
    SourceLocation location{};
    std::shared_ptr<Aggregate> empty_integer_range{};
    std::shared_ptr<Aggregate> empty_collection{};
    std::vector<std::pair<Type,std::weak_ptr<Aggregate>>> empty_enumerators{};
    std::vector<std::pair<Type,std::weak_ptr<Aggregate>>> default_comparers{};
    std::vector<std::pair<Type,std::weak_ptr<Aggregate>>> default_equalities{};
    std::size_t sequence_depth=0;
    std::map<std::pair<std::uintptr_t,Type>,std::weak_ptr<Aggregate>> user_enumerables{};
    std::size_t cleanup_depth=0;
    std::function<void(bool)> cleanup_mode{};
    std::size_t list_storage_remaining=remaining;
    std::function<Value(std::size_t,std::span<const Value>)> invoke{};
    std::function<Value(const std::shared_ptr<Aggregate>&,std::span<const Value>)> invoke_closure{};
    std::function<void(const std::shared_ptr<Aggregate>&)> observe_object{};
    std::function<Value(std::size_t,std::span<const Value>)> invoke_builtin{};
    std::size_t callable_target(const std::shared_ptr<Aggregate>& callable) const {
        if (!callable || callable->nil) throw runtime_error(RuntimeError::nil_reference,location);
        if (!callable->routine || *callable->routine>=program.routines.size()
            || !callable_matches(type_info(program,callable->type),program.routines[*callable->routine]))
            throw runtime_error(RuntimeError::invalid_bytecode,location);
        if (callable->captures.size()!=program.routines[*callable->routine].captures.size()) throw runtime_error(RuntimeError::invalid_bytecode,location);
        return *callable->routine;
    }
    Value invoke_value(const std::shared_ptr<Aggregate>& callable,std::span<const Value> arguments) {
        const auto target=callable_target(callable);
        if (!invoke) throw runtime_error(RuntimeError::invalid_bytecode,location);
        return invoke_closure ? invoke_closure(callable,arguments) : invoke(target,arguments);
    }
    void number(const Value& value) {
        if (const auto* big=std::get_if<BigInteger>(&value)) {
            const auto size=(big->bits()+31)/32;
            if (size>remaining) throw runtime_error(RuntimeError::allocation_limit,location);
            remaining-=size;
        }
    }
    std::shared_ptr<Cell> cell(Type type, Value value) {
        if (!remaining) throw runtime_error(RuntimeError::allocation_limit, location);
        --remaining;
        return std::make_shared<Cell>(Cell{type, std::move(value)});
    }
    Value zero(Type type, bool constructed = false, bool generic = false) {
        switch (type) {
        case Type::integer: return std::int32_t{0};
        case Type::int64: return std::int64_t{0};
        case Type::real: return 0.0;
        case Type::single: return 0.0F;
        case Type::boolean: return false;
        case Type::character: return char16_t{0};
        case Type::word: return std::uint16_t{0};
        case Type::byte: return std::uint8_t{0};
        case Type::shortint: return std::int8_t{0};
        case Type::smallint: return std::int16_t{0};
        case Type::longword: return std::uint32_t{0};
        case Type::uint64: return std::uint64_t{0};
        case Type::big_integer: return BigInteger{};
        case Type::type_descriptor: return ReflectedType{};
        case Type::string: return generic ? Value{NilString{}} : Value{std::u16string{}};
        case Type::text_file: return generic ? Value{std::shared_ptr<TextFile>{}} : Value{std::make_shared<TextFile>()};
        default: {
            const auto& info = type_info(program, type);
            if(info.kind==CompoundKind::new_set)return NewSetValue{type,cell(info.key,zero(info.key,!generic,generic))};
            if(info.kind==CompoundKind::enumeration)return EnumValue{type,0};
            if (buffer_collection(info.kind) && constructed) return buffer(type,0);
            const bool object=info.kind==CompoundKind::object_class;
            const bool iterator=info.kind==CompoundKind::enumerator;
            const bool dictionary=info.kind==CompoundKind::dictionary || info.kind==CompoundKind::dictionary_keys || info.kind==CompoundKind::dictionary_values;
            if ((object || iterator || dictionary || info.kind==CompoundKind::list || info.kind==CompoundKind::hash_set || info.kind==CompoundKind::empty_collection) && constructed) {
                if (!remaining) throw runtime_error(RuntimeError::allocation_limit,location);
                --remaining;
            }
            auto aggregate = std::make_shared<Aggregate>(); aggregate->type = type;
            aggregate->nil = info.kind != CompoundKind::record && !constructed;
            if((info.kind==CompoundKind::hash_set || info.kind==CompoundKind::dictionary) && constructed)aggregate->hash_set=std::make_shared<HashSetState>();
            if (info.kind == CompoundKind::linked_list && constructed) aggregate->links = std::make_unique<LinkedLinks>();
            if (endpoint_collection(info.kind) && constructed)
                aggregate->ends = std::make_unique<std::deque<std::shared_ptr<Cell>>>();
            if (info.kind == CompoundKind::record || (object && constructed))
                for (const auto& field : info.fields) aggregate->items.push_back(cell(field.type, zero(field.type,false,generic || object || key_value_pair_type(info) || dictionary_enumerator_type(info) || collection_enumerator_type(info))));
            if ((object || iterator || dictionary || info.kind==CompoundKind::list || info.kind==CompoundKind::hash_set) && constructed && observe_object) observe_object(aggregate);
            return aggregate;
        }
        }
    }
    Value copy(const Value& value) {
        if(const auto* set=std::get_if<NewSetValue>(&value)){if(!set->storage)throw runtime_error(RuntimeError::invalid_bytecode,location);return NewSetValue{set->type,cell(set->storage->type,set->storage->value)};}
        if (const auto* aggregate = std::get_if<std::shared_ptr<Aggregate>>(&value);
            aggregate && !(*aggregate)->boxed && type_info(program, (*aggregate)->type).kind == CompoundKind::record) {
            auto result = std::make_shared<Aggregate>(); result->type = (*aggregate)->type;
            for (const auto& field : (*aggregate)->items) result->items.push_back(cell(field->type, copy(field->value)));
            return result;
        }
        return value;
    }
    void own_list_cell(const std::shared_ptr<Cell>& cell, const std::shared_ptr<Aggregate>& list) {
        cell->list_owner=list;
        if(const auto* set=std::get_if<NewSetValue>(&cell->value))own_list_cell(set->storage,list);
        if (compound(cell->type) && type_info(program,cell->type).kind==CompoundKind::record)
            for (const auto& field:std::get<std::shared_ptr<Aggregate>>(cell->value)->items) own_list_cell(field,list);
    }
    void write(const std::shared_ptr<Cell>& target, const Value& value,bool versioned=true) {
        if(versioned&&!array_cell_reference(target))if (const auto owner=target->list_owner.lock()) ++owner->version;
        if(const auto* set=std::get_if<NewSetValue>(&target->value)){const auto& source=std::get<NewSetValue>(value);write(set->storage,source.storage->value,false);return;}
        if (compound(target->type) && type_info(program,target->type).kind == CompoundKind::record) {
            const auto original = std::get<std::shared_ptr<Aggregate>>(target->value);
            const auto source = std::get<std::shared_ptr<Aggregate>>(value);
            if (original == source) return;
            for (std::size_t i = 0; i < original->items.size(); ++i) write(original->items[i],source->items[i]->value,false);
        } else target->value = value;
    }
    bool equal(const Value& left, const Value& right) const {
        if (const auto* a = std::get_if<std::shared_ptr<Aggregate>>(&left)) {
            const auto& b = std::get<std::shared_ptr<Aggregate>>(right);
            if(record_box_reference(a->get())||record_box_reference(b.get()))return record_box_identity(a->get())==record_box_identity(b.get());
            const auto kind=type_info(program,(*a)->type).kind;
            if (kind==CompoundKind::sequence && !(*a)->nil && !b->nil && (*a)->sequence && b->sequence
                && (collection_sequence((*a)->sequence->provider) || (*a)->sequence->provider==SequenceProvider::hash_set || dictionary_provider((*a)->sequence->provider)) && (collection_sequence(b->sequence->provider) || b->sequence->provider==SequenceProvider::hash_set || dictionary_provider(b->sequence->provider)))
                return (*a)->sequence->source==b->sequence->source;
            if (kind==CompoundKind::callable) return (*a)->nil || b->nil ? (*a)->nil==b->nil : ((*a)->routine==b->routine && (*a)->captures==b->captures);
            if (kind!=CompoundKind::record && kind!=CompoundKind::tuple) return ((*a)->nil && b->nil) || *a == b;
            if ((*a)->nil || b->nil) return (*a)->nil && b->nil;
            for (std::size_t i = 0; i < (*a)->items.size(); ++i)
                if (!equal((*a)->items[i]->value,b->items[i]->value)) return false;
            return true;
        }
        return left == right;
    }
    std::shared_ptr<ListStorage> list_backing(const std::shared_ptr<Aggregate>&);
    std::shared_ptr<Aggregate> list_adopt_array(Type,const std::shared_ptr<Aggregate>&);
    void set_list_capacity(const std::shared_ptr<Aggregate>&,std::size_t);
    void list_append_cell(const std::shared_ptr<Aggregate>&,std::shared_ptr<Cell>);
    void list_insert_values(const std::shared_ptr<Aggregate>&,std::size_t,std::span<const Value>);
    void list_erase(const std::shared_ptr<Aggregate>&,std::size_t,std::size_t);
    void grow_list(const std::shared_ptr<Aggregate>& list, std::size_t required) {
        if (required<=list->list_capacity) return;
        if (required>INT32_MAX) throw runtime_error(RuntimeError::allocation_limit,location);
        const auto doubled=list->list_capacity ? std::min<std::size_t>(INT32_MAX,list->list_capacity*2) : 4;
        set_list_capacity(list,std::max(doubled,required));
    }
    std::shared_ptr<Aggregate> array(Type type, std::size_t size, bool generic = false) {
        if (size > remaining) throw runtime_error(RuntimeError::allocation_limit, location);
        auto result = std::make_shared<Aggregate>(); result->type = type;
        const auto element = type_info(program, type).element;
        if (type_info(program,type).kind==CompoundKind::list) set_list_capacity(result,size);
        const bool list=type_info(program,type).kind==CompoundKind::list;
        for (std::size_t i = 0; i < size; ++i) {auto item=cell(element,zero(element,false,generic));if(list)list_append_cell(result,std::move(item));else result->items.push_back(std::move(item));}
        if(list && observe_object){if(!remaining)throw runtime_error(RuntimeError::allocation_limit,location);--remaining;observe_object(result);}
        return result;
    }
    std::shared_ptr<Aggregate> matrix(Type type, std::int32_t rows, std::int32_t columns, bool generic = false) {
        if (rows<0 || columns<0) throw runtime_error(RuntimeError::array_length,location);
        const auto r=static_cast<std::size_t>(rows), c=static_cast<std::size_t>(columns);
        if (r && c>remaining/r) throw runtime_error(RuntimeError::allocation_limit,location);
        auto result=array(type,r*c,generic); result->rows=r; result->columns=c; return result;
    }
    std::shared_ptr<Aggregate> buffer(Type type, std::int32_t size) {
        if (size < 0) throw runtime_error(RuntimeError::array_length,location);
        if (size > (INT32_MAX-4)/2) throw runtime_error(RuntimeError::integer_overflow,location);
        auto result = array(type,static_cast<std::size_t>(size)*2+4,true);
        result->count = static_cast<std::size_t>(size); return result;
    }
    void reserve_buffer(const std::shared_ptr<Aggregate>& target, std::int32_t capacity) {
        if (capacity < 0 || static_cast<std::size_t>(capacity) <= target->items.size()) return;
        auto replacement = array(target->type,static_cast<std::size_t>(capacity),true);
        for (std::size_t i=0;i<target->items.size();++i) write(replacement->items[i],target->items[i]->value);
        target->items.swap(replacement->items);
    }
    void resize_buffer(const std::shared_ptr<Aggregate>& target, std::int32_t size) {
        if (size < 0) throw runtime_error(RuntimeError::array_length,location);
        if (static_cast<std::size_t>(size) > target->items.size()) {
            if (size > INT32_MAX/2) throw runtime_error(RuntimeError::integer_overflow,location);
            reserve_buffer(target,size*2);
            const auto element=type_info(program,target->type).element;
            for (std::size_t i=target->count;i<static_cast<std::size_t>(size);++i) write(target->items[i],zero(element,false,true));
        }
        target->count = static_cast<std::size_t>(size);
    }
};
} // namespace abc

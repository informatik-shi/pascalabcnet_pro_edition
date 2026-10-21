#pragma once
#include "runtime_values.hpp"

namespace abc {
// The execution owns nodes; links remain weak even when Pascal creates cycles.
// Retaining nodes until execution ends also preserves removed-node references.
struct LinkedRuntime {
    Values& values;
    std::vector<std::shared_ptr<Aggregate>> nodes;
    std::shared_ptr<Aggregate> node(Type type, const Value& value,
            std::shared_ptr<Aggregate> prev = {}, std::shared_ptr<Aggregate> next = {}) {
        auto result = std::make_shared<Aggregate>(); result->type = type;
        result->items.push_back(values.cell(type_info(values.program,type).element,values.copy(value)));
        result->links = std::make_unique<LinkedLinks>();
        if (prev && !prev->nil) result->links->prev = prev;
        if (next && !next->nil) result->links->next = next;
        nodes.push_back(result); return result;
    }
    std::shared_ptr<Aggregate> require(std::shared_ptr<Aggregate> value) {
        if (!value || value->nil) throw runtime_error(RuntimeError::nil_reference,values.location);
        return value;
    }
    void add(const std::shared_ptr<Aggregate>& list, const Value& value, bool front) {
        auto first = list->links->prev.lock(), last = list->links->next.lock();
        auto created = node(type_info(values.program,list->type).key,value,front ? nullptr : last,front ? first : nullptr);
        if (front) {
            if (first) first->links->prev = created;
            list->links->prev = created; if (!last) list->links->next = created;
        } else {
            if (last) last->links->next = created;
            list->links->next = created; if (!first) list->links->prev = created;
        }
    }
    void remove_end(const std::shared_ptr<Aggregate>& list, bool front) {
        auto node = (front ? list->links->prev : list->links->next).lock();
        if (!node) throw runtime_error(RuntimeError::linked_empty,values.location);
        if (front) {
            auto next = node->links->next.lock(); list->links->prev = next;
            if (next) next->links->prev.reset(); else list->links->next.reset();
        } else {
            auto prev = node->links->prev.lock(); list->links->next = prev;
            if (prev) prev->links->next.reset(); else list->links->prev.reset();
        }
    }
    void insert(const std::shared_ptr<Aggregate>& list, std::shared_ptr<Aggregate> position, const Value& value, bool before) {
        position = require(std::move(position));
        if (position == (before ? list->links->prev : list->links->next).lock()) { add(list,value,before); return; }
        auto prev = before ? position->links->prev.lock() : position;
        auto next = before ? position : position->links->next.lock();
        auto created = node(type_info(values.program,list->type).key,value,prev,next);
        if (before) { require(prev)->links->next = created; position->links->prev = created; }
        else { require(next)->links->prev = created; position->links->next = created; }
    }
    void remove(const std::shared_ptr<Aggregate>& list, std::shared_ptr<Aggregate> position) {
        position = require(std::move(position));
        if (position == list->links->prev.lock()) { remove_end(list,true); return; }
        if (position == list->links->next.lock()) { remove_end(list,false); return; }
        auto prev = position->links->prev.lock(), next = position->links->next.lock();
        require(prev)->links->next = next; require(next)->links->prev = prev;
    }
};
} // namespace abc

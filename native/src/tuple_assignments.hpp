#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace abc {
enum class TupleOperandKind { local, outer, by_reference, member, index, pointer, expression, constant };
struct TupleOperand {
    TupleOperandKind kind;
    std::string identity;
    std::string name;
};
enum class TupleOperandSource { target, value, temporary };
struct TupleOperandRef {
    TupleOperandSource source;
    std::size_t index;
};
struct TupleAssignment {
    TupleOperandRef target, value;
};
std::vector<TupleAssignment> order_tuple_assignments(const std::vector<TupleOperand>& targets,
    const std::vector<TupleOperand>& values);
}

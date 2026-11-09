#pragma once
#include "abc/core.hpp"
namespace abc {
// The reference's control block records array origin without changing Cell/Value.
struct ArrayCellReference {
    std::shared_ptr<Cell> retained;
    void operator()(Cell*) const noexcept {}
};
inline bool array_cell_reference(const std::shared_ptr<Cell>& cell){return std::get_deleter<ArrayCellReference>(cell)!=nullptr;}
inline std::shared_ptr<Cell> array_cell_reference_copy(const std::shared_ptr<Cell>& cell){
    return array_cell_reference(cell)?cell:std::shared_ptr<Cell>(cell.get(),ArrayCellReference{cell});
}
inline std::shared_ptr<Cell> descendant_reference(const Value& parent,const std::shared_ptr<Cell>& field){
    const auto* cell=std::get_if<std::shared_ptr<Cell>>(&parent);
    return cell&&array_cell_reference(*cell)?array_cell_reference_copy(field):field;
}
}

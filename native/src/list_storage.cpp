#include "runtime_values.hpp"
#include <array>
namespace abc {
std::shared_ptr<ListStorage> Values::list_backing(const std::shared_ptr<Aggregate>& list){
 if(!list || list->nil || type_info(program,list->type).kind!=CompoundKind::list || list->items.size()>list->list_capacity)throw runtime_error(RuntimeError::invalid_bytecode,location);
 if(!list->list_storage){auto storage=std::make_shared<ListStorage>();storage->cells.resize(list->list_capacity);for(std::size_t i=0;i<list->items.size();++i)storage->cells[i]=list->items[i];list->list_storage=std::move(storage);}
 return list->list_storage;
}
void Values::set_list_capacity(const std::shared_ptr<Aggregate>& list,std::size_t capacity){
 if(capacity<list->items.size())throw runtime_error(RuntimeError::list_capacity,location);
 if(capacity==list->list_capacity)return;
 if(capacity>INT32_MAX || capacity>list_storage_remaining)throw runtime_error(RuntimeError::allocation_limit,location);
 try{
  auto storage=std::make_shared<ListStorage>();storage->cells.resize(capacity);
  std::vector<std::shared_ptr<Cell>> active;active.reserve(list->items.size());
  const auto element=type_info(program,list->type).element;
  for(std::size_t i=0;i<list->items.size();++i){auto item=cell(element,copy(list->items[i]->value));own_list_cell(item,list);storage->cells[i]=item;active.push_back(std::move(item));}
  // A sort may still own the old array. Detached writes must not change List version.
  if(list->list_storage){for(const auto& item:list->list_storage->cells)if(item)own_list_cell(item,{});}
  else for(const auto& item:list->items)own_list_cell(item,{});
  list->items.swap(active);list->list_storage=std::move(storage);list->list_capacity=capacity;list_storage_remaining-=capacity;
 }catch(const std::bad_alloc&){throw runtime_error(RuntimeError::allocation_limit,location);}catch(const std::length_error&){throw runtime_error(RuntimeError::allocation_limit,location);}
}
void Values::list_append_cell(const std::shared_ptr<Aggregate>& list,std::shared_ptr<Cell> item){
 grow_list(list,list->items.size()+1);auto storage=list_backing(list);const auto index=list->items.size();
 if(storage->cells[index]){write(storage->cells[index],item->value,false);item=storage->cells[index];}else storage->cells[index]=item;
 own_list_cell(item,list);list->items.push_back(std::move(item));
}
void Values::list_insert_values(const std::shared_ptr<Aggregate>& list,std::size_t index,std::span<const Value> input){
 const auto count=list->items.size();if(index>count)throw runtime_error(RuntimeError::list_index,location);
 if(input.size()>static_cast<std::size_t>(INT32_MAX)-count)throw runtime_error(RuntimeError::allocation_limit,location);if(input.empty())return;
 std::vector<Value> incoming;incoming.reserve(input.size());for(const auto& value:input)incoming.push_back(copy(value));
 grow_list(list,count+input.size());auto storage=list_backing(list);const auto element=type_info(program,list->type).element;
 for(std::size_t i=count;i<count+input.size();++i){auto& item=storage->cells[i];if(!item)item=cell(element,zero(element,false,true));own_list_cell(item,list);list->items.push_back(item);}
 for(std::size_t i=count;i>index;--i)write(list->items[i-1+input.size()],list->items[i-1]->value,false);
 for(std::size_t i=0;i<incoming.size();++i)write(list->items[index+i],incoming[i],false);
}
void Values::list_erase(const std::shared_ptr<Aggregate>& list,std::size_t index,std::size_t count){
 const auto size=list->items.size();if(index>size || count>size-index)throw runtime_error(RuntimeError::list_range,location);if(!count)return;
 for(std::size_t i=index;i<size-count;++i)write(list->items[i],list->items[i+count]->value,false);
 const auto element=type_info(program,list->type).element;for(std::size_t i=size-count;i<size;++i)write(list->items[i],zero(element,false,true),false);
 list->items.resize(size-count);
}
}

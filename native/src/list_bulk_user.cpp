#include "list_bulk_user.hpp"
#include "runtime_values.hpp"
#include "collection_interface.hpp"
#include "interfaces.hpp"
#include <array>
namespace abc {
namespace {
void work(Values& values,const ExecutionOptions& options,std::uint64_t& used){
 if(used>=options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,values.location);
 if((used++&255U)==0&&options.cancelled&&options.cancelled())throw runtime_error(RuntimeError::cancelled,values.location);
}
std::shared_ptr<Cell> slot(Values& values,const std::shared_ptr<Aggregate>& list,const std::shared_ptr<ListStorage>& storage,std::size_t index){
 if(index>=storage->cells.size())throw runtime_error(RuntimeError::index_bounds,values.location);
 auto& result=storage->cells[index];
 if(!result){const auto element=type_info(values.program,list->type).element;result=values.cell(element,values.zero(element,false,true));values.own_list_cell(result,list);}
 return result;
}
}
bool list_user_bulk(Values& values,const std::shared_ptr<Aggregate>& target,const std::shared_ptr<Aggregate>& input,std::size_t position,const ExecutionOptions& options){
 const auto owner=sequence_collection_owner(values.program,input);if(owner==Type::invalid)return false;
 const auto receiver=input->sequence->source;
 const auto signed_count=std::get<std::int32_t>(interface_invoke(values,owner,0,receiver,{},options));
 if(signed_count<=0){++target->version;return true;}
 const auto count=static_cast<std::size_t>(signed_count);const auto size=target->items.size();
 if(size>INT32_MAX||count>static_cast<std::size_t>(INT32_MAX)-size||count>values.remaining)throw runtime_error(RuntimeError::allocation_limit,values.location);
 values.grow_list(target,size+count);auto storage=values.list_backing(target);std::uint64_t used=0;
 // Framework shifts the physical array first, retaining the old logical Count
 // and version while calling arbitrary user CopyTo code.
 for(std::size_t n=size;n>position;--n){work(values,options,used);values.write(slot(values,target,storage,n-1+count),slot(values,target,storage,n-1)->value,false);}
 const auto temporary=values.array(type_info(values.program,owner).key,count,true);
 const std::array<Value,2> arguments{temporary,std::int32_t{0}};
 (void)interface_invoke(values,owner,5,receiver,arguments,options);
 // CopyTo may replace the target's backing array through a capacity change.
 storage=values.list_backing(target);
 if(position>target->list_capacity||count>target->list_capacity-position)throw runtime_error(RuntimeError::list_copy_destination_range,values.location);
 for(std::size_t n=0;n<count;++n){work(values,options,used);values.write(slot(values,target,storage,position+n),temporary->items[n]->value,false);}
 const auto current=target->items.size();
 if(current>target->list_capacity||count>target->list_capacity-current)throw runtime_error(RuntimeError::index_bounds,values.location);
 for(std::size_t n=current;n<current+count;++n){work(values,options,used);target->items.push_back(slot(values,target,storage,n));}
 ++target->version;return true;
}
}

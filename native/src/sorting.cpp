#include "sorting.hpp"
#include "runtime_values.hpp"
#include "comparison.hpp"
#include "enumeration.hpp"
#include <array>
namespace abc {
namespace {
using Object=std::shared_ptr<Aggregate>;
bool is_nil(const Value& value){if(std::holds_alternative<NilString>(value))return true;if(const auto* p=std::get_if<Object>(&value))return !*p || (*p)->nil;return false;}
[[noreturn]] void bad_order(SourceLocation location){throw Error("E_COMPARER",location,"The comparison function produces inconsistent results","Функция сравнения возвращает несогласованные результаты");}
struct Entry{Value key,item;};
class Sorter {
 Values& values;const ExecutionOptions& options;Object array;Type element,key_type;
 Object callback;SortMethod method;bool specialized,ordinal;std::uint64_t work=0;std::vector<Value> keys;
 void step(){if(work++>=options.instruction_limit)throw runtime_error(RuntimeError::instruction_limit,values.location);if(options.cancelled && options.cancelled())throw runtime_error(RuntimeError::cancelled,values.location);}
 std::size_t checked(int i){step();if(i<0 || static_cast<std::size_t>(i)>=array->items.size())bad_order(values.location);return static_cast<std::size_t>(i);}
 Value key(int i){const auto n=checked(i);return method==SortMethod::key ? keys[n] : array->items[n]->value;}
 Entry hold(int i){const auto n=checked(i);return {values.copy(method==SortMethod::key ? keys[n] : array->items[n]->value),method==SortMethod::key ? values.copy(array->items[n]->value) : Value{std::int32_t{0}}};}
 void put(int i,const Entry& entry){const auto n=checked(i);if(method==SortMethod::key){keys[n]=values.copy(entry.key);values.write(array->items[n],entry.item);}else values.write(array->items[n],entry.key);}
 void move(int to,int from){auto entry=hold(from);put(to,entry);}
 void swap(int a,int b){step();if(a==b)return;auto first=hold(a),second=hold(b);put(a,second);put(b,first);}
 int compare(const Value& a,const Value& b){
  step();if(method==SortMethod::comparison || method==SortMethod::less){const std::array<Value,2> arguments{a,b};const auto result=values.invoke_value(callback,arguments);if(method==SortMethod::comparison)return std::get<std::int32_t>(result);if(std::get<bool>(result))return -1;const std::array<Value,2> reverse{b,a};return std::get<bool>(values.invoke_value(callback,reverse)) ? 1 : 0;}
  return comparison_value(values,key_type,a,b,!specialized,ordinal,options);
 }
 void ordered_pair(int a,int b){if(a==b)return;auto left=key(a);if(specialized && is_nil(left))return;if(compare(left,key(b))>0)swap(a,b);}
 void insertion(int first,int last){for(int i=first+1;i<=last;++i){auto saved=hold(i);int position=i-1;while(position>=first && ((specialized && is_nil(saved.key)) || compare(saved.key,key(position))<0)){move(position+1,position);--position;}put(position+1,saved);}}
 void sift(int root,int count,int base){
  auto saved=hold(base+root-1);
  while(root<=count/2){int child=root*2;auto left=key(base+child-1);if(child<count && ((specialized && is_nil(left)) || compare(left,key(base+child))<0))++child;
   auto candidate=key(base+child-1);if(specialized ? is_nil(candidate) || compare(candidate,saved.key)<0 : compare(saved.key,candidate)>=0)break;
   move(base+root-1,base+child-1);root=child;
  }put(base+root-1,saved);
 }
 void heap(int first,int last){int count=last-first+1;for(int parent=count/2;parent>0;--parent)sift(parent,count,first);for(int size=count;size>1;--size){swap(first,first+size-1);sift(1,size-1,first);}}
 int partition(int first,int last){
  int middle=first+(last-first)/2;ordered_pair(first,middle);ordered_pair(first,last);ordered_pair(middle,last);
  auto pivot=hold(middle).key;swap(middle,last-1);int left=first,right=last-1;
  while(left<right){
   if(specialized && is_nil(pivot)){while(left<last-1 && is_nil(key(++left))){}while(right>first && !is_nil(key(--right))){}}
   else if(specialized){while(compare(pivot,key(++left))>0){}while(compare(pivot,key(--right))<0){}}
   else{while(compare(key(++left),pivot)<0){}while(compare(pivot,key(--right))<0){}}
   if(left>=right)break;swap(left,right);
  }swap(left,last-1);return left;
 }
 void intro(int first,int last,int depth){while(first<last){step();const auto size=last-first+1;if(size<=16){if(size==2)ordered_pair(first,last);else if(size==3){ordered_pair(first,last-1);ordered_pair(first,last);ordered_pair(last-1,last);}else insertion(first,last);return;}if(depth==0){heap(first,last);return;}--depth;const auto pivot=partition(first,last);intro(pivot+1,last,depth);last=pivot-1;}}
public:
 Sorter(Values& v,const ExecutionOptions& o,Object target,Type type,std::size_t code,Object function):values(v),options(o),array(std::move(target)),element(type_info(v.program,type).element),key_type(sort_method(code)==SortMethod::key ? type_info(v.program,sort_callback(code)).element : element),callback(std::move(function)),method(sort_method(code)),specialized((method==SortMethod::default_order || method==SortMethod::key) && default_comparable(v.program.types,key_type) && !enumeration(v.program,key_type)),ordinal(method==SortMethod::ordinal_order || (method==SortMethod::key && key_type==Type::string && !(code&sort_generic_key))){}
 void extract(){
  if(method!=SortMethod::key)return;const auto size=array->items.size();if(size>values.remaining)throw runtime_error(RuntimeError::allocation_limit,values.location);values.remaining-=size;keys.reserve(size);
  for(std::size_t i=0;i<size;++i){step();const std::array<Value,1> argument{array->items[i]->value};keys.push_back(values.copy(values.invoke_value(callback,argument)));}
  if(ordinal)specialized=false;
 }
 void run(){const auto size=array->items.size();if(size>INT32_MAX)throw runtime_error(RuntimeError::allocation_limit,values.location);if(size<2)return;int depth=0;for(auto n=size;n;n/=2)++depth;intro(0,static_cast<int>(size)-1,2*depth);}
 void reverse(){for(std::size_t i=0,n=array->items.size();i<n/2;++i)swap(static_cast<int>(i),static_cast<int>(n-1-i));}
};
}
bool sort_signature(const Program& program,Type owner,std::size_t code){
 if((code>>8U)>64U+program.types.size() || !compound(owner) || !valid_value_type(program,owner))return false;const auto& array=type_info(program,owner);if(array.kind!=CompoundKind::array || array.rank!=1 || (code&128U) || static_cast<unsigned>(sort_method(code))>static_cast<unsigned>(SortMethod::key))return false;
 const auto method=sort_method(code);const auto callback=sort_callback(code);
 if((code&sort_generic_key) && method!=SortMethod::key)return false;
 if(method==SortMethod::default_order || method==SortMethod::ordinal_order)return callback==Type::invalid && (method!=SortMethod::ordinal_order || array.element==Type::string);
 if(!compound(callback) || !valid_value_type(program,callback))return false;const auto& shape=type_info(program,callback);
 if(shape.kind!=CompoundKind::callable || shape.parameters!=std::vector<CallableParameter>(method==SortMethod::key ? 1U : 2U,{array.element,false,false}))return false;
 return method==SortMethod::key ? valid_value_type(program,shape.element) : shape.element==(method==SortMethod::less ? Type::boolean : Type::integer);
}
void sort_array(Values& values,Type owner,std::size_t code,const Value& source,const Value* callback,const ExecutionOptions& options){
 if(!sort_signature(values.program,owner,code))throw runtime_error(RuntimeError::invalid_bytecode,values.location);
 const auto array=std::get<Object>(source);Object function;
 const auto check_callback=[&]{if(callback){function=std::get<Object>(*callback);(void)values.callable_target(function);}};
 if(code&sort_extension)check_callback();
 if(!array || array->nil)throw runtime_error(RuntimeError::nil_reference,values.location);
 if(!(code&sort_extension))check_callback();
 Sorter sorter(values,options,array,owner,code,function);sorter.extract();
 try{sorter.run();}catch(const Error& failure){if(failure.code=="E_LIMIT" || failure.code=="E_CANCELLED" || failure.code=="E_COMPARER")throw;if(failure.code=="E_BOUNDS")bad_order(values.location);throw Error("E_SORT",values.location,std::string("Failed to compare two elements in the array: ")+failure.what(),std::string("Не удалось сравнить два элемента массива: ")+failure.russian);}
 if(code&sort_descending)sorter.reverse();
}
}

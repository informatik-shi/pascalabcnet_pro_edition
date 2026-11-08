#pragma once
#include "sequences.hpp"
#include <exception>
#include <optional>
#include <type_traits>
#include <utility>
namespace abc {
// Explicit finally scopes may propagate Dispose errors; shared_ptr destructors cannot.
void sequence_finish(Values&,const std::shared_ptr<Aggregate>&,const ExecutionOptions&,std::exception_ptr);
void enumerator_finish(Values&,Type,const Value&,const ExecutionOptions&,std::exception_ptr);
template<class F> auto enumerator_finally(Values& values,Type type,const Value& iterator,const ExecutionOptions& options,F&& body)->std::invoke_result_t<F> {
    using Result=std::invoke_result_t<F>;static_assert(!std::is_void_v<Result> && !std::is_reference_v<Result>);
    std::optional<Result> result;std::exception_ptr failure;
    try{result.emplace(std::forward<F>(body)());}catch(...){failure=std::current_exception();}
    enumerator_finish(values,type,iterator,options,failure);return std::move(*result);
}
template<class F> auto sequence_finally(Values& values,const std::shared_ptr<Aggregate>& cursor,const ExecutionOptions& options,F&& body)->std::invoke_result_t<F> {
    using Result=std::invoke_result_t<F>;
    static_assert(!std::is_reference_v<Result>);
    std::exception_ptr failure;
    if constexpr(std::is_void_v<Result>){
        try{std::forward<F>(body)();}catch(...){failure=std::current_exception();}
        sequence_finish(values,cursor,options,failure);
    }else{
        std::optional<Result> result;
        try{result.emplace(std::forward<F>(body)());}catch(...){failure=std::current_exception();}
        sequence_finish(values,cursor,options,failure);
        return std::move(*result);
    }
}
}

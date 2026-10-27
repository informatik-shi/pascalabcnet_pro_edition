#pragma once
#include "abc/core.hpp"
#include "diagnostics.hpp"
#include <new>
#include <stdexcept>

namespace abc {
template<class F> auto checked_big(SourceLocation location,F action) {
    try { return action(); }
    catch (const std::length_error&) { throw runtime_error(RuntimeError::big_integer_limit,location); }
    catch (const std::overflow_error&) { throw runtime_error(RuntimeError::integer_overflow,location); }
    catch (const std::invalid_argument&) { throw runtime_error(RuntimeError::number_text,location,"BigInteger"); }
    catch (const std::domain_error&) { throw runtime_error(RuntimeError::math_domain,location); }
    catch (const std::bad_alloc&) { throw runtime_error(RuntimeError::allocation_limit,location); }
}
Value big_cast(const Value& value,Type target,SourceLocation location);
BigInteger parse_big(std::u16string_view text,SourceLocation location,const ExecutionOptions* options=nullptr);
BigInteger big_binary(Op operation,const BigInteger& a,const BigInteger& b,SourceLocation location,const ExecutionOptions& options);
BigInteger big_unary(Op operation,const BigInteger& value,SourceLocation location,const ExecutionOptions& options);
BigInteger big_shift(Op operation,const BigInteger& value,std::int32_t count,SourceLocation location,const ExecutionOptions& options);
void big_work(std::uint64_t work,SourceLocation location,const ExecutionOptions& options);
} // namespace abc

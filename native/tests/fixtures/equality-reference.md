# Independent closed equality and hash reference

[Generator](../reference/generate-equality.ps1) compiles
[equality.cs](../reference/equality.cs) with the installed Framework C# compiler
and System.Numerics into ignored native/build. The process targets Framework
4.7.2 and fixes CurrentCulture/CurrentUICulture to en-US. No original Pascal
compiler is run. Managed code, binaries and generators are not installed.

The scalar fixture contains 942 typed pairs, comparing Equals, both GetHashCode
results and concrete default implementation kind. It covers all native integral
widths, char, boolean, UTF16 strings (including nil, embedded zero and unpaired
surrogates), BigInteger and int32 enums. IEEE inputs retain exact binary32/
binary64 bits for subnormals, infinities, signed zeros and quiet/signaling NaNs.
Scalar tests invoke the closed native equality ABI with compiler-verified
metadata, without claiming source BitConverter or nonfinite literals support.

The class fixture contains 1125 independent executions: nine scenarios × five
first operands × five second operands × five modes. Scenarios cover default
IEquatable class, user/named-inherited IEqualityComparer, non-equatable class,
wrong closed IEquatable contract, declared interface default equality, direct
IEquatable call, array identity and ordinary value-record fields. Modes cover
normal results, false user equality, arithmetic faults, nil comparer/receiver
and mutation of the first operand from callbacks. Logs retain exact call/hash
order, nil skips, same-reference calls, return values and observable operand
state. Class/record identity hashes are compared for same-value stability and
nil behavior, rather than process-dependent numeric values.

The native [class program](equality-classes-program.pas) is compiled only by the
native compiler. It uses checked interface-to-concrete-class casts and existing
Pascal Pos for implementation-name serialization. Additional tests cover exact
static signatures, record parameter copies, typed nils, malformed metadata,
class cast identity/nil/failure and native work/cancellation/recursion limits.

| Fixture | SHA-256 after CRLF-to-LF normalization |
| --- | --- |
| equality-reference.tsv | e2e52486a3d4ebe5863208f3e6d45261bbdc3923d27c8a95bde303bb75548385 |
| equality-classes-reference.tsv | 3d5829e89b68da279bd4d311b1c51462d18f9367f7a7b34dca45e4950aa38d5d |

Primary contracts:
[EqualityComparer](https://github.com/microsoft/referencesource/blob/main/mscorlib/system/collections/generic/equalitycomparer.cs),
[String](https://github.com/microsoft/referencesource/blob/main/mscorlib/system/string.cs),
[Single](https://github.com/microsoft/referencesource/blob/main/mscorlib/system/single.cs).

Collection comparer constructors/properties/policies are not connected yet.
Object virtual overrides, boxing/variance, record interface methods and general
class inheritance remain pending. Native record/tuple/object hash layouts are
implementation-dependent. This foundation does not complete a library unit.

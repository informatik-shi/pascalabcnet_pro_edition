# RealRange equality and boxing

The development-only reference uses an actual Framework struct implementing
`IEquatable<R>` and the equality/operator/hash recipes of RealRange in
`bin/Lib/PABCSystem.pas:939-975`. It executes 900 outcomes on Windows .NET
Framework 4.0.30319.42000. The original Pascal compiler is never invoked.

Fifteen bound pairs cover empty/nonempty intervals, signed zero, the smallest
positive subnormal, maximum finite doubles, infinities and distinct NaN bit
patterns. Every pair of intervals is tested through four nil-reference routes.
The corpus checks operator/value equality, separate boxes versus aliases,
default interface comparers (including a box compared with itself), tuples,
List.Contains, HashSet counts, interface invocation and checked unboxing.

RealRange's Equals uses its IEEE comparison operator: a NaN field makes value
equality false even for the same box. Reference operator equality still treats
an alias as the same reference. This also applies when RealRange is nested in
a tuple or stored through an interface, so generic structural Double.Equals
cannot replace the record's object equality override.

The C++ runtime consumes all 900 outcomes with the exact double bit patterns
from the TSV. The finite subset additionally executes 256 typed Pascal
program cases. Nonfinite source constants remain unsupported; these runtime
tests do not claim support for them. The native test contains 1746 checks,
including actual type names, copy lifetimes, static rejection before output,
bilingual nil/bad casts, instruction limits and malformed metadata.

Run `../reference/generate-real-range-interface.ps1` to regenerate the TSV and
four normalized-LF hashes. Columns are first interval index, second interval
index, nil mask, low/high bits of the first and second intervals, hexadecimal
UTF-16 stdout and error code. Bit 0 clears the first box; bit 1 clears the
second. The reference tools and managed runtime are not installed. General
user-record methods/interfaces, Object boxing, source units/generics and the
remaining standard library are unfinished.

# Lazy set queries and equality policies

Three independent Framework CLR 4.0.30319.42000 corpora contain 34280 executions:

| Corpus | Rows | SHA-256 of LF-normalized text |
| --- | ---: | --- |
| sequence-sets-reference.tsv | 31960 | 65efadbf780ebc2f0da433afe71b5ef6d4e977eddca8877ec2fe31ab4582497d |
| sequence-set-classes-reference.tsv | 2080 | b7c8a3fa8a4d9631f1f9def53de2ae80752b3e6ad831e7a2f038633cd2f0edca |
| sequence-set-scalars-reference.tsv | 240 | cc0005ca9d703754d70992792c253e0db364dd62766feb6fb11346c5223eca3e |

Generate using [generate-sequence-sets.ps1](../reference/generate-sequence-sets.ps1),
[generate-sequence-set-classes.ps1](../reference/generate-sequence-set-classes.ps1)
and [generate-sequence-set-scalars.ps1](../reference/generate-sequence-set-scalars.ps1).
These compile independent C# programs using the installed Framework64 compiler;
no managed source, executable or runtime is installed in the application. The
original Pascal compiler is never run. The first two corpora compile paired
Pascal programs once and compare exact native traces and error codes per row.
The scalar corpus injects exact IEEE input bits through compiler-verified native
array/sequence types and compares every output bit; it does not claim source
BitConverter or nonfinite literal/arithmetic support.

The integer corpus covers all four operators, typed nil/empty/short/long sources,
arrays, List, user IEnumerable, Select, Where, HashSet, IEnumerable-only NewSet
and Dictionary.Values. Policies include missing, explicit nil, explicit Default,
modular equality, negative constant collision hashes and identity integer hashes.
Consumers include full traversal, Count, Any, Take(1), manual Current/MoveNext,
Dispose before first use and repeated Dispose, Reset failure, two independent
cursors and replay after policy/source effects. User GetEnumerator/MoveNext/Current/
Dispose and policy Equals/GetHashCode calls are logged exactly, including finite
faults, source grow/clear/replace, global source reassignment, callback reentry and
disposal through Select/Where, including predicates that reject initial elements.

The class corpus distinguishes declared Item from IEquatable<Item>, nil operands,
custom nil equality, hashes that collide with nil, and actual same-reference
IEquatable calls that return false. It intentionally uses only one nonnil object
per case; no expected trace depends on platform-specific object hash values or
collisions between unrelated objects. The scalar corpus covers empty/reversed/
repeated sources, signed zeros, infinities and canonical/payload/signaling NaNs.
Legacy raw IEEE hash behavior can retain different NaN payloads as distinct even
when value equality treats them as equal.

Source contracts are [Framework Enumerable](https://github.com/microsoft/referencesource/blob/main/System.Core/System/Linq/Enumerable.cs)
around Distinct/Union/Intersect/Except at lines 824–906, internal Set at 2380,
and unary iterator classes near the start of that inspected file. Validation of
source nil is eager, while internal set construction and enumeration are deferred.
Union reads first then second; Intersect/Except read all of second before first.
Internal Set starts with seven slots and grows by count*2+1, preserving cached
31-bit hashes and nil hash zero without calling a user hash for nil.

Generated query Dispose before first MoveNext is a no-op. During MoveNext,
reentrant MoveNext returns false, while Dispose can invoke an active yield-scope
source Dispose without aborting the executing step. A subsequent yield rearms
the scope. Fault cleanup must not repeat a disarmed finally, but ordinary loop
completion still closes its source. The non-yield preload loop of Intersect/
Except has its own ordinary finally and is not closed by an external iterator
Dispose while preloading. Query Current retains the last yield after completion.
Native producer leases and Current snapshots keep a disposing Select/Where
callback from invalidating the executing C++ state. Array/list traversal and
cleared enumerable fields preserve their distinct iterator behavior.

Fault observers use the implemented Dispose protocol rather than general Pascal
try/finally, with explicit cleanup reserves of 100000 instructions and 65536 cells.
Application defaults remain 10000 and 4096 with the existing shared deadline and
native stack bounds. Additional host checks cover value-record argument copies,
named interfaces, known-policy inference for nil sources, wrong types, forged
method/policy/owner/stack metadata, work/cancellation, infinite Dispose, Halt and
100 nested Distinct operations on the ordinary native stack.

General source generics/method values/unit loading, inheritance/Object virtual
methods, record interfaces, boxing/variance, full reflection and nongeneric
protocols, StringComparer and remaining equality-aware APIs are still required.
These corpora do not complete those features, a source unit or the standard library.

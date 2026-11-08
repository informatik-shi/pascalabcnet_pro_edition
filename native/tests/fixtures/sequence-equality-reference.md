# Typed SequenceEqual and nested interface enumeration

The independent Framework CLR 4.0.30319.42000 corpus contains 22616 executions.
LF-normalized SHA-256: 0f3cfbe3e9f2307f5f3dd74a65784a100003b5a131d1406cee9adb949a1db2a3.

[generate-sequence-equality.ps1](../reference/generate-sequence-equality.ps1)
compiles the independent [C# oracle](../reference/sequence-equality.cs) with the
installed Framework64 compiler and generates sequence-equality-reference.tsv.
These are development tools; no managed reference code or runtime is installed.
The original Pascal compiler is never run. The native test compiles the paired
[Pascal program](sequence-equality-program.pas) once and compares exact output
and native error code for every row.

The first twelve columns are left/right provider, left/right length, value shape,
policy, callback site, mutation/fault action, trigger invocation, protocol flags,
global/member call style and same-source alias. The last columns contain the
exact call/result/final-state trace and error code. Length -1 represents a typed
nil source. Providers cover arrays, List, user IEnumerable, Select, Where,
HashSet, IEnumerable-only NewSet and Dictionary.Values. Values include distinct
IEquatable<Item> objects with equal data, unequal data, identical object references
and nil elements. Policies are omitted, explicit nil, custom modular equality,
custom always-false equality and explicit EqualityComparer<Item>.Default.

Selected finite callbacks run during GetEnumerator/MoveNext/Current/Dispose,
Select/Where evaluation or Equals. They throw division/bounds errors, grow/clear/
replace a collection, mutate objects or reassign the global source. Stored HashSet
policy calls and unexpected equality hashing are marked in the exact trace.
Queries must use declared element equality rather than the collection policy.

Source factories log left, right and policy evaluation before validation. The
corpus checks asymmetric exhaustion and stops, side effects even for equal source
references, and two sources returning nil or the same actual IEnumerator instance.
Nested scopes dispose the right then left enumerator; both Dispose methods may
throw, and the later failure must win. The same shared enumerator is disposed
once per nonnil using scope. User Current is called only after both MoveNext
results succeed. List/HashSet/Dictionary Current uses the value captured earlier
by MoveNext, whereas array Current observes a subsequent physical cell write.

The native host also checks static interface equality (without inferring the
runtime class), record copies and contained reference aliasing, named inherited
interfaces, empty ranges and string adapters, corrupt opcode metadata, work and
cancellation bounds, infinite Dispose under the bounded cleanup reserve, and
Halt skipping user Dispose while retaining its exit code. Fault state observers
use the implemented Dispose protocol, not general source try/finally. They
explicitly request 100000 cleanup instructions and 65536 cells; application
defaults remain 10000 and 4096 with the existing shared deadline and stack bounds.

The primary contract is [Framework Enumerable.SequenceEqual](https://github.com/microsoft/referencesource/blob/main/System.Core/System/Linq/Enumerable.cs)
around line 916 of the inspected source: acquire both enumerators, move left,
move right, read both Current values, compare, and move right once after left
exhaustion. This version has no ICollection count or same-reference shortcut.
The oracle exercises actual installed Framework behavior rather than assuming
newer runtime optimizations apply.

General Pascal try/finally, source generic method inference and builtin function
values, Object virtual overrides and inheritance, record interface methods,
boxing/variance, nongeneric protocols, StringComparer and other equality-aware
queries remain required. This profile does not complete those language features,
collection APIs or the standard library.

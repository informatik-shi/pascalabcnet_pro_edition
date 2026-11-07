# Dictionary equality policies and declared value equality

Two independent Framework CLR 4.0.30319.42000 corpora contain 13980 executions:

- dictionary-comparers-reference.tsv: 9040 integer key/value executions; normalized
  LF SHA-256 535761b1e7cad310986f9f9b43d2cd5500ad1641f6c1e6eabbb34a7b6700ce77.
- dictionary-value-equality-reference.tsv: 4940 string-key/user-value executions;
  normalized LF SHA-256 e4ef47d9e2af94c7cea296e04ba366932f4f082586f51d11d83bf3a22d506249.

Generate with [generate-dictionary-comparers.ps1](../reference/generate-dictionary-comparers.ps1)
and [generate-dictionary-value-equality.ps1](../reference/generate-dictionary-value-equality.ps1).
They compile the paired independent C# programs with the installed Framework64
compiler; neither reference executable nor managed source is installed. The original
Pascal compiler is never run. The native test compiles each checked-in Pascal
program once and executes it for every row, comparing exact output and error codes.

The integer corpus covers Default and user comparers, negative constant hashes,
modular equivalence, fresh/initialized/cleared/all-removed/sparse/nil dictionaries,
copy and capacity constructors, retained Comparer identity, PABCSystem Copy,
Default constructor/operator copies, lookup, insertion, overwrite, duplicate keys,
Get/GetOrAdd including factory effects, Update/merge/removal, Keys/Values views,
CopyTo, membership and old enumerators. Selected finite hash/equality/factory
callbacks throw, grow, clear, remove, overwrite or replace/nil the external variable.

The value corpus uses actual IEquatable<Item> and key IEqualityComparer<string>
classes. It verifies nil key rejection before hash calls, nil value short paths,
actual same-reference Equals calls, declared Item versus IEquatable<Item> default
selection, call order, value-object mutation, source growth/clear/removal/replacement,
copied key/value snapshots and old cursor state. ContainsValue and Values.Contains
scan live physical entries without introducing enumeration version checks.

Source-level fault observers use the implemented IEnumerator.Dispose protocol;
this does not implement general Pascal try/finally. The test host requests bounded
cleanup reserves of 100000 instructions and 65536 cells for detailed final-state
observers. Application defaults remain 10000 and 4096, with their existing shared
cleanup deadline and native stack bounds. Arbitrary legacy corrupted-chain loops
are not treated as a compatibility guarantee; the selected reference cases terminate.

Primary source contracts:
[Framework Dictionary](https://github.com/microsoft/referencesource/blob/main/mscorlib/system/collections/generic/dictionary.cs),
local PABCSystem.pas Copy at line 10266 and Get/GetOrAdd/Update/operators around
16348–16455. Copy(nil) returns nil and nonnil Copy explicitly passes d.Comparer;
ordinary copy constructors and Dict/operator copies use Default. Dictionary copy
always inserts entries and rehashes, even for the same comparer. Each constructor
argument is evaluated before collection/capacity validation.

Additional native checks cover exact type rejection, inherited named interfaces,
record argument/value copies, malformed bytecode, cancellation/work limits,
argument order and recoverable entry allocation failures. This profile does not
claim IDictionary/nongeneric/serialization interfaces, general class inheritance,
Object.Equals/GetHashCode overrides, record interface methods, boxing/variance,
StringComparer, remaining comparer-aware queries or complete library migration.

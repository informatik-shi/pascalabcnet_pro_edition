# IntRange/CharRange interfaces and boxing

The independent development-only C# reference implements the recipes from
`bin/Lib/PABCSystem.pas:803-942` as actual Framework value types implementing
`ICollection<T>` and `IEquatable<RangeValue<T>>`. It executes 2688 cases on
Windows .NET Framework 4.0.30319.42000. It does not run the original Pascal
compiler and is never installed in the native application.

The matrix covers integer and UTF-16 character bounds, empty/nonempty ranges,
Count, IsReadOnly, Contains, CopyTo, enumeration, forbidden mutations, nil
receivers/arrays, separate boxes versus copied references, unboxing and
cross-interface casts. Standard equality comparers, List.Contains, HashSet,
Enumerable.Count and ToArray test consumer behavior. CopyTo uses the shipped
truncating recipe and unchecked int32 arithmetic, including extreme offsets.

Interface assignment retains a box; converting a record value creates a fresh
box, and unboxing copies the value. Operator equality on interface references
compares box identity, whereas the generic default equality comparer uses the
range's value equality and hash. IEnumerable adapters retain the identity and
runtime type of their boxed source. Heterogeneous collection values must not
compare equal to a range with the same apparent element list.

Run `../reference/generate-range-interface.ps1` to regenerate both typed Pascal
fixtures, the TSV and six normalized-LF artifact hashes. TSV columns are kind,
low, high, operation, item, offset, array length, nil receiver, nil array,
hexadecimal UTF-16 stdout and error code. Kind 0 is IntRange; kind 1 is CharRange.
The native test also checks static rejection before output, nil/bad unboxing,
runtime names, box copy isolation, cancellation and malformed metadata.

This is selected native range compatibility, not general user-record methods,
record interface implementations, source generics, Object boxing or completion
of PABCSystem or the standard library. It adds a flag only to the heap aggregate;
TypeInfo, Cell, Value and VM frame layouts and the normal Windows stack limit
remain unchanged.

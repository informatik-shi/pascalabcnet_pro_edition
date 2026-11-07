# Independent List sorting and physical backing reference

[Generator](../reference/generate-list-sorting.ps1) compiles
[list-sorting.cs](../reference/list-sorting.cs) using the installed Framework C#
compiler into ignored native/build, then runs a dedicated .NETFramework 4.7.2
executable. It asserts the Framework 4.5+ sort compatibility switch and fixes
CurrentCulture/CurrentUICulture to en-US. No original Pascal compiler is run.
Neither managed code, generated binaries nor generators are installed.

The fixture contains 14231 independent executions:
- 14076 combinations: 17 overload scenarios × 9 sizes × 2 capacities × 23 modes
  × 2 nil patterns.
- Four 1024-element organ-pipe cases prove actual Framework DownHeap/Heapsort
  fallback frames for ordinary/default and paired class-key sorting, including
  capacity larger than logical Count.
- 150 range validation cases cover negative/empty/overlong/Int32.MaxValue
  index/count values with default and user comparers.
- One physical record-field borrow case retains a reference into the old private
  backing array across Capacity/TrimExcess, proving value copies, detached slots
  and unchanged enumerator version independently of the native implementation.

TSV: scenario, size, extra capacity/range index, mode/range count, nil pattern,
exact output, mapped error. UTF-8 LF SHA-256: 8488ef65a4f333c55f21ae02e07c1b729b8401fd9d519579824ffc95558e7130.

The native harness compiles list-sorting-program.pas once and runs all rows.
Journals cover real CompareTo/delegate/less/IComparer calls, key evaluations,
actual List and alias contents, reference identity and old IEnumerator state.
A one-item user enumerable supplies a real finally/Dispose observer, including
failure replacement during unwinding. The source runtime remains authoritative
for comparison order, backing-array contents and version rules.

Modes replace/swap entries, mutate contained objects, add within/growing capacity,
change Capacity, clear/remove/insert/range-copy, insert into Self-related data,
trim, replace the global List variable (including nil), sort recursively,
remove/add reused slots, clear/add and add empty array/general enumerable sources.
The empty enumerator observes the old cursor during Dispose, before InsertRange's
final version increment. Key sorting uses a snapshot and replaces the var List
after success. Descending sorting keeps a value-captured receiver for ordinary
SortDescending, and a var receiver for key SortDescending.

Framework List.Sort(Comparison<T>) does not increment version; default,
IComparer and range sorts do so only after success, including empty ranges.
SortDescending then reverses the current backing/count of its captured List.
Capacity changes preserve version but replace physical slots; original cells
remain available to a sort already holding the former array. Empty AddRange/
InsertRange still perform the final version change. Native artificial resource/
cancellation failures retain their own category rather than being wrapped.

Additional native tests verify record copies, nil handlers, strict signatures,
assignable var targets, qualified generic forms, contextual key conversions,
malformed bytecode, normal cancellation/instruction limits and recursive comparer
limits. The old source-only generic slot/vector model is not used for List storage.

Primary source contracts:
[Framework List](https://github.com/microsoft/referencesource/blob/main/mscorlib/system/collections/generic/list.cs),
[Framework ArraySortHelper](https://github.com/microsoft/referencesource/blob/main/mscorlib/system/collections/generic/arraysorthelper.cs).
Pascal wrappers are the supplied bin/Lib/PABCSystem.pas Sort/SortDescending bodies.

This prerequisite does not complete List/PABCSystem/Collections or bin/Lib.
Public paired System.Array key/item APIs, custom equality comparers, nongeneric protocols,
variance, general source-unit loading and remaining source modules are pending.

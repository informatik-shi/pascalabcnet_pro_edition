# Independent static System.Array sorting reference

[Generator](../reference/generate-array-sorting.ps1) compiles
[array-sorting.cs](../reference/array-sorting.cs) into ignored native/build using
the installed Framework C# compiler. A dedicated Framework 4.7.2 target asserts
the Framework 4.5+ introsort compatibility switch and fixes CurrentCulture and
CurrentUICulture to en-US. No original Pascal compiler is run. Managed probes,
reference binaries and generators are development-only and never installed.

The fixture contains 3791 executions:

- 3564 combinations: 11 static overload scenarios × 9 sizes × 2 seeds × 9 modes
  × 2 nil patterns. Sizes 0/1/2/3/4/8/16/17/32 cover short/insertion/introsort
  paths. Scenarios include default, Comparison, recognized native Default,
  Create, user/inherited IComparer, range default/nil/user comparer and typed
  nil comparer/comparison.
- 225 boundary cases cover three range forms, sizes 0/1/3 and index/count
  -1/0/1/3/Int32.MaxValue.
- Two 1024-element organ-pipe cases assert actual Framework DownHeap/Heapsort
  frames for default and user-comparer range sorting.

[The Pascal program](array-sorting-program.pas) is compiled only by the native
implementation. Each row compares its complete call/mutation/partial-array
trace and terminal diagnostic category against the independent Framework run.
Callback modes cover reverse order, arithmetic/index failures, inconsistent
comparison, array-cell swaps/replacement, contained-object mutation and
replacement of the external array variable while the original array is sorted.

Fixture SHA-256 after normalizing CRLF to LF:
cbdc45474e87002c0cf67f14291211d7b7b3aff9d5189d35ebbe2de22cb72343.

Additional native checks cover explicit element types, record copies, culture,
local/member shadows, argument evaluation and captured-array identity, typed
overload rejection, malformed metadata, cancellation/instruction/recursion limits
and bilingual errors. Limits/cancellation remain native diagnostic categories.

Primary source contracts: Framework reference-source
[Array](https://github.com/microsoft/referencesource/blob/main/mscorlib/system/array.cs)
and [ArraySortHelper](https://github.com/microsoft/referencesource/blob/main/mscorlib/system/collections/generic/arraysorthelper.cs).
Paired public key/item Array.Sort is now covered by paired-sorting-reference.md.
Nongeneric/boxed Array APIs and full library migration remain pending. No bin/Lib unit is marked complete by this step.

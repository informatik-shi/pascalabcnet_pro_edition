# Independent paired Array.Sort and primitive CLR sorting reference

[Generator](../reference/generate-paired-sorting.ps1) compiles
[paired-sorting.cs](../reference/paired-sorting.cs) into ignored native/build,
using the installed Framework C# compiler. The process targets Framework 4.7.2,
asserts Framework 4.5+ introsort compatibility and fixes both culture settings
to en-US. No original Pascal compiler is run. Managed probes, generators and
reference binaries are never installed in the portable application.

Four fixtures contain 14805 independent executions:

- paired-sorting-reference.tsv: 13101 class executions. 11520 combinations cover
  eight overload forms × eight sizes × six item-array kinds × fifteen callback
  modes × two nil patterns. Another 1575 cover range validation: three range
  forms × sizes 0/1/3 × seven item-array kinds × five index/count boundaries.
  Six 1024-element organ-pipe cases include four independently confirmed
  Framework DownHeap/Heapsort fallbacks and two same-array alias cases.
- paired-primitive-reference.tsv: 846 executions: seven overload forms, ten
  sizes, six integral/string item-array kinds and two seeds, plus six large
  organ-pipe cases. These independently exercise CLR native fast sorting versus
  managed generic helpers and same-array alias behavior.
- paired-floating-reference.tsv: 704 executions: two IEEE precisions × eight
  array/pair/List forms × eleven sizes × four seeds. Keys and payloads are
  compared as exact binary32/binary64 bits, including distinct quiet/signaling
  NaN payloads, signed zeros, infinities, nil items and same-array aliases.
- paired-enum-reference.tsv: 154 executions: seven nominal-enum array/pair/List
  forms × eleven sizes × two seeds. Enum/enum versus enum/integer payloads
  distinguish native primitive dispatch from managed non-generic comparison.

Class modes include reverse order, arithmetic/index faults, inconsistent
comparison, key-cell swaps/replacement, contained-object mutation, replacement
of either external array variable and nil reassignment. Every class execution
observes complete current/captured arrays and identity after success or failure.
Nil, shorter, longer, shared-array and shared-contained-object item sources are
included. Selected ranges must fit both nonnil arrays; nil keys take priority.

The native source programs use existing IEnumerator.Dispose cleanup for final
state observation, corresponding to the C# probe's finally. General source
try/finally remains pending. Large fault observers explicitly request bounded
cleanup reserves of 1000000 instructions and 262144 cells/list positions;
production defaults remain 10000/4096 with the same shared one-second deadline.
Native tests verify both default exhaustion and configured successful cleanup.

Floating tests call the typed native sorting ABI with compiler-verified closed
metadata and inspect values directly. They do not claim source BitConverter or
nonfinite literal construction support. Additional tests cover record copying,
explicit/inferred types, nil overloads, argument evaluation order, corrupted
metadata, bilingual errors and execution/cancellation/native recursion limits.

SHA-256 with CRLF normalized to LF:

| Fixture | SHA-256 |
| --- | --- |
| paired-sorting-reference.tsv | 61f588adf858ab5344d1bea19508e77e09f004ff63beea61852ac2cfce249676 |
| paired-primitive-reference.tsv | a9f15bcfdef4c8bb34f2fc48b38d5f88aac8dd103dde35c3285be99c421369e1 |
| paired-floating-reference.tsv | 78b1e906e771a5c225feb345ed00f36c84a3f0d27ef2e1925d8e781f12537bdc |
| paired-enum-reference.tsv | 5365228c913104b7631b99f88ef757dbe20d7cfc0e7d5fd8cbeb94b21fbe87d2 |

Primary contracts are Microsoft reference-source
[Array](https://github.com/microsoft/referencesource/blob/main/mscorlib/system/array.cs),
[paired ArraySortHelper](https://github.com/microsoft/referencesource/blob/main/mscorlib/system/collections/generic/arraysorthelper.cs),
and archived CLR native
[dispatch](https://github.com/dotnet/coreclr/blob/release/1.0.0/src/classlibnative/bcltype/arrayhelpers.cpp)/
[sort helper](https://github.com/dotnet/coreclr/blob/release/1.0.0/src/classlibnative/bcltype/arrayhelpers.h).

StringComparer/equality comparers, nongeneric/boxed Array contracts, variance,
source exception statements and remaining modules still need migration.
This phase does not complete any bin/Lib source unit or the library inventory.

# Independent array sorting reference

[sorting.cs](../reference/sorting.cs) calls Framework Array.Sort/ConvertAll/Reverse
directly. [generate-sorting.ps1](../reference/generate-sorting.ps1) builds and runs
a development-only C# executable under native/build, never the original Pascal
compiler. Managed code and scripts are not installed in the native application.

The executable declares .NETFramework,Version=v4.7.2 and explicitly verifies the
Framework 4.5+ sort compatibility switch. This matches the TargetFramework
attribute emitted by the original NetGenerator/NETGenerator.cs (line 1287).
Running the generator inside the parent PowerShell process without an explicit
target could select legacy quicksort and would be the wrong reference.

Checked fixture runtime: Framework 4.0.30319.42000, target 4.7.2, introsort=true.
SHA-256 (UTF-8 with LF): 137cffbcd6c72be42a12b566ec80f431620c27c87e40a1c3a295387a5b65e2ef.

12 scenarios × 10 lengths × 2 seeds × 8 modes × 2 nil patterns = 3840 executions.
Scenarios cover global/member/default/descending sorting, integer comparison,
less predicates, integer/class/string keys, qualified generic key type choices.
Lengths straddle the insertion threshold, including 16, 17, 31 and 64 elements.

Modes cover ordinary/reverse/inconsistent order, divide-by-zero and index faults,
swapping source array elements, modifying a contained object's score, replacing
a source cell during comparison/key extraction. Journals identify real receiver/
argument objects, key evaluations and comparison order; fault cases include the
actual partially changed array before the failing call. Successful outputs record
the final object order and state, including equal keys and nil key behavior.

sorting-program.pas is independently compiled once by the native compiler and
executed for every row. TSV columns: scenario, length, seed, mode, nil pattern,
exact output, mapped error category. Comparison faults map to E_SORT; an index
fault inside comparison maps to E_COMPARER, while key extraction propagates
E_BOUNDS/E_DIV_ZERO. Resource/cancellation faults retain their native category.

Additional native tests cover type/overload selection, strings and Arrays unit
ownership, value-copy record semantics, enum/BigInteger order, reference identity,
empty/noncomparable arrays, nil callbacks, qualified explicit types, malformed
bytecode, instruction/cancellation and inconsistent comparer bounds.

This fixture proves the array prerequisite. List callback/backing-array mutation,
IComparer injection through System.Array/List APIs, nongeneric/boxed comparison,
variance and user source-unit loading are separate remaining work.

Primary algorithm reference:
[Framework ArraySortHelper](https://github.com/microsoft/referencesource/blob/main/mscorlib/system/collections/generic/arraysorthelper.cs).
The source library bodies are bin/Lib/PABCSystem.pas Sort/SortDescending and array
extensions, plus bin/Lib/Arrays.pas generic wrappers.


Two additional 1024-element organ-pipe inputs cover default class and paired
class-key sorting. The independent Framework process asserts that actual
DownHeap/Heapsort frames are observed, proving the depth-limited heap fallback
is reached. Native execution matches both full method journals and final arrays.
Total corpus: 3842 executions. Heap cases use no nils, seed 17 and ordinary order.
Recursive Pascal sorting is additionally checked for a bounded E_LIMIT.

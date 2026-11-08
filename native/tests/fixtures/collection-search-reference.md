# Collection search and Contains equality policies

Three independent Framework CLR 4.0.30319.42000 corpora contain 37296 cases:

| Corpus | Rows | SHA-256 of LF-normalized text |
| --- | ---: | --- |
| collection-search-reference.tsv | 33416 | 0915723004dd5bf3e8eee48e12fc4eabafe1bf6282ea9aaa05112c676c21b6f0 |
| contains-policies-reference.tsv | 3800 | 5331fc2cc64c6163f12fff6a6f33451a718ed9b720dc677d72ecdb1c6d5291bf |
| array-search-scalars-reference.tsv | 80 | 0216765a6b9581d64e0b4b559def6ed97cb75f3bc99083690e8fb2b37ee509cc |

Generate using [generate-collection-search.ps1](../reference/generate-collection-search.ps1),
[generate-contains-policies.ps1](../reference/generate-contains-policies.ps1)
and [generate-array-search-scalars.ps1](../reference/generate-array-search-scalars.ps1).
Each compiles its independent C# source with the installed Framework64 compiler.
Reference sources/executables are development-only and are never installed.
The original Pascal compiler is never run. The first two corpora compile paired
Pascal programs once and compare exact native output and error code for every row.
The scalar corpus injects exact IEEE bits into compiler-verified native typed
arrays/sequences; it does not claim source BitConverter or nonfinite literals.

The main corpus covers List.Contains, IndexOf/LastIndexOf (whole/from/range),
Remove, generic Array.IndexOf/LastIndexOf, missing-policy/explicit-nil/custom
Contains, Where and membership. It includes nil/empty/1/2/5/17-element sources,
spare physical capacity, nil/same/new-equal/missing class probes, declared
IEquatable methods and INT_MIN/INT_MAX range validation order. Finite callbacks
throw or mutate the item/probe/global/source: grow, clone capacity, clear,
remove/insert/reverse/replace, trim, remove ranges and empty AddRange. Optional
old-cursor observation checks both the original failure and cases where a later
MoveNext replaces it. Dispose observers explicitly request bounded cleanup
reserves of 100000 instructions and 65536 cells; application defaults stay
10000 and 4096 with their existing shared cleanup deadline and native stack guard.

The policy corpus covers arrays, lists, custom-policy HashSet, IEnumerable-only
NewSet wrappers, Dictionary entries/keys/values, user enumerables and Where/Select
adapters. Missing policy, explicit nil, custom modular equality, the stored policy
and explicit Default are compared. Source/value/policy argument effects precede
nil validation. User GetEnumerator, MoveNext, Current and Dispose traces include
short-circuit success, faults and nil cursors. A public Contains method on an
IEnumerable-only class must not be used for ICollection delegation. Explicit
comparers run Equals, never GetHashCode; version checks and disposal still apply.
The 80 scalar rows cover signed zeros, infinities, canonical and payload/signaling
NaNs, empty/reversed/repeated arrays for both Single and Double.

Framework legacy array behavior is deliberate: ICollection<T>.Contains and
IList<T>.IndexOf use nongeneric Object.Equals, while generic Array.IndexOf<T>
and LastIndexOf<T> use EqualityComparer<T>.Default. A class implementing only
IEquatable<T> can therefore have different answers. Both routes treat IEEE NaNs
as equal. Missing-policy Enumerable.Contains delegates to ICollection when
available. Passing a comparer, even nil or Default, always uses foreach instead.
List.Contains reads live size/storage; List.IndexOf/LastIndexOf retain the old
physical backing and initial range. List.Remove then applies the found index to
the current list, so a callback Clear can cause a range error. Dictionary pair
Contains searches by the stored key policy and compares the current value by
declared TValue Default equality. PABCSystem NewSet implements only IEnumerable,
so its direct hs-backed Contains can differ from enumerable Contains.

Primary contracts: [Framework Array](https://github.com/microsoft/referencesource/blob/main/mscorlib/system/array.cs),
[List](https://github.com/microsoft/referencesource/blob/main/mscorlib/system/collections/generic/list.cs),
[Enumerable](https://github.com/microsoft/referencesource/blob/main/System.Core/System/Linq/Enumerable.cs),
[Dictionary](https://github.com/microsoft/referencesource/blob/main/mscorlib/system/collections/generic/dictionary.cs),
and local bin/Lib/PABCSystem.pas NewSet declarations around line 507.
The ARRAY_GENERIC_METHODS conditional in Array source must not be mistaken for
the installed legacy array ICollection implementation; the independent oracle
exercises both generic and collection routes explicitly.

Additional native checks cover static interfaces, record values/copies, rejected
closed type mismatches, every argument evaluated once, malformed result/rank/mode/
policy bytecode, instruction limits and cancellation. Remaining work includes
Object virtual overrides and inheritance, record interface methods, boxing and
variance, nongeneric Array protocols, generic named builtin method values,
StringComparer and other equality-aware queries. ArrEqual and matrix membership
use Pascal operators and require a separate compiler-contract audit. This profile
does not complete those operations, the collection APIs or the standard library.

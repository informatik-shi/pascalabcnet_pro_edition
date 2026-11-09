# Mutable records, interfaces and retained collection arrays

The development-only Framework reference executes 3132 independent outcomes:
1320 mutable Counter cases, 252 collection/enumerator cases and 1560 retained
CopyTo array cases. The native test adds static rejection, malformed metadata,
call limits, external definitions, overloads/default/params arguments and field
reference survival across assignment to Self: 4551 checks in total.

Counter is an actual Framework struct implementing inherited interfaces and
IEquatable<Counter>. It has independent ordinary/explicit properties and methods,
indexed properties, mutable Self, reference parameters and interface reboxing.
The corpus distinguishes value copies, array/class field references, temporary
receivers and List element copies. Generic equality callbacks must mutate their
own value copy; direct interface calls mutate the shared box. Assigning Self
retains the box and any outstanding field reference. Reading plain Self yields
a value copy, so comparing it or boxing it again uses value semantics.

The shipped NETGenerator/NETGenerator.cs IThisNode visitor (around line 10794)
loads the value with Ldobj when Self is read as a value. Its method-call visitors
and NETGenegratorTools.cs CreateLocalAndLoad distinguish record addresses and
temporaries. These source paths support the receiver model; the original Pascal
compiler is never executed. Framework boxing/unboxing and C# structs provide the
independent executable reference, including an explicit fieldwise operator for
Pascal's generated record equality. Its C# warnings about retaining the default
ValueType.Equals/GetHashCode alongside that operator are intentional.

Bag is a struct ICollection<int> with a shared List field and mutable getter/copy
counters. Cursor is a struct IEnumerator<int> whose MoveNext and Reset change
the shared box, whose unboxed copy is independent, and whose Dispose prints a
trace marker. Count, ToArray, ToList, HashSet construction, manual/early-break
iteration and bad Current access test actual standard-library consumers.

List constructor/ToList cases use genuine Framework class and struct collections
whose CopyTo retains the destination array. Integer and record elements test
shared writes, nested fields, record methods, sorting/reversal, clear/reuse and
capacity changes. Array-origin writes leave the List enumerator version intact;
List-origin writes invalidate it, even inside an array-write right-hand-side
callback. Array reference origin lives in the shared pointer control block;
Cell, Value, TypeInfo and VM frame layouts do not grow. Negative Count, zero
Count, insufficient arrays and failures partway through CopyTo retain their
source call order, output prefix and error category.

Regenerate all three TSVs, four closed alias-program variants and fourteen
normalized-LF artifact hashes with ../reference/generate-record-method.ps1.
The first two corpora have four integer inputs, hexadecimal UTF-16 stdout and
error code. The retained-array corpus prepends owner kind (record/class) and
element kind (integer/record) to Count, failure index, operation, constructor
route, stdout and error. Constructor route 0 uses ToList; route 1 uses new List.

The application installs none of these managed development tools. General record
constructor overloads, static members/operators, inferred member signatures, general
source generics, Object boxing, source units, general inheritance and full record
closure/const-receiver semantics remain unfinished. Custom ICollection bulk
AddRange/InsertRange alias/version behavior still needs its own complete review.
This evidence does not complete PABCSystem, Collections or the standard library.

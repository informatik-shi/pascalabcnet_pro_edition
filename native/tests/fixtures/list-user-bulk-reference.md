# User ICollection bulk insertion

The development-only executable calls the actual Framework List.AddRange and
List.InsertRange with class and struct ICollection implementations. It does not
invoke the Pascal compiler or substitute a hand-written List algorithm.

The integer corpus contains 4200 outcomes: owner kind, advertised Count
(-2, 0, 1, 2, 4), backing capacity (3 or 10), insertion index, failure position,
AddRange/InsertRange and seven callback modes. A further 336 outcomes use record
elements with an integer field, mutable method and shared array field. The native
test executes all 4536 outcomes, plus type/cancellation checks: 9083 checks.

Count is called once; nonpositive Count skips CopyTo but still changes the List
version on successful return. Positive Count grows storage and shifts its physical
contents before CopyTo. During that callback the logical Count and enumerator
version are still unchanged. CopyTo receives a fresh temporary array of precisely
Count elements, at offset zero. Unlike the List constructor, bulk insertion does
not adopt that array as the List's backing storage.

Seed collections retain the actual constructor CopyTo array. Removing surplus
elements preserves that backing array and yields lists with equal logical Count
but different capacity. The corpus checks shifts visible through the retained
array, growth detachment and writes through that array without invalidating a List
enumerator. Callback modes also add/remove or clear the target in Count, change
capacity in Count or CopyTo, add/remove during CopyTo, or mutate the retained
array. Successful temporary copying must use the current backing array after
CopyTo, and only then publish the new logical Count/version. Exceptions retain
the source output prefix and partial physical shifts.

Record cases distinguish copied scalar fields from shared array fields. Calling
a mutable method on the retained temporary does not change the inserted record,
while changing its array field is visible in the inserted copy. Unwritten generic
temporary/storage cells keep raw record defaults. The tests also check raw cells
exposed when a callback replaces storage after the shift.

Run ../reference/generate-list-user-bulk.ps1 to regenerate both TSVs, four closed
Pascal programs and eleven normalized-LF artifact hashes. Integer rows have owner,
Count, capacity, index, failure position, operation, callback, UTF-16 stdout hex
and error. Record rows omit failure position. Operation 0 is AddRange; 1 is
InsertRange. Callbacks: 0 none; 1 Count add/remove; 2 Count clear; 3 Count capacity;
4 CopyTo add/remove; 5 CopyTo capacity; 6 retained backing-array mutation.

The native path reuses closed ICollection contracts and ordinary callback frames.
Hidden storage cells use raw generic defaults, never executable Pascal record
initializers. No TypeInfo, Cell, Value or VM frame fields, bytecode opcodes, or
Windows stack-reservation changes are introduced. Managed tools remain outside
the application package.

This does not complete List, Collections, PABCSystem or the whole library. Broader
reentrant callbacks that grow logical Count past backing capacity, all malformed
collection contracts, nongeneric interfaces and source generic units still need
their own compatibility work. Native storage bounds remain checked.

# Predicate and Func identities

The reference executes 504 scenarios using actual Framework Predicate<int> and
Func<int,bool> delegates. They have the same invocation signature but different
type identities. The corpus checks typeof/GetType, HashSet<Type>, captured and
named targets, nil, implicit conversions, stored callbacks, record/class fields,
LINQ consumers, generic delegate equality and hash consistency.

The Pascal declaration at PABCSystem.pas:244 aliases Predicate<T> to
System.Predicate<T>; Func aliases System.Func. Native callable type interning
must retain that distinction before execution. Matching invocation signatures
still permit the source delegate conversion.

The authoritative conversion is delegate_to_delegate_type_converter in
TreeConverter/TreeRealization/type_table.cs, around lines 914-981. It emits a
conditional: a nil source gives a nil target; otherwise a new target delegate
binds the source delegate's Invoke method. The source expression occurs in both
the test and constructor branch. The reference explicitly reproduces that
source-derived guard for factories, then relies on genuine CLR delegate
construction, invocation and equality. No Pascal compiler is executed.

Consequences distinguished by the corpus:

- Two named Predicate objects can compare equal while remaining distinct target
  objects. Func wrappers around those two objects then compare unequal.
- Two wrappers around the same Predicate object compare equal. Changing the
  original variable later does not change the retained target.
- A round trip through another delegate type retains an Invoke wrapper; it is
  not identical to the original delegate.
- A nil first factory result is evaluated once. A nonnil first result causes
  a second evaluation. A nil second result makes CLR construction throw an
  ArgumentException, represented by bilingual E_ARGUMENT in the native runtime.

The native closure heap object retains the converted target. Its executable
routine/captured cells remain closed and statically verified; no runtime type
inference is introduced. The existing cast/conditional instructions carry the
conversion, without new bytecode opcodes or larger TypeInfo, Cell, Value or VM
frames. End-of-execution cleanup clears the additional strong target/capture
edges. A compiler expression-copy budget bounds nested conversion expansion.

Regenerate the TSV and artifact hashes with ../reference/generate-callable-family.ps1.
Columns: seed, argument, target route, operation, UTF-16 stdout hex, error.
This evidence does not complete delegates, reflection or PABCSystem. General
source delegate identities, multicast/method-target APIs, variance and raw type
Name/FullName properties remain separate unfinished work.

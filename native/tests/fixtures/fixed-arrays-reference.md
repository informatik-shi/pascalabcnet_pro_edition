# Fixed-bound array compiler recipes

Fixed arrays use typed wrappers around zero-based storage, following
TreeConverter/TreeConversion/type_constructor.cs and NETGenerator/NETGenerator.cs.
The wrapper shape and ordinal bounds are verified before execution. Multiple
dimensions are nested wrappers. Program metadata holds the bounds; TypeInfo,
Cell, Value and the VM frame do not grow, and no opcode is added.

Pascal assignment and by-value arguments clone owned fixed storage. Managed
element references retain their source sharing behavior. A const argument
keeps its wrapper; a var argument keeps the original assignable cell. Raw
generic/container record copies remain shallow, as emitted CLR struct copies
do. A normal declaration constructs a wrapper, while default(A) produces the
source null wrapper. Copying that null value raises E_NIL after the destination
constructor has run. Low/High discard runtime evaluation of the receiver.

The independent development reference executes source-derived C# wrapper and
record methods plus public BinaryReader/BinaryWriter APIs. Its 18 executions
cover three lower bounds, three lengths, successful reads and EOF retries.
Framework and modern .NET outputs match byte for byte. Native Pascal recipes
check every output trace, packed file byte and outcome. The trace observes
record initializers during construction, Clone, by-value/const calls, the
RunTimeSizeOf constructor probe and typed-file reads. This is not an oracle
from running the original Pascal compiler; that compiler is never invoked.
The development runtimes are not included in the portable application.

Additional native tests cover signed/character/boolean/enum bounds, small
ordinal index types, tuple initializers, independent nested copies, var and
const calls, function results, foreach, bounds and nil evaluation order, and
forged shape metadata. The unmodified original tests fileof2, fileofenum1,
arrayoffile1 and binfile1 run through the native interpreter only.

Remaining positive requirements include subrange type declarations, short
strings and sets in typed files, structurally compatible distinct fixed-array
types, complete generated wrapper reflection, anonymous record initialization,
and source overload/operator rules for all element types. This stage does not
complete the language, PABCSystem, any other partial unit or the library.

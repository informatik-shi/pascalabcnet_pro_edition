# Static class fields

Static fields have verified per-class storage and a guarded initializer. They
use ordinary typed globals and native calls; no opcode or TypeInfo/Cell/Value/VM
field is added. The class's static storage begins with CLR zero values. The
initializer runs once on first field access or object construction; recursive
access sees the currently initialized/default fields. Implicit numeric fields
do not overwrite earlier reentrant writes.

NETGenerator's class types omit BeforeFieldInit and use DefineTypeInitializer.
Reads/reference arguments trigger initialization before later operands; a
direct store evaluates its right-hand value first. New-object arguments are
evaluated before class initialization and the instance constructor. Low/High of
a fixed field discard receiver evaluation, as the source compiler does.

The independent development reference executes actual CLR class initialization
using fresh closed classes and an explicit strict static constructor. Its 160
Framework/modern traces check unused classes, repeated reads, stores, constructor
arguments, by-reference calls, instance use, Low/High, owned fixed copies and
reentrant writes and early array-field publication. The array storage is assigned
to the field before initializer elements are evaluated, following the source
CreateArrayForClassField/GenerateArrayInitCode sequence. Generated tag classes merely provide fresh CLR state; generic
Pascal classes are not claimed. The original compiler is never executed.
The native publication recipes also cover global variables, constants, instance
and record fields. Captured loop declarations receive a new cell before early
publication, keeping earlier closures unchanged. Ordinary assignments and
explicit new-array expressions preserve later publication. Neither development runtime is included in the portable application.

Original arrays12 runs natively. Static methods/properties, explicit static
constructors, record statics, inheritance/generic declarations, full source
visibility/reflection and complete standard-library units remain unfinished.

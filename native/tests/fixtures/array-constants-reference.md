# Typed array constants and nested indices

Typed scalar/enum array constants use closed native array types and static
constant pools. Fixed wrappers copy owned storage, while dynamic arrays and
managed inner arrays retain reference sharing. Constants inside procedures
retain their pools across calls. This follows NETGenerator ConvertArrayConstantDef,
ConvertCommonFunctionConstantDefinitions and IArrayConstantNode's static-field
load. Constant initialization precedes ordinary namespace variable initializers.
Implicit numeric globals retain CLR zero storage rather than overwriting writes
made by initializer callbacks; explicit initializers still run afterward.
Direct writes to constant array elements are rejected during binding;
aliases retain the source reference/copy rules rather than freezing heap cells.

The development C# reference independently executes CLR static arrays plus the
source-derived fixed-wrapper Array.Copy methods. All 81 traces are identical
under Framework and modern .NET. Native Pascal recipes check three bounds,
three sizes, three call counts and three update amounts, including dynamic
alias updates, independent fixed scalar copies and shared managed inner arrays. Each trace also observes nested initializer callbacks and
the preservation of a default global counter. Standard factories and input,
random and file calls can initialize values at startup. Compilation only
classifies their types and never executes these calls. The source conversion
rules distinguish root user calls from nested element calls.
The original Pascal compiler is never invoked; this is not its oracle.

Additional native checks cover nested comma indices across dynamic/fixed arrays
and matrices, type-before-execution validation, nil and bounds errors, and pure
constant indexing, standard factory/input initialization and callback ordering.
Only provably pure scalar values enter the analyzer cache. An analyzer-only constant cache folds scalar array reads
inside constant declarations and explicit type bounds. Ordinary runtime reads
stay dynamic, preserving observable alias mutations and error timing. Scalar
constant evaluation retains the complete fixed-array shape metadata. No new
opcode or TypeInfo/Cell/Value/VM field is introduced. No Python/.NET is shipped.

Original arrays2 and arrays3 run through the native interpreter. Records, sets,
pointers, general rectangular constant syntax, complete source rejection rules,
generated reflection and whole standard-library units remain unfinished.

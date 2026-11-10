# Statically typed nested procedures and functions

The parser accepts nested routine declarations in routine declaration sections.
All parameter/result types are resolved before any program execution. Named
local callable cells are declared before closure creation, allowing recursion;
function-name assignment targets the function result while explicit calls
resolve the corresponding callable. Nested routine names are read-only and
remain local to the declaring scope. Captured cells use the existing native
closure implementation, including cycle cleanup and call/allocation limits.
No new opcode or TypeInfo/Cell/Value/VM frame field is required.

The independent development oracle executes C# delegates under Framework and
modern .NET. Its 1,071 output traces are byte-identical between runtimes.
Compiled native Pascal recipes compare escaping counters, shared aliases,
independent parent activations, recursively nested calls, function-name result
assignments and multi-level access to outer locals/parameters against those
traces. This is a source-derived compiler recipe, not a Pascal compiler oracle.
Neither development runtime is installed with the application.

Additional native tests exercise local procedure calls, by-reference and const
parameters, variadic arguments, lexical shadowing, higher-order calls, short
function bodies, early Exit, static argument errors, inaccessible names,
duplicate/read-only declarations and native recursion/parser depth guards.
Named routines additionally capture outer var/out cells and record Self, and
can access an outer function Result. Anonymous lambda capture restrictions
remain unchanged. The unmodified original TestSuite/fileof1.pas runs with the native
interpreter, covering scalar typed files in global, routine and nested scopes.
Original nested1, nested3, nested4 and fResult3 also run natively, covering
record/class methods, recursive by-reference record/class access and result
shadowing. The original Pascal compiler is never executed by this workflow.

Positive remaining requirements include local default arguments, overloads and
forward declarations, local type sections, and complete source-generated
delegate/reflection metadata. Existing
closure restrictions also apply to unsupported loop-control captures and
anonymous record Self references. Parser and binder guards measure real native
frames against a bounded compilation stack budget; excessive nesting raises
a bilingual E_LIMIT instead of overflowing the native stack. This extension
does not complete source units or the standard library. Fixed-bound arrays
and the other binary-file gaps remain.

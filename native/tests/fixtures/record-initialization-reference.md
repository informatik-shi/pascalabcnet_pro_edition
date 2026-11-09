# Record field initialization

The reference contains 1215 outcomes, executed natively against both explicitly
typed and inferred-field Pascal programs. Inputs select three integer biases,
three lengths, five failure positions and 27 operations. Output records field
order, constructor arguments/bodies, shared array fields, copied records, loops,
named and explicit-interface function results, ordinary/default/generic collection values and array resizing.

This is a source-derived lowering oracle, not output from a Pascal compiler.
C# 4 does not support struct field initializers. Explicit Init(ref value)
recipes model the following inspected original paths:

- NETGenerator/NETGenerator.cs GenerateInitCodeForField initializes fields in
  declaration order. An explicit nonconstant expression replaces implicit
  initialization of that field.
- ConvertLocalVariable, AddInitCall and function-result variable creation in
  TreeConverter/TreeConversion/convertion_data_and_alghoritms.cs initialize
  ordinary record locals/results. Copies and generic default values do not
  invoke field initializers.
- GetInitalValueForVariable in compilation_context.cs selects a declared
  zero-argument record constructor for local declarations. Global variables
  and nested fields use field initialization; the explicit constructor body
  is not substituted there.
- GenerateStandardFuncCall/NewArray and CreateInitCodeForNDimUnsizedArray
  initialize new record arrays in index order, before target assignment.
- The Array.Resize path remembers the old length, calls Resize by reference,
  then initializes only the new tail. An initializer sees the resized target.
- syntax_tree_visitor.cs lowers multidimensional SetLength to new array,
  CopyWithSize and target assignment. All new cells are initialized first;
  PABCSystem.CopyWithSize then copies the intersection. Initializers still see
  the old target, and a failure leaves it assigned to the old matrix.

The development-only Framework executable supplies actual struct copies,
array reference sharing, Array.Resize, List capacity, Dictionary.TryGetValue,
LINQ generic defaults and exception propagation. An explicit field-init trace
is followed by hexadecimal UTF-16 stdout and normalized error categories.
The original Pascal compiler is never invoked. The native application ships
none of these managed development tools.

Regenerate the TSV, two concrete Pascal programs and six LF-normalized hashes
with ../reference/generate-record-initialization.ps1. Additional native checks
cover static typing, inferred arithmetic promotion, global/field name clashes,
initializer recursion, cancellation and malformed initializer operands/metadata.
Original TestSuite/records13.pas and records15.pas run unchanged as native tests.

All field types and initializer targets are closed before execution. Initializer
routine ids reuse verified instruction operands. Raw default storage and copies
stay separate from executable initialization; TypeInfo, Cell, Value, VM frames,
the instruction set and the normal Windows stack reservation do not grow.

The supported inferred forms include literals, known fields, typed globals,
declared result signatures, new values/arrays, Arr and already bound binary
operators. Other forms require an explicit field type and fail before execution.
Anonymous record field initialization, implicit result initialization of anonymous functions, general constructor overloads, static
members/operators, inferred routine signatures, source generics/units and full
library compatibility remain unfinished. These tests do not complete a unit.

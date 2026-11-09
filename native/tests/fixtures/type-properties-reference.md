# CLR type properties: reference data in progress

The development-only executable records 736 outcomes for Name, FullName,
Namespace and AssemblyQualifiedName over 92 concrete type expressions and two
source module names. All 92 expressions are accepted by the current native
checker before adding these property reads; that check does not prove that the
properties themselves are implemented.

Actual Framework types supply primitive, array, generic, tuple, delegate,
collection/view/enumerator, encoding, comparer and runtime-Type metadata.
Reflection.Emit supplies source-defined record/class/interface/enum and unit
type identities without executing the Pascal compiler. Emitted unit shapes
model names and generic arity, not implementations of Pascal library classes.

The authoritative Pascal source for emitted identities is NETGenerator.cs:
BuildTypeName prefixes an unqualified type name with cur_unit; for a main program
cur_unit initially comes from Path.GetFileNameWithoutExtension(SourceFileName).
The emitted application assembly uses version 1.0.0.0 (NETGenerator.cs).
Type creation in syntax_tree_visitor.cs appends the generic arity tick. Unit
declarations supply Collections, PABCSystem, IniFile and School namespaces.

These are raw CLR properties. Pascal TypeToTypeName is a separate formatter and
cannot substitute for them. In particular, a nested generic Dictionary enumerator
has Name Enumerator, while its FullName retains Dictionary's generic arguments
and declaring type. Array nesting is constructed explicitly in the reference
to avoid C# array-notation precedence mistakes.

Regenerate the TSV with ../reference/generate-type-properties.ps1. Columns are
source module, Pascal type-producing expression, property, UTF-16 result hex and
normalized error. A nil string is encoded as nil; an empty string as a dash.
For E_NIL there is no returned string. No expected failure of a native property
read is made a compatibility requirement.

Current work must also preserve distinct delegate identities: PABCSystem declares
Predicate<T> as System.Predicate<T>, whereas Func<T,boolean> is System.Func<T,bool>.
The native signature-interning model currently merges them; the new property
implementation must not conceal that difference behind a formatted label.
Full source-generated delegate and iterator identities still require source
evidence. This reference does not complete reflection or a standard-library unit.

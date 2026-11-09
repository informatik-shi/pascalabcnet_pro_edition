# Named Pascal delegates

The Framework reference declares two separate function delegates, two ref
procedure delegates and two parameterless procedure delegates. It records
1296 outcomes across two logical source modules, three seeds, three arguments,
four source routes and 18 operations. Metadata includes actual Name, FullName,
Namespace and AssemblyQualifiedName, arrays and nested generic uses.

The authoritative source rules are syntax_tree_visitor.cs: named classic
function/procedure headers create a delegate with the type declaration's name
(12395 onward), whereas modern_proc_type is lowered to System.Func or
System.Action (21903 onward). NETGenerator prefixes an unqualified type name
with the source-file stem and uses application version 1.0.0.0. Development
executables therefore use matching namespaces and assembly names.

As in the callable-family reference, type_table.cs 914–981 supplies the explicit
conditional conversion guard: the source is evaluated once for nil, and again
for the Invoke target when nonnil. CLR delegates supply the resulting target
identity, equality, hashing and invocation. Nil second targets report
ArgumentException. These guards are a source-derived lowering recipe, not a
claim of executing the original Pascal compiler.

The matrix covers different named types with identical signatures, compatible
conversions and round trips, retained targets and captured cells, stored queries,
collections and fields, nil/failing factories, ref invocation and procedures.
It does not settle general cross-family operator overload resolution, variance,
multicast, generic source declarations, arbitrary anonymous generated names or
full reflection. Existing arrow aliases must retain their System.Func/Action
identity rather than acquire the Pascal alias name.

Regenerate with ../reference/generate-named-callables.ps1. TSV fields are module,
seed, argument, source route, operation, UTF-16 output hex and normalized error.
Outputs normalize CRLF to LF; an empty output uses a dash. The reference tools
are development-only and never installed in the portable application.

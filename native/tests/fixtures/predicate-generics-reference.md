# Generic standard predicate identities

PABCSystem.pas declares Predicate2<T1,T2> and Predicate3<T1,T2,T3> as classic
generic function types (247–250), while its Func aliases name System.Func.
syntax_tree_visitor.cs 12395 onward creates the declared delegate name with
its generic arity. These are separate nominal types even when their closed
Invoke signatures match a Func signature.

The development reference declares the matching public generic delegates in
namespace PABCSystem. Four Framework executables use application version
1.0.0.0 and the two logical source assembly names. Actual CLR metadata supplies
Name, FullName, Namespace and AssemblyQualifiedName, nested generic arguments,
arrays and different argument orderings. No Pascal compiler is executed.

The 2304 rows cover two arities, two source modules, three seeds, three integer
arguments, four source routes and 16 operations. String and boolean arguments
affect results. Operations check invocation, nil and captured factories,
conversion targets, equality/hash coherence, collections, fields, round trips,
reassignment, stored predicates inside query closures and metadata.

As in the callable-family corpus, type_table.cs 914–981 supplies the explicit
source-derived conditional guard. A nil source yields nil; a nonnil first
result causes a second evaluation before binding Invoke. CLR delegates supply
the invocation and target identity; a nil second target is ArgumentException.
This recipe is not a claim of executing the original Pascal compiler.

TSV fields: module, arity, seed, argument, route, operation, UTF-16 output hex,
normalized error. CRLF is normalized to LF and an empty output uses a dash.
Regenerate with ../reference/generate-predicate-generics.ps1. Tools are never
installed in the portable application. General source generic declarations,
variance, multicast, anonymous names and full overload resolution remain open.

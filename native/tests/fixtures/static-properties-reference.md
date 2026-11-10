# Static properties

64 actual CLR executions under Framework and modern .NET compare reads, stores, compound evaluation, accessor/delegate order, strict initialization, reentry and defaults. Indexed/default static properties use the same actual CLR accessor calls with explicit once-evaluated indices; C# does not offer static indexer declaration syntax. Fresh closed classes isolate static state, without claiming generic Pascal declarations. The original Pascal compiler is never executed; development runtimes are not shipped.

Native properties have known types and verified separate Program metadata. Existing call instructions invoke accessors without an instance receiver. Class initialization runs at accessor entry after arguments; callable-property arguments follow getter evaluation. Index temporaries preserve single evaluation during compound stores. Static record methods and computed properties without record static fields use the same known signatures. General record static storage/initialization, explicit constructors, operators, full inference, inheritance/generic declarations, complete visibility/reflection and error wrapping remain unfinished.

The original grammar accepts tkAssignOrEqual for typed var/field initializers. Both := and = use native declaration initialization. Original defaultparams4 and lambda_static_1 are checked only through the native interpreter.

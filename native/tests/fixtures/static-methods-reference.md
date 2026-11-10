# Static class methods

Independent actual CLR traces check strict class initialization after argument evaluation, lazy delegate creation, reentry, typed var arguments, recursion, defaults, params, overload selection and captured callbacks. Fresh closed reference classes isolate each case; generic Pascal declarations are not claimed. The original Pascal compiler is never executed. Development runtimes are not included in the portable application.

Native static methods use existing typed call frames, with an initialization guard at entry when the class has static fields. Separate verified Program metadata preserves the class/member identity without adding an instance receiver or changing TypeInfo/Cell/Value/VM fields or opcodes. Same-module visibility follows current native class support. Static properties, operators, explicit static constructors, record statics, mixed instance/static overloads, inheritance/generic declarations, complete visibility/reflection and initialization exception wrapping remain unfinished.

The source NETGenerator emits static call arguments before OpCodes.Call; delegate construction emits Ldftn and Newobj. Those sequences and the strict class type initializer determine the checked order. The native source recipe uses the supported if/then/else form; general case statements are outside this stage.

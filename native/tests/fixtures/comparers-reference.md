# Independent comparison reference

Development-only generator: [comparers.cs](../reference/comparers.cs) and
[generate-comparers.ps1](../reference/generate-comparers.ps1).
Runtime used for this checked-in fixture: Framework 4.0.30319.42000.
Neither the original Pascal compiler nor generated managed artifacts are used by
the native tests or included in the portable application.

The oracle independently calls Framework Comparer<T>.Default/Create,
IComparable<T>, IComparer<T>, and direct C# translations of the original
bin/Lib/PABCSystem.pas InRange/Between/Clamp bodies (lines 11460–11495).
The native test compiles comparers-program.pas once and executes it using
the same inputs, comparing the exact output journal and located bilingual
native error category. It does not call the oracle during a native test run.

12 scenarios × 4 first values × 4 second values × 4 third values × 3 modes =
2304 executions. Values include nil, negative, exception-triggering and positive
objects. Modes use ordinary subtraction, a failing CompareTo or reverse order.
The journal distinguishes actual CompareTo calls and user/delegate comparer calls.
Coverage includes identical references, nil arguments, short-circuit evaluation,
reversed bounds, original Clamp error timing and propagated method failures.

TSV columns: scenario, a, b, c, mode, exact output, error code.
SHA-256 (UTF-8 with LF): b0d5a28fa5302b226c4c3af1f709e63a776b93605f75dda30432aea8d8655090.

Additional native tests cover primitive/enum/BigInteger/string behavior, IEEE
NaN/signed-zero/infinities, callback captures, reference identity, static
signatures, aliases/qualified names, reflection, malformed bytecode metadata,
cancellation, allocation and recursive native callback limits.

Comparator injection into sorting/collections, nongeneric/boxed protocols,
distinct tuple comparisons, class inheritance/variance and record methods are
not implied by this fixture.


comparers-scalars-reference.tsv adds 148 independent primitive pairs, covering
both direct IComparable<T>.CompareTo and Comparer<T>.Default. Small integral
types and char return the exact integer difference, while Int32/UInt32/Int64/
UInt64/Boolean return the source sign. Scalar TSV SHA-256 (UTF-8 with LF):
3069d039e23440b71153223fc39d86155329437b42879d9038a9e5e1df7a2a1f.

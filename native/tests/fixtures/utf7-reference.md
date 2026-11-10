# Native UTF-7 reference

The development-only public API oracle uses UTF7Encoding(false) on Framework
and .NET 10.0.12, with actual throwing encoder/decoder fallback cross-checks.
No Pascal compiler is invoked. The modern runtime disables the GetEncoding
factory for 65000 by default; its explicit constructor supplies the independent
algorithm reference. Native factory behavior targets the original Framework.

All 22 reachable encoder residual states are constructed through public calls:
ASCII and every residual value with zero, two or four pending bits. Every
UTF-16 unit is appended in every state (1,441,792 inputs), including lone
surrogates. A retained high surrogate emits bytes immediately, unlike UTF-8.
Each outcome is checked with final=true and with non-final input followed by
an empty final call, also using an actual strict encoder. All 1,048,576
supplementary scalars additionally cover whole and surrogate-split encoding.

The 21,847 decoder prefixes cover ASCII, the first plus and every residual
value for all eight even bit counts below sixteen. Every possible next byte
is checked with default and actual throwing fallback, an empty final split
and state reset (5,592,832 suffixes). All 65,536 fresh byte pairs also cover
every split. Tests consume all recorded outcomes, preserving raw UTF-16 units,
partial-bit discard, plus/minus rules, high-byte zero extension and the
historical invalid-NUL fallback distinction.

Both file corpora have 1,421 equal recipes. These cover all single bytes,
complete/incomplete base64 prefixes, reader boundaries, BOM overrides,
surrogate/buffer splits, explicit/repeated Flush, literal plus inside versus
outside a shifted run, fresh append, writer Encoding snapshots, Rewrite and
raw surrogate halves separated by Flush. Native tests additionally reject an
oversized preview, then compare close bytes with an unchanged source baseline.
Final padding and the terminating minus participate in bilingual size errors.
ReadFileToEnd follows the source ReadLine/AppendLine recipe, normalizing host
newlines to LF in both the development reference and native comparison.

The native target performs 30,283,665 checks. The byte-identical primitive
fixture has 34,932,673 bytes, compressed losslessly to 6,396,247 bytes.
The lossless ZIP contains only utf7-reference.bin; CMake extracts it only for
tests. The application implements the bit transform and per-reader/writer
state in C++, with no platform codec, registry, managed runtime or archive
dependency and no new VM/Value/Cell/TypeInfo fields or opcodes. Regenerate with
../reference/generate-utf7.ps1. The generator writes no C++ source or tables.

The optional-character constructor, complete Encoding/fallback/offset APIs,
code-page zero/default resolution and full module/library compatibility
remain unfinished. All 140 Framework codec numbers are supported; this does
not claim that System.Text or a whole standard-library module is complete.

The official [UTF7 implementation](https://raw.githubusercontent.com/dotnet/runtime/v10.0.0/src/libraries/System.Private.CoreLib/src/System/Text/UTF7Encoding.cs)
provides source context for raw UTF-16 and historical fallback behavior;
fixture results come from public API execution, not extracted private data.
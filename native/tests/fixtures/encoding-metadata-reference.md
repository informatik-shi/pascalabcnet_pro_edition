# Encoding invariant metadata and maximum-size reference

The development oracle executes the public Framework API on all 140 default
read-only codec objects. No Pascal compiler or private runtime data is used.
Native scope is ten invariant properties: WebName, BodyName, HeaderName,
WindowsCodePage, IsSingleByte, IsBrowserDisplay, IsBrowserSave,
IsMailNewsDisplay, IsMailNewsSave and IsReadOnly, plus GetMaxByteCount and
GetMaxCharCount for the current immutable default factory encodings.

Both maximum-size functions are independently called at every input from zero
to 65535 for every codec (18,350,080 outcomes). The observed formulas are fitted
and checked against every call. Each complete sequence is also fingerprinted
with FNV-1a-64 over the little-endian Int32 result bytes. Native tests recompute
these independent public-API fingerprints, rather than reproducing the model
formula in their expected-value code. FNV is an outcome checksum, not a security
hash; the manifest separately uses SHA-256 for all artifact integrity.

There are 6980 additional actual calls: negative inputs, large counts and both
sides of the Int32 overflow boundary. These preserve source argument errors,
including zero-byte UTF-7, Unicode and GB18030 distinctions and all ISO-2022
mode constants. Native methods return statically known integer values without
allocating buffers; overflow/negative errors are bilingual and retain their
source call position and preceding output. Integer arithmetic uses a wider
intermediate, and the verifier checks method, receiver, parameter and result
metadata before execution. No VM/Value/Cell/TypeInfo fields or new opcodes are
needed.

The metadata TSV also records actual EncodingName results for en-US and ru-RU.
Those two columns are positive requirements for future UI-culture work; they
are not part of the ten native properties in this stage. Similarly the
197-declaration public API inventory is a positive gap inventory, not native
coverage. Full constructors, fallback objects, offset APIs, UI culture,
default/code-page zero, source class/unit behavior and complete standard-library
modules remain unfinished. No module is promoted by this property extension.

Regenerate with ../reference/generate-encoding-metadata.ps1 only while no C++
compiler is active: the final Node generator writes the C++ data include.
The Node generator --check and PowerShell -ManifestOnly modes are read-only
with respect to C++ source. The application uses compiled static data and no
reference/generator, Python, .NET or registry dependency.
The affine/rounded default-count forms are also cross-checked with official
[SBCS](https://raw.githubusercontent.com/microsoft/referencesource/main/mscorlib/system/text/sbcscodepageencoding.cs),
[Unicode](https://raw.githubusercontent.com/microsoft/referencesource/main/mscorlib/system/text/unicodeencoding.cs),
[UTF32](https://raw.githubusercontent.com/microsoft/referencesource/main/mscorlib/system/text/utf32encoding.cs) and
[ISO2022](https://raw.githubusercontent.com/dotnet/runtime/v10.0.0/src/libraries/System.Text.Encoding.CodePages/src/System/Text/ISO2022Encoding.cs) source context.
Fixture values and fingerprints come from actual Framework public calls.

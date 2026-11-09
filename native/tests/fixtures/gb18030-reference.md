# Native GB18030 reference

The development tools invoke actual Framework and .NET 10.0.12 public encoding
APIs. Their complete ABCGB181 primitive fixtures must be byte-identical before
generation succeeds. No Pascal compiler is invoked and no development runtime
or generator is installed with the application.

The reference records every individual BMP encoding, every single byte and
byte pair with default/exact flags, all 1,587,600 syntactically shaped four-byte
inputs, all 1,048,576 supplementary scalar encodings and 134,400 variable stream
inputs. All four-byte inputs are verified through actual throwing fallback and
every split, including a final empty flush. Every variable input is recorded
at all splits with both default and actual strict decoders. Native tests consume
all fields independently and exercise the public native operations, not only
the generated tables.

GB18030 has observable retained-decoder behavior at incomplete EOF prefixes.
For example 81 30 decodes to '?' statelessly, but to '?0' after either byte has
been retained by a non-final decoder call. The implementation preserves the
separate pending-input and fresh-input consumption rules. It reuses the native
TextDecoder carry string without adding fields to TextDecoder, VM, Value, Cell
or TypeInfo. The short GB18030 BOM-detection window ends when input cannot be
a Unicode preamble, allowing the underlying decoder to retain the prefix.
Other codecs keep their existing detection behavior.

The source-derived grammar uses one ASCII byte, two-byte mappings or four-byte
mixed-radix offsets. Static BMP/two-byte/four-byte-BMP data comes from the actual
reference. Supplementary ranges and unmapped gaps are exhaustively checked
against both runtimes; no newer GB18030 revision is claimed. See the official
[GB18030 implementation](https://raw.githubusercontent.com/dotnet/runtime/v10.0.0/src/libraries/System.Text.Encoding.CodePages/src/System/Text/GB18030Encoding.cs).

Each runtime also produces 786 StreamReader/StreamWriter file recipes: every
single byte, short incomplete prefixes, BOM overrides, four-byte characters
across each reader boundary, pending EOF prefixes, surrogate writer buffers,
split writes and explicit Flush between surrogate halves. Native follows the
modern flush behavior selected by earlier file work. Both corpora remain in
the repository; the 490 differing Framework EOF rows are compatibility gaps,
not claimed legacy passes. ReadFileToEnd uses the source ReadLine/AppendLine
recipe, with platform newlines normalized to LF in both reference and tests.

The cached Pascal Encoding factory supports 54936 and the independently verified
gb18030 alias, distinct GB18030Encoding metadata and full supplementary Unicode
round trips. Expanded output respects the 1 MiB bound and errors retain EN/RU
messages and source positions. No platform codec, registry or managed runtime
is used by the C++ implementation.

Regenerate with ../reference/generate-gb18030.ps1 while no C++ compiler is
active; it writes ../../src/gb18030_data.inc. The --check mode of
../../tools/generate-gb18030.mjs is read-only. The JSON manifest distinguishes
raw binary hashes from LF-normalized source hashes. This is one Encoding
dependency: full Encoding/object APIs, sixteen remaining Framework pages and
the whole standard library remain unfinished.

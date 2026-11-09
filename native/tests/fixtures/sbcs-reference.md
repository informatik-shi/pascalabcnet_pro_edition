# Native single-byte encoding reference

The development-only Framework executable enumerates all 95 encodings whose
IsSingleByte is true. The binary ABCSBCS2 records every one of 256 byte mappings
and 65,536 individual UTF-16 encoding mappings per code page, default fallback,
exact mapping flags and whether an encoder retains a final high surrogate.
ASCII and Windows-1251 were already supported; this adds 93 encodings.

Exact encoding membership is observed with a two-character replacement fallback:
one byte means an exact mapping, two means fallback. Every exact mapping, every
nontrivial best-fit mapping, every surrogate and a sample of the remaining BMP
blocks is independently checked with actual EncoderExceptionFallback. All 256
decoder inputs are checked with DecoderExceptionFallback. Native tests compare
all 6,225,920 default/exact encoding entries, whole-BMP batches, strict valid
batches, rejected best-fit/unrepresentable inputs and streaming decoding.

Framework's internal EncodingTable supplies the complete 405 alias records.
The pointer layout/count is inspected only by the development executable and
every extracted entry is validated using public Encoding.GetEncoding. The 318
aliases belonging to the 95 single-byte and five existing Unicode codecs are
installed as static native data. Invalid names and whitespace are checked by
a separate 13-row public-API reference; old unsupported synonyms are not retained.
Code-page zero/default resolution, the remaining 40 variable-width/stateful
encodings and full Encoding/object APIs are not completed by this work.

The 570 file recipes use actual StreamWriter/StreamReader with the selected
encoding and the FilesOperations.pas line/AppendLine recipe. They cover
DefaultEncoding writes, ReadAllText, ReadFileToEnd, BOM override and a surrogate
pair crossing the 1024-character writer buffer, both in one write and split
writes. ReadFileToEnd reference rows normalize host newlines to LF; native tests
make the same platform-only normalization. Pascal source routines run in the
native interpreter; no original Pascal compiler is executed.

The generated C++ data uses sparse BMP mappings, exact bit flags and binary
search. It needs no platform codec, registry, framework, Python or generator at
runtime. GetType retains the distinct Latin1Encoding class. No runtime VM/value/
type-info layout or opcode changes are needed.

Regenerate with ../reference/generate-sbcs.ps1. The generator also updates the
portable table with ../../tools/generate-sbcs.mjs; its --check mode verifies the
checked-in data. The JSON manifest hashes binary bytes and canonical LF text
separately. File and name TSV outputs use hexadecimal bytes or UTF-16 units.

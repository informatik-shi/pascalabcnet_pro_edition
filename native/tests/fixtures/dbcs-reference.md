# Native double-byte encoding reference

The development-only Framework reference enumerates all 22 encodings whose
runtime class is DBCSCodePageEncoding: 932, 936, 949, 950, 1361, 10001, 10002,
10003, 10008, 20000–20005, 20261, 20932, 20936, 20949, 50227, 51936 and 51949.
This classification refers to Framework, not modern provider implementation names.
Additional EUCJPEncoding 51932 uses the same two-byte grammar with different
tables and distinct runtime metadata. The complete data covers 23 codecs.

ABCDBCS1 records every BMP UTF-16 encoding input, every single byte, every byte
pair, exact-map flags, leading-byte membership and default replacements. A
three-character replacement distinguishes fallback from one/two-byte exact
encoding. Every exact map, nontrivial best-fit input, surrogate and sampled
remaining BMP block is independently checked with throwing encoder fallback.
Every decoder input is checked with throwing decoder fallback. All byte pairs
are also checked through the actual decoder with one byte per call. Each codec
consumes a leading byte and its successor together, including invalid pairs;
the final lone leading byte uses the recorded fallback. All 1,048,576
supplementary scalars per codec and high-surrogate encoder carry are verified.
All 65,536 EUC-JP inputs prefixed with 8F are independently checked with actual
decoding and both possible decoder splits. Like the
[source implementation](https://raw.githubusercontent.com/dotnet/runtime/main/src/libraries/System.Text.Encoding.CodePages/src/System/Text/EUCJPEncoding.cs),
this preserves fallback consumption rather than claiming support for the
three-byte JIS X 0212 plane.

Native tests consume the binary independently through public codec operations,
strict modes, every split pair, bounded supplementary batches, Pascal cached
Encoding factories and DBCSCodePageEncoding metadata. The 63 aliases come from
the independently validated complete Framework alias fixture used by SBCS.
Streaming decoding retains a trailing leading byte, reuses the existing carry
buffer and supports BOM switching. Expanded output respects the 1 MiB limit.
No VM/value/type-info layout or opcode changes are required.

The two 161-row file corpora run actual StreamReader/StreamWriter with a
1024-character buffer: writes, ReadAllText, the FilesOperations ReadLine/AppendLine
recipe, split surrogate writes, BOM override and a two-byte character crossing
the reader block boundary. In the last recipe each file ends with a lone lead.
Framework StreamReader drops it; .NET 10.0.12 flushes replacement. Native follows
the modern behavior already selected by the earlier file-operations work. Both
results are retained; these 23 legacy differences are explicit compatibility
gaps. The other 138 recipes agree. No original Pascal compiler is executed.

The C++ runtime uses static tables and binary search, without platform codecs,
registry access, Python or managed dependencies in the application. Reference
and generator tools are development-only. Regenerate using
../reference/generate-dbcs.ps1 when no C++ compiler is active; the generator
writes ../../src/dbcs_data.inc. ../../tools/generate-dbcs.mjs --check is read-only.
The JSON manifest hashes binary bytes and LF-normalized text separately.

Together with the earlier SBCS and Unicode codecs this supports 123 of the 140
Framework pages. Remaining: ISO-2022/HZ 50220/50221/50222/50225/52936, GB18030 54936, ISCII 57002–57011 and UTF-7 65000. Code-page zero,
fallback objects, offset/count APIs, full class semantics and the standard
library as a whole remain unfinished.

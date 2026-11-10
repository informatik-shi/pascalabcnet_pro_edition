# Native ISO-2022/HZ reference

The development-only public-API oracle covers 50220, 50221, 50222, 50225 and
52936. Both Framework and .NET 10.0.12 produce the complete ABCISO01 fixture;
generation requires byte-identical primitive outcomes. No Pascal compiler is
invoked. The 95,408,601-byte raw fixture is stored in a lossless ZIP to avoid
large repository blobs. CMake extracts its sole named member only for the C++
test; the application has no archive or development-runtime dependency.

Each codec records every default/strict BMP encoding, controlled raw lookup
matrices and every BMP suffix in retained or finalized encoder contexts. The
3,735,552 contextual encodings cover pending high surrogates, ASCII/JIS/halfwidth
transitions, the Japanese shift-in return mode, Korean designator reuse/reset,
HZ shifts and tilde escaping. Actual strict encoders independently cross-check
all representable inputs, all surrogates and sampled remaining BMP blocks.

The decoder fixture stores 11,206,656 default/strict byte-pair suffixes after
complete and incomplete mode prefixes. Each is also checked with the suffix
split at both boundaries, including an empty final flush. Prefixes cover
ASCII, JIS, halfwidth Katakana and its saved return mode, historical NOOP and
0212 interpretations, Korean designation, HZ mode and partial escape queues.
Native tests consume every recorded outcome and verify all three splits.

Static C++ maps are derived from observed canonical symbols, raw lookup
matrices and exact flags. The native state machine preserves queue versus
fresh-input consumption, historical high-bit/PUA maps and halfwidth aliases,
Korean whitespace passthrough and HZ space/high-bit/error handling. See the
official [ISO2022 implementation](https://raw.githubusercontent.com/dotnet/runtime/v10.0.0/src/libraries/System.Text.Encoding.CodePages/src/System/Text/ISO2022Encoding.cs).

Each runtime produces 9,725 file recipes: all single bytes, complete/incomplete
escape contexts, reader boundaries, BOM overrides, surrogate and writer-buffer
splits, explicit Flush, final ASCII restoration, fresh append, the snapshot
encoding of an open writer and Rewrite after emitted non-ASCII text. A native
resource test rejects an oversized write on a copied encoder and compares the
subsequent close bytes with an unchanged source baseline, including Korean
designation state. The 4,062 different Framework EOF rows remain explicit
legacy compatibility gaps; native follows the modern reader policy already
selected by previous file work. ReadFileToEnd uses the source ReadLine/AppendLine
recipe and normalizes host newlines to LF in both reference and tests.

Factories retain distinct ISO2022Encoding metadata, integer code pages,
seven validated aliases and cached identities. No new VM, Value, Cell or
TypeInfo fields/opcodes or platform codecs/registry/runtime dependencies are
required; encoder and reader state remain in their own heap-backed objects.

Regenerate using ../reference/generate-iso2022.ps1 only when no C++ compiler is
active: it writes ../../src/iso2022_data.inc. The Node generator --check mode
is read-only and uses Node's built-in ZIP/deflate handling for development data.
The JSON manifest hashes archive bytes, LF-normalized sources and uncompressed
reference bytes separately. UTF-7, full Encoding/class APIs and the whole
standard library remain unfinished.

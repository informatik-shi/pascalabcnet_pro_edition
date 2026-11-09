# Native ISCII and persistent text encoder reference

The development-only generator invokes actual Framework and .NET 10.0.12
public APIs for every code page 57002–57011. The complete ABCISC01 primitive
fixtures must be byte-identical before the table generator runs. No Pascal
compiler is invoked and no development runtime is shipped.

For every page the fixture records all 65,536 default/strict single UTF-16
encodings, every single byte, pending-byte classification, composed Nukta
alternatives and all 65,536 byte pairs with actual strict outcomes. Each pair
is independently checked through actual default/strict decoders at both splits.
Decoder.Reset does not restore the selected script; the oracle explicitly
selects the initial default with ATR 40 between independent cases.

All 65,536 UTF-16 suffixes are recorded in twenty retained encoder contexts
per codec: empty state, a pending high surrogate, nine representative letters
and nine Viramas. This yields 13,107,200 default contextual results. Strict
membership is independently checked with actual throwing encoders for every
representable scalar, every surrogate, join controls and sampled remaining BMP
blocks. Consecutive identical results are stored as lossless runs; no tested
input is removed. All 6,553,600 byte pairs after selecting each script are
checked against the corresponding direct codec, and 51,200 ATR 40/41 cases
prove restoration of the initial default, including its distinct nominal
script when two pages share glyph mappings.

The C++ implementation uses 690 generated Indic mappings and ten small
byte/alternate/mode tables derived from public outcomes, without private
runtime-table extraction. It preserves script switches, final default-font
reset, Virama plus join controls, Nukta composition and Devanagari extensions.
The source retains a default-font encoder Virama across a final call; the
native state follows that distinction. See the official
[ISCII implementation](https://raw.githubusercontent.com/dotnet/runtime/v10.0.0/src/libraries/System.Text.Encoding.CodePages/src/System/Text/ISCIIEncoding.cs).

TextEncoder now owns the encoding and retained state for each open text writer.
Buffer emission keeps that state, explicit Flush/Close performs a final call,
Rewrite preserves the existing encoder and pending characters, and a new
writer receives a new state. Size preflight runs on a copied encoder; bytes and
state are committed together after writing. The previous Unicode/SBCS/DBCS/
GB18030 high-surrogate rules remain in the shared encoder. Its state lives in
the heap-backed text file; no VM, Value, Cell or TypeInfo layout or opcode is
expanded. The reader has its own small script/pending state.

Each runtime produces 9,490 file recipes over all ten pages, including single
bytes, special lookahead, mixed scripts, ATR/reset and reader block boundaries,
BOM override, writer buffer boundaries, surrogate splits, Flush after Virama,
changed-font flush, append and the encoding snapshot of an open writer, Rewrite after emitted font changes and recovery after a rejected
native size preflight. Native
follows the modern StreamReader flush policy selected by prior file work. Both
corpora are retained; the 958 different legacy EOF rows are explicit remaining
compatibility gaps. ReadFileToEnd normalizes host AppendLine newlines to LF.

Integer/name factories use the ten validated aliases from the complete alias
fixture and preserve ISCIIEncoding metadata and per-execution cache identity.
The runtime needs no platform codec, registry, Python or managed dependencies.
Regenerate with ../reference/generate-iscii.ps1 only when no C++ compiler is
active: it writes ../../src/iscii_data.inc. The table generator --check mode
is read-only; the JSON manifest hashes raw binary and canonical LF text
separately. Six Framework codecs, complete Encoding/object APIs and the whole
standard library remain unfinished.

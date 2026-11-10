# Native binary files: compatibility evidence and remaining scope

The application implements binary file I/O in C++ using CreateFileW on Windows
and 64-bit POSIX file operations on Linux/macOS. Exclusive opens follow the
FileStream ShareNone contract; refused creation cannot truncate an existing
file. Paths and registries belong to one execution. Open handles are bounded
and released on normal completion and errors. No Python or managed runtime is
part of the application.

The development oracle calls public FileStream/BinaryReader/BinaryWriter APIs.
It never invokes the original Pascal compiler. There are 4,320 actual file
traces, including partial EOF reads, seek beyond 4 GiB, sparse gaps, interleaved
reads/writes, truncation, exclusive conflicts and close/reopen. Those traces
are byte-identical under Framework and modern .NET.

The compressed codec corpus contains 410,752 actual Framework steps. It covers
all 140 numbered encodings, every Word/SmallInt bit pattern, Byte/SByte,
arbitrary Int32/UInt32/Int64/UInt64 and IEEE Single/Double bits, malformed and
incomplete input, decoder state retained across reads/seeks, surrogate errors,
the one-character reader buffer, seven-bit string byte lengths and the writer's
distinct short/long-string encoder paths. Native tests make 489,444 assertions.
The 311 differing modern outcomes are retained separately, not silently used
to replace the Framework target. The ZIP and unpacked bytes have independent
SHA-256 integrity checks. Runtime code reads neither fixture nor archive.

The typed-file layer follows PABCSystem.pas lines 8876–9320 and RunTimeSizeOf
16796–16850, not Marshal.SizeOf. It uses a packed sum of field sizes without
alignment; a Char occupies one file byte and an enum four. All 140 encodings
are checked through compiled native Pascal: single-byte codecs round-trip nine
53-byte records containing all primitive widths, an enum and a nested record;
other codecs fail before file creation. Written bytes and decoded character
units are checked against independent public API results, which agree across
Framework and modern .NET. These recipes validate source-derived behavior,
not Pascal compiler compatibility by themselves.

The native Pascal tests also cover shared file references, read/write overloads,
byte arrays, multiple destinations, open-time encoding snapshots, reset versus
rewrite, reassignment while open, holes, truncation, exact EOF equality, invalid
lengths, negative positions, unassigned/closed files, sharing with text streams,
and corrupted operation metadata. Errors retain EN/RU text and source locations.
No new opcode or TypeInfo/Cell/Value/VM frame field is required. Binary objects
and open-file registries live on the heap.

Positive remaining requirements include fixed-array/subrange/short-string/set
file elements, all source conversions through System.Convert, mutable public
TypedFile.ElementType with its frozen element size, canonical reflection of
file types, AbstractBinaryFile/TypedFile/BinaryFile class declarations and
inheritance, and BinaryFile Serialize/Deserialize object graphs. Dynamic arrays
are write-only as in the supported source path; their use as typed-file
elements or Read targets is not claimed. This stage does not complete
PABCSystem, FilesOperations, Encoding APIs or the whole standard library.

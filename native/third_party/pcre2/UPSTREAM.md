# PCRE2 10.49

Source: [official release](https://github.com/PCRE2Project/pcre2/releases/tag/pcre2-10.49),
released 28 September 2026.

Archive: `pcre2-10.49.tar.gz`, 2,880,625 bytes.
SHA-256: `929f0b20e62879252a15886b06c89f1edef61a363cbd5826fb041080a5e557ae`.
The digest was checked against the official GitHub release asset metadata.

Only the C library sources, headers, default character tables and notices are
vendored. They are unchanged. The local CMake and source/definition manifests
build the static 16-bit C99 library with Unicode tables, LF newlines, four-byte
internal links and bounded default matching/depth/heap settings. JIT, grep,
tests, POSIX wrappers and development scripts are not built or distributed.
Generic headers and character tables are copied into the build directory;
source files are not generated or downloaded during the application build.

License: `BSD-3-Clause WITH PCRE2-exception`; the complete upstream notice is in
`LICENCE.md` and in the application's `licenses/PCRE2-LICENCE.md`.
Configuration follows the upstream `NON-AUTOTOOLS-BUILD` instructions.

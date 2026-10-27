LibTomMath 1.3.0, native multiple-precision integer arithmetic in C.

Source: https://github.com/libtom/libtommath/archive/refs/tags/v1.3.0.tar.gz

Archive SHA-256: `6d099e93ff00fa9b18346f4bcd97dcc48c3e91286f7e16c4ac5515a7171c3149`.

Root `bn_*.c`, `tommath*.h`, `sources.cmake`, LICENSE and README.md are unchanged
upstream sources. CMakeLists.txt and this file belong to the native application.
Development tools, demos, generated binaries and upstream language bindings are
excluded. Normal builds use these local sources without downloads.

The project selects 28-bit digits stored in uint32_t with uint64_t intermediates
on every platform (`MP_32BIT`), disables file IO, and fixes algorithm cutoffs.
The C++ wrapper owns immutable values; native Pascal runtime Random stays separate.
The unused upstream mp_set_double translation unit is excluded from compilation;
the wrapper performs the IEEE-754 decomposition itself, checked in C++.
BigInteger-to-real conversion follows the mantissa truncation in
System.Numerics.BigInteger and NumericsHelpers.GetDoubleFromParts:
https://raw.githubusercontent.com/dotnet/runtime/v10.0.0/src/libraries/System.Runtime.Numerics/src/System/Numerics/NumericsHelpers.cs

License: Unlicense, reproduced in LICENSE and the application's license directory.

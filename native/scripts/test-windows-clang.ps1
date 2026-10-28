param(
    [string]$LlvmBin = 'C:\Program Files\LLVM\bin',
    [switch]$BackendOnly
)
$ErrorActionPreference = 'Stop'
$nativeRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$mathRoot = Join-Path $nativeRoot 'third_party/libtommath'
$buildRoot = Join-Path $nativeRoot 'build/clang-core'
$null = New-Item -ItemType Directory -Path $buildRoot -Force
$compiler = Join-Path $LlvmBin 'clang.exe'
$cxx = Join-Path $LlvmBin 'clang++.exe'
$librarian = Join-Path $LlvmBin 'llvm-lib.exe'
$defines = @('-DMP_32BIT=', '-DMP_NO_FILE', '-DMP_FIXED_CUTOFFS', '-DMP_LOW_MEM')
$objects = @()
$sources = Get-Content -LiteralPath (Join-Path $mathRoot 'sources.cmake') | Where-Object { $_ -match '^bn_[a-z0-9_]+\.c$' -and $_ -ne 'bn_mp_set_double.c' }
foreach ($source in $sources) {
    $object = Join-Path $buildRoot ([System.IO.Path]::GetFileNameWithoutExtension($source)+'.obj')
    & $compiler -std=c99 -O2 @defines "-I$mathRoot" -c (Join-Path $mathRoot $source) -o $object
    if ($LASTEXITCODE -ne 0) { throw "C compilation failed: $source" }
    $objects += $object
}
$library = Join-Path $buildRoot 'abc-math.lib'
& $librarian "/OUT:$library" @objects
if ($LASTEXITCODE -ne 0) { throw 'C library archive failed' }
$backend = Join-Path $buildRoot 'big-integer-tests.exe'
& $cxx -std=c++20 -Wall -Wextra -Wpedantic -O2 @defines "-I$mathRoot" "-I$nativeRoot/include" (Join-Path $nativeRoot 'src/big_integer.cpp') (Join-Path $nativeRoot 'tests/big_integer_tests.cpp') $library -ladvapi32 -o $backend
if ($LASTEXITCODE -ne 0) { throw 'BigInteger C++ compilation failed' }
& $backend
if ($LASTEXITCODE -ne 0) { throw 'BigInteger backend tests failed' }
if (!$BackendOnly) {
    $regexRoot = Join-Path $nativeRoot 'third_party/pcre2'
    $regexBuild = Join-Path $buildRoot 'pcre2'
    $null = New-Item -ItemType Directory -Path $regexBuild -Force
    Copy-Item -LiteralPath (Join-Path $regexRoot 'src/config.h.generic') -Destination (Join-Path $regexBuild 'config.h')
    Copy-Item -LiteralPath (Join-Path $regexRoot 'src/pcre2.h.generic') -Destination (Join-Path $regexBuild 'pcre2.h')
    Copy-Item -LiteralPath (Join-Path $regexRoot 'src/pcre2_chartables.c.dist') -Destination (Join-Path $regexBuild 'pcre2_chartables.c')
    $regexDefines = Get-Content -LiteralPath (Join-Path $regexRoot 'definitions.cmake') | Where-Object { $_ -match '^    [A-Z][A-Z0-9_]*(=\d+)?$' } | ForEach-Object { '-D'+$_.Trim() }
    $regexSources = Get-Content -LiteralPath (Join-Path $regexRoot 'sources.cmake') | Where-Object { $_ -match '^    src/pcre2_[a-z0-9_]+\.c$' } | ForEach-Object { Join-Path $regexRoot $_.Trim() }
    $regexObjects = @()
    foreach ($source in (@($regexSources)+(Join-Path $regexBuild 'pcre2_chartables.c'))) {
        $object = Join-Path $regexBuild ([System.IO.Path]::GetFileNameWithoutExtension($source)+'.obj')
        & $compiler -std=c99 -O2 @regexDefines "-I$regexBuild" "-I$regexRoot/src" -c $source -o $object
        if ($LASTEXITCODE -ne 0) { throw "Regex C compilation failed: $source" }
        $regexObjects += $object
    }
    $regexLibrary = Join-Path $regexBuild 'abc-regex.lib'
    & $librarian "/OUT:$regexLibrary" @regexObjects
    if ($LASTEXITCODE -ne 0) { throw 'Regex C library archive failed' }
    $core = @('big_integer','big_integer_runtime','stdlib','random','console_input','console_output','formatting','files','file_moves','file_operations','text_file','stdlib_text','string_arrays','school','school_text','regex','school_ip','sequences','real_range','diagnostics','types','pascal','bytecode','vm') | ForEach-Object { Join-Path $nativeRoot "src/$_.cpp" }
    $tests = Join-Path $buildRoot 'core-tests.exe'
    & $cxx -std=c++20 -Wall -Wextra -Wpedantic -O2 -DABC_HAS_FLOAT_FROM_CHARS=1 @defines "-I$mathRoot" "-I$regexBuild" "-I$nativeRoot/include" @core (Join-Path $nativeRoot 'tests/core_tests.cpp') $library $regexLibrary -ladvapi32 -o $tests
    if ($LASTEXITCODE -ne 0) { throw 'Core C++ compilation failed' }
    & $tests
    if ($LASTEXITCODE -ne 0) { throw 'Core tests failed' }
}

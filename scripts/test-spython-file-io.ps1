[CmdletBinding()]
param(
    [ValidateSet('classic', 'net10')][string]$Runtime = 'classic',
    [string]$DotnetRoot = '',
    [string]$PackageRoot = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$runtimeRoot = if ($PackageRoot) { [IO.Path]::GetFullPath($PackageRoot) } else { $root }
$build = Join-Path $root '.codex-build'
$source = Join-Path $root 'TestSuiteAdditionalLanguages\SPythonTests\CompilationSamples\file_io_struct.pys'
$program = Join-Path $build 'file-io-test.pys'
$executable = [IO.Path]::ChangeExtension($program, '.exe')
$compiler = if ($Runtime -eq 'net10') {
    Join-Path $runtimeRoot 'bin-net10\pabcnetcclear.exe'
} else {
    Join-Path $runtimeRoot 'bin\pabcnetcclear.exe'
}

if (-not (Test-Path -LiteralPath $compiler)) { throw "Compiler not found: $compiler" }
if ($Runtime -eq 'net10') {
    if ($PackageRoot -and -not $DotnetRoot) { $DotnetRoot = Join-Path $runtimeRoot 'dotnet' }
    if (-not $DotnetRoot) {
        foreach ($candidate in @($env:DOTNET_ROOT, (Join-Path $env:USERPROFILE '.dotnet'),
                'C:\Program Files\dotnet')) {
            if ($candidate -and (Test-Path -LiteralPath (Join-Path $candidate 'shared\Microsoft.NETCore.App'))) {
                $DotnetRoot = $candidate
                break
            }
        }
    }
    if (-not $DotnetRoot) { throw 'A .NET 10 runtime is required for the net10 test.' }
    $env:DOTNET_ROOT = [IO.Path]::GetFullPath($DotnetRoot)
    $env:DOTNET_ROOT_X64 = $env:DOTNET_ROOT
}
New-Item -ItemType Directory -Path $build -Force | Out-Null
Copy-Item -LiteralPath $source -Destination $program -Force
[IO.File]::WriteAllText((Join-Path $build 'file-io-test.txt'),
    "one`r`ntwo`rthree`n", [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText((Join-Path $build 'file-io-unicode.txt'),
    "é`n", [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllBytes((Join-Path $build 'file-io-test.bin'),
    [byte[]](42, 0, 0, 0, 254, 255, 10, 97))
[IO.File]::WriteAllBytes((Join-Path $build 'file-io-float.bin'),
    [byte[]](0, 0, 192, 63, 0, 0, 0, 0, 0, 0, 248, 63, 0, 62))
[IO.File]::WriteAllBytes((Join-Path $build 'file-io-pascal.bin'),
    [byte[]](5, 72, 101, 108, 108, 111, 0, 0))

Push-Location $root
try {
    & $compiler $program
    if ($LASTEXITCODE -ne 0) { throw "SPython compilation failed: $Runtime" }
    if (-not (Test-Path -LiteralPath $executable)) { throw 'Test executable missing.' }
    $lines = if ($Runtime -eq 'net10') {
        @(& (Join-Path $env:DOTNET_ROOT 'dotnet.exe') exec $executable)
    } else {
        @(& $executable)
    }
    if ($LASTEXITCODE -ne 0) { throw "SPython test failed at runtime: $Runtime" }
    if ($lines.Count -ne 37 -or @($lines | Where-Object { $_ -ne 'True' }).Count -ne 0) {
        throw "Unexpected SPython test output ($Runtime): $($lines -join ', ')"
    }
    Write-Host "SPython file IO and struct test passed: $Runtime ($($lines.Count) assertions)."
}
finally {
    Pop-Location
}

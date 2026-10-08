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
$source = Join-Path $root 'TestSuiteAdditionalLanguages\SPythonTests\CompilationSamples\read_sbl_lambda.pys'
$program = Join-Path $build 'read-sbl-lambda-test.pys'
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
[IO.File]::WriteAllBytes((Join-Path $build 'sbl-2.bin'), [byte[]](254, 255, 44, 1))
[IO.File]::WriteAllBytes((Join-Path $build 'sbl-4.bin'), [byte[]](254, 255, 255, 255, 44, 1, 0, 0))
[IO.File]::WriteAllBytes((Join-Path $build 'sbl-8.bin'),
    [byte[]](254, 255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 0, 0, 1, 0, 0))
[IO.File]::WriteAllBytes((Join-Path $build 'sbl-odd.bin'), [byte[]](1, 2, 3))

$expected = @('-2', '300', '-2', '300', '-2', '1099511627776', '0', '0',
    '3', '13', 'ok!', '42', '7', 'caught')
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
    if ($LASTEXITCODE -ne 0) { throw "SPython program failed at runtime: $Runtime" }
    if ($lines.Count -ne $expected.Count) {
        throw "Unexpected number of output lines ($Runtime): $($lines -join ', ')"
    }
    for ($i = 0; $i -lt $expected.Count; $i++) {
        if ($lines[$i] -ne $expected[$i]) {
            throw "Unexpected line $($i + 1) ($Runtime): got '$($lines[$i])', expected '$($expected[$i])'"
        }
    }
    Write-Host "SPython readSbl and lambda test passed: $Runtime ($($expected.Count) checks)."
}
finally {
    Pop-Location
}

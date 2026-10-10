[CmdletBinding()]
param(
    [ValidateSet('classic', 'net10')][string]$Runtime = 'classic',
    [string]$DotnetRoot = '',
    [string]$PackageRoot = '',
    [string]$PythonExe = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$runtimeRoot = if ($PackageRoot) { [IO.Path]::GetFullPath($PackageRoot) } else { $root }
$source = Join-Path $root 'TestSuiteAdditionalLanguages\SPythonTests\CompilationSamples\itertools_core.pys'
$build = Join-Path $root '.codex-build'
$program = Join-Path $build 'itertools-core.pys'
$executable = [IO.Path]::ChangeExtension($program, '.exe')
$compiler = if ($Runtime -eq 'net10') {
    Join-Path $runtimeRoot 'bin-net10\pabcnetcclear.exe'
} else {
    Join-Path $runtimeRoot 'bin\pabcnetcclear.exe'
}
if (-not (Test-Path -LiteralPath $compiler)) { throw "Compiler not found: $compiler" }
if (-not $PythonExe) {
    $python = Get-Command python.exe -ErrorAction SilentlyContinue
    if ($python) { $PythonExe = $python.Source }
    else {
        $python = Get-Command py.exe -ErrorAction SilentlyContinue
        if ($python) { $PythonExe = $python.Source }
    }
}
if (-not $PythonExe) { throw 'Python 3 is required for the differential test; pass -PythonExe.' }
if (-not $DotnetRoot) {
    foreach ($candidate in @($env:DOTNET_ROOT, (Join-Path $env:USERPROFILE '.dotnet'), 'C:\Program Files\dotnet')) {
        if ($candidate -and (Test-Path -LiteralPath (Join-Path $candidate 'dotnet.exe'))) {
            $DotnetRoot = $candidate
            break
        }
    }
}
if ($Runtime -eq 'net10' -and -not $DotnetRoot) { throw 'A .NET 10 runtime is required.' }
if ($Runtime -eq 'net10') {
    $env:DOTNET_ROOT = [IO.Path]::GetFullPath($DotnetRoot)
    $env:DOTNET_ROOT_X64 = $env:DOTNET_ROOT
}

New-Item -ItemType Directory -Path $build -Force | Out-Null
Copy-Item -LiteralPath $source -Destination $program -Force
Push-Location $root
try {
    & $compiler $program
    if ($LASTEXITCODE -ne 0) { throw "SPython compilation failed: $Runtime" }
    $actual = if ($Runtime -eq 'net10') {
        @(& (Join-Path $env:DOTNET_ROOT 'dotnet.exe') exec $executable)
    } else {
        @(& $executable)
    }
    if ($LASTEXITCODE -ne 0) { throw "SPython execution failed: $Runtime" }
    $expected = @(& $PythonExe $source)
    if ($LASTEXITCODE -ne 0) { throw 'CPython execution failed.' }
    if ($actual.Count -ne $expected.Count) {
        throw "Output line count differs: SPython $($actual.Count), CPython $($expected.Count)."
    }
    for ($i = 0; $i -lt $actual.Count; $i++) {
        if (-not [string]::Equals($actual[$i], $expected[$i], [StringComparison]::Ordinal)) {
            throw "Output differs on line $($i + 1): SPython '$($actual[$i])', CPython '$($expected[$i])'."
        }
    }
    Write-Host "SPython itertools matched CPython: $Runtime ($($actual.Count) lines)."
}
finally {
    Pop-Location
}

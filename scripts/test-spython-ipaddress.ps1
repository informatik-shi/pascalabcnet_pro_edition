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
$build = Join-Path $root '.codex-build'
$compiler = if ($Runtime -eq 'net10') {
    Join-Path $runtimeRoot 'bin-net10\pabcnetcclear.exe'
} else {
    Join-Path $runtimeRoot 'bin\pabcnetcclear.exe'
}
if (-not (Test-Path -LiteralPath $compiler)) { throw "Compiler not found: $compiler" }
if (-not $PythonExe) {
    $candidate = Get-Command python.exe -ErrorAction SilentlyContinue
    if ($candidate) { $PythonExe = $candidate.Source }
}
if (-not $PythonExe) { throw 'Python 3 is required; pass -PythonExe.' }
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
Push-Location $root
try {
    foreach ($name in @('ipaddress_core', 'ipaddress_invalid', 'ipaddress_properties')) {
        $source = Join-Path $root "TestSuiteAdditionalLanguages\SPythonTests\CompilationSamples\$name.pys"
        $program = Join-Path $build "$name.pys"
        $executable = [IO.Path]::ChangeExtension($program, '.exe')
        Copy-Item -LiteralPath $source -Destination $program -Force
        & $compiler $program
        if ($LASTEXITCODE -ne 0) { throw "SPython compilation failed: $Runtime, $name" }
        $actual = if ($Runtime -eq 'net10') {
            @(& (Join-Path $env:DOTNET_ROOT 'dotnet.exe') exec $executable)
        } else {
            @(& $executable)
        }
        if ($LASTEXITCODE -ne 0) { throw "SPython execution failed: $Runtime, $name" }
        $expected = @(& $PythonExe $source)
        if ($LASTEXITCODE -ne 0) { throw "CPython execution failed: $name" }
        if ($actual.Count -ne $expected.Count) {
            throw "Line count differs in $name`: SPython $($actual.Count), CPython $($expected.Count)."
        }
        for ($i = 0; $i -lt $actual.Count; $i++) {
            if (-not [string]::Equals($actual[$i], $expected[$i], [StringComparison]::Ordinal)) {
                throw "Mismatch in $name line $($i + 1): SPython '$($actual[$i])', CPython '$($expected[$i])'."
            }
        }
        Write-Host "SPython ipaddress matched CPython: $Runtime, $name ($($actual.Count) lines)."
    }
}
finally {
    Pop-Location
}

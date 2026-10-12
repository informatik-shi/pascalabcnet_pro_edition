[CmdletBinding()]
param(
    [string]$BindHost = '0.0.0.0',
    [ValidateRange(1,65535)][int]$Port = 8080,
    [string]$PackageRoot = 'Release\PascalABCNET-Portable-win-x64',
    [string]$DataDir = 'ClassroomServer\data',
    [switch]$Setup
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$venv = [IO.Path]::GetFullPath((Join-Path $root '.codex-build\classroom-venv'))
if (-not $venv.StartsWith($root + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw "Unexpected virtual environment path: $venv"
}
$python = Join-Path $venv 'Scripts\python.exe'
$compiler = [IO.Path]::GetFullPath((Join-Path $root $PackageRoot))
$compiler = Join-Path $compiler 'bin-net10'
if (-not (Test-Path -LiteralPath (Join-Path $compiler 'pabcnetcclear.dll') -PathType Leaf)) {
    throw "Compiler missing: $compiler. Build the portable package first."
}
$data = [IO.Path]::GetFullPath((Join-Path $root $DataDir))
if (-not (Test-Path -LiteralPath $python -PathType Leaf)) {
    py -3 -m venv $venv
    if ($LASTEXITCODE -ne 0) { throw 'Could not create the Python virtual environment.' }
    $Setup = $true
}
if ($Setup) {
    & $python -m pip install -r (Join-Path $root 'ClassroomServer\requirements.txt')
    if ($LASTEXITCODE -ne 0) { throw 'Could not install Flask and Waitress.' }
}
& $python -c 'import flask, waitress'
if ($LASTEXITCODE -ne 0) {
    throw 'Server dependencies are missing. Rerun with -Setup.'
}
Write-Host "Python venv: $venv"
Write-Host "Compiler: $compiler"
Write-Host "Data: $data"
Push-Location -LiteralPath $root
try {
    & $python -m ClassroomServer --data $data serve --host $BindHost --port $Port --judge local --compiler $compiler
    if ($LASTEXITCODE -ne 0) { throw "Classroom server exited with code $LASTEXITCODE" }
}
finally {
    Pop-Location
}

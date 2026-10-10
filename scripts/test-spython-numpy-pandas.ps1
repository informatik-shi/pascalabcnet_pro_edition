[CmdletBinding()]
param(
    [ValidateSet('classic', 'net10')][string]$Runtime = 'classic',
    [string]$PackageRoot = '',
    [string]$PythonExe = '',
    [string]$DotnetRoot = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$package = if ($PackageRoot) { [IO.Path]::GetFullPath($PackageRoot) } else { $root }
$compiler = if ($Runtime -eq 'net10') {
    Join-Path $package 'bin-net10\pabcnetcclear.exe'
} else { Join-Path $package 'bin\pabcnetcclear.exe' }
if (-not (Test-Path -LiteralPath $compiler)) { throw "Compiler not found: $compiler" }
if (-not $PythonExe) {
    $bundled = Join-Path $package 'python\python.exe'
    if (Test-Path -LiteralPath $bundled) { $PythonExe = $bundled }
    else { $PythonExe = (Get-Command python.exe -ErrorAction Stop).Source }
}
$env:PABCNET_ROOT = $package
$env:PABC_PYTHON_EXE = [IO.Path]::GetFullPath($PythonExe)
$env:PABC_PYTHON_BRIDGE = Join-Path $package 'bin\Lib\SPython\matplotlib_bridge.py'
$testPackages = Join-Path $root '.codex-build\python-matplotlib\site-packages'
if (-not $PackageRoot -and (Test-Path -LiteralPath $testPackages)) {
    $env:PYTHONPATH = $testPackages
} else {
    Remove-Item Env:PYTHONPATH -ErrorAction SilentlyContinue
}
if ($Runtime -eq 'net10') {
    if (-not $DotnetRoot) { $DotnetRoot = Join-Path $env:USERPROFILE '.dotnet' }
    $env:DOTNET_ROOT = [IO.Path]::GetFullPath($DotnetRoot)
    $env:DOTNET_ROOT_X64 = $env:DOTNET_ROOT
}
$env:MPLBACKEND = 'Agg'
$source = Join-Path $root 'TestSuiteAdditionalLanguages\SPythonTests\CompilationSamples\numpy_pandas_plot.pys'
$build = Join-Path $root ".codex-build\numpy-pandas-tests\$Runtime"
$reference = Join-Path $root '.codex-build\numpy-pandas-tests\cpython'
New-Item -ItemType Directory -Path $build, $reference -Force | Out-Null
$program = Join-Path $build 'numpy_pandas_plot.pys'
Copy-Item -LiteralPath $source -Destination $program -Force
& $compiler $program
if ($LASTEXITCODE -ne 0) { throw 'SPython compilation failed.' }
Push-Location $build
try {
    $executable = [IO.Path]::ChangeExtension($program, '.exe')
    if ($Runtime -eq 'net10') {
        $spython = & (Join-Path $env:DOTNET_ROOT 'dotnet.exe') exec $executable 2>&1
    } else { $spython = & $executable 2>&1 }
    if ($LASTEXITCODE -ne 0) { throw "SPython execution failed: $spython" }
} finally { Pop-Location }
Push-Location $reference
try {
    $cpython = & $env:PABC_PYTHON_EXE $source 2>&1
    if ($LASTEXITCODE -ne 0) { throw "CPython execution failed: $cpython" }
} finally { Pop-Location }
if (($spython -join "`n").Trim() -ne ($cpython -join "`n").Trim()) {
    throw "Output differs from CPython. SPython: $spython; CPython: $cpython"
}
foreach ($name in @('numpy_pandas_plot.csv', 'numpy_pandas_plot.png')) {
    $actual = Join-Path $build $name
    $expected = Join-Path $reference $name
    if (-not (Test-Path -LiteralPath $actual) -or -not (Test-Path -LiteralPath $expected) -or
        (Get-FileHash -LiteralPath $actual -Algorithm SHA256).Hash -ne
        (Get-FileHash -LiteralPath $expected -Algorithm SHA256).Hash) {
        throw "File differs from CPython: $name"
    }
}
Write-Host "PASS: $Runtime NumPy/pandas output, CSV, and PNG match CPython"

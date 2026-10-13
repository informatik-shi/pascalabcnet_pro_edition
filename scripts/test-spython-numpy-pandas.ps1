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
$testPackages = Join-Path $root '.abcpro-build\python-matplotlib\site-packages-native-spython'
if (-not (Test-Path -LiteralPath $testPackages)) {
    $testPackages = Join-Path $root '.abcpro-build\python-matplotlib\site-packages'
}
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
$build = Join-Path $root ".abcpro-build\numpy-pandas-tests\$Runtime"
$reference = Join-Path $root '.abcpro-build\numpy-pandas-tests\cpython'
New-Item -ItemType Directory -Path $build, $reference -Force | Out-Null
$nativeSource = Join-Path $root 'TestSuiteAdditionalLanguages\SPythonTests\CompilationSamples\numpy_pandas_native.pys'
$nativeProgram = Join-Path $build 'numpy_pandas_native.pys'
Copy-Item -LiteralPath $nativeSource -Destination $nativeProgram -Force
& $compiler $nativeProgram
if ($LASTEXITCODE -ne 0) { throw 'Native NumPy/pandas compilation failed.' }
$realPythonExe = $env:PABC_PYTHON_EXE
$env:PABC_PYTHON_EXE = Join-Path $build 'python-must-not-run.exe'
Push-Location $build
try {
    $nativeExecutable = [IO.Path]::ChangeExtension($nativeProgram, '.exe')
    if ($Runtime -eq 'net10') {
        $nativeOutput = @(& (Join-Path $env:DOTNET_ROOT 'dotnet.exe') exec $nativeExecutable 2>&1)
    } else { $nativeOutput = @(& $nativeExecutable 2>&1) }
    if ($LASTEXITCODE -ne 0) { throw "Native NumPy/pandas execution failed: $nativeOutput" }
} finally {
    Pop-Location
    $env:PABC_PYTHON_EXE = $realPythonExe
}
$expectedNative = @(
    '(2, 2)', 'int64', '[[1, 2], [3, 4]]', '[[11, 22], [13, 24]]',
    '[4 6]', '2.5', '[1, 3, 5]', '[0, 0.5, 1]', '[1, 2, 3]',
    '[[1, 3], [2, 4]]', '(3, 2)', '60', '20', '20',
    '[11, 21, 31]', '[10, 20, 30]', '(6, 2)', '(1, 3)', '[9]'
)
if (($nativeOutput -join "`n").Trim() -ne ($expectedNative -join "`n")) {
    throw "Native NumPy/pandas results differ: $nativeOutput"
}
$expectedCsv = @('x,y', '1,10', '2,20', '3,30')
$actualCsv = @(Get-Content -LiteralPath (Join-Path $build 'numpy_pandas_native.csv'))
if (($actualCsv -join "`n") -ne ($expectedCsv -join "`n")) {
    throw "Native pandas CSV differs: $actualCsv"
}
Write-Host "PASS: $Runtime native NumPy/pandas runs without Python"

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
    $cpython = & $env:PABC_PYTHON_EXE -c 'import matplotlib.pyplot as plt; plt.plot(range(5), [0, 2, 4, 6, 8]); plt.savefig("numpy_pandas_plot.png")' 2>&1
    if ($LASTEXITCODE -ne 0) { throw "Matplotlib reference failed: $cpython" }
} finally { Pop-Location }
if (($spython -join "`n") -notmatch 'numpy pandas ok') {
    throw "Plotting sample did not complete: SPython: $spython"
}
$plotCsv = @(Get-Content -LiteralPath (Join-Path $build 'numpy_pandas_plot.csv'))
if (($plotCsv -join "`n") -ne (@('x,y', '0,0', '1,2', '2,4', '3,6', '4,8') -join "`n")) {
    throw "Plotting sample CSV differs: $plotCsv"
}
$actualPng = Join-Path $build 'numpy_pandas_plot.png'
$expectedPng = Join-Path $reference 'numpy_pandas_plot.png'
if (-not (Test-Path -LiteralPath $actualPng) -or -not (Test-Path -LiteralPath $expectedPng) -or
    (Get-FileHash -LiteralPath $actualPng -Algorithm SHA256).Hash -ne
    (Get-FileHash -LiteralPath $expectedPng -Algorithm SHA256).Hash) {
    throw 'Matplotlib PNG differs from reference.'
}
Write-Host "PASS: $Runtime native data exports and Matplotlib PNG match reference"

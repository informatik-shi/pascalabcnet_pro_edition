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
if (-not $DotnetRoot) {
    foreach ($candidate in @($env:DOTNET_ROOT, (Join-Path $env:USERPROFILE '.dotnet'),
            'C:\Program Files\dotnet')) {
        if ($candidate -and (Test-Path -LiteralPath (Join-Path $candidate 'dotnet.exe'))) {
            $DotnetRoot = $candidate
            break
        }
    }
}
$env:PABCNET_ROOT = $package
$env:PABC_PYTHON_EXE = [IO.Path]::GetFullPath($PythonExe)
$env:PABC_PYTHON_BRIDGE = Join-Path $package 'bin\Lib\SPython\matplotlib_bridge.py'
if (-not (Test-Path -LiteralPath $env:PABC_PYTHON_BRIDGE)) {
    throw "Bridge not found: $env:PABC_PYTHON_BRIDGE"
}
$testPackages = Join-Path $root '.codex-build\python-packages'
if (-not $PackageRoot -and (Test-Path -LiteralPath $testPackages)) {
    $env:PYTHONPATH = $testPackages
}
if ($Runtime -eq 'net10') {
    if (-not $DotnetRoot) { throw 'A .NET 10 runtime is required.' }
    $env:DOTNET_ROOT = [IO.Path]::GetFullPath($DotnetRoot)
    $env:DOTNET_ROOT_X64 = $env:DOTNET_ROOT
}
Add-Type -AssemblyName System.Drawing
$build = Join-Path $root '.codex-build\matplotlib-tests'
New-Item -ItemType Directory -Path $build -Force | Out-Null

foreach ($name in @('matplotlib_pyplot', 'matplotlib_axes', 'matplotlib_imports')) {
    $source = Join-Path $root "TestSuiteAdditionalLanguages\SPythonTests\CompilationSamples\$name.pys"
    $case = Join-Path $build "$Runtime-$name"
    New-Item -ItemType Directory -Path $case -Force | Out-Null
    $program = Join-Path $case "$name.pys"
    Copy-Item -LiteralPath $source -Destination $program -Force
    & $compiler $program
    if ($LASTEXITCODE -ne 0) { throw "SPython compilation failed: $Runtime, $name" }
    $executable = [IO.Path]::ChangeExtension($program, '.exe')
    Push-Location $case
    try {
        if ($Runtime -eq 'net10') {
            $output = & (Join-Path $env:DOTNET_ROOT 'dotnet.exe') exec $executable 2>&1
        } else { $output = & $executable 2>&1 }
        if ($LASTEXITCODE -ne 0) { throw "SPython execution failed: $output" }
        $imagePath = Join-Path $case "$name.png"
        if (-not (Test-Path -LiteralPath $imagePath)) { throw "No image: $imagePath" }
        $bitmap = [Drawing.Bitmap]::FromFile($imagePath)
        try {
            $width = $bitmap.Width
            $height = $bitmap.Height
            if ($width -lt 300 -or $height -lt 200) {
                throw "Image is too small: ${width}x${height}"
            }
        } finally { $bitmap.Dispose() }
        if (($output -join "`n") -notlike '*matplotlib * ok*') {
            throw "Missing program output: $output"
        }
        $reference = Join-Path $build "cpython-$name"
        New-Item -ItemType Directory -Path $reference -Force | Out-Null
        Push-Location $reference
        try {
            $env:MPLBACKEND = 'Agg'
            $pythonOutput = & $env:PABC_PYTHON_EXE $source 2>&1
            if ($LASTEXITCODE -ne 0) { throw "CPython execution failed: $pythonOutput" }
        } finally { Pop-Location }
        $referenceImage = Join-Path $reference "$name.png"
        if (-not (Test-Path -LiteralPath $referenceImage) -or
            (Get-FileHash -LiteralPath $imagePath -Algorithm SHA256).Hash -ne
            (Get-FileHash -LiteralPath $referenceImage -Algorithm SHA256).Hash) {
            throw "PNG differs from CPython: $name"
        }
        Write-Host "PASS: $Runtime / $name / PNG ${width}x${height} matches CPython"
    } finally { Pop-Location }
}

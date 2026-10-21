param(
    [Parameter(Mandatory = $true)][string]$QtRoot,
    [Parameter(Mandatory = $true)][string]$TclRoot,
    [Parameter(Mandatory = $true)][string]$TclBin,
    [string]$CMake = 'cmake'
)

$ErrorActionPreference = 'Stop'
$nativeRoot = Split-Path -Parent $PSScriptRoot
$cmakeCommand = (Get-Command $CMake -ErrorAction Stop).Source
$ctestCommand = Join-Path (Split-Path -Parent $cmakeCommand) 'ctest.exe'
$distRoot = Join-Path $nativeRoot 'dist'
New-Item -ItemType Directory -Force -Path $distRoot | Out-Null

function Invoke-BuildTool([string]$Tool, [string[]]$Arguments) {
    & $Tool @Arguments | Where-Object { $_ -notmatch '^-- (Installing|Up-to-date):' }
    if ($LASTEXITCODE -ne 0) { throw "$Tool failed with exit code $LASTEXITCODE" }
}

$variants = @(
    @{ Name = 'Qt'; Directory = 'qt-editor'; Target = 'abc-ide.exe'; Options = @('-DABC_BUILD_IDE=ON', "-DCMAKE_PREFIX_PATH=$QtRoot") },
    @{ Name = 'TclTk'; Directory = 'tk'; Target = 'abc-tk.exe'; Options = @(
        '-DABC_BUILD_TK=ON', "-DABC_TCL_LIBRARY=$TclRoot/tcl86t.lib", "-DABC_TK_LIBRARY=$TclRoot/tk86t.lib",
        "-DABC_TCL_RUNTIME_DIR=$TclRoot", "-DABC_TCL_DLL=$TclBin/tcl86t.dll", "-DABC_TK_DLL=$TclBin/tk86t.dll"
    ) }
)

$results = @()
foreach ($variant in $variants) {
    $buildDir = Join-Path $nativeRoot "build/$($variant.Directory)"
    $packageDir = Join-Path $distRoot $variant.Name
    $zipPath = Join-Path $distRoot "ABC-Native-$($variant.Name)-win-x64.zip"
    Invoke-BuildTool $cmakeCommand (@('-S', $nativeRoot, '-B', $buildDir, '-G', 'Visual Studio 17 2022', '-A', 'x64') + $variant.Options)
    Invoke-BuildTool $cmakeCommand @('--build', $buildDir, '--config', 'Release', '--parallel', '6')
    Invoke-BuildTool $ctestCommand @('--test-dir', $buildDir, '-C', 'Release', '--output-on-failure')
    Invoke-BuildTool $cmakeCommand @('--install', $buildDir, '--config', 'Release', '--prefix', $packageDir)
    $launcher = "@echo off`r`nstart `"`" `"%~dp0bin\$($variant.Target)`"`r`n"
    Set-Content -LiteralPath (Join-Path $packageDir 'Launch.cmd') -Value $launcher -Encoding ascii
    $startText = @"
ABC Native — $($variant.Name)

Распакуйте архив целиком и запустите Launch.cmd.
F8 — проверить программу. F9 — запустить.
Введите число в строке под консолью и нажмите Enter, когда программа ждёт Readln.
Кнопка «Остановить» завершает выполняемую программу.

Это экспериментальный прототип: поддерживаются integer, int64, real/double,
boolean, string, условия if и циклы while. Поддержка PascalABC.NET пока частичная.
Установка .NET, Python, Qt или Tcl/Tk для запуска этого пакета не требуется.
"@
    Set-Content -LiteralPath (Join-Path $packageDir 'START-HERE.txt') -Value $startText -Encoding utf8
    Set-Content -LiteralPath (Join-Path $packageDir 'README.md') -Value $startText -Encoding utf8
    Compress-Archive -Path (Join-Path $packageDir '*') -DestinationPath $zipPath -Force
    $files = Get-ChildItem -LiteralPath $packageDir -File -Recurse
    $results += [pscustomobject]@{
        variant = $variant.Name
        runtimeBytes = ($files | Measure-Object -Property Length -Sum).Sum
        archiveBytes = (Get-Item -LiteralPath $zipPath).Length
        files = $files.Count
    }
}
$results | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $distRoot 'comparison-sizes.json') -Encoding utf8
$results | Format-Table

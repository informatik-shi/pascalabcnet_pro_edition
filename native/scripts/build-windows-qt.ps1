param(
    [Parameter(Mandatory = $true)][string]$QtRoot,
    [string]$CMake = 'cmake'
)

$ErrorActionPreference = 'Stop'
$nativeRoot = Split-Path -Parent $PSScriptRoot
$cmakeCommand = (Get-Command $CMake -ErrorAction Stop).Source
$ctestCommand = Join-Path (Split-Path -Parent $cmakeCommand) 'ctest.exe'
$buildDir = Join-Path $nativeRoot 'build/qt-editor'
$packageDir = Join-Path $nativeRoot 'dist/Qt'
$zipPath = Join-Path $nativeRoot 'dist/ABC-Native-Qt-win-x64.zip'

function Invoke-BuildTool([string]$Tool, [string[]]$Arguments) {
    & $Tool @Arguments | Where-Object { $_ -notmatch '^-- (Installing|Up-to-date):' }
    if ($LASTEXITCODE -ne 0) { throw "$Tool failed with exit code $LASTEXITCODE" }
}

Invoke-BuildTool $cmakeCommand @('-S', $nativeRoot, '-B', $buildDir,
    '-G', 'Visual Studio 17 2022', '-A', 'x64', '-DABC_BUILD_IDE=ON', '-DABC_BUILD_TK=OFF',
    '-DBUILD_TESTING=ON', "-DCMAKE_PREFIX_PATH=$QtRoot")
Invoke-BuildTool $cmakeCommand @('--build', $buildDir, '--config', 'Release', '--parallel', '6')
Invoke-BuildTool $ctestCommand @('--test-dir', $buildDir, '-C', 'Release', '--output-on-failure')
Invoke-BuildTool $cmakeCommand @('--install', $buildDir, '--config', 'Release', '--prefix', $packageDir)
Set-Content -LiteralPath (Join-Path $packageDir 'Launch.cmd') -Encoding ascii -Value "@echo off`r`nstart `"`" `"%~dp0bin\abc-ide.exe`"`r`n"
$startText = @"
ABC Native — Qt — экспериментальная версия

Распакуйте архив целиком и запустите Launch.cmd.
Ночная тема включена по умолчанию.
F8 — проверить программу. F9 — запустить.
Когда программа ждёт Readln, введите значение в строке под консолью и нажмите Enter.
Кнопка «Остановить» завершает выполняемую программу.
Ошибки выполнения показываются на английском (EN) и русском (RU),
с кодом ошибки, строкой и столбцом исходника.

Поддерживаются integer, int64, real/double, boolean, string,
динамические массивы, записи, пользовательские функции и процедуры с var-параметрами,
условия if и циклы while, базовая математика, строки, List<T>, Stack<T>, Queue<T>,
DynArray<T>, SimpleSet<T>, AssocArray<K,V>, LinkedList<T>, LinkedListNode<T>, SingleNode<T> и текстовые файлы UTF-8.
Для классов Collections добавьте uses Collections;
Примеры: examples/structures.pas, examples/files.pas, examples/collections.pas, examples/buffers.pas,
examples/associations.pas, examples/linked-lists.pas, examples/single-nodes.pas, examples/standard-library.pas,
examples/text-numbers.pas, examples/string-editing.pas, examples/nil-strings.pas.
Строковый nil отличается от пустой строки; стандартные строковые операции учитывают это различие.
Совместимость с PascalABC.NET частичная.
Установка .NET, Python или Qt для запуска этого пакета не требуется.
"@
Set-Content -LiteralPath (Join-Path $packageDir 'START-HERE.txt') -Value $startText -Encoding utf8
Set-Content -LiteralPath (Join-Path $packageDir 'README.md') -Value $startText -Encoding utf8
Invoke-BuildTool $cmakeCommand @("-DABC_PACKAGE_DIR=$packageDir", '-P', (Join-Path $nativeRoot 'cmake/VerifyNoPython.cmake'))
Compress-Archive -Path (Join-Path $packageDir '*') -DestinationPath $zipPath -Force
[pscustomobject]@{
    variant = 'Qt'
    runtimeBytes = (Get-ChildItem -LiteralPath $packageDir -File -Recurse | Measure-Object -Property Length -Sum).Sum
    archiveBytes = (Get-Item -LiteralPath $zipPath).Length
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $nativeRoot 'dist/qt-package.json') -Encoding utf8
Write-Output $zipPath

[CmdletBinding()]
param(
    [string]$PackageRoot = 'Release\PascalABCNET-Portable-win-x64'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..')).TrimEnd('\')
$package = [IO.Path]::GetFullPath((Join-Path $root $PackageRoot))
$compiler = Join-Path $package 'bin-net10\pabcnetcclear.dll'
if (-not (Test-Path -LiteralPath $compiler -PathType Leaf)) {
    throw "Compiler missing: $compiler"
}
$stage = [IO.Path]::GetFullPath((Join-Path $root ('.abcpro-build\classroom-package-' + [guid]::NewGuid().ToString('N'))))
$outputDir = [IO.Path]::GetFullPath((Join-Path $root 'Release-classroom'))
$zip = Join-Path $outputDir 'abc-pro-classroom-local-win-x64.zip'
if (-not $stage.StartsWith($root + '\', [StringComparison]::OrdinalIgnoreCase) -or
    -not $outputDir.StartsWith($root + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Unexpected packaging paths'
}
New-Item -ItemType Directory -Path $stage -Force | Out-Null
try {
    $server = Join-Path $stage 'ClassroomServer'
    $scripts = Join-Path $stage 'scripts'
    $docs = Join-Path $stage 'docs'
    $compilerTarget = Join-Path $stage 'Release\PascalABCNET-Portable-win-x64'
    New-Item -ItemType Directory -Path $server,$scripts,$docs,$compilerTarget -Force | Out-Null
    Copy-Item -Path (Join-Path $root 'ClassroomServer\*.py') -Destination $server
    Copy-Item -LiteralPath (Join-Path $root 'ClassroomServer\requirements.txt') -Destination $server
    Copy-Item -LiteralPath (Join-Path $root 'ClassroomServer\templates') -Destination $server -Recurse
    Copy-Item -LiteralPath (Join-Path $root 'scripts\start-classroom-server.ps1') -Destination $scripts
    Copy-Item -LiteralPath (Join-Path $root 'docs\classroom.md') -Destination $docs
    Copy-Item -LiteralPath (Join-Path $root 'docs\classroom-no-docker.md') -Destination $docs
    Copy-Item -LiteralPath (Join-Path $package 'bin-net10') -Destination $compilerTarget -Recurse
    @'
Сервер класса abc.pro без Docker

Требования: Windows, Python 3.10 или новее, среда выполнения .NET 10.
Распакуйте архив в доступную для записи папку и откройте там PowerShell.

Первый запуск:
  py -3 -m venv .abcpro-build\classroom-venv
  .\.abcpro-build\classroom-venv\Scripts\python.exe -m pip install -r ClassroomServer\requirements.txt
  .\.abcpro-build\classroom-venv\Scripts\python.exe -m ClassroomServer init-admin teacher
  powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\start-classroom-server.ps1

Следующие запуски:
  powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\start-classroom-server.ps1

Откройте http://127.0.0.1:8080/admin и войдите как преподаватель.
Для учеников сообщите IP-адрес компьютера преподавателя в локальной сети.
Данные находятся в ClassroomServer\data. Храните резервную копию отдельно.
Подробная инструкция и ограничения: docs\classroom-no-docker.md.
'@ | Set-Content -LiteralPath (Join-Path $stage 'README.txt') -Encoding UTF8
    New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
    Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -Force
    Write-Host "Classroom package: $zip"
}
finally {
    if (Test-Path -LiteralPath $stage) {
        $resolved = [IO.Path]::GetFullPath($stage)
        if (-not $resolved.StartsWith($root + '\', [StringComparison]::OrdinalIgnoreCase)) {
            throw "Unsafe cleanup path: $resolved"
        }
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
}

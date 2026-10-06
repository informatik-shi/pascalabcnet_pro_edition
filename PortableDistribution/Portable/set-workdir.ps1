param([Parameter(Mandatory = $true)][string]$Root)
$ErrorActionPreference = 'Stop'
$rootPath = [IO.Path]::GetFullPath($Root)
$work = Join-Path $rootPath 'Work'
$bin = Join-Path $rootPath 'bin'
New-Item -ItemType Directory -Path $work -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $work 'Output') -Force | Out-Null
[IO.File]::WriteAllText((Join-Path $bin 'pabcworknet.ini'), $work,
    (New-Object System.Text.UTF8Encoding($false)))

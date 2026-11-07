# Development-only independent oracle, never installed.
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$program=Join-Path $projectRoot 'native/build/equality-reference.exe'
& $compiler /nologo /r:System.Numerics.dll /target:exe /out:$program (Join-Path $PSScriptRoot 'equality.cs')
if($LASTEXITCODE -ne 0){throw 'Reference compilation failed'}
& $program (Join-Path $PSScriptRoot '../fixtures/equality-reference.tsv')
if($LASTEXITCODE -ne 0){throw 'Reference generation failed'}

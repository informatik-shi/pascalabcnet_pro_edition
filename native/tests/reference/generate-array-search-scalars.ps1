# Development-only independent oracle, never installed.
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$program=Join-Path $projectRoot 'native/build/array-search-scalars-reference.exe'
& $compiler /nologo /target:exe /out:$program (Join-Path $PSScriptRoot 'array-search-scalars.cs')
if($LASTEXITCODE -ne 0){throw 'Reference compilation failed'}
& $program (Join-Path $PSScriptRoot '../fixtures/array-search-scalars-reference.tsv')
if($LASTEXITCODE -ne 0){throw 'Reference generation failed'}

# Development-only independent oracle, never installed.
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$program=Join-Path $projectRoot 'native/build/sequence-sets-reference.exe'
& $compiler /nologo /target:exe /out:$program (Join-Path $PSScriptRoot 'sequence-sets.cs')
if($LASTEXITCODE -ne 0){throw 'Reference compilation failed'}
& $program (Join-Path $PSScriptRoot '../fixtures/sequence-sets-reference.tsv')
if($LASTEXITCODE -ne 0){throw 'Reference generation failed'}

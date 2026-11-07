# Development-only independent oracle, never installed.
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$program=Join-Path $projectRoot 'native/build/dictionary-value-equality-reference.exe'
& $compiler /nologo /target:exe /out:$program (Join-Path $PSScriptRoot 'dictionary-value-equality.cs')
if($LASTEXITCODE -ne 0){throw 'Reference compilation failed'}
& $program (Join-Path $PSScriptRoot '../fixtures/dictionary-value-equality-reference.tsv')
if($LASTEXITCODE -ne 0){throw 'Reference generation failed'}

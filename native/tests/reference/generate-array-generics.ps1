# Development-only independent recipes, never installed.
param([string]$RuntimeHost=(Join-Path $env:USERPROFILE '.dotnet/dotnet.exe'))
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$program=Join-Path $projectRoot 'native/build/array-generics-reference.exe'
& $compiler /nologo /target:exe /out:$program (Join-Path $PSScriptRoot 'array-generics.cs')
if($LASTEXITCODE -ne 0){throw 'Reference compilation failed'}
$config='{"runtimeOptions":{"tfm":"net10.0","framework":{"name":"Microsoft.NETCore.App","version":"10.0.0"}}}'
[IO.File]::WriteAllText([IO.Path]::ChangeExtension($program,'.runtimeconfig.json'),$config,[Text.UTF8Encoding]::new($false))
& $RuntimeHost $program (Join-Path $PSScriptRoot '../fixtures/array-generics-reference.tsv')
if($LASTEXITCODE -ne 0){throw 'Reference generation failed'}

# Development-only independent oracle, never installed.
param([string]$RuntimeHost=(Join-Path $env:USERPROFILE '.dotnet/dotnet.exe'))
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$program=Join-Path $projectRoot 'native/build/file-operations-reference.exe'
& $compiler /nologo /target:exe /out:$program (Join-Path $PSScriptRoot 'file-operations.cs')
if($LASTEXITCODE -ne 0){throw 'Reference compilation failed'}
if(!(Test-Path -LiteralPath $RuntimeHost)){throw 'Specify the .NET 10 host for development reference generation'}
$runtimeConfig='{"runtimeOptions":{"tfm":"net10.0","framework":{"name":"Microsoft.NETCore.App","version":"10.0.0"}}}'
[IO.File]::WriteAllText([IO.Path]::ChangeExtension($program,'.runtimeconfig.json'),$runtimeConfig,[Text.UTF8Encoding]::new($false))
& $RuntimeHost $program (Join-Path $PSScriptRoot '../fixtures/file-operations-reference.tsv') (Join-Path $projectRoot 'native/build')
if($LASTEXITCODE -ne 0){throw 'Reference generation failed'}

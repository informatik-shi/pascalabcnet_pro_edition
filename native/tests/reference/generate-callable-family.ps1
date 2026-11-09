# Development only; no original Pascal compiler.
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$program=Join-Path $projectRoot 'native/build/callable-family-reference.exe'
& $compiler /nologo /target:exe "/out:$program" (Join-Path $PSScriptRoot 'callable-family.cs')
if($LASTEXITCODE -ne 0){throw 'Delegate reference compilation failed'}
& $program (Join-Path $PSScriptRoot '../fixtures/callable-family-reference.tsv')
if($LASTEXITCODE -ne 0){throw 'Delegate reference generation failed'}
function Get-CanonicalHash([string]$Path){$hash=[Security.Cryptography.SHA256]::Create();try{return ([BitConverter]::ToString($hash.ComputeHash([Text.Encoding]::UTF8.GetBytes([IO.File]::ReadAllText($Path).Replace("`r`n","`n"))))).Replace('-','').ToLowerInvariant()}finally{$hash.Dispose()}}
$files=@('reference/callable-family.cs','reference/generate-callable-family.ps1','fixtures/callable-family-program.pas','fixtures/callable-family-reference.tsv','fixtures/callable-family-reference.md')
$metadata=[ordered]@{schema=1;runtime='Windows .NET Framework 4.0.30319.42000';outcomes=504;scope='Actual CLR Predicate/Func type identity, invocation, wrapper-target equality, collections, nil and source-derived conditional conversion guards. Not full delegates, reflection or library coverage.';artifacts=@($files|ForEach-Object{[ordered]@{path=$_;sha256_lf=Get-CanonicalHash (Join-Path $PSScriptRoot "../$_")}})}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '../fixtures/callable-family-reference.json'),(($metadata|ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))

# Development-only metadata oracle; no original Pascal compiler is invoked.
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$program=Join-Path $projectRoot 'native/build/type-properties-reference.exe'
& $compiler /nologo /target:exe /r:System.Numerics.dll "/out:$program" (Join-Path $PSScriptRoot 'type-properties.cs')
if($LASTEXITCODE -ne 0){throw 'Metadata reference compilation failed'}
& $program (Join-Path $PSScriptRoot '../fixtures/type-properties-reference.tsv')
if($LASTEXITCODE -ne 0){throw 'Metadata reference generation failed'}
function Get-CanonicalHash([string]$Path){$hash=[Security.Cryptography.SHA256]::Create();try{return ([BitConverter]::ToString($hash.ComputeHash([Text.Encoding]::UTF8.GetBytes([IO.File]::ReadAllText($Path).Replace("`r`n","`n"))))).Replace('-','').ToLowerInvariant()}finally{$hash.Dispose()}}
$files=@('reference/type-properties.cs','reference/generate-type-properties.ps1','fixtures/type-properties-reference.tsv','fixtures/type-properties-reference.md')
$metadata=[ordered]@{schema=1;runtime='Windows .NET Framework 4.0.30319.42000';outcomes=736;scope='Positive requirements for raw CLR type properties over 92 type expressions and two source modules. Actual Framework metadata and source-derived emitted type identities; not a claim of native property implementation or full library coverage.';artifacts=@($files|ForEach-Object{[ordered]@{path=$_;sha256_lf=Get-CanonicalHash (Join-Path $PSScriptRoot "../$_")}})}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '../fixtures/type-properties-reference.json'),(($metadata|ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))

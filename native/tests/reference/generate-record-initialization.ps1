# Development only. The original Pascal compiler is never invoked.
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$program=Join-Path $projectRoot 'native/build/record-initialization-reference.exe'
& $compiler /nologo /target:exe "/out:$program" (Join-Path $PSScriptRoot 'record-initialization.cs')
if($LASTEXITCODE -ne 0){throw 'Reference compilation failed'}
& $program (Join-Path $PSScriptRoot '../fixtures/record-initialization-reference.tsv')
if($LASTEXITCODE -ne 0){throw 'Reference generation failed'}
function Get-CanonicalHash([string]$Path){$hash=[Security.Cryptography.SHA256]::Create();try{return ([BitConverter]::ToString($hash.ComputeHash([Text.Encoding]::UTF8.GetBytes([IO.File]::ReadAllText($Path).Replace("`r`n","`n"))))).Replace('-','').ToLowerInvariant()}finally{$hash.Dispose()}}
$template=[IO.File]::ReadAllText((Join-Path $PSScriptRoot '../fixtures/record-initialization-program.pas.in'))
$files=@('reference/record-initialization.cs','reference/generate-record-initialization.ps1','fixtures/record-initialization-program.pas.in','fixtures/record-initialization-reference.tsv')
foreach($variant in 'typed','inferred'){
 $source=if($variant -eq 'typed'){$template.Replace('%INTEGER%',':integer').Replace('%ARRAY%',':array of integer')}else{$template.Replace('%INTEGER%','').Replace('%ARRAY%','')}
 $file="fixtures/record-initialization-$variant.pas";$files+=$file
 [IO.File]::WriteAllText((Join-Path $PSScriptRoot "../$file"),$source,[Text.UTF8Encoding]::new($false))
}
$metadata=[ordered]@{schema=1;runtime='Windows .NET Framework 4.0.30319.42000';outcomes=1215;variants=@('typed','inferred');scope='Source-derived Pascal record field-initialization recipes executed with genuine Framework struct/reference/array/generic behavior. Not a Pascal compiler oracle or full language/library coverage.';artifacts=@($files | ForEach-Object {[ordered]@{path=$_;sha256_lf=Get-CanonicalHash (Join-Path $PSScriptRoot "../$_")}})}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '../fixtures/record-initialization-reference.json'),(($metadata | ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))

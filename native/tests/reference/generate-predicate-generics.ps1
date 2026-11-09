# Development-only Framework reference; never invokes the Pascal compiler.
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$output=Join-Path $PSScriptRoot '../fixtures/predicate-generics-reference.tsv'
$append=$false
foreach($arity in @(2,3)){foreach($module in @('Program','Другой.Модуль')){
 $directory=Join-Path $projectRoot ('native/build/predicate-generics-reference/'+$arity)
 [IO.Directory]::CreateDirectory($directory)|Out-Null
 $program=Join-Path $directory ($module+'.exe')
 $defines=@();if($arity -eq 3){$defines+='THREE'};if($module -ne 'Program'){$defines+='UNICODE_MODULE'}
 $flags=@('/nologo','/target:exe','/r:System.Core.dll',('/out:'+$program));if($defines.Count){$flags+=('/define:'+($defines -join ';'))}
 & $compiler @flags (Join-Path $PSScriptRoot 'predicate-generics.cs')
 if($LASTEXITCODE -ne 0){throw 'Generic predicate reference compilation failed'}
 if($append){& $program $output append}else{& $program $output;$append=$true}
 if($LASTEXITCODE -ne 0){throw 'Generic predicate reference generation failed'}
}}
function Get-CanonicalHash([string]$Path){$hash=[Security.Cryptography.SHA256]::Create();try{return ([BitConverter]::ToString($hash.ComputeHash([Text.Encoding]::UTF8.GetBytes([IO.File]::ReadAllText($Path).Replace("`r`n","`n"))))).Replace('-','').ToLowerInvariant()}finally{$hash.Dispose()}}
$files=@('reference/predicate-generics.cs','reference/generate-predicate-generics.ps1','fixtures/predicate2-program.pas','fixtures/predicate3-program.pas','fixtures/predicate-generics-reference.tsv','fixtures/predicate-generics-reference.md')
$metadata=[ordered]@{schema=1;runtime='Windows .NET Framework 4.0.30319.42000';outcomes=2304;scope='PABCSystem Predicate2/Predicate3 source-declared generic delegate identities and actual CLR conversions/invocation/metadata, with source-derived conditional conversion guards. Two arities, two source modules, varied scalar/string/boolean arguments and 16 operations. Not a Pascal compiler oracle or full generic-language/delegate/library coverage.';artifacts=@($files|ForEach-Object{[ordered]@{path=$_;sha256_lf=Get-CanonicalHash (Join-Path $PSScriptRoot "../$_")}})}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '../fixtures/predicate-generics-reference.json'),(($metadata|ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))

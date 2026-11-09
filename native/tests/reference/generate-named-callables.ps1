# Development-only Framework oracle. Never invokes the original Pascal compiler.
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$directory=Join-Path $projectRoot 'native/build/named-callables-reference'
[IO.Directory]::CreateDirectory($directory)|Out-Null
$output=Join-Path $PSScriptRoot '../fixtures/named-callables-reference.tsv'
foreach($module in @('Program','Другой.Модуль')){
 $program=Join-Path $directory ($module+'.exe')
 $flags=@('/nologo','/target:exe','/r:System.Core.dll',('/out:'+$program))
 if($module -ne 'Program'){$flags+='/define:UNICODE_MODULE'}
 & $compiler @flags (Join-Path $PSScriptRoot 'named-callables.cs')
 if($LASTEXITCODE -ne 0){throw 'Named delegate reference compilation failed'}
 if($module -eq 'Program'){& $program $output}else{& $program $output append}
 if($LASTEXITCODE -ne 0){throw 'Named delegate reference generation failed'}
}
function Get-CanonicalHash([string]$Path){$hash=[Security.Cryptography.SHA256]::Create();try{return ([BitConverter]::ToString($hash.ComputeHash([Text.Encoding]::UTF8.GetBytes([IO.File]::ReadAllText($Path).Replace("`r`n","`n"))))).Replace('-','').ToLowerInvariant()}finally{$hash.Dispose()}}
$files=@('reference/named-callables.cs','reference/generate-named-callables.ps1','fixtures/named-callables-program.pas','fixtures/named-callables-reference.tsv','fixtures/named-callables-reference.md')
$metadata=[ordered]@{schema=1;runtime='Windows .NET Framework 4.0.30319.42000';outcomes=1296;scope='Actual CLR named delegate identities, conversions, invocation and metadata in two source modules, with explicitly source-derived conversion guards. Classic named headers differ from System.Func/Action arrow aliases. No original Pascal compiler is executed; general anonymous/generic delegate and operator overload resolution remain outside this corpus.';artifacts=@($files|ForEach-Object{[ordered]@{path=$_;sha256_lf=Get-CanonicalHash (Join-Path $PSScriptRoot "../$_")}})}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '../fixtures/named-callables-reference.json'),(($metadata|ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))

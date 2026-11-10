# Development-only CLR closure oracle. Does not invoke the Pascal compiler.
param([string]$RuntimeHost=(Join-Path $env:USERPROFILE '.dotnet/dotnet.exe'),[switch]$ManifestOnly)
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$fixture=Join-Path $PSScriptRoot '../fixtures/nested-routines-reference.tsv'
if(!$ManifestOnly){
 $compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
 $program=Join-Path $projectRoot 'native/build/nested-routines-reference.exe'
 $modern=Join-Path $projectRoot 'native/build/nested-routines-modern-reference.tsv'
 & $compiler /nologo /target:exe ('/out:'+$program) (Join-Path $PSScriptRoot 'nested-routines.cs')
 if($LASTEXITCODE -ne 0){throw 'Closure oracle compilation failed'}
 [IO.File]::WriteAllText([IO.Path]::ChangeExtension($program,'.runtimeconfig.json'),'{"runtimeOptions":{"tfm":"net10.0","framework":{"name":"Microsoft.NETCore.App","version":"10.0.0"}}}',[Text.UTF8Encoding]::new($false))
 & $program $fixture;if($LASTEXITCODE -ne 0){throw 'Framework closure execution failed'}
 & $RuntimeHost $program $modern;if($LASTEXITCODE -ne 0){throw 'Modern closure execution failed'}
 if((Get-FileHash -LiteralPath $fixture).Hash -ne (Get-FileHash -LiteralPath $modern).Hash){throw 'Closure executions differ across runtimes'}
}
function Hash([string]$Path){$sha=[Security.Cryptography.SHA256]::Create();try{$text=[Text.UTF8Encoding]::new($false,$true).GetString([IO.File]::ReadAllBytes($Path));$bytes=[Text.Encoding]::UTF8.GetBytes($text.Replace("`r`n","`n"));return ([BitConverter]::ToString($sha.ComputeHash($bytes))).Replace('-','').ToLowerInvariant()}finally{$sha.Dispose()}}
$files=@('reference/nested-routines.cs','reference/generate-nested-routines.ps1','fixtures/nested-routines-reference.tsv','fixtures/nested-routines-reference.pas','fixtures/nested-routines-reference.md','../../TestSuite/fileof1.pas','../../TestSuite/nested1.pas','../../TestSuite/nested3.pas','../../TestSuite/nested4.pas','../../TestSuite/fResult3.pas')
$metadata=[ordered]@{schema=1;reference_runtimes=@('Windows .NET Framework 4.0.30319.42000','Windows .NET 10.0.12');reference_executions=1071;fixtures_identical=$true;scope='Independent CLR delegate/closure execution and source-derived typed native Pascal recipes: escaped local state, shared aliases versus independent activations, recursive function-name result assignment, nested access to outer parameters and locals. Named routines also capture outer var/out cells and record Self, and can access an enclosing function Result. Five original source tests fileof1/nested1/nested3/nested4/fResult3 run through the native interpreter only. No original Pascal compiler oracle is claimed or invoked. Local default arguments, forward declarations/overloads, local type sections and complete source-generated metadata remain positive requirements. No Python/.NET is shipped.';artifacts=@($files|ForEach-Object{[ordered]@{path=$_;sha256_lf=Hash (Join-Path $PSScriptRoot "../$_")}})}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '../fixtures/nested-routines-reference.json'),(($metadata|ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))

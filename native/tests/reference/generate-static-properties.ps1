# Development-only fresh CLR types. Never invokes the Pascal compiler.
param([string]$RuntimeHost=(Join-Path $env:USERPROFILE '.dotnet/dotnet.exe'),[switch]$ManifestOnly)
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$fixture=Join-Path $PSScriptRoot '../fixtures/static-properties-reference.tsv'
if(!$ManifestOnly){
 $compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
 $program=Join-Path $projectRoot 'native/build/static-properties-reference.exe'
 $modern=Join-Path $projectRoot 'native/build/static-properties-modern.tsv'
 & $compiler /nologo /target:exe ('/out:'+$program) (Join-Path $PSScriptRoot 'static-properties.cs')
 if($LASTEXITCODE -ne 0){throw 'Static property reference compilation failed'}
 [IO.File]::WriteAllText([IO.Path]::ChangeExtension($program,'.runtimeconfig.json'),'{"runtimeOptions":{"tfm":"net10.0","framework":{"name":"Microsoft.NETCore.App","version":"10.0.0"}}}',[Text.UTF8Encoding]::new($false))
 & $program $fixture;if($LASTEXITCODE -ne 0){throw 'Framework static property execution failed'}
 & $RuntimeHost $program $modern;if($LASTEXITCODE -ne 0){throw 'Modern static property execution failed'}
 if((Get-FileHash -LiteralPath $fixture).Hash -ne (Get-FileHash -LiteralPath $modern).Hash){throw 'Class initialization executions differ across runtimes'}
}
function Hash([string]$Path){$sha=[Security.Cryptography.SHA256]::Create();try{$text=[Text.UTF8Encoding]::new($false,$true).GetString([IO.File]::ReadAllBytes($Path));return ([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($text.Replace("`r`n","`n"))))).Replace('-','').ToLowerInvariant()}finally{$sha.Dispose()}}
$files=@('reference/static-properties.cs','reference/generate-static-properties.ps1','fixtures/static-properties-reference.tsv','fixtures/static-properties-template.pas','fixtures/static-properties-reference.md','../../TestSuite/defaultparams4.pas','../../TestSuite/lambda_static_1.pas','../../Parsers/PascalABCParserNewSaushkin/ABCPascal.y','../../TestSuite/intellisense_tests/property5.pas')
$metadata=[ordered]@{schema=1;reference_runtimes=@('Windows .NET Framework 4.0.30319.42000','Windows .NET 10.0.12');reference_executions=64;fixtures_identical=$true;scope='Actual strict CLR static property reads/writes, callable getters and accessors, compound/index argument order, initialization/reentry and default arguments. Static indexed/default-property recipes lower to actual typed CLR accessor methods; C# has no static indexer syntax. Fresh closed reference types isolate state and do not claim Pascal generic declarations. No original Pascal compiler invoked and no development runtime shipped. Record static storage, explicit constructors, complete inference/visibility/reflection and complete standard units remain unfinished.';artifacts=@($files|ForEach-Object{[ordered]@{path=$_;sha256_lf=Hash (Join-Path $PSScriptRoot "../$_")}})}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '../fixtures/static-properties-reference.json'),(($metadata|ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))

# Development-only fresh CLR types. Never invokes the Pascal compiler.
param([string]$RuntimeHost=(Join-Path $env:USERPROFILE '.dotnet/dotnet.exe'),[switch]$ManifestOnly)
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$fixture=Join-Path $PSScriptRoot '../fixtures/static-fields-reference.tsv'
if(!$ManifestOnly){
 $compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
 $program=Join-Path $projectRoot 'native/build/static-fields-reference.exe'
 $modern=Join-Path $projectRoot 'native/build/static-fields-modern.tsv'
 & $compiler /nologo /target:exe ('/out:'+$program) (Join-Path $PSScriptRoot 'static-fields.cs')
 if($LASTEXITCODE -ne 0){throw 'Static field reference compilation failed'}
 [IO.File]::WriteAllText([IO.Path]::ChangeExtension($program,'.runtimeconfig.json'),'{"runtimeOptions":{"tfm":"net10.0","framework":{"name":"Microsoft.NETCore.App","version":"10.0.0"}}}',[Text.UTF8Encoding]::new($false))
 & $program $fixture;if($LASTEXITCODE -ne 0){throw 'Framework static initializer execution failed'}
 & $RuntimeHost $program $modern;if($LASTEXITCODE -ne 0){throw 'Modern static initializer execution failed'}
 if((Get-FileHash -LiteralPath $fixture).Hash -ne (Get-FileHash -LiteralPath $modern).Hash){throw 'Class initialization executions differ across runtimes'}
}
function Hash([string]$Path){$sha=[Security.Cryptography.SHA256]::Create();try{$text=[Text.UTF8Encoding]::new($false,$true).GetString([IO.File]::ReadAllBytes($Path));return ([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($text.Replace("`r`n","`n"))))).Replace('-','').ToLowerInvariant()}finally{$sha.Dispose()}}
$files=@('reference/static-fields.cs','reference/generate-static-fields.ps1','fixtures/static-fields-reference.tsv','fixtures/static-fields-template.pas','fixtures/static-fields-reference.md','../../TestSuite/arrays12.pas')
$metadata=[ordered]@{schema=1;reference_runtimes=@('Windows .NET Framework 4.0.30319.42000','Windows .NET 10.0.12');reference_executions=160;fixtures_identical=$true;scope='Actual strict CLR static initialization on fresh closed classes, checking reads, stores, object constructor arguments, references, instance use, unevaluated Low/High, owned fixed copies reentrant initialization and field publication before element evaluation. Source-derived native Pascal recipes and original arrays12 run only through the native interpreter. No original Pascal compiler oracle is invoked or claimed. Static routines/properties, record statics, generic class declarations, full reflection and complete modules remain unfinished. No development runtime is shipped.';artifacts=@($files|ForEach-Object{[ordered]@{path=$_;sha256_lf=Hash (Join-Path $PSScriptRoot "../$_")}})}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '../fixtures/static-fields-reference.json'),(($metadata|ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))

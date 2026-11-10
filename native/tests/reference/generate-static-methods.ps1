# Development-only fresh CLR types. Never invokes the Pascal compiler.
param([string]$RuntimeHost=(Join-Path $env:USERPROFILE '.dotnet/dotnet.exe'),[switch]$ManifestOnly)
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$fixture=Join-Path $PSScriptRoot '../fixtures/static-methods-reference.tsv'
if(!$ManifestOnly){
 $compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
 $program=Join-Path $projectRoot 'native/build/static-methods-reference.exe'
 $modern=Join-Path $projectRoot 'native/build/static-methods-modern.tsv'
 & $compiler /nologo /target:exe ('/out:'+$program) (Join-Path $PSScriptRoot 'static-methods.cs')
 if($LASTEXITCODE -ne 0){throw 'Static method reference compilation failed'}
 [IO.File]::WriteAllText([IO.Path]::ChangeExtension($program,'.runtimeconfig.json'),'{"runtimeOptions":{"tfm":"net10.0","framework":{"name":"Microsoft.NETCore.App","version":"10.0.0"}}}',[Text.UTF8Encoding]::new($false))
 & $program $fixture;if($LASTEXITCODE -ne 0){throw 'Framework static method execution failed'}
 & $RuntimeHost $program $modern;if($LASTEXITCODE -ne 0){throw 'Modern static method execution failed'}
 if((Get-FileHash -LiteralPath $fixture).Hash -ne (Get-FileHash -LiteralPath $modern).Hash){throw 'Class initialization executions differ across runtimes'}
}
function Hash([string]$Path){$sha=[Security.Cryptography.SHA256]::Create();try{$text=[Text.UTF8Encoding]::new($false,$true).GetString([IO.File]::ReadAllBytes($Path));return ([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($text.Replace("`r`n","`n"))))).Replace('-','').ToLowerInvariant()}finally{$sha.Dispose()}}
$files=@('reference/static-methods.cs','reference/generate-static-methods.ps1','fixtures/static-methods-reference.tsv','fixtures/static-methods-template.pas','fixtures/static-methods-reference.md','../../TestSuite/lambda_nonstatic_from_static.pas','../../NETGenerator/NETGenerator.cs')
$metadata=[ordered]@{schema=1;reference_runtimes=@('Windows .NET Framework 4.0.30319.42000','Windows .NET 10.0.12');reference_executions=64;fixtures_identical=$true;scope='Actual CLR static calls, reference arguments, typed delegates, overloads, defaults, params, recursion, closures and strict class initialization. Fresh tag types isolate runtime state; generic Pascal classes are not claimed. Native recipes follow the source compiler. No original Pascal compiler or development runtime is shipped. Static properties/operators/explicit constructors, record statics, inheritance/generics, complete visibility/reflection/exception wrapping and complete modules remain unfinished.';artifacts=@($files|ForEach-Object{[ordered]@{path=$_;sha256_lf=Hash (Join-Path $PSScriptRoot "../$_")}})}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '../fixtures/static-methods-reference.json'),(($metadata|ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))

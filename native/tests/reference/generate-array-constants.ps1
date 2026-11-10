# Development-only static pools and source-derived emitted wrapper operations.
param([string]$RuntimeHost=(Join-Path $env:USERPROFILE '.dotnet/dotnet.exe'),[switch]$ManifestOnly)
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$fixture=Join-Path $PSScriptRoot '../fixtures/array-constants-reference.tsv'
if(!$ManifestOnly){
 $compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
 $program=Join-Path $projectRoot 'native/build/array-constants-reference.exe'
 $modern=Join-Path $projectRoot 'native/build/array-constants-modern.tsv'
 & $compiler /nologo /target:exe ('/out:'+$program) (Join-Path $PSScriptRoot 'array-constants.cs')
 if($LASTEXITCODE -ne 0){throw 'Array constant reference compilation failed'}
 [IO.File]::WriteAllText([IO.Path]::ChangeExtension($program,'.runtimeconfig.json'),'{"runtimeOptions":{"tfm":"net10.0","framework":{"name":"Microsoft.NETCore.App","version":"10.0.0"}}}',[Text.UTF8Encoding]::new($false))
 & $program $fixture;if($LASTEXITCODE -ne 0){throw 'Framework constant-pool reference failed'}
 & $RuntimeHost $program $modern;if($LASTEXITCODE -ne 0){throw 'Modern constant-pool reference failed'}
 if((Get-FileHash -LiteralPath $fixture).Hash -ne (Get-FileHash -LiteralPath $modern).Hash){throw 'Static pool executions differ across runtimes'}
}
function Hash([string]$Path){$sha=[Security.Cryptography.SHA256]::Create();try{$text=[Text.UTF8Encoding]::new($false,$true).GetString([IO.File]::ReadAllBytes($Path));return ([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($text.Replace("`r`n","`n"))))).Replace('-','').ToLowerInvariant()}finally{$sha.Dispose()}}
$files=@('reference/array-constants.cs','reference/generate-array-constants.ps1','fixtures/array-constants-reference.tsv','fixtures/array-constants-template.pas','fixtures/array-constants-reference.md','../../TestSuite/arrays2.pas','../../TestSuite/arrays3.pas')
$metadata=[ordered]@{schema=1;reference_runtimes=@('Windows .NET Framework 4.0.30319.42000','Windows .NET 10.0.12');reference_executions=81;fixtures_identical=$true;scope='Independent CLR static arrays and source-derived fixed-wrapper Array.Copy recipes. Native Pascal checks pool persistence, dynamic alias sharing, owned fixed scalar copies and shared managed inner arrays and initializer callbacks before namespace variable initialization, preserving CLR numeric zero storage. Standard runtime factory/input initialization is checked separately without executing it at compile time. Two original Pascal array-constant tests run natively. Compile-time element folding is verified separately as a native known-type requirement; no original Pascal compiler oracle is invoked or claimed. No development runtime is shipped. Records, sets, pointers, general generated reflection and full unit coverage remain unfinished.';artifacts=@($files|ForEach-Object{[ordered]@{path=$_;sha256_lf=Hash (Join-Path $PSScriptRoot "../$_")}})}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '../fixtures/array-constants-reference.json'),(($metadata|ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))

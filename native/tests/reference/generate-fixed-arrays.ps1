# Development-only source-derived wrapper/record oracle. Never invokes Pascal.
param([string]$RuntimeHost=(Join-Path $env:USERPROFILE '.dotnet/dotnet.exe'),[switch]$ManifestOnly)
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$fixture=Join-Path $PSScriptRoot '../fixtures/fixed-array-copy-reference.tsv'
if(!$ManifestOnly){
 $compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
 $program=Join-Path $projectRoot 'native/build/fixed-array-copy-reference.exe'
 $modern=Join-Path $projectRoot 'native/build/fixed-array-copy-modern.tsv'
 & $compiler /nologo /target:exe ('/out:'+$program) (Join-Path $PSScriptRoot 'fixed-array-copy.cs')
 if($LASTEXITCODE -ne 0){throw 'Fixed-array reference compilation failed'}
 [IO.File]::WriteAllText([IO.Path]::ChangeExtension($program,'.runtimeconfig.json'),'{"runtimeOptions":{"tfm":"net10.0","framework":{"name":"Microsoft.NETCore.App","version":"10.0.0"}}}',[Text.UTF8Encoding]::new($false))
 & $program $fixture;if($LASTEXITCODE -ne 0){throw 'Framework fixed-array execution failed'}
 & $RuntimeHost $program $modern;if($LASTEXITCODE -ne 0){throw 'Modern fixed-array execution failed'}
 if((Get-FileHash -LiteralPath $fixture).Hash -ne (Get-FileHash -LiteralPath $modern).Hash){throw 'Fixed-array executions differ across runtimes'}
}
function Hash([string]$Path){$sha=[Security.Cryptography.SHA256]::Create();try{$text=[Text.UTF8Encoding]::new($false,$true).GetString([IO.File]::ReadAllBytes($Path));return ([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($text.Replace("`r`n","`n"))))).Replace('-','').ToLowerInvariant()}finally{$sha.Dispose()}}
$files=@('reference/fixed-array-copy.cs','reference/generate-fixed-arrays.ps1','fixtures/fixed-array-copy-reference.tsv','fixtures/fixed-array-copy-template.pas','fixtures/fixed-arrays-reference.md','../../TestSuite/fileof2.pas','../../TestSuite/fileofenum1.pas','../../TestSuite/arrayoffile1.pas','../../TestSuite/binfile1.pas','../../TestSuite/arrayoftext1.pas','../../TestSuite/arrayinitializer1.pas','../../TestSuite/arrays7.pas')
$metadata=[ordered]@{schema=1;reference_runtimes=@('Windows .NET Framework 4.0.30319.42000','Windows .NET 10.0.12');reference_executions=18;fixtures_identical=$true;scope='Independent source-derived wrapper/record constructor and Clone methods plus public binary APIs. Every native Pascal trace, packed file byte and outcome is checked. Seven original Pascal tests run through the native interpreter only. No original Pascal compiler oracle is invoked or claimed. Full fixed-array conversions, subranges, short strings, sets, reflection and whole library modules remain incomplete. No Python/.NET is shipped.';artifacts=@($files|ForEach-Object{[ordered]@{path=$_;sha256_lf=Hash (Join-Path $PSScriptRoot "../$_")}})}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '../fixtures/fixed-arrays-reference.json'),(($metadata|ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))

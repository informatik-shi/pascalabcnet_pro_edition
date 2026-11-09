# Development-only reference generation; no Pascal compiler is invoked.
param([string]$RuntimeHost=(Join-Path $env:USERPROFILE '.dotnet/dotnet.exe'),[switch]$ManifestOnly)
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
if(!$ManifestOnly){
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$program=Join-Path $projectRoot 'native/build/dbcs-reference.exe'
& $compiler /nologo /target:exe ('/out:'+$program) (Join-Path $PSScriptRoot 'dbcs-encodings.cs')
if($LASTEXITCODE -ne 0){throw 'DBCS reference compilation failed'}
& $program (Join-Path $PSScriptRoot '../fixtures/dbcs-reference.bin') (Join-Path $PSScriptRoot '../fixtures/dbcs-files-framework-reference.tsv')
if($LASTEXITCODE -ne 0){throw 'Framework DBCS reference generation failed'}
if(!(Test-Path -LiteralPath $RuntimeHost)){throw 'Specify the .NET 10 host for development file reference generation'}
$runtimeConfig='{"runtimeOptions":{"tfm":"net10.0","framework":{"name":"Microsoft.NETCore.App","version":"10.0.0"}}}'
[IO.File]::WriteAllText([IO.Path]::ChangeExtension($program,'.runtimeconfig.json'),$runtimeConfig,[Text.UTF8Encoding]::new($false))
& $RuntimeHost $program unused (Join-Path $PSScriptRoot '../fixtures/dbcs-files-reference.tsv') files-only
if($LASTEXITCODE -ne 0){throw 'Modern file reference generation failed'}
# This writes C++ source: run only when no C++ compiler is active.
& node (Join-Path $projectRoot 'native/tools/generate-dbcs.mjs')
if($LASTEXITCODE -ne 0){throw 'Portable DBCS table generation failed'}
}
function Get-CanonicalHash([string]$Path,[bool]$Binary){$hash=[Security.Cryptography.SHA256]::Create();try{$bytes=if($Binary){[IO.File]::ReadAllBytes($Path)}else{[Text.Encoding]::UTF8.GetBytes([IO.File]::ReadAllText($Path).Replace("`r`n","`n"))};return ([BitConverter]::ToString($hash.ComputeHash($bytes))).Replace('-','').ToLowerInvariant()}finally{$hash.Dispose()}}
$files=@('reference/dbcs-encodings.cs','reference/generate-dbcs.ps1','fixtures/dbcs-reference.bin','fixtures/encoding-aliases-reference.tsv','fixtures/dbcs-files-reference.tsv','fixtures/dbcs-files-framework-reference.tsv','fixtures/dbcs-reference.md','../tools/generate-dbcs.mjs')
$metadata=[ordered]@{schema=2;mapping_runtime='Windows .NET Framework 4.0.30319.42000';file_runtime='Windows .NET 10.0.12';codecs=23;utf16_encoding_inputs=1507328;single_byte_decoding_inputs=5888;byte_pair_decoding_inputs=1507328;split_decoding_inputs=1507328;supplementary_scalar_inputs=24117248;eucjp_prefix_triples=65536;native_supported_aliases=63;modern_file_recipes=161;legacy_file_recipes=161;scope='All 22 DBCSCodePageEncoding codecs and EUCJPEncoding 51932 exposed by this Framework, exhaustive BMP/default/exact maps, every byte pair and decoder split, all supplementary scalar fallbacks, throwing-fallback cross-checks and actual encoder carry. File recipes use the modern .NET stream behavior selected by the earlier native file implementation. Framework StreamReader drops a final pending lead in 23 EOF recipes; the native reader intentionally flushes replacement like .NET 10. These legacy differences are preserved, not reported as compatibility passes. Native supports 123 of the 140 Framework code pages; 17 stateful/variable-width codecs and full Encoding/class/module APIs remain open. No Pascal compiler is invoked.';artifacts=@($files|ForEach-Object{$binary=$_.EndsWith('.bin');$item=[ordered]@{path=$_};$item[$(if($binary){'sha256_bytes'}else{'sha256_lf'})]=Get-CanonicalHash (Join-Path $PSScriptRoot "../$_") $binary;$item})}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '../fixtures/dbcs-reference.json'),(($metadata|ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))

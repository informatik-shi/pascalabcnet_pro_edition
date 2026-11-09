# Development-only mapping generator. No Pascal compiler is invoked.
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$program=Join-Path $projectRoot 'native/build/sbcs-reference.exe'
& $compiler /nologo /unsafe /target:exe ('/out:'+$program) (Join-Path $PSScriptRoot 'sbcs-encodings.cs')
if($LASTEXITCODE -ne 0){throw 'Single-byte reference compilation failed'}
& $program (Join-Path $PSScriptRoot '../fixtures/sbcs-reference.bin') (Join-Path $PSScriptRoot '../fixtures/encoding-aliases-reference.tsv') (Join-Path $PSScriptRoot '../fixtures/sbcs-files-reference.tsv') (Join-Path $PSScriptRoot '../fixtures/encoding-name-reference.tsv')
if($LASTEXITCODE -ne 0){throw 'Single-byte reference generation failed'}
& node (Join-Path $projectRoot 'native/tools/generate-sbcs.mjs')
if($LASTEXITCODE -ne 0){throw 'Portable codec generation failed'}
function Get-CanonicalHash([string]$Path,[bool]$Binary){$hash=[Security.Cryptography.SHA256]::Create();try{$bytes=if($Binary){[IO.File]::ReadAllBytes($Path)}else{[Text.Encoding]::UTF8.GetBytes([IO.File]::ReadAllText($Path).Replace("`r`n","`n"))};return ([BitConverter]::ToString($hash.ComputeHash($bytes))).Replace('-','').ToLowerInvariant()}finally{$hash.Dispose()}}
$files=@('reference/sbcs-encodings.cs','reference/generate-sbcs.ps1','fixtures/sbcs-reference.bin','fixtures/encoding-aliases-reference.tsv','fixtures/sbcs-files-reference.tsv','fixtures/encoding-name-reference.tsv','fixtures/sbcs-reference.md','../tools/generate-sbcs.mjs')
$metadata=[ordered]@{schema=2;runtime='Windows .NET Framework 4.0.30319.42000';codecs=95;utf16_encoding_inputs=6225920;byte_decoding_inputs=24320;framework_aliases=405;native_supported_aliases=318;file_recipes=570;name_edge_cases=13;scope='All 95 single-byte encodings exposed by this Framework, exhaustive default BMP encoding/byte decoding and exact-map classification, actual throwing-fallback cross-checks, surrogate encoder state, raw framework aliases and source-derived file recipes. Native supports these plus five existing Unicode codecs; 40 remaining Framework variable-width/stateful encodings and full Encoding/class/module APIs remain unfinished. No Pascal compiler is invoked.';artifacts=@($files|ForEach-Object{$binary=$_.EndsWith('.bin');$item=[ordered]@{path=$_};$item[$(if($binary){'sha256_bytes'}else{'sha256_lf'})]=Get-CanonicalHash (Join-Path $PSScriptRoot "../$_") $binary;$item})}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '../fixtures/sbcs-reference.json'),(($metadata|ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))

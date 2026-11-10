# Development only: executes public APIs, never the original Pascal compiler.
param([string]$RuntimeHost=(Join-Path $env:USERPROFILE '.dotnet/dotnet.exe'),[switch]$ManifestOnly)
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$fixtures=Join-Path $PSScriptRoot '../fixtures'
$work=Join-Path $projectRoot 'native/build/binary-oracle'
$archive=Join-Path $fixtures 'binary-codec-reference.zip'
$raw=Join-Path $work 'binary-codec-reference.tsv'
$null=New-Item -ItemType Directory -Force -Path $work
if(!$ManifestOnly){
 $compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
 if(!(Test-Path -LiteralPath $RuntimeHost)){throw 'Specify the modern development host'}
 foreach($name in @('binary-stream','binary-codec','binary-file')){
  $program=Join-Path $work "$name.exe"
  & $compiler /nologo /target:exe ('/out:'+$program) (Join-Path $PSScriptRoot "$name.cs") (Join-Path $PSScriptRoot 'binary-oracle-support.cs')
  if($LASTEXITCODE -ne 0){throw "Reference compilation failed: $name"}
  [IO.File]::WriteAllText([IO.Path]::ChangeExtension($program,'.runtimeconfig.json'),'{"runtimeOptions":{"tfm":"net10.0","framework":{"name":"Microsoft.NETCore.App","version":"10.0.0"}}}',[Text.UTF8Encoding]::new($false))
  $modern=Join-Path $work "$name-modern.tsv"
  switch($name){
   'binary-stream' {
    $legacy=Join-Path $fixtures 'binary-stream-reference.tsv'
    & $program $legacy;if($LASTEXITCODE -ne 0){throw 'Framework stream reference failed'}
    & $RuntimeHost $program $modern;if($LASTEXITCODE -ne 0){throw 'Modern stream reference failed'}
   }
   'binary-file' {
    $legacy=Join-Path $fixtures 'binary-file-reference.tsv'
    $pages=Join-Path $fixtures 'encoding-metadata-reference.tsv'
    & $program $pages $legacy;if($LASTEXITCODE -ne 0){throw 'Framework packed reference failed'}
    & $RuntimeHost $program $pages $modern modern;if($LASTEXITCODE -ne 0){throw 'Modern packed reference failed'}
   }
   'binary-codec' {
    $legacy=$raw;$pages=Join-Path $fixtures 'encoding-metadata-reference.tsv'
    & $program $legacy $pages;if($LASTEXITCODE -ne 0){throw 'Framework binary codec reference failed'}
    & $RuntimeHost $program $modern $pages modern;if($LASTEXITCODE -ne 0){throw 'Modern binary codec reference failed'}
    & node (Join-Path $PSScriptRoot 'compare-binary-codecs.mjs') $legacy $modern (Join-Path $fixtures 'binary-codec-runtime-differences.json')
    if($LASTEXITCODE -ne 0){throw 'Binary codec comparison failed'}
   }
  }
  if($name -ne 'binary-codec' -and (Get-FileHash -LiteralPath $legacy).Hash -ne (Get-FileHash -LiteralPath $modern).Hash){throw "$name public API fixtures differ across runtimes"}
 }
 Add-Type -AssemblyName System.IO.Compression
 $stream=[IO.File]::Open($archive,[IO.FileMode]::Create)
 $zip=[IO.Compression.ZipArchive]::new($stream,[IO.Compression.ZipArchiveMode]::Create)
 try{
  $entry=$zip.CreateEntry('binary-codec-reference.tsv',[IO.Compression.CompressionLevel]::Optimal)
  $entry.LastWriteTime=[DateTimeOffset]'2000-01-01T00:00:00+00:00'
  $destination=$entry.Open()
  try{$inputStream=[IO.File]::OpenRead($raw);try{$inputStream.CopyTo($destination)}finally{$inputStream.Dispose()}}finally{$destination.Dispose()}
 }finally{$zip.Dispose();$stream.Dispose()}
}
if(!(Test-Path -LiteralPath $raw)){
 $zip=[IO.Compression.ZipFile]::OpenRead($archive)
 try{$entry=$zip.GetEntry('binary-codec-reference.tsv');$inputStream=$entry.Open();$destination=[IO.File]::Create($raw);try{$inputStream.CopyTo($destination)}finally{$inputStream.Dispose();$destination.Dispose()}}finally{$zip.Dispose()}
}
function Hash([string]$Path,[bool]$Binary){$sha=[Security.Cryptography.SHA256]::Create();try{$bytes=if($Binary){[IO.File]::ReadAllBytes($Path)}else{[Text.Encoding]::UTF8.GetBytes([IO.File]::ReadAllText($Path).Replace("`r`n","`n"))};return ([BitConverter]::ToString($sha.ComputeHash($bytes))).Replace('-','').ToLowerInvariant()}finally{$sha.Dispose()}}
$files=@('reference/binary-stream.cs','reference/binary-codec.cs','reference/binary-file.cs','reference/binary-oracle-support.cs','reference/compare-binary-codecs.mjs','reference/generate-binary-files.ps1','fixtures/binary-stream-reference.tsv','fixtures/binary-codec-reference.zip','fixtures/binary-codec-runtime-differences.json','fixtures/binary-file-reference.tsv','fixtures/binary-file-reference.pas','fixtures/binary-files-reference.md','binary-codec-reference.cmake')
$metadata=[ordered]@{schema=1;compatibility_runtime='Windows .NET Framework 4.0.30319.42000';comparison_runtime='Windows .NET 10.0.12';stream_traces=4320;stream_fixtures_identical=$true;codec_traces=410752;codec_runtime_differences=311;packed_encoding_rows=140;packed_fixtures_identical=$true;uncompressed_codec_bytes=(Get-Item -LiteralPath $raw).Length;sha256_uncompressed_codec_bytes=Hash $raw $true;scope='Native binary streams, Framework scalar/character/string codec, statically typed primitive/enum/packed recursive value records, raw byte arrays and file positioning. Public API traces, plus source-derived PABCSystem guards and packed layout; not an original Pascal compiler oracle. Fixed arrays/subranges/short strings/sets, System.Convert object coercions, mutable TypedFile.ElementType, reflection identity, named file classes/inheritance, object serialization and full modules remain positive gaps. No Python/.NET is shipped.';artifacts=@($files|ForEach-Object{$binary=$_.EndsWith('.zip');$item=[ordered]@{path=$_};$item[$(if($binary){'sha256_bytes'}else{'sha256_lf'})]=Hash (Join-Path $PSScriptRoot "../$_") $binary;$item})}
[IO.File]::WriteAllText((Join-Path $fixtures 'binary-files-reference.json'),(($metadata|ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))

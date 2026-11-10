# Development-only public API reference; no Pascal compiler or application dependency.
param([string]$RuntimeHost=(Join-Path $env:USERPROFILE '.dotnet/dotnet.exe'),[switch]$ManifestOnly,[string]$CMake=(Join-Path $PSScriptRoot '../../build/tools/cmake/data/bin/cmake.exe'))
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$referenceDir=Join-Path $projectRoot 'native/build/utf7-oracle'
$framework=Join-Path $referenceDir 'utf7-reference.bin'
$archive=Join-Path $PSScriptRoot '../fixtures/utf7-reference.zip'
if(!$ManifestOnly){
 $null=New-Item -ItemType Directory -Force -Path $referenceDir
 $compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
 if(!(Test-Path -LiteralPath $RuntimeHost)){throw 'Specify the .NET 10 development host'}
 foreach($name in @('utf7-encoding','utf7-files')){
  $program=Join-Path $projectRoot "native/build/$name.exe"
  & $compiler /nologo /target:exe ('/out:'+$program) (Join-Path $PSScriptRoot "$name.cs")
  if($LASTEXITCODE -ne 0){throw 'UTF7 reference compilation failed'}
  [IO.File]::WriteAllText([IO.Path]::ChangeExtension($program,'.runtimeconfig.json'),'{"runtimeOptions":{"tfm":"net10.0","framework":{"name":"Microsoft.NETCore.App","version":"10.0.0"}}}',[Text.UTF8Encoding]::new($false))
  if($name -eq 'utf7-encoding'){
   $modern=Join-Path $projectRoot 'native/build/utf7-modern-reference.bin'
   & $program $framework;if($LASTEXITCODE -ne 0){throw 'Framework UTF7 reference failed'}
   & $RuntimeHost $program $modern;if($LASTEXITCODE -ne 0){throw 'Modern UTF7 reference failed'}
   if((Get-FileHash -LiteralPath $framework).Hash -ne (Get-FileHash -LiteralPath $modern).Hash){throw 'Framework and modern UTF7 primitive references differ'}
   (Get-Item -LiteralPath $framework).LastWriteTimeUtc=[datetime]'2000-01-01T00:00:00Z'
   Push-Location $referenceDir
   try{& $CMake -E tar cf $archive --format=zip -- utf7-reference.bin;if($LASTEXITCODE -ne 0){throw 'Reference ZIP creation failed'}}finally{Pop-Location}
  }else{
   & $program (Join-Path $PSScriptRoot '../fixtures/utf7-files-framework-reference.tsv');if($LASTEXITCODE -ne 0){throw 'Framework UTF7 file reference failed'}
   & $RuntimeHost $program (Join-Path $PSScriptRoot '../fixtures/utf7-files-reference.tsv');if($LASTEXITCODE -ne 0){throw 'Modern UTF7 file reference failed'}
  }
 }
}
if(!(Test-Path -LiteralPath $framework)){
 $null=New-Item -ItemType Directory -Force -Path $referenceDir;Push-Location $referenceDir
 try{& $CMake -E tar xf $archive;if($LASTEXITCODE -ne 0){throw 'Reference ZIP extraction failed'}}finally{Pop-Location}
}
function Get-CanonicalHash([string]$Path,[bool]$Binary){$hash=[Security.Cryptography.SHA256]::Create();try{$bytes=if($Binary){[IO.File]::ReadAllBytes($Path)}else{[Text.Encoding]::UTF8.GetBytes([IO.File]::ReadAllText($Path).Replace("`r`n","`n"))};return ([BitConverter]::ToString($hash.ComputeHash($bytes))).Replace('-','').ToLowerInvariant()}finally{$hash.Dispose()}}
$files=@('reference/utf7-encoding.cs','reference/utf7-files.cs','reference/generate-utf7.ps1','fixtures/utf7-reference.zip','fixtures/utf7-files-reference.tsv','fixtures/utf7-files-framework-reference.tsv','fixtures/utf7-reference.md','fixtures/encoding-aliases-reference.tsv','utf7-reference.cmake')
$metadata=[ordered]@{schema=2;primitive_runtimes=@('Windows .NET Framework 4.0.30319.42000','Windows .NET 10.0.12');primitive_fixtures_identical=$true;uncompressed_fixture_bytes=(Get-Item -LiteralPath $framework).Length;sha256_uncompressed_fixture_bytes=Get-CanonicalHash $framework $true;code_page=65000;encoder_contexts=22;contextual_bmp_inputs=1441792;supplementary_scalars=1048576;decoder_contexts=21847;contextual_decoder_bytes=5592832;fresh_byte_pairs=65536;aliases=6;file_recipes_per_runtime=1421;file_fixtures_identical=$true;scope='Actual public UTF7Encoding(false) primitive and file fixtures are byte-identical across Framework/.NET 10. Every UTF-16 unit in every encoder residual state, all supplementary scalars and splits, every decoder residual value and phase with every next byte, default/strict outcomes and final resets are covered. This ports Framework code page 65000; the modern runtime disables its GetEncoding factory by default and is used only through its explicit constructor in this development oracle. All 140 Framework code pages have native codecs; full Encoding/object/class APIs, code-page zero/default and complete standard-library modules remain unfinished. No Pascal compiler is invoked.';artifacts=@($files|ForEach-Object{$binary=$_.EndsWith('.zip');$item=[ordered]@{path=$_};$item[$(if($binary){'sha256_bytes'}else{'sha256_lf'})]=Get-CanonicalHash (Join-Path $PSScriptRoot "../$_") $binary;$item})}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '../fixtures/utf7-reference.json'),(($metadata|ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))
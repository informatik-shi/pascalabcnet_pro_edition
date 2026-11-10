# Development-only public API reference. No Pascal compiler is invoked.
param([string]$RuntimeHost=(Join-Path $env:USERPROFILE '.dotnet/dotnet.exe'),[switch]$ManifestOnly,[string]$CMake=(Join-Path $PSScriptRoot '../../build/tools/cmake/data/bin/cmake.exe'))
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$referenceDir=Join-Path $projectRoot 'native/build/iso2022-oracle'
$framework=Join-Path $referenceDir 'iso2022-reference.bin'
$archive=Join-Path $PSScriptRoot '../fixtures/iso2022-reference.zip'
if(!$ManifestOnly){
 $null=New-Item -ItemType Directory -Force -Path $referenceDir
 $compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
 if(!(Test-Path -LiteralPath $RuntimeHost)){throw 'Specify the .NET 10 development host'}
 foreach($name in @('iso2022-encoding','iso2022-files')){
  $program=Join-Path $projectRoot "native/build/$name.exe"
  & $compiler /nologo /target:exe ('/out:'+$program) (Join-Path $PSScriptRoot "$name.cs")
  if($LASTEXITCODE -ne 0){throw 'ISO reference compilation failed'}
  [IO.File]::WriteAllText([IO.Path]::ChangeExtension($program,'.runtimeconfig.json'),'{"runtimeOptions":{"tfm":"net10.0","framework":{"name":"Microsoft.NETCore.App","version":"10.0.0"}}}',[Text.UTF8Encoding]::new($false))
  if($name -eq 'iso2022-encoding'){
   $modern=Join-Path $projectRoot 'native/build/iso2022-modern-reference.bin'
   & $program $framework;if($LASTEXITCODE -ne 0){throw 'Framework ISO reference failed'}
   & $RuntimeHost $program $modern modern;if($LASTEXITCODE -ne 0){throw 'Modern ISO reference failed'}
   if((Get-FileHash -LiteralPath $framework).Hash -ne (Get-FileHash -LiteralPath $modern).Hash){throw 'Framework and modern ISO primitive references differ'}
   (Get-Item -LiteralPath $framework).LastWriteTimeUtc=[datetime]'2000-01-01T00:00:00Z'
   Push-Location $referenceDir
   try{& $CMake -E tar cf $archive --format=zip -- iso2022-reference.bin;if($LASTEXITCODE -ne 0){throw 'Reference ZIP creation failed'}}finally{Pop-Location}
  }else{
   & $program (Join-Path $PSScriptRoot '../fixtures/iso2022-files-framework-reference.tsv');if($LASTEXITCODE -ne 0){throw 'Framework ISO file reference failed'}
   & $RuntimeHost $program (Join-Path $PSScriptRoot '../fixtures/iso2022-files-reference.tsv') modern;if($LASTEXITCODE -ne 0){throw 'Modern ISO file reference failed'}
  }
 }
 # This writes C++ source: no C++ compiler may be active.
 & node (Join-Path $projectRoot 'native/tools/generate-iso2022.mjs');if($LASTEXITCODE -ne 0){throw 'ISO table generation failed'}
}
if(!(Test-Path -LiteralPath $framework)){
 $null=New-Item -ItemType Directory -Force -Path $referenceDir;Push-Location $referenceDir
 try{& $CMake -E tar xf $archive;if($LASTEXITCODE -ne 0){throw 'Reference ZIP extraction failed'}}finally{Pop-Location}
}
function Get-CanonicalHash([string]$Path,[bool]$Binary){$hash=[Security.Cryptography.SHA256]::Create();try{$bytes=if($Binary){[IO.File]::ReadAllBytes($Path)}else{[Text.Encoding]::UTF8.GetBytes([IO.File]::ReadAllText($Path).Replace("`r`n","`n"))};return ([BitConverter]::ToString($hash.ComputeHash($bytes))).Replace('-','').ToLowerInvariant()}finally{$hash.Dispose()}}
$files=@('reference/iso2022-encoding.cs','reference/iso2022-files.cs','reference/generate-iso2022.ps1','fixtures/iso2022-reference.zip','fixtures/iso2022-files-reference.tsv','fixtures/iso2022-files-framework-reference.tsv','fixtures/iso2022-reference.md','fixtures/encoding-aliases-reference.tsv','../tools/generate-iso2022.mjs','iso2022-reference.cmake')
$metadata=[ordered]@{schema=2;primitive_runtimes=@('Windows .NET Framework 4.0.30319.42000','Windows .NET 10.0.12');primitive_fixtures_identical=$true;uncompressed_fixture_bytes=(Get-Item -LiteralPath $framework).Length;sha256_uncompressed_fixture_bytes=Get-CanonicalHash $framework $true;codecs=5;bmp_inputs=327680;contextual_bmp_inputs=3735552;contextual_byte_pairs=11206656;splits_per_suffix=3;aliases=7;file_recipes_per_runtime=9725;legacy_file_differences=4062;scope='Actual complete default/strict ISO-2022-JP/KR/HZ primitive fixtures are byte-identical between both runtimes. Every UTF-16 BMP input, all suffixes in retained/finalized encoder states and every byte pair after complete/incomplete mode prefixes are checked, including all splits. Native file behavior follows the selected modern reader flush policy; 4062 legacy file rows remain explicit compatibility gaps. 139 of 140 Framework code pages supported; UTF-7, full Encoding/object APIs and whole library modules remain unfinished. No Pascal compiler is invoked.';artifacts=@($files|ForEach-Object{$binary=$_.EndsWith('.zip');$item=[ordered]@{path=$_};$item[$(if($binary){'sha256_bytes'}else{'sha256_lf'})]=Get-CanonicalHash (Join-Path $PSScriptRoot "../$_") $binary;$item})}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '../fixtures/iso2022-reference.json'),(($metadata|ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))

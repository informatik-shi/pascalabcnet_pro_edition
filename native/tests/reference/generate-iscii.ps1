# Development-only public API reference. No Pascal compiler is invoked.
param([string]$RuntimeHost=(Join-Path $env:USERPROFILE '.dotnet/dotnet.exe'),[switch]$ManifestOnly)
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
if(!$ManifestOnly){
 $compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
 if(!(Test-Path -LiteralPath $RuntimeHost)){throw 'Specify the .NET 10 development host'}
 foreach($name in @('iscii-encoding','iscii-files')){
  $program=Join-Path $projectRoot "native/build/$name.exe"
  & $compiler /nologo /target:exe ('/out:'+$program) (Join-Path $PSScriptRoot "$name.cs")
  if($LASTEXITCODE -ne 0){throw 'ISCII reference compilation failed'}
  [IO.File]::WriteAllText([IO.Path]::ChangeExtension($program,'.runtimeconfig.json'),'{"runtimeOptions":{"tfm":"net10.0","framework":{"name":"Microsoft.NETCore.App","version":"10.0.0"}}}',[Text.UTF8Encoding]::new($false))
  if($name -eq 'iscii-encoding'){
   $framework=Join-Path $PSScriptRoot '../fixtures/iscii-reference.bin';$modern=Join-Path $projectRoot 'native/build/iscii-modern-reference.bin'
   & $program $framework;if($LASTEXITCODE -ne 0){throw 'Framework ISCII primitive reference failed'}
   & $RuntimeHost $program $modern modern;if($LASTEXITCODE -ne 0){throw 'Modern ISCII primitive reference failed'}
   if((Get-FileHash -LiteralPath $framework).Hash -ne (Get-FileHash -LiteralPath $modern).Hash){throw 'Framework and modern ISCII primitive fixtures differ'}
  }else{
   & $program (Join-Path $PSScriptRoot '../fixtures/iscii-files-framework-reference.tsv');if($LASTEXITCODE -ne 0){throw 'Framework ISCII file reference failed'}
   & $RuntimeHost $program (Join-Path $PSScriptRoot '../fixtures/iscii-files-reference.tsv') modern;if($LASTEXITCODE -ne 0){throw 'Modern ISCII file reference failed'}
  }
 }
 # Writes C++ source; run only when no C++ compiler is active.
 & node (Join-Path $projectRoot 'native/tools/generate-iscii.mjs');if($LASTEXITCODE -ne 0){throw 'ISCII static table generation failed'}
}
function Get-CanonicalHash([string]$Path,[bool]$Binary){$hash=[Security.Cryptography.SHA256]::Create();try{$bytes=if($Binary){[IO.File]::ReadAllBytes($Path)}else{[Text.Encoding]::UTF8.GetBytes([IO.File]::ReadAllText($Path).Replace("`r`n","`n"))};return ([BitConverter]::ToString($hash.ComputeHash($bytes))).Replace('-','').ToLowerInvariant()}finally{$hash.Dispose()}}
$files=@('reference/iscii-encoding.cs','reference/iscii-files.cs','reference/generate-iscii.ps1','fixtures/iscii-reference.bin','fixtures/iscii-files-reference.tsv','fixtures/iscii-files-framework-reference.tsv','fixtures/iscii-reference.md','fixtures/encoding-aliases-reference.tsv','../tools/generate-iscii.mjs')
$metadata=[ordered]@{schema=2;primitive_runtimes=@('Windows .NET Framework 4.0.30319.42000','Windows .NET 10.0.12');primitive_fixtures_identical=$true;codecs=10;bmp_encoding_inputs=655360;single_byte_inputs=2560;byte_pair_inputs=655360;encoder_contexts_per_codec=20;contextual_bmp_inputs=13107200;selected_script_pairs=6553600;default_restore_inputs=51200;file_recipes_per_runtime=9490;legacy_file_differences=958;scope='Exhaustive default/exact BMP encodings and every byte pair/split for all ten ISCII pages, actual strict cross-checks, all BMP suffixes in twenty encoder contexts per page, every selected-script pair and ATR default restoration. Context strict classification is verified by actual throwing encoders on every representable scalar, all surrogates, join controls and sampled remaining BMP blocks. Primitive fixtures are byte-identical across both runtimes. Native file behavior follows the selected modern StreamReader policy; 958 legacy file EOF differences remain explicit gaps. 134 of 140 Framework pages supported; six codecs, full Encoding/object APIs and full modules/library remain unfinished.';artifacts=@($files|ForEach-Object{$binary=$_.EndsWith('.bin');$item=[ordered]@{path=$_};$item[$(if($binary){'sha256_bytes'}else{'sha256_lf'})]=Get-CanonicalHash (Join-Path $PSScriptRoot "../$_") $binary;$item})}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '../fixtures/iscii-reference.json'),(($metadata|ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))

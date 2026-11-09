# Development-only reference and static table generation. No Pascal compiler.
param([string]$RuntimeHost=(Join-Path $env:USERPROFILE '.dotnet/dotnet.exe'),[switch]$ManifestOnly)
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
if(!$ManifestOnly){
 $compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
 if(!(Test-Path -LiteralPath $RuntimeHost)){throw 'Specify the .NET 10 development host'}
 foreach($name in @('gb18030-encoding','gb18030-files')){
  $program=Join-Path $projectRoot "native/build/$name.exe"
  & $compiler /nologo /target:exe ('/out:'+$program) (Join-Path $PSScriptRoot "$name.cs")
  if($LASTEXITCODE -ne 0){throw 'GB18030 reference compilation failed'}
  [IO.File]::WriteAllText([IO.Path]::ChangeExtension($program,'.runtimeconfig.json'),'{"runtimeOptions":{"tfm":"net10.0","framework":{"name":"Microsoft.NETCore.App","version":"10.0.0"}}}',[Text.UTF8Encoding]::new($false))
  if($name -eq 'gb18030-encoding'){
   $framework=Join-Path $PSScriptRoot '../fixtures/gb18030-reference.bin';$modern=Join-Path $projectRoot 'native/build/gb18030-modern-reference.bin'
   & $program $framework;if($LASTEXITCODE -ne 0){throw 'Framework primitive reference failed'}
   & $RuntimeHost $program $modern modern;if($LASTEXITCODE -ne 0){throw 'Modern primitive reference failed'}
   if((Get-FileHash -LiteralPath $framework).Hash -ne (Get-FileHash -LiteralPath $modern).Hash){throw 'Framework and modern primitive references differ'}
  }else{
   & $program (Join-Path $PSScriptRoot '../fixtures/gb18030-files-framework-reference.tsv');if($LASTEXITCODE -ne 0){throw 'Framework file reference failed'}
   & $RuntimeHost $program (Join-Path $PSScriptRoot '../fixtures/gb18030-files-reference.tsv') modern;if($LASTEXITCODE -ne 0){throw 'Modern file reference failed'}
  }
 }
 # Writes C++ source. Do not run while any C++ compiler is active.
 & node (Join-Path $projectRoot 'native/tools/generate-gb18030.mjs');if($LASTEXITCODE -ne 0){throw 'GB18030 static table generation failed'}
}
function Get-CanonicalHash([string]$Path,[bool]$Binary){$hash=[Security.Cryptography.SHA256]::Create();try{$bytes=if($Binary){[IO.File]::ReadAllBytes($Path)}else{[Text.Encoding]::UTF8.GetBytes([IO.File]::ReadAllText($Path).Replace("`r`n","`n"))};return ([BitConverter]::ToString($hash.ComputeHash($bytes))).Replace('-','').ToLowerInvariant()}finally{$hash.Dispose()}}
$files=@('reference/gb18030-encoding.cs','reference/gb18030-files.cs','reference/generate-gb18030.ps1','fixtures/gb18030-reference.bin','fixtures/gb18030-files-reference.tsv','fixtures/gb18030-files-framework-reference.tsv','fixtures/gb18030-reference.md','fixtures/encoding-aliases-reference.tsv','../tools/generate-gb18030.mjs')
$metadata=[ordered]@{schema=2;mapping_runtimes=@('Windows .NET Framework 4.0.30319.42000','Windows .NET 10.0.12');primitive_fixtures_identical=$true;code_page=54936;bmp_encoding_inputs=65536;single_byte_inputs=256;byte_pair_inputs=65536;four_byte_inputs=1587600;four_byte_splits_per_input=4;supplementary_scalars=1048576;variable_streaming_inputs=134400;file_recipes_per_runtime=786;legacy_file_differences=490;scope='Actual default/strict and streaming GB18030 primitive outcomes are byte-identical between both reference runtimes, including incomplete EOF prefix differences between stateless and retained decoders. Native implements the Framework GB18030 mapping with modern StreamReader flush behavior; the 490 differing legacy file rows remain explicit compatibility gaps. 124 of 140 Framework pages supported; 16 other codecs, full Encoding/class/module APIs and whole standard library remain unfinished. No Pascal compiler is invoked.';artifacts=@($files|ForEach-Object{$binary=$_.EndsWith('.bin');$item=[ordered]@{path=$_};$item[$(if($binary){'sha256_bytes'}else{'sha256_lf'})]=Get-CanonicalHash (Join-Path $PSScriptRoot "../$_") $binary;$item})}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '../fixtures/gb18030-reference.json'),(($metadata|ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))

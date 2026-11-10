# Actual public Framework API oracle. No Pascal compiler or application dependency.
param([switch]$ManifestOnly)
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
if(!$ManifestOnly){
 $compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
 foreach($name in @('encoding-metadata','encoding-max')){
  $program=Join-Path $projectRoot "native/build/$name.exe"
  & $compiler /nologo /target:exe ('/out:'+$program) (Join-Path $PSScriptRoot "$name.cs")
  if($LASTEXITCODE -ne 0){throw 'Encoding public API reference compilation failed'}
  if($name -eq 'encoding-metadata'){
   & $program (Join-Path $PSScriptRoot '../fixtures/encoding-positive-public-surface.tsv') (Join-Path $PSScriptRoot '../fixtures/encoding-metadata-reference.tsv')
  }else{
   & $program (Join-Path $PSScriptRoot '../fixtures/encoding-max-models.tsv') (Join-Path $PSScriptRoot '../fixtures/encoding-max-boundaries.tsv')
  }
  if($LASTEXITCODE -ne 0){throw 'Encoding public API reference execution failed'}
 }
 # This writes C++ source: no C++ compiler may be active.
 & node (Join-Path $projectRoot 'native/tools/generate-encoding-metadata.mjs')
 if($LASTEXITCODE -ne 0){throw 'Native encoding metadata generation failed'}
}
function Get-CanonicalHash([string]$Path){$hash=[Security.Cryptography.SHA256]::Create();try{$bytes=[Text.Encoding]::UTF8.GetBytes([IO.File]::ReadAllText($Path).Replace("`r`n","`n"));return ([BitConverter]::ToString($hash.ComputeHash($bytes))).Replace('-','').ToLowerInvariant()}finally{$hash.Dispose()}}
$files=@('reference/encoding-metadata.cs','reference/encoding-max.cs','reference/generate-encoding-metadata.ps1','fixtures/encoding-metadata-reference.tsv','fixtures/encoding-max-models.tsv','fixtures/encoding-max-boundaries.tsv','fixtures/encoding-metadata-reference.md','../tools/generate-encoding-metadata.mjs')
$metadata=[ordered]@{schema=1;runtime='Windows .NET Framework 4.0.30319.42000';codecs=140;native_properties=10;native_count_methods=2;actual_max_count_outcomes=18350080;actual_max_boundary_outcomes=6980;actual_total_count_checks=18357060;positive_public_declarations=197;scope='Actual Framework public API metadata for all 140 numbered default read-only codec objects. Native covers ten invariant properties and GetMaxByteCount/GetMaxCharCount. For each codec both count functions are called at every input 0..65535; little-endian Int32 result sequences have independent FNV64 fingerprints. Models are also checked against actual negative, large and Int32 overflow boundary calls. EncodingName en-US/ru-RU columns and the 197 public-declaration inventory are positive requirements, not implemented API coverage. Constructors, EncodingName/current UI culture, default/code-page zero, fallback/offset and whole module/library behavior remain unfinished. No Pascal compiler is invoked.';artifacts=@($files|ForEach-Object{[ordered]@{path=$_;sha256_lf=Get-CanonicalHash (Join-Path $PSScriptRoot "../$_")}});positive_requirements=@([ordered]@{path='fixtures/encoding-positive-public-surface.tsv';sha256_lf=Get-CanonicalHash (Join-Path $PSScriptRoot '../fixtures/encoding-positive-public-surface.tsv')})}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '../fixtures/encoding-metadata-reference.json'),(($metadata|ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))
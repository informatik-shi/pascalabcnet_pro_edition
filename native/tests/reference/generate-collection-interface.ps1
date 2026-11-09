# Development-only independent recipes, never installed.
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$program=Join-Path $projectRoot 'native/build/collection-interface-reference.exe'
& $compiler /nologo /target:exe "/out:$program" (Join-Path $PSScriptRoot 'collection-interface.cs')
if($LASTEXITCODE -ne 0){throw 'Reference compilation failed'}
& $program (Join-Path $PSScriptRoot '../fixtures/collection-interface-reference.tsv')
if($LASTEXITCODE -ne 0){throw 'Reference generation failed'}
function Get-CanonicalHash([string]$Path) {
 $hash=[Security.Cryptography.SHA256]::Create()
 try {return ([BitConverter]::ToString($hash.ComputeHash([Text.Encoding]::UTF8.GetBytes([IO.File]::ReadAllText($Path).Replace("`r`n","`n"))))).Replace('-','').ToLowerInvariant()}
 finally {$hash.Dispose()}
}
$files=@('reference/collection-interface.cs','reference/generate-collection-interface.ps1','fixtures/collection-interface-program.pas','fixtures/collection-interface-reference.tsv')
$artifacts=@($files | ForEach-Object {[ordered]@{path=$_;sha256_lf=Get-CanonicalHash (Join-Path $PSScriptRoot "../$_")}})
$metadata=[ordered]@{schema=1;runtime='Windows .NET Framework 4.0.30319.42000';outcomes=768;scope='Independent ICollection protocol and selected legacy Enumerable consumers; not complete unit coverage';artifacts=$artifacts}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '../fixtures/collection-interface-reference.json'),(($metadata | ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))

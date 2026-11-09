# Independent development-only recipes; never installed.
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$program=Join-Path $projectRoot 'native/build/list-interface-reference.exe'
& $compiler /nologo /target:exe "/out:$program" (Join-Path $PSScriptRoot 'list-interface.cs')
if($LASTEXITCODE -ne 0){throw 'Reference compilation failed'}
& $program (Join-Path $PSScriptRoot '../fixtures/list-interface-reference.tsv') (Join-Path $PSScriptRoot '../fixtures/school-ilist-reference.tsv')
if($LASTEXITCODE -ne 0){throw 'Reference generation failed'}
function Get-CanonicalHash([string]$Path){$hash=[Security.Cryptography.SHA256]::Create();try{return ([BitConverter]::ToString($hash.ComputeHash([Text.Encoding]::UTF8.GetBytes([IO.File]::ReadAllText($Path).Replace("`r`n","`n"))))).Replace('-','').ToLowerInvariant()}finally{$hash.Dispose()}}
$files=@('reference/list-interface.cs','reference/generate-list-interface.ps1','fixtures/list-interface-program.pas','fixtures/list-interface-reference.tsv','fixtures/school-ilist-program.pas','fixtures/school-ilist-reference.tsv')
$metadata=[ordered]@{schema=1;runtime='Windows .NET Framework 4.0.30319.42000';list_outcomes=456;school_outcomes=1152;scope='IList protocol, custom indexer callbacks and the shipped School.ReplaceMany IList algorithm; not whole unit coverage';artifacts=@($files | ForEach-Object {[ordered]@{path=$_;sha256_lf=Get-CanonicalHash (Join-Path $PSScriptRoot "../$_")}})}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '../fixtures/list-interface-reference.json'),(($metadata | ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))

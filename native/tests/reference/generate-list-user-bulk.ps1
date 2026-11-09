# Development only. Never invokes the original Pascal compiler.
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$program=Join-Path $projectRoot 'native/build/list-user-bulk-reference.exe'
& $compiler /nologo /target:exe "/out:$program" (Join-Path $PSScriptRoot 'list-user-bulk.cs') (Join-Path $PSScriptRoot 'list-user-record-bulk.cs')
if($LASTEXITCODE -ne 0){throw 'Reference compilation failed'}
& $program (Join-Path $PSScriptRoot '../fixtures/list-user-bulk-reference.tsv') (Join-Path $PSScriptRoot '../fixtures/list-user-record-bulk-reference.tsv')
if($LASTEXITCODE -ne 0){throw 'Reference generation failed'}
function Get-CanonicalHash([string]$Path){$hash=[Security.Cryptography.SHA256]::Create();try{return ([BitConverter]::ToString($hash.ComputeHash([Text.Encoding]::UTF8.GetBytes([IO.File]::ReadAllText($Path).Replace("`r`n","`n"))))).Replace('-','').ToLowerInvariant()}finally{$hash.Dispose()}}
$template=[IO.File]::ReadAllText((Join-Path $PSScriptRoot '../fixtures/list-user-bulk-program.pas.in'))
$files=@('reference/list-user-bulk.cs','reference/generate-list-user-bulk.ps1','fixtures/list-user-bulk-program.pas.in','fixtures/list-user-bulk-reference.tsv')
foreach($owner in 0,1){$file="fixtures/list-user-bulk-$owner.pas";$files+=$file;$kind=if($owner -eq 0){'record'}else{'class'};[IO.File]::WriteAllText((Join-Path $PSScriptRoot "../$file"),$template.Replace('%OWNER%',$kind),[Text.UTF8Encoding]::new($false))}
$files+=@('reference/list-user-record-bulk.cs','fixtures/list-user-record-bulk-program.pas.in','fixtures/list-user-record-bulk-reference.tsv')
$template=[IO.File]::ReadAllText((Join-Path $PSScriptRoot '../fixtures/list-user-record-bulk-program.pas.in'))
foreach($owner in 0,1){$file="fixtures/list-user-record-bulk-$owner.pas";$files+=$file;$kind=if($owner -eq 0){'record'}else{'class'};[IO.File]::WriteAllText((Join-Path $PSScriptRoot "../$file"),$template.Replace('%OWNER%',$kind),[Text.UTF8Encoding]::new($false))}
$metadata=[ordered]@{schema=1;runtime='Windows .NET Framework 4.0.30319.42000';outcomes=4200;record_outcomes=336;scope='Actual Framework List AddRange/InsertRange with class/record ICollection, integer/record elements, Count/CopyTo ordering, partial shifts, temporary arrays, capacity, retained backing arrays, reentrant Count/CopyTo callbacks, failures and enumerator versions. Not full List or standard-library coverage.';artifacts=@($files | ForEach-Object {[ordered]@{path=$_;sha256_lf=Get-CanonicalHash (Join-Path $PSScriptRoot "../$_")}})}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '../fixtures/list-user-bulk-reference.json'),(($metadata | ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))

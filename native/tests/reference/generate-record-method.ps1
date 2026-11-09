# Development-only oracle; no original Pascal compiler is invoked.
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$program=Join-Path $projectRoot 'native/build/record-method-reference.exe'
& $compiler /nologo /target:exe "/out:$program" (Join-Path $PSScriptRoot 'record-method.cs') (Join-Path $PSScriptRoot 'record-collection.cs') (Join-Path $PSScriptRoot 'record-list-alias.cs')
if($LASTEXITCODE -ne 0){throw 'Reference compilation failed'}
& $program (Join-Path $PSScriptRoot '../fixtures/record-method-reference.tsv') (Join-Path $PSScriptRoot '../fixtures/record-collection-reference.tsv') (Join-Path $PSScriptRoot '../fixtures/record-list-alias-reference.tsv')
if($LASTEXITCODE -ne 0){throw 'Reference generation failed'}
function Get-CanonicalHash([string]$Path){$hash=[Security.Cryptography.SHA256]::Create();try{return ([BitConverter]::ToString($hash.ComputeHash([Text.Encoding]::UTF8.GetBytes([IO.File]::ReadAllText($Path).Replace("`r`n","`n"))))).Replace('-','').ToLowerInvariant()}finally{$hash.Dispose()}}
$template=[IO.File]::ReadAllText((Join-Path $PSScriptRoot '../fixtures/record-list-alias-program.pas.in'))
$files=@('reference/record-method.cs','reference/record-collection.cs','reference/record-list-alias.cs','reference/generate-record-method.ps1','fixtures/record-method-program.pas','fixtures/record-method-reference.tsv','fixtures/record-collection-program.pas','fixtures/record-collection-reference.tsv','fixtures/record-list-alias-program.pas.in','fixtures/record-list-alias-reference.tsv')
foreach($owner in 0,1){foreach($kind in 0,1){
 $values=if($kind -eq 0){@{ELEMENT='integer';MAKE='function Make(n:integer):V:=n;';PRINT='a.ToArray';UPDATE='state.Saved[0]+=5';BUMP='state.Saved[0]+=5';RHS='state.Saved[0]+=Rhs(data)'}}else{@{ELEMENT='Item';MAKE='function Make(n:integer):V;begin Result.X:=n;end;';PRINT='a.Select(v->v.X).ToArray';UPDATE='state.Saved[0].X+=5';BUMP='state.Saved[0].Bump(5)';RHS='state.Saved[0].X+=Rhs(data)'}}
 $values.OWNER=if($owner -eq 0){'record'}else{'class'};$source=$template;foreach($key in $values.Keys){$source=$source.Replace('%'+$key+'%',$values[$key])}
 $file="fixtures/record-list-alias-$owner-$kind.pas";$files+=$file;[IO.File]::WriteAllText((Join-Path $PSScriptRoot "../$file"),$source,[Text.UTF8Encoding]::new($false))
}}
$metadata=[ordered]@{schema=1;runtime='Windows .NET Framework 4.0.30319.42000';outcomes=1320;collection_outcomes=252;retained_array_outcomes=1560;scope='Selected user-record mutable methods/properties/constructors, value/reference parameters, boxing, checked casts, generic equality, collection/enumerator consumers and retained CopyTo arrays shared with List. Not full record language or library coverage.';artifacts=@($files | ForEach-Object {[ordered]@{path=$_;sha256_lf=Get-CanonicalHash (Join-Path $PSScriptRoot "../$_")}})}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '../fixtures/record-method-reference.json'),(($metadata | ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))

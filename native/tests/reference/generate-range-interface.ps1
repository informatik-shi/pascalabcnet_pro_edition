# Development-only independent reference; never installed.
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$program=Join-Path $projectRoot 'native/build/range-interface-reference.exe'
& $compiler /nologo /target:exe "/out:$program" (Join-Path $PSScriptRoot 'range-interface.cs')
if($LASTEXITCODE -ne 0){throw 'Reference compilation failed'}
& $program (Join-Path $PSScriptRoot '../fixtures/range-interface-reference.tsv')
if($LASTEXITCODE -ne 0){throw 'Reference generation failed'}
$template=[IO.File]::ReadAllText((Join-Path $PSScriptRoot '../fixtures/range-interface-program.pas.in'))
foreach($kind in 0,1){
 $values=if($kind -eq 0){@{RANGE='IntRange';ELEMENT='integer';NUMBER='n';LOW='low';HIGH='high';ITEM='item';NEXTLOW='low+1';NEXTHIGH='high+1';ARRAY='data';COPYLOW='copy.Low';COPYHIGH='copy.High'}}else{@{RANGE='CharRange';ELEMENT='char';NUMBER='Ord(n)';LOW='Chr(low and 65535)';HIGH='Chr(high and 65535)';ITEM='Chr(item and 65535)';NEXTLOW='Chr((low+1) and 65535)';NEXTHIGH='Chr((high+1) and 65535)';ARRAY='data.Select(n->Ord(n)).ToArray';COPYLOW='Ord(copy.Low)';COPYHIGH='Ord(copy.High)'}}
 $source=$template;foreach($key in $values.Keys){$source=$source.Replace('%'+$key+'%',$values[$key])}
 [IO.File]::WriteAllText((Join-Path $PSScriptRoot "../fixtures/range-interface-$kind.pas"),$source,[Text.UTF8Encoding]::new($false))
}
function Get-CanonicalHash([string]$Path){$hash=[Security.Cryptography.SHA256]::Create();try{return ([BitConverter]::ToString($hash.ComputeHash([Text.Encoding]::UTF8.GetBytes([IO.File]::ReadAllText($Path).Replace("`r`n","`n"))))).Replace('-','').ToLowerInvariant()}finally{$hash.Dispose()}}
$files=@('reference/range-interface.cs','reference/generate-range-interface.ps1','fixtures/range-interface-program.pas.in','fixtures/range-interface-0.pas','fixtures/range-interface-1.pas','fixtures/range-interface-reference.tsv')
$metadata=[ordered]@{schema=1;runtime='Windows .NET Framework 4.0.30319.42000';outcomes=2688;scope='Native source range collection/equality interfaces, boxed identity, unboxing and selected consumers; not full user-record language or library coverage';artifacts=@($files | ForEach-Object {[ordered]@{path=$_;sha256_lf=Get-CanonicalHash (Join-Path $PSScriptRoot "../$_")}})}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '../fixtures/range-interface-reference.json'),(($metadata | ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))

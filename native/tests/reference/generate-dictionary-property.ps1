# Independent development-only fixtures. No original Pascal compiler is invoked.
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$program=Join-Path $projectRoot 'native/build/dictionary-property-reference.exe'
& $compiler /nologo /target:exe "/out:$program" (Join-Path $PSScriptRoot 'dictionary-property.cs')
if($LASTEXITCODE -ne 0){throw 'Reference compilation failed'}
& $program (Join-Path $PSScriptRoot '../fixtures/dictionary-property-reference.tsv') (Join-Path $PSScriptRoot '../fixtures/dictionary-property-lambda-reference.tsv')
if($LASTEXITCODE -ne 0){throw 'Reference generation failed'}
$template=[IO.File]::ReadAllText((Join-Path $PSScriptRoot '../fixtures/dictionary-property-program.pas.in'))
$kinds=@(
 @{Type='integer';A1='2';A2='3';B1='20';B2='30';Seed='0';Rhs='5';Mutate="a['a']:=99;";Ops=@(0,1,2)},
 @{Type='real';A1='2.5';A2='3.0';B1='20.5';B2='30.0';Seed='0.0';Rhs='2.0';Mutate="a['a']:=99.0;";Ops=@(0,1,2,3)},
 @{Type='string';A1="'x'";A2="'y'";B1="'u'";B2="'v'";Seed='nil';Rhs="'z'";Mutate="a['a']:='Q';";Ops=@(0,2)},
 @{Type='array of integer';A1='Arr(1,2)';A2='Arr(3,4)';B1='Arr(5,6)';B2='Arr(7,8)';Seed='nil';Rhs='Arr(9)';Mutate="a['a'][0]:=99;";Ops=@(0,2)},
 @{Type='List<integer>';A1='Lst(1,2)';A2='Lst(3,4)';B1='Lst(5,6)';B2='Lst(7,8)';Seed='nil';Rhs='Lst(9)';Mutate="a['a'].Add(99);";Ops=@(0,2)}
)
$files=@('reference/dictionary-property.cs','reference/generate-dictionary-property.ps1','fixtures/dictionary-property-program.pas.in','fixtures/dictionary-property-reference.tsv','fixtures/dictionary-property-lambda-reference.tsv')
for($kind=0;$kind -lt $kinds.Count;$kind++){
 $definition=$kinds[$kind]
 foreach($op in $definition.Ops){
  $source=$template
  foreach($key in @('Type','A1','A2','B1','B2','Seed','Mutate')){$source=$source.Replace('@@'+$key.ToUpperInvariant()+'@@',$definition[$key])}
  $count=$kind -ge 2 -and $op -eq 2
  $source=$source.Replace('@@RHS_TYPE@@',$(if($count){'integer'}else{$definition.Type})).Replace('@@RHS@@',$(if($count){'2'}else{$definition.Rhs})).Replace('@@OP@@',@('+=','-=','*=','/=')[$op])
  $file="fixtures/dictionary-property-$kind-$op.pas";$files+=$file
  [IO.File]::WriteAllText((Join-Path $PSScriptRoot "../$file"),$source.Replace("`r`n","`n"),[Text.UTF8Encoding]::new($false))
 }
}
function Get-CanonicalHash([string]$Path){$hash=[Security.Cryptography.SHA256]::Create();try{return ([BitConverter]::ToString($hash.ComputeHash([Text.Encoding]::UTF8.GetBytes([IO.File]::ReadAllText($Path).Replace("`r`n","`n"))))).Replace('-','').ToLowerInvariant()}finally{$hash.Dispose()}}
$metadata=[ordered]@{schema=1;runtime='Windows .NET Framework 4.0.30319.42000';outcomes=9984;lambda_outcomes=6;scope='Independent explicitly lowered Pascal dictionary property assignments, not whole unit coverage';artifacts=@($files | ForEach-Object {[ordered]@{path=$_;sha256_lf=Get-CanonicalHash (Join-Path $PSScriptRoot "../$_")}})}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '../fixtures/dictionary-property-reference.json'),(($metadata | ConvertTo-Json -Depth 5).Replace("`r`n","`n")+"`n"),[Text.UTF8Encoding]::new($false))

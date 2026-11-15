# Regenerates the original PascalABC.NET file outcomes in native/build.
param([switch]$Verify)

$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$fixtures=Join-Path $PSScriptRoot '../fixtures'
$scratch=Join-Path $projectRoot ('native/build/file-operations-source-'+[guid]::NewGuid().ToString('N'))
$compiler=Join-Path $projectRoot 'bin/pabcnetcclear.exe'
$null=New-Item -ItemType Directory -Path $scratch -Force
$utf8=[Text.UTF8Encoding]::new($false)

function Get-SourceOutput([string]$Source,[string]$Output,[string]$InputPath) {
    & $compiler $Source | Out-Host
    if($LASTEXITCODE -ne 0){throw "Original Pascal compilation failed for $Source"}
    $command=@{
        FilePath=[IO.Path]::ChangeExtension($Source,'.exe')
        WorkingDirectory=$scratch
        RedirectStandardOutput=$Output
        Wait=$true
        PassThru=$true
        WindowStyle='Hidden'
    }
    if($InputPath){$command.RedirectStandardInput=$InputPath}
    $process=Start-Process @command
    if($process.ExitCode -ne 0){throw "Original Pascal program failed for $Source"}
}

$suite=Join-Path $scratch 'files-operations-source-compat.pas'
$suiteOutput=[IO.Path]::ChangeExtension($suite,'.out')
Copy-Item -LiteralPath (Join-Path $fixtures 'files-operations-source-compat.pas') -Destination $suite
Get-SourceOutput -Source $suite -Output $suiteOutput

$requests=[Collections.Generic.List[string]]::new()
$modern=[Collections.Generic.List[string]]::new()
$rows=[Collections.Generic.List[int]]::new()
$index=-1
foreach($recipe in Get-Content -LiteralPath (Join-Path $fixtures 'file-operations-reference.tsv')){
    if(!$recipe.Length -or $recipe.StartsWith('#')){continue}
    ++$index
    $fields=$recipe -split [char]9
    if($fields[0] -ne 'R'){continue}
    $bytes=[Convert]::FromHexString($fields[2].Replace('-',''))
    [IO.File]::WriteAllBytes((Join-Path $scratch "raw-$index.bin"),$bytes)
    $requests.Add("$($fields[1]) $index")
    $modern.Add($fields[3])
    $rows.Add($index)
}
$readSource=Join-Path $scratch 'file-operations-source-read.pas'
$readProgram=@'
program FilesOperationsSourceRead;
uses FilesOperations;
begin
  loop #COUNT# do
  begin
    var cp, number: integer;
    Readln(cp,number);
    DefaultEncoding:=Encoding.GetEncoding(cp);
    var data:=ReadFileToEnd('raw-'+IntToStr(number)+'.bin');
    foreach var c in data do
      if c <> #13 then Write(Ord(c).ToString('x4'));
    Writeln;
  end;
end.
'@
[IO.File]::WriteAllText($readSource,$readProgram.Replace('#COUNT#',$requests.Count),$utf8)
$inputPath=Join-Path $scratch 'file-operations-source-read.in'
$readOutput=[IO.Path]::ChangeExtension($readSource,'.out')
[IO.File]::WriteAllLines($inputPath,$requests,$utf8)
Get-SourceOutput -Source $readSource -Output $readOutput -InputPath $inputPath

$outputs=@(Get-Content -LiteralPath $readOutput)
if($outputs.Count -ne $requests.Count){throw 'Original read result count differs from recipe count'}
$delta=[Collections.Generic.List[string]]::new()
$delta.Add('# row: zero-based recipe index excluding comments and blank rows; original PascalABC.NET ReadFileToEnd UTF-16 hex')
for($n=0;$n -lt $outputs.Count;++$n){
    $actual=if($outputs[$n].Length){$outputs[$n]}else{'-'}
    if($actual -cne $modern[$n]){$delta.Add("$($rows[$n])"+[char]9+$actual)}
}
$deltaPath=Join-Path $scratch 'file-operations-framework-read-delta.tsv'
[IO.File]::WriteAllLines($deltaPath,$delta,$utf8)
if($Verify){
    foreach($generated in @($suiteOutput,$deltaPath)){
        $expected=Join-Path $fixtures ([IO.Path]::GetFileName($generated))
        $actualText=[IO.File]::ReadAllText($generated).Replace([string]([char]13)+[char]10,[string][char]10)
        $expectedText=[IO.File]::ReadAllText($expected).Replace([string]([char]13)+[char]10,[string][char]10)
        if($actualText -cne $expectedText){throw "Original results differ from $expected"}
    }
}
Get-Item -LiteralPath $suiteOutput,$deltaPath | Select-Object FullName,Length

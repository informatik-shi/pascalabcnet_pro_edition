[CmdletBinding()]
param([string]$DotnetRoot = "$env:USERPROFILE\.dotnet")
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$tests = Join-Path $root 'tests\LanguageLoading.Tests'
& (Join-Path $DotnetRoot 'dotnet.exe') build (Join-Path $tests 'LanguageLoading.Tests.csproj') -c Release -p:NuGetAudit=false --nologo -v:q --tl:off
if ($LASTEXITCODE -ne 0) { throw 'Language loading test build failed.' }
$output = Join-Path $tests 'bin\Release\net472'
$fixture = Join-Path $tests 'Fixture\bin\Release\net472\PascalABCLanguageInfo.dll'
$cases = Join-Path $root ('.abcpro-build\language-tests-' + [guid]::NewGuid().ToString('N'))
foreach ($mode in @('normal','missing','optional','corrupt','blocked')) {
    $directory = Join-Path $cases $mode
    New-Item -ItemType Directory -Path $directory -Force | Out-Null
    foreach ($name in @('LanguageLoading.Tests.exe','LanguageLoading.Tests.exe.config','Contracts.dll')) {
        Copy-Item -LiteralPath (Join-Path $output $name) -Destination $directory
    }
    $main = Join-Path $directory 'PascalABCLanguageInfo.dll'
    if ($mode -eq 'missing') { Copy-Item -LiteralPath $fixture -Destination (Join-Path $directory 'SPythonLanguageInfo.dll') }
    elseif ($mode -eq 'corrupt') { [IO.File]::WriteAllText($main, 'invalid assembly fixture') }
    else { Copy-Item -LiteralPath $fixture -Destination $main }
    if ($mode -eq 'optional') { [IO.File]::WriteAllText((Join-Path $directory 'SPythonLanguageInfo.dll'), 'invalid optional fixture') }
    if ($mode -eq 'blocked') { Set-Content -LiteralPath $main -Stream Zone.Identifier -Value @('[ZoneTransfer]','ZoneId=3') -Encoding ascii }
    & (Join-Path $directory 'LanguageLoading.Tests.exe') $mode
    if ($LASTEXITCODE -ne 0) { throw "Language loading check failed: $mode" }
}

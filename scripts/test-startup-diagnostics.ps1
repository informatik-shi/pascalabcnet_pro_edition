[CmdletBinding()]
param([string]$DotnetRoot = "$env:USERPROFILE\.dotnet")
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$project = Join-Path $root 'tests\StartupDiagnostics.Tests\StartupDiagnostics.Tests.csproj'
& (Join-Path $DotnetRoot 'dotnet.exe') build $project -c Release -p:NuGetAudit=false --nologo -v:q --tl:off
if ($LASTEXITCODE -ne 0) { throw 'Diagnostic test build failed.' }
& (Join-Path $root 'tests\StartupDiagnostics.Tests\bin\Release\net472\StartupDiagnostics.Tests.exe')
if ($LASTEXITCODE -ne 0) { throw 'Diagnostic handler checks failed.' }

# Execute the production bootstrap with only its message-box function replaced.
# The real error path and file writing run before any managed launcher exists.
$fixture = Join-Path $root ('.abcpro-build\bootstrap-test-' + [guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($fixture) | Out-Null
$source = Get-Content -LiteralPath (Join-Path $root 'PortableDistribution\Portable\launch-ide.ps1') -Raw
$tokens = $null; $parseErrors = $null
$ast = [Management.Automation.Language.Parser]::ParseInput($source, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw "Bootstrap parse errors: $parseErrors" }
$dialog = $ast.Find({ param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Show-LaunchError' }, $true)
$source = $source.Substring(0, $dialog.Extent.StartOffset) + 'function Show-LaunchError([string]$message) { Write-Output $message }' + $source.Substring($dialog.Extent.EndOffset)
$script = Join-Path $fixture 'bootstrap.ps1'
[IO.File]::WriteAllText($script, $source, [Text.UTF8Encoding]::new($true))
$blocked = Join-Path $fixture 'blocked'
[IO.File]::WriteAllText($blocked, 'not a directory')
$oldLocal = $env:LOCALAPPDATA; $oldTemp = $env:TEMP; $oldRoaming = $env:APPDATA
try {
    $env:LOCALAPPDATA = $blocked; $env:TEMP = $fixture; $env:APPDATA = $blocked
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $script -Root (Join-Path $fixture 'missing-package')
    if ($LASTEXITCODE -ne 1) { throw 'Early missing launcher must return 1.' }
    $report = @(Get-ChildItem -LiteralPath (Join-Path $fixture 'abc.pro\Logs') -Filter 'bootstrap-*.log')
    if ($report.Count -ne 1) { throw 'Early bootstrap fallback report not found.' }
    $text = Get-Content -LiteralPath $report[0].FullName -Raw
    if (-not $text.Contains('FAILURE (observed)') -or -not $text.Contains('missing-package') -or -not $text.Contains('Bootstrap started')) { throw 'Early bootstrap report is incomplete.' }
    Write-Host 'PASS: early missing launcher / unavailable primary log directory / fallback bootstrap report'
} finally {
    $env:LOCALAPPDATA = $oldLocal; $env:TEMP = $oldTemp; $env:APPDATA = $oldRoaming
}

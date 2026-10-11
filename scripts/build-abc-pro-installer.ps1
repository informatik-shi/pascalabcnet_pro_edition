[CmdletBinding()]
param(
    [string]$PackageRoot = 'Release\PascalABCNET-Portable-win-x64',
    [string]$OutputRoot = 'Release-abc-pro',
    [string]$DotnetRoot = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$package = [IO.Path]::GetFullPath((Join-Path $root $PackageRoot))
$release = [IO.Path]::GetFullPath((Join-Path $root $OutputRoot))
if (-not $release.StartsWith($root + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw "Installer output must stay inside the repository: $release"
}
foreach ($required in @(
    'bin\PascalABCNET.exe', 'bin\Lib\SPython\numpy1.pcu',
    'bin\Lib\SPython\pandas1.pcu', 'bin-net10\pabcnetcclear.exe',
    'dotnet\dotnet.exe', 'python\python.exe',
    'notebook\PascalABCNotebook.dll', 'Work\Samples', 'Work\Files'
)) {
    if (-not (Test-Path -LiteralPath (Join-Path $package $required))) {
        throw "Incomplete portable package: $required ($package)"
    }
}

if (-not $DotnetRoot) {
    foreach ($candidate in @($env:DOTNET_ROOT, (Join-Path $env:USERPROFILE '.dotnet'),
            'C:\Program Files\dotnet')) {
        if ($candidate -and (Test-Path -LiteralPath (Join-Path $candidate 'sdk'))) {
            $DotnetRoot = $candidate
            break
        }
    }
}
if (-not $DotnetRoot) { throw 'A .NET SDK is required. Pass -DotnetRoot.' }
$DotnetRoot = [IO.Path]::GetFullPath($DotnetRoot)
$dotnet = Join-Path $DotnetRoot 'dotnet.exe'
$env:DOTNET_ROOT = $DotnetRoot
$env:PATH = "$DotnetRoot;$env:PATH"

$versionFile = Join-Path $root 'ReleaseGenerators\PascalABCNET_version.nsh'
$versionText = Get-Content -LiteralPath $versionFile -Raw
if ($versionText -notmatch "!define VERSION .([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).") {
    throw "Cannot read four-part version from $versionFile"
}
$version = $Matches[1]

$ideProject = Join-Path $root 'VisualPascalABCNET\VisualPascalABCNET.csproj'
$launcherProject = Join-Path $root 'ABCProLauncher\ABCProLauncher.csproj'
foreach ($project in @($ideProject, $launcherProject)) {
    & $dotnet build $project --configuration Release "-p:Version=$version" -p:NuGetAudit=false --disable-build-servers --nologo -v:q --tl:off
    if ($LASTEXITCODE -ne 0) { throw "Build failed: $project" }
}

New-Item -ItemType Directory -Path $release -Force | Out-Null
$output = Join-Path $release "abc.pro-Setup-$version-win-x64.exe"
$launcher = Join-Path $root 'ABCProLauncher\bin\Release\net472\abc.pro.exe'
$brandedIde = Join-Path $root 'bin\PascalABCNET.exe'
$notebookLauncher = Join-Path $root 'ABCProLauncher\abc.pro-notebook.cmd'
$ideCommand = Join-Path $root 'ABCProLauncher\PascalABCNET.cmd'
$notebookCommand = Join-Path $root 'ABCProLauncher\PascalABCNotebook.cmd'
$makensis = Join-Path $root 'Utils\NSIS2\makensis.exe'
$script = Join-Path $root 'ReleaseGenerators\abc-pro.nsi'
foreach ($file in @($launcher, $brandedIde, $notebookLauncher, $ideCommand,
        $notebookCommand, $makensis, $script)) {
    if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { throw "Missing file: $file" }
}

Push-Location (Join-Path $root 'ReleaseGenerators')
try {
    & $makensis "/DSOURCE_DIR=$package" "/DLAUNCHER=$launcher" "/DBRANDED_IDE=$brandedIde" "/DNOTEBOOK_LAUNCHER=$notebookLauncher" "/DIDE_COMMAND=$ideCommand" "/DNOTEBOOK_COMMAND=$notebookCommand" "/DOUTPUT_FILE=$output" "/DAPP_VERSION=$version" $script
    if ($LASTEXITCODE -ne 0) { throw 'NSIS installer build failed.' }
} finally { Pop-Location }
if (-not (Test-Path -LiteralPath $output -PathType Leaf)) {
    throw "Installer was not produced: $output"
}
Write-Host "abc.pro installer: $output"

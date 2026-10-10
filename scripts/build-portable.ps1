[CmdletBinding()]
param(
    [switch]$SkipBuild,
    [string]$DotnetRoot = '',
    [string]$OutputRoot = '',
    [string]$PythonExe = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$release = if ($OutputRoot) {
    [IO.Path]::GetFullPath((Join-Path $root $OutputRoot))
} else {
    Join-Path $root 'Release'
}
if (-not $release.StartsWith($root + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw "Portable output must stay inside the repository: $release"
}
$packageName = 'PascalABCNET-Portable-win-x64'
$stage = [IO.Path]::GetFullPath((Join-Path $release $packageName))
$zip = [IO.Path]::GetFullPath((Join-Path $release ($packageName + '.zip')))
$notebookPublish = Join-Path $root '.codex-build\notebook-publish'

if ([string]::IsNullOrWhiteSpace($DotnetRoot)) {
    foreach ($candidate in @($env:DOTNET_ROOT, (Join-Path $env:USERPROFILE '.dotnet'),
            'C:\Program Files\dotnet')) {
        if ($candidate -and (Test-Path -LiteralPath (Join-Path $candidate 'sdk'))) {
            $DotnetRoot = $candidate
            break
        }
    }
}
if (-not $DotnetRoot) { throw 'A .NET 10 SDK is required to build the portable package. Pass -DotnetRoot.' }
$DotnetRoot = [IO.Path]::GetFullPath($DotnetRoot)
$dotnet = Join-Path $DotnetRoot 'dotnet.exe'
if (-not (Test-Path -LiteralPath $dotnet)) { throw "Missing dotnet.exe: $dotnet" }
$env:PATH = "$DotnetRoot;$env:PATH"
$env:DOTNET_ROOT = $DotnetRoot

function Assert-File([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Missing build output: $Path" }
}

if (-not $SkipBuild) {
    & $dotnet build (Join-Path $root 'PascalABCNET.sln') --configuration Release `
        -p:PABCNET_LEGACY_ONLY=true -p:NuGetAudit=false --disable-build-servers --nologo -v:q --tl:off
    if ($LASTEXITCODE -ne 0) { throw 'Legacy IDE build failed.' }

    & (Join-Path $PSScriptRoot 'build-net10-runtime.ps1') -Configuration Release
    & $dotnet build (Join-Path $root 'VisualPlugins\CompileNet10\CompileNet10.csproj') `
        --configuration Release -p:NuGetAudit=false --disable-build-servers --nologo -v:q --tl:off
    if ($LASTEXITCODE -ne 0) { throw '.NET 10 IDE plugin build failed.' }

    & $dotnet publish (Join-Path $root 'PascalABCNotebook\PascalABCNotebook.csproj') `
        --configuration Release --output $notebookPublish `
        -p:NuGetAudit=false --disable-build-servers --nologo -v:q --tl:off
    if ($LASTEXITCODE -ne 0) { throw 'Notebook build failed.' }

    $pcuBuildDirectory = Join-Path $root 'ReleaseGenerators'
    $libDllNames = @(Get-ChildItem -LiteralPath (Join-Path $root 'bin\Lib') -File -Filter '*.dll' |
        Select-Object -ExpandProperty Name)
    $preexistingDlls = @($libDllNames | Where-Object {
        Test-Path -LiteralPath (Join-Path $pcuBuildDirectory $_)
    })
    Push-Location $pcuBuildDirectory
    try {
        & (Join-Path $root 'bin\pabcnetcclear.exe') 'RebuildStandartModules.pas'
        if ($LASTEXITCODE -ne 0) { throw 'Classic standard units build failed.' }
        & (Join-Path $root 'bin\pabcnetcclear.exe') 'RebuildStandartModulesSPython.pas'
        if ($LASTEXITCODE -ne 0) { throw 'SPython standard units build failed.' }
        & (Join-Path $root 'bin-net10\pabcnetcclear.exe') 'RebuildStandartModulesNet10.pas'
        if ($LASTEXITCODE -ne 0) { throw '.NET 10 standard units build failed.' }
        & (Join-Path $root 'bin-net10\pabcnetcclear.exe') 'RebuildStandartModulesSPython.pas'
        if ($LASTEXITCODE -ne 0) { throw '.NET 10 SPython standard units build failed.' }
        foreach ($extension in @('.exe', '.exe.config', '.pdb', '.runtimeconfig.json')) {
            $generated = Join-Path $root ('ReleaseGenerators\RebuildStandartModulesNet10' + $extension)
            if (Test-Path -LiteralPath $generated) { Remove-Item -LiteralPath $generated -Force }
        }
        $generated = Join-Path $root 'ReleaseGenerators\RebuildStandartModulesSPython.runtimeconfig.json'
        if (Test-Path -LiteralPath $generated) { Remove-Item -LiteralPath $generated -Force }
    }
    finally {
        Pop-Location
        foreach ($name in $libDllNames) {
            if ($name -in $preexistingDlls) { continue }
            $generated = Join-Path $pcuBuildDirectory $name
            if (Test-Path -LiteralPath $generated) { Remove-Item -LiteralPath $generated -Force }
        }
    }
}

$bin = Join-Path $root 'bin'
$modern = Join-Path $root 'bin-net10'
foreach ($file in @('PascalABCNET.exe', 'pabcnetc.exe', 'pabcnetcclear.exe',
        'Compiler.dll', 'CompileNet10Plugin.dll', 'CompileNet10Plugin.ini',
        'Lib\PABCRtl.dll', 'Lib\PABCSystem.pcu')) {
    Assert-File (Join-Path $bin $file)
}
Assert-File (Join-Path $modern 'net10-runtime.manifest.json')
if (@(Get-ChildItem -LiteralPath (Join-Path $modern 'Lib') -Recurse -File -Filter '*.pcu').Count -lt 26) {
    throw 'Missing .NET 10 standard units; run a full portable build.'
}

$pythonRuntime = & (Join-Path $PSScriptRoot 'prepare-python-matplotlib.ps1') -PythonExe $PythonExe

$fxrVersion = @(Get-ChildItem -LiteralPath (Join-Path $DotnetRoot 'host\fxr') -Directory |
    Where-Object Name -Like '10.*' | Sort-Object { [version]$_.Name } -Descending |
    Select-Object -First 1 -ExpandProperty Name)
$coreVersion = @(Get-ChildItem -LiteralPath (Join-Path $DotnetRoot 'shared\Microsoft.NETCore.App') -Directory |
    Where-Object Name -Like '10.*' | Sort-Object { [version]$_.Name } -Descending |
    Select-Object -First 1 -ExpandProperty Name)
$desktopVersion = @(Get-ChildItem -LiteralPath (Join-Path $DotnetRoot 'shared\Microsoft.WindowsDesktop.App') -Directory |
    Where-Object Name -Like '10.*' | Sort-Object { [version]$_.Name } -Descending |
    Select-Object -First 1 -ExpandProperty Name)
if (-not $fxrVersion -or -not $coreVersion -or -not $desktopVersion) {
    throw 'The .NET 10 host, core runtime, and Windows Desktop runtime must be installed in DotnetRoot.'
}

New-Item -ItemType Directory -Path $release -Force | Out-Null
# Delete only the two fixed release outputs below the verified repository Release directory.
if (-not $stage.StartsWith($release + '\', [StringComparison]::OrdinalIgnoreCase) -or
    -not $zip.StartsWith($release + '\', [StringComparison]::OrdinalIgnoreCase) -or
    [IO.Path]::GetFileName($stage) -ne $packageName -or
    [IO.Path]::GetFileName($zip) -ne ($packageName + '.zip')) {
    throw 'Unsafe portable release path.'
}
if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
if (Test-Path -LiteralPath $zip) { Remove-Item -LiteralPath $zip -Force }
New-Item -ItemType Directory -Path $stage -Force | Out-Null

Copy-Item -LiteralPath $bin -Destination (Join-Path $stage 'bin') -Recurse -Force
$stageBin = Join-Path $stage 'bin'
Get-ChildItem -LiteralPath $stageBin -Recurse -File -Filter '*.pdb' | Remove-Item -Force
$oldIni = Join-Path $stageBin 'pabcworknet.ini'
if (Test-Path -LiteralPath $oldIni) { Remove-Item -LiteralPath $oldIni -Force }

$libSource = Join-Path $stageBin 'LibSource'
Get-ChildItem -LiteralPath (Join-Path $stageBin 'Lib') -Recurse -File |
    Where-Object { $_.Extension -in @('.pas', '.vb') } | ForEach-Object {
        $relative = $_.FullName.Substring((Join-Path $stageBin 'Lib').Length + 1)
        $target = Join-Path $libSource $relative
        New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
        Copy-Item -LiteralPath $_.FullName -Destination $target -Force
    }

$modernStage = Join-Path $stage 'bin-net10'
& (Join-Path $PSScriptRoot 'export-net10-runtime.ps1') -Destination $modernStage -IncludeCompiledUnits
Assert-File (Join-Path $modernStage 'pabcnetc.dll')

$runtimeStage = Join-Path $stage 'dotnet'
New-Item -ItemType Directory -Path (Join-Path $runtimeStage 'host\fxr') -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $runtimeStage 'shared\Microsoft.NETCore.App') -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $runtimeStage 'shared\Microsoft.WindowsDesktop.App') -Force | Out-Null
Copy-Item -LiteralPath $dotnet -Destination $runtimeStage -Force
foreach ($notice in @('LICENSE.txt', 'ThirdPartyNotices.txt')) {
    $source = Join-Path $DotnetRoot $notice
    if (Test-Path -LiteralPath $source) { Copy-Item -LiteralPath $source -Destination $runtimeStage -Force }
}
Copy-Item -LiteralPath (Join-Path $DotnetRoot "host\fxr\$fxrVersion") `
    -Destination (Join-Path $runtimeStage 'host\fxr') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $DotnetRoot "shared\Microsoft.NETCore.App\$coreVersion") `
    -Destination (Join-Path $runtimeStage 'shared\Microsoft.NETCore.App') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $DotnetRoot "shared\Microsoft.WindowsDesktop.App\$desktopVersion") `
    -Destination (Join-Path $runtimeStage 'shared\Microsoft.WindowsDesktop.App') -Recurse -Force

$pythonStage = Join-Path $stage 'python'
Copy-Item -LiteralPath $pythonRuntime.Runtime -Destination $pythonStage -Recurse -Force
$pythonSite = Join-Path $pythonStage 'Lib\site-packages'
New-Item -ItemType Directory -Path $pythonSite -Force | Out-Null
Copy-Item -Path (Join-Path $pythonRuntime.Packages '*') -Destination $pythonSite -Recurse -Force
Set-Content -LiteralPath (Join-Path $pythonStage 'python313._pth') `
    -Value @('python313.zip', '.', 'Lib\site-packages') -Encoding ascii
& (Join-Path $pythonStage 'python.exe') -c `
    'import matplotlib, numpy; print("Bundled Python:", matplotlib.__version__, numpy.__version__)'
if ($LASTEXITCODE -ne 0) { throw 'Bundled Matplotlib does not start.' }

Copy-Item -Path (Join-Path $root 'PortableDistribution\*') -Destination $stage -Recurse -Force
Assert-File (Join-Path $notebookPublish 'PascalABCNotebook.dll')
Copy-Item -LiteralPath $notebookPublish -Destination (Join-Path $stage 'notebook') -Recurse -Force
Assert-File (Join-Path $stage 'notebook\wwwroot\index.html')
foreach ($document in @('License.txt', 'License_en.txt', 'copyright.txt')) {
    Copy-Item -LiteralPath (Join-Path $root "ReleaseGenerators\$document") -Destination $stage -Force
}
New-Item -ItemType Directory -Path (Join-Path $stage 'Work') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'InstallerSamples') `
    -Destination (Join-Path $stage 'Work\Samples') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'ReleaseGenerators\Files') `
    -Destination (Join-Path $stage 'Work\Files') -Recurse -Force

$env:DOTNET_ROOT = $runtimeStage
$env:DOTNET_ROOT_X64 = $runtimeStage
& (Join-Path $runtimeStage 'dotnet.exe') --list-runtimes | Out-Host
if ($LASTEXITCODE -ne 0) { throw 'Bundled .NET runtime does not start.' }
Compress-Archive -LiteralPath $stage -DestinationPath $zip -CompressionLevel Optimal
Assert-File $zip
Write-Host "Portable package: $zip"

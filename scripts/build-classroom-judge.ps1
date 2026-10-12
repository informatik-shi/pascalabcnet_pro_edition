[CmdletBinding()]
param(
    [string]$PackageRoot = 'Release\PascalABCNET-Portable-win-x64',
    [string]$Image = 'abc-pro-judge:latest'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$package = [IO.Path]::GetFullPath((Join-Path $root $PackageRoot))
$compiler = Join-Path $package 'bin-net10\pabcnetcclear.dll'
if (-not (Test-Path -LiteralPath $compiler -PathType Leaf)) {
    throw "Missing .NET 10 compiler: $compiler"
}
$context = [IO.Path]::GetFullPath((Join-Path $root ('.codex-build\classroom-judge-' + [guid]::NewGuid().ToString('N'))))
if (-not $context.StartsWith($root + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw "Unsafe Docker context: $context"
}
New-Item -ItemType Directory -Path $context -Force | Out-Null
try {
    Copy-Item -LiteralPath (Join-Path $root 'ClassroomServer\Dockerfile') -Destination (Join-Path $context 'Dockerfile')
    Copy-Item -LiteralPath (Join-Path $package 'bin-net10') -Destination (Join-Path $context 'bin-net10') -Recurse
    docker build --tag $Image $context
    if ($LASTEXITCODE -ne 0) { throw 'Docker judge image build failed.' }
    $smoke = Join-Path $context 'smoke.pas'
    [IO.File]::WriteAllText($smoke, 'begin Writeln(5) end.', [Text.UTF8Encoding]::new($false))
    $mount = "type=bind,src=$context,dst=/work"
    & docker run --rm --network none --read-only --tmpfs '/tmp:rw,nosuid,size=64m' `
        --mount $mount $Image dotnet /opt/abc/bin-net10/pabcnetcclear.dll `
        /Output:/work/smoke.exe /work/smoke.pas
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath (Join-Path $context 'smoke.exe'))) {
        throw 'The Linux judge image could not compile a Pascal smoke program.'
    }
    $answer = & docker run --rm --network none --read-only --tmpfs '/tmp:rw,nosuid,size=64m' `
        --mount $mount $Image dotnet exec /work/smoke.exe
    if ($LASTEXITCODE -ne 0 -or ($answer -join "`n").Trim() -ne '5') {
        throw "The Linux judge image did not run the Pascal smoke program: $answer"
    }
    Write-Host "Classroom judge image: $Image"
}
finally {
    if (Test-Path -LiteralPath $context) {
        $resolved = [IO.Path]::GetFullPath($context)
        if (-not $resolved.StartsWith($root + '\', [StringComparison]::OrdinalIgnoreCase)) {
            throw "Unsafe cleanup path: $resolved"
        }
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
}

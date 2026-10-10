[CmdletBinding()]
param([string]$PythonExe = '')

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$cache = Join-Path $root '.codex-build\python-matplotlib'
$archive = Join-Path $cache 'python-3.13.16-embed-amd64.zip'
$runtime = Join-Path $cache 'runtime'
$packages = Join-Path $cache 'site-packages'
$requirements = Join-Path $PSScriptRoot 'matplotlib-requirements.txt'
$marker = Join-Path $packages '.pabc-requirements.sha256'
$expectedArchiveHash = '97dae5274cc54867065e8d5a3226e48c35017ed332a0fdb0e27d5b5821961297'
$requirementsHash = (Get-FileHash -LiteralPath $requirements -Algorithm SHA256).Hash

New-Item -ItemType Directory -Path $cache -Force | Out-Null
if (-not (Test-Path -LiteralPath $archive)) {
    Invoke-WebRequest 'https://www.python.org/ftp/python/3.13.16/python-3.13.16-embed-amd64.zip' `
        -OutFile $archive
}
if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $expectedArchiveHash) {
    throw "Python archive SHA256 mismatch: $archive"
}
if (-not (Test-Path -LiteralPath (Join-Path $runtime 'python.exe'))) {
    New-Item -ItemType Directory -Path $runtime -Force | Out-Null
    Expand-Archive -LiteralPath $archive -DestinationPath $runtime -Force
}
if (-not (Test-Path -LiteralPath $marker) -or
    (Get-Content -LiteralPath $marker -Raw).Trim() -ne $requirementsHash) {
    if (-not $PythonExe) { $PythonExe = (Get-Command python.exe -ErrorAction Stop).Source }
    $version = & $PythonExe -c 'import sys; print(f"{sys.version_info.major}.{sys.version_info.minor}")'
    if ($LASTEXITCODE -ne 0 -or $version.Trim() -ne '3.13') {
        throw 'Python 3.13 is required to install Matplotlib wheels; pass -PythonExe.'
    }
    New-Item -ItemType Directory -Path $packages -Force | Out-Null
    & $PythonExe -m pip install --disable-pip-version-check `
        --only-binary=:all: --target $packages -r $requirements
    if ($LASTEXITCODE -ne 0) { throw 'Matplotlib wheel installation failed.' }
    Set-Content -LiteralPath $marker -Value $requirementsHash -Encoding ascii
}
foreach ($name in @('matplotlib\__init__.py', 'numpy\__init__.py')) {
    if (-not (Test-Path -LiteralPath (Join-Path $packages $name))) {
        throw "Missing Python package: $name"
    }
}
Write-Host "Python Matplotlib runtime ready: $cache"
return [pscustomobject]@{ Runtime = $runtime; Packages = $packages }

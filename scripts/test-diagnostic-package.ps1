[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$PackageRoot)
$ErrorActionPreference = 'Stop'
$package = [IO.Path]::GetFullPath($PackageRoot)
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = [Security.Principal.WindowsPrincipal]::new($identity)
if ($principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) { throw 'Run this package check without elevation.' }
Write-Host 'PASS: ordinary user token (no administrator role)'
$ide = Join-Path $package 'bin\PascalABCNET.exe'
$launcher = Join-Path $package 'abc.pro.exe'
$assembly = [Reflection.Assembly]::ReflectionOnlyLoadFrom($ide)
if ($assembly.EntryPoint.DeclaringType.FullName -ne 'VisualPascalABC.DiagnosticBootstrap') { throw 'Packaged IDE has no diagnostic bootstrap.' }
Write-Host "PASS: packaged diagnostic entry point, version $($assembly.GetName().Version)"
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $package 'Portable\launch-ide.ps1') -Root $package --self-test
if ($LASTEXITCODE -ne 0) { throw 'Packaged PowerShell/bootstrap/launcher self-test failed.' }
Write-Host 'PASS: actual packaged PowerShell -> launcher self-test'

function Read-Report([string]$component, [string]$session) {
    foreach ($base in @($env:LOCALAPPDATA, $env:TEMP, $env:APPDATA)) {
        $path = Join-Path $base "abc.pro\Logs\$component-$session.log"
        if (Test-Path -LiteralPath $path) { return [IO.File]::ReadAllText($path) }
    }
    return ''
}
foreach ($mode in @('writable', 'read-only')) {
    $savedAcl = $null; $child = $null; $monitor = $null
    $session = [guid]::NewGuid().ToString('N')
    $previousSession = $env:PABC_DIAGNOSTIC_SESSION
    try {
        if ($mode -eq 'read-only') {
            $savedAcl = Get-Acl -LiteralPath $package
            $acl = Get-Acl -LiteralPath $package
            $deny = [Security.AccessControl.FileSystemAccessRule]::new($identity.User, 'Write', 'ContainerInherit,ObjectInherit', 'None', 'Deny')
            $acl.AddAccessRule($deny)
            Set-Acl -LiteralPath $package -AclObject $acl
        }
        $env:PABC_DIAGNOSTIC_SESSION = $session
        $monitor = Start-Process -FilePath $launcher -ArgumentList '--portable' -WindowStyle Hidden -PassThru
        for ($i=0; $i -lt 80; $i++) {
            $launchLog = Read-Report 'launcher' $session
            if ($launchLog -match 'IDE PID=(\d+)') { $child = Get-Process -Id ([int]$Matches[1]) -ErrorAction SilentlyContinue }
            $ideLog = Read-Report 'ide' $session
            if ($ideLog.Contains('Compiler initialization completed') -or $ideLog.Contains('FAILURE')) { break }
            Start-Sleep -Milliseconds 500
        }
        if (-not $child -or -not $ideLog.Contains('Compiler initialization completed') -or $ideLog.Contains('FAILURE')) { throw "IDE failed startup ($mode); session=$session" }
        if ($mode -eq 'read-only' -and -not $launchLog.Contains('Fallback work directory: writable')) { throw 'Read-only package did not use checked user work directory.' }
        $child.Refresh()
        if (-not $child.CloseMainWindow()) { throw 'Could not request normal IDE closure.' }
        if (-not $child.WaitForExit(20000)) { throw 'IDE did not close normally.' }
        if (-not $monitor.WaitForExit(10000)) { throw 'Launcher did not finish monitoring.' }
        $launchLog = Read-Report 'launcher' $session
        # Get-Process can lose the cached exit status after a non-child process exits.
        # Check the status recorded by its actual parent/monitor instead.
        if (-not $launchLog.Contains('IDE exit code=0 (0x00000000)') -or $launchLog.Contains('FAILURE') -or
            -not $launchLog.Contains('Launcher completed')) { throw "False crash reported ($mode)." }
        Write-Host "PASS: actual IDE startup and clean close ($mode); session=$session"
    } finally {
        if ($savedAcl) { Set-Acl -LiteralPath $package -AclObject $savedAcl }
        $env:PABC_DIAGNOSTIC_SESSION = $previousSession
        if ($child -and -not $child.HasExited) { $child.Kill() }
        if ($monitor -and -not $monitor.HasExited) { $monitor.Kill() }
    }
}

param(
    [Parameter(Mandatory = $true)][string]$Root,
    [switch]$Installed,
    [Parameter(ValueFromRemainingArguments = $true)][string[]]$IdeArguments
)
$ErrorActionPreference = 'Stop'
$session = [guid]::NewGuid().ToString('N')
$env:PABC_DIAGNOSTIC_SESSION = $session
$log = $null
$pending = ''
function Write-LaunchLog([string]$message) {
    $script:pending += [DateTime]::UtcNow.ToString('o') + ' ' + $message + [Environment]::NewLine
    foreach ($base in @($env:LOCALAPPDATA, $env:TEMP, $env:APPDATA)) {
        try {
            if (-not $base -or -not [IO.Path]::IsPathRooted($base)) { continue }
            $directory = Join-Path $base 'abc.pro\Logs'
            [IO.Directory]::CreateDirectory($directory) | Out-Null
            $candidate = Join-Path $directory ("bootstrap-$session.log")
            [IO.File]::AppendAllText($candidate, $script:pending, [Text.UTF8Encoding]::new($false))
            $script:log = $candidate
            $script:pending = ''
            return
        } catch { $script:pending += "Log destination unavailable: $base; $($_.Exception.Message)" + [Environment]::NewLine }
    }
}
function Show-LaunchError([string]$message) {
    Write-Host $message
    try {
        $shell = New-Object -ComObject WScript.Shell
        $shell.Popup($message + [Environment]::NewLine + 'Ctrl+C: скопировать сообщение.', 0, 'abc.pro — отчёт о запуске', 16) | Out-Null
    } catch { }
}
# Standard Windows quoting, including trailing backslashes before the closing quote.
function Quote-Argument([string]$value) {
    '"' + [regex]::Replace([regex]::Replace($value, '(\\*)"', '$1$1\"'), '(\\+)$', '$1$1') + '"'
}
Write-LaunchLog "Bootstrap started; session=$session; OS=$([Environment]::OSVersion); OS64=$([Environment]::Is64BitOperatingSystem); Process64=$([Environment]::Is64BitProcess); CLR=$([Environment]::Version); PowerShell=$($PSVersionTable.PSVersion)"
try {
    $rootPath = [IO.Path]::GetFullPath($Root)
    Write-LaunchLog "Stage: package root; $rootPath"
    $launcher = Join-Path $rootPath 'abc.pro.exe'
    if (-not (Test-Path -LiteralPath $launcher -PathType Leaf)) { throw "Не найден $launcher. Распакуйте архив целиком." }
    $framework = Get-ItemProperty -LiteralPath 'HKLM:\SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\Full' -Name Release -ErrorAction SilentlyContinue
    $release = if ($framework) { $framework.Release } else { $null }
    Write-LaunchLog "Required .NET Framework >=4.7.2; installed Release=$release"
    if (-not $release -or $release -lt 461808) { throw 'Не подтверждено наличие .NET Framework 4.7.2 или новее, необходимой для IDE. Передайте отчёт администратору компьютера.' }
    Write-LaunchLog 'Stage: start managed launcher'
    $env:PABC_DIAGNOSTIC_LAUNCH_LOG = $log
    $arguments = @($IdeArguments)
    if (-not $Installed) { $arguments = @('--portable') + $arguments }
    $start = [Diagnostics.ProcessStartInfo]::new()
    $start.FileName = $launcher
    $start.WorkingDirectory = $rootPath
    $start.UseShellExecute = $false
    $start.Arguments = ($arguments | Where-Object { $null -ne $_ } | ForEach-Object { Quote-Argument $_ }) -join ' '
    $process = [Diagnostics.Process]::Start($start)
    $process.WaitForExit()
    Write-LaunchLog "Managed launcher exit code=$($process.ExitCode)"
    if ($process.ExitCode -ne 0) {
        $managedReport = $false
        foreach ($base in @($env:LOCALAPPDATA, $env:TEMP, $env:APPDATA)) {
            try {
                $path = Join-Path $base "abc.pro\Logs\launcher-$session.log"
                if ([IO.File]::Exists($path) -and [IO.File]::ReadAllText($path).Contains('Failure dialog displayed')) { $managedReport = $true }
            } catch { }
        }
        if (-not $managedReport) { throw "Запускатель завершился с кодом $($process.ExitCode), не показав отчёт. Причина не установлена." }
    }
    exit $process.ExitCode
} catch {
    Write-LaunchLog ("FAILURE (observed): " + $_.Exception.ToString() + [Environment]::NewLine + $_.ScriptStackTrace)
    $location = if ($log) { "Передайте файл: $log" } else { 'Не удалось записать отчёт в пользовательские папки.' }
    Show-LaunchError ($_.Exception.Message + [Environment]::NewLine + $location)
    exit 1
}

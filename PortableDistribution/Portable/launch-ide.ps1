param(
    [Parameter(Mandatory = $true)][string]$Root,
    [Parameter(ValueFromRemainingArguments = $true)][string[]]$IdeArguments
)

$ErrorActionPreference = 'Stop'

function Show-LaunchError([string]$message) {
    Write-Host $message
    try {
        Add-Type -AssemblyName System.Windows.Forms
        [Windows.Forms.MessageBox]::Show($message, 'abc.pro portable', 'OK', 'Error') | Out-Null
    }
    catch { }
}

function Test-WritableDirectory([string]$path) {
    try {
        New-Item -ItemType Directory -Path $path -Force | Out-Null
        $probe = Join-Path $path ('.abc-pro-write-test-' + [guid]::NewGuid().ToString('N'))
        [IO.File]::WriteAllText($probe, '')
        Remove-Item -LiteralPath $probe -Force
        return $true
    }
    catch { return $false }
}

function Quote-Argument([string]$value) {
    return '"' + ($value -replace '"', '\"') + '"'
}

$rootPath = [IO.Path]::GetFullPath($Root)
$logDirectory = Join-Path $env:LOCALAPPDATA 'abc.pro\Logs'
try { New-Item -ItemType Directory -Path $logDirectory -Force | Out-Null }
catch { $logDirectory = $env:TEMP }
$log = Join-Path $logDirectory 'portable-launch.log'

function Write-LaunchLog([string]$message) {
    try {
        [IO.File]::AppendAllText($log, (Get-Date -Format 'yyyy-MM-dd HH:mm:ss') + ' ' +
            $message + [Environment]::NewLine)
    }
    catch { }
}

try {
    $ide = Join-Path $rootPath 'bin\PascalABCNET.exe'
    if (-not (Test-Path -LiteralPath $ide -PathType Leaf)) {
        throw "Не найден $ide. Распакуйте архив целиком."
    }

    $release = (Get-ItemProperty -LiteralPath 'HKLM:\SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\Full' `
        -Name Release -ErrorAction SilentlyContinue).Release
    if ($release -and $release -lt 461808) {
        throw 'Для IDE требуется .NET Framework 4.7.2 или новее. Обратитесь к администратору компьютера.'
    }
    if (-not $release) { Write-LaunchLog 'Unable to determine installed .NET Framework version.' }

    $work = Join-Path $rootPath 'Work'
    if (-not (Test-WritableDirectory $work)) {
        $work = Join-Path $env:LOCALAPPDATA 'abc.pro\Work'
        if (-not (Test-WritableDirectory $work)) {
            throw "Нет доступа на запись ни в папку архива, ни в $work."
        }
        Write-LaunchLog "Package is read-only; using user work directory: $work"
    }
    foreach ($subdirectory in @('Output', 'Matplotlib')) {
        New-Item -ItemType Directory -Path (Join-Path $work $subdirectory) -Force | Out-Null
    }

    $env:PABC_WORK_DIRECTORY = $work
    $env:PABCNET_ROOT = $rootPath
    $env:DOTNET_ROOT = Join-Path $rootPath 'dotnet'
    $env:DOTNET_ROOT_X64 = $env:DOTNET_ROOT
    $env:MPLCONFIGDIR = Join-Path $work 'Matplotlib'
    $python = Join-Path $rootPath 'python\python.exe'
    if (Test-Path -LiteralPath $python) { $env:PABC_PYTHON_EXE = $python }
    $env:PATH = $env:DOTNET_ROOT + [IO.Path]::PathSeparator + $env:PATH

    $bin = Join-Path $rootPath 'bin'
    $start = @{ FilePath = $ide; WorkingDirectory = $bin; PassThru = $true }
    if ($IdeArguments.Count -gt 0) {
        $start.ArgumentList = ($IdeArguments | ForEach-Object { Quote-Argument $_ }) -join ' '
    }
    Write-LaunchLog "Starting IDE: $ide; work: $work"
    $process = Start-Process @start
    if ($process.WaitForExit(10000)) {
        if ($process.ExitCode -eq 0) {
            $existing = @(Get-Process PascalABCNET -ErrorAction SilentlyContinue | Where-Object {
                try { $_.MainWindowHandle -ne [IntPtr]::Zero -and $_.Path -eq $ide }
                catch { $false }
            })
            if ($existing.Count -gt 0) {
                Write-LaunchLog 'An already running IDE received the request.'
                exit 0
            }
        }
        Write-LaunchLog "IDE exited during startup; exit code: $($process.ExitCode)"
        throw "IDE закрылась сразу после запуска (код $($process.ExitCode)). Журналы: $log и $logDirectory\ide-startup.log"
    }
    Write-LaunchLog 'IDE remains running after startup check.'
    exit 0
}
catch {
    Write-LaunchLog $_.Exception.ToString()
    Show-LaunchError ($_.Exception.Message + [Environment]::NewLine + "Журнал запуска: $log")
    exit 1
}

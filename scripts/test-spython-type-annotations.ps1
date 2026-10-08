[CmdletBinding()]
param(
    [ValidateSet('classic', 'net10')][string]$Runtime = 'classic',
    [string]$PackageRoot = '',
    [string]$DotnetRoot = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$runtimeRoot = if ($PackageRoot) { [IO.Path]::GetFullPath($PackageRoot) } else { $root }
$compiler = if ($Runtime -eq 'net10') {
    Join-Path $runtimeRoot 'bin-net10\pabcnetcclear.exe'
} else {
    Join-Path $runtimeRoot 'bin\pabcnetcclear.exe'
}
if (-not (Test-Path -LiteralPath $compiler)) { throw "Compiler not found: $compiler" }
if ($Runtime -eq 'net10') {
    if (-not $DotnetRoot) {
        $DotnetRoot = if ($PackageRoot) { Join-Path $runtimeRoot 'dotnet' } else {
            Join-Path $env:USERPROFILE '.dotnet'
        }
    }
    if (-not (Test-Path -LiteralPath (Join-Path $DotnetRoot 'dotnet.exe'))) {
        throw "Missing .NET runtime: $DotnetRoot"
    }
    $env:DOTNET_ROOT = [IO.Path]::GetFullPath($DotnetRoot)
    $env:DOTNET_ROOT_X64 = $env:DOTNET_ROOT
}

$testRoot = Join-Path $root ('.codex-build\type-annotations-test-' + $Runtime)
New-Item -ItemType Directory -Path $testRoot -Force | Out-Null
$cases = @(
    @{ Name = 'unchanged_type'; Source = "a: int = 1`na = 2`na + 4`nprint(a)`n"; Output = '2' },
    @{ Name = 'expression_side_effect'; Source = "counter: int = 0`ndef bump() -> int:`n    global counter`n    counter = counter + 1`n    return counter`nbump() + 1`nprint(counter)`n"; Output = '1' },
    @{ Name = 'same_type_reannotation'; Source = "a: int = 1`na: int = 2`na: int`nprint(a)`n"; Output = '2' },
    @{ Name = 'uninitialized_reannotation'; Source = "a: int`na: int`na = 3`nprint(a)`n"; Output = '3' },
    @{ Name = 'inferred_type_reannotation'; Source = "a = 1`na: int = 2`nprint(a)`n"; Output = '2' },
    @{ Name = 'generic_reannotation'; Source = "a: list[int] = [1]`na: list[int] = [2]`nprint(a[0])`n"; Output = '2' },
    @{ Name = 'local_reannotation'; Source = "def f() -> int:`n    a: int = 1`n    a: int = 2`n    return a`nprint(f())`n"; Output = '2' },
    @{ Name = 'collections'; Source = "d: dict[str, int] = {'a': 3}`nt: tuple[int, str] = (2, 'b')`nxs: list[str] = ['c', 'd']`nprint(d['a'])`nprint(t[1])`nprint(xs[0])`n"; Output = "3`nb`nc" },
    @{ Name = 'function_annotations'; Source = "def echo(x: str) -> str:`n    return x`nprint(echo('z'))`n"; Output = 'z' },
    @{ Name = 'default_string_annotation'; Source = "def label(x: str = 'a') -> str:`n    return x`nprint(label())`n"; Output = 'a' },
    @{ Name = 'inner_scope'; Source = "a: int = 1`ndef f() -> str:`n    a: str = 'x'`n    return a`nprint(f())`nprint(a)`n"; Output = "x`n1" },
    @{ Name = 'lambda_reannotation'; Source = "a = lambda x: x + 1`nprint(a(2))`na: int = 10`na + 4`n"; Error = '\[3,1\]' },
    @{ Name = 'different_type_reannotation'; Source = "a: int = 1`na: str = 'x'`n"; Error = '\[2,1\]' },
    @{ Name = 'implicit_conversion_is_not_reannotation'; Source = "a = 1`na: bool = True`n"; Error = '\[2,1\]' },
    @{ Name = 'lambda_reassignment'; Source = "a = lambda x: x + 1`na = 10`n"; Error = '\[2,' },
    @{ Name = 'annotated_type_change'; Source = "a: int = 1`na = 'x'`n"; Error = '\[2,' }
)

foreach ($case in $cases) {
    $source = Join-Path $testRoot ($case.Name + '.pys')
    [IO.File]::WriteAllText($source, $case.Source, [Text.UTF8Encoding]::new($false))
    $compileOutput = @(& $compiler $source 2>&1) -join "`n"
    $compileExit = $LASTEXITCODE
    if ($case.ContainsKey('Error')) {
        if ($compileExit -eq 0 -or $compileOutput -notmatch $case.Error) {
            throw "Expected '$($case.Error)' for $($case.Name), got exit=$compileExit : $compileOutput"
        }
        continue
    }
    if ($compileExit -ne 0) { throw "Compilation failed for $($case.Name): $compileOutput" }
    $program = [IO.Path]::ChangeExtension($source, '.exe')
    $actual = if ($Runtime -eq 'net10') {
        @(& (Join-Path $env:DOTNET_ROOT 'dotnet.exe') exec $program)
    } else {
        @(& $program)
    }
    if ($LASTEXITCODE -ne 0) { throw "Execution failed for $($case.Name)" }
    $actualText = ($actual -join "`n").Trim()
    if ($actualText -ne $case.Output) {
        throw "Unexpected output for $($case.Name): '$actualText' (expected '$($case.Output)')"
    }
}
Write-Host "SPython type annotation tests passed: $Runtime ($($cases.Count) cases)."

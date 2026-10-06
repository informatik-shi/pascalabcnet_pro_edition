[CmdletBinding()]
param([string]$BinDirectory = '')

$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$bin = if ($BinDirectory) { [IO.Path]::GetFullPath($BinDirectory) } else { Join-Path $root 'bin' }

if ([Threading.Thread]::CurrentThread.ApartmentState -ne 'STA') {
    throw 'Run this script with powershell.exe -STA.'
}

Add-Type -Path (Join-Path $bin 'ICSharpCode.TextEditor.dll')
[Reflection.Assembly]::LoadFrom((Join-Path $bin 'PascalABCNET.exe')) | Out-Null

$provider = New-Object ICSharpCode.TextEditor.Document.FileSyntaxModeProvider((Join-Path $bin 'Highlighting') + '\')
[ICSharpCode.TextEditor.Document.HighlightingManager]::Manager.AddSyntaxModeFileProvider($provider)
try {
    $highlighter = [ICSharpCode.TextEditor.Document.HighlightingManager]::Manager.FindHighlighterForFile('sample.pys')
}
catch { throw $_.Exception.ToString() }
if ($highlighter.Name -ne 'Spython') { throw "Wrong highlighter: $($highlighter.Name)" }
$default = $highlighter.GetColorFor('Default')
if ($default.Color.Name -ne 'ffd4d4d4' -or $default.BackgroundColor.Name -ne 'ff1e1e1e') {
    throw "Wrong editor colors: $($default.Color), $($default.BackgroundColor)"
}

$cases = @(
    @('if ready:', 4),
    @('    for i in range(3):', 8),
    @('while active:', 4),
    @('elif other:', 4),
    @('else:', 4),
    @('def f(x):', 4),
    @('class Box:', 4),
    @('try:', 4),
    @('except ValueError:', 4),
    @('finally:', 4),
    @('with open("x") as f:', 4),
    @('async def f():', 4),
    @('match value:', 4),
    @('case 1:', 4),
    @('if data[1:2]: # slice', 4),
    @('if ready: # comment', 4),
    @('    print(i)', 4),
    @('if ready: pass', 0),
    @('values = {"a": 1}', 0),
    @('if value == "x:": pass', 0),
    @('print("if ready:")', 0),
    @('# if ready:', 0),
    @('elsewhere:', 0)
)

foreach ($case in $cases) {
    $editor = New-Object ICSharpCode.TextEditor.TextEditorControl
    try {
        $editor.Document.FormattingStrategy = New-Object VisualPascalABC.SPythonFormattingStrategy
        $editor.TextEditorProperties.IndentStyle = [ICSharpCode.TextEditor.Document.IndentStyle]::Smart
        $editor.TextEditorProperties.IndentationSize = 4
        $editor.Document.TextContent = [string]$case[0]
        $area = $editor.ActiveTextAreaControl.TextArea
        $area.Caret.Position = New-Object ICSharpCode.TextEditor.TextLocation(([string]$case[0]).Length, 0)
        (New-Object ICSharpCode.TextEditor.Actions.Return).Execute($area)
        $actual = $area.Caret.Column
        if ($actual -ne [int]$case[1]) {
            throw "Enter after '$($case[0])': expected indent $($case[1]), got $actual"
        }
        $newLine = [ICSharpCode.TextEditor.Document.TextUtilities]::GetLineAsString($editor.Document, 1)
        if ($newLine -ne (' ' * [int]$case[1])) {
            throw "Enter after '$($case[0])': wrong new line content '$newLine'"
        }
    }
    finally { $editor.Dispose() }
}

$editor = New-Object ICSharpCode.TextEditor.TextEditorControl
try {
    $editor.Document.FormattingStrategy = New-Object VisualPascalABC.SPythonFormattingStrategy
    $editor.TextEditorProperties.IndentStyle = [ICSharpCode.TextEditor.Document.IndentStyle]::Smart
    $editor.TextEditorProperties.IndentationSize = 2
    $editor.Document.TextContent = 'for item in items:'
    $area = $editor.ActiveTextAreaControl.TextArea
    $area.Caret.Position = New-Object ICSharpCode.TextEditor.TextLocation(18, 0)
    (New-Object ICSharpCode.TextEditor.Actions.Return).Execute($area)
    if ($area.Caret.Column -ne 2 -or
        [ICSharpCode.TextEditor.Document.TextUtilities]::GetLineAsString($editor.Document, 1) -ne '  ') {
        throw 'SPython indentation did not follow the configured two-space width.'
    }
}
finally { $editor.Dispose() }

Write-Host "PASS: SPython Enter indentation ($($cases.Count + 1) cases) and dark highlighting."

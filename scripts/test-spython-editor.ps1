[CmdletBinding()]
param([string]$BinDirectory = '')

$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$bin = if ($BinDirectory) { [IO.Path]::GetFullPath($BinDirectory) } else { Join-Path $root 'bin' }

if ([Threading.Thread]::CurrentThread.ApartmentState -ne 'STA') {
    throw 'Run this script with powershell.exe -STA.'
}

Add-Type -Path (Join-Path $bin 'ICSharpCode.TextEditor.dll')
Add-Type -Path (Join-Path $bin 'WeifenLuo.WinFormsUI.Docking.dll')
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
$lightHighlighter = [ICSharpCode.TextEditor.Document.HighlightingManager]::Manager.FindHighlighter('SpythonLight')
if ($lightHighlighter.Name -ne 'SpythonLight') { throw "Wrong light highlighter: $($lightHighlighter.Name)" }
$lightDefault = $lightHighlighter.GetColorFor('Default')
if ($lightDefault.Color.ToArgb() -ne [Drawing.SystemColors]::WindowText.ToArgb() -or
    $lightDefault.BackgroundColor.ToArgb() -ne [Drawing.SystemColors]::Window.ToArgb()) {
    throw "Wrong light editor colors: $($lightDefault.Color), $($lightDefault.BackgroundColor)"
}

$resourceProvider = New-Object ICSharpCode.TextEditor.Document.ResourceSyntaxModeProvider
$syntaxProviders = [ICSharpCode.TextEditor.Document.ISyntaxModeFileProvider[]]@($resourceProvider, $provider)
$darkProvider = New-Object VisualPascalABC.DarkSyntaxModeProvider -ArgumentList (,$syntaxProviders)
[ICSharpCode.TextEditor.Document.HighlightingManager]::Manager.AddSyntaxModeFileProvider($darkProvider)
foreach ($mode in $darkProvider.SyntaxModes) {
    $darkHighlighter = [ICSharpCode.TextEditor.Document.HighlightingManager]::Manager.FindHighlighter($mode.Name)
    $darkDefault = $darkHighlighter.GetColorFor('Default')
    if ($darkDefault.BackgroundColor.ToArgb() -ne [Drawing.Color]::FromArgb(30, 30, 30).ToArgb() -or
        $darkDefault.Color.ToArgb() -ne [Drawing.Color]::FromArgb(212, 212, 212).ToArgb()) {
        throw "Wrong dark editor colors for $($mode.Name)"
    }
}

# Check the IDE's selection of a highlighter, including Pascal and unknown files.
$form = [Runtime.Serialization.FormatterServices]::GetUninitializedObject([VisualPascalABC.Form1])
$options = New-Object VisualPascalABC.UserOptions
$optionsField = [VisualPascalABC.Form1].GetField('UserOptions',
    [Reflection.BindingFlags]'Instance,NonPublic,Public')
$optionsField.SetValue($form, $options)
$darkOptionField = [VisualPascalABC.UserOptions].GetField('DarkTheme',
    [Reflection.BindingFlags]'Instance,NonPublic,Public')
$themeMethod = [VisualPascalABC.Form1].GetMethod('GetEditorHighlighter',
    [Reflection.BindingFlags]'Instance,NonPublic,Public')
foreach ($sample in @(@('sample.pas', 'PascalABC.NET Dark', 'PascalABC.NET'),
                     @('sample.pys', 'Spython', 'SpythonLight'),
                     @('sample.cs', 'C# Dark', 'C#'),
                     @('sample.txt', 'Default Dark', 'Default'))) {
    foreach ($dark in @($true, $false)) {
        $darkOptionField.SetValue($options, $dark)
        $expected = if ($dark) { $sample[1] } else { $sample[2] }
        $actual = $themeMethod.Invoke($form, @($sample[0])).Name
        if ($actual -ne $expected) {
            throw "Theme choice for $($sample[0]) (dark=$dark): expected $expected, got $actual"
        }
    }
}

$pascalEditor = New-Object ICSharpCode.TextEditor.TextEditorControl
try {
    $pascalEditor.Document.TextContent = "begin writeln('test'); end."
    $pascalEditor.Document.HighlightingStrategy =
        [ICSharpCode.TextEditor.Document.HighlightingManager]::Manager.FindHighlighter('PascalABC.NET Dark')
    $begin = $pascalEditor.Document.GetLineSegment(0).Words |
        Where-Object { $_.Word -eq 'begin' } | Select-Object -First 1
    if ($null -eq $begin -or $begin.Color.ToArgb() -ne [Drawing.Color]::FromArgb(212, 212, 212).ToArgb()) {
        throw 'Pascal keywords are unreadable on the dark editor background.'
    }
    $pascalEditor.Document.HighlightingStrategy =
        [ICSharpCode.TextEditor.Document.HighlightingManager]::Manager.FindHighlighterForFile('sample.pas')
    if ($pascalEditor.Document.HighlightingStrategy.GetColorFor('Default').BackgroundColor.ToArgb() -ne
        [Drawing.SystemColors]::Window.ToArgb()) {
        throw 'Pascal editor did not return to the light palette.'
    }
}
finally { $pascalEditor.Dispose() }

# Exercise the real output controls in both directions, including existing text.
$outputForm = [Runtime.Serialization.FormatterServices]::GetUninitializedObject([VisualPascalABC.OutputWindowForm])
$outputBox = New-Object Windows.Forms.RichTextBox
$inputBox = New-Object Windows.Forms.TextBox
$flags = [Reflection.BindingFlags]'Instance,NonPublic,Public'
[VisualPascalABC.OutputWindowForm].GetField('outputTextBox', $flags).SetValue($outputForm, $outputBox)
[VisualPascalABC.OutputWindowForm].GetField('InputTextBox', $flags).SetValue($outputForm, $inputBox)
$outputBox.Text = 'existing output'
try {
    foreach ($dark in @($true, $false)) {
        $outputForm.ApplyTheme($dark)
        $expectedBackground = if ($dark) { [Drawing.Color]::FromArgb(30, 30, 30) } else { [Drawing.Color]::White }
        $expectedForeground = if ($dark) { [Drawing.Color]::FromArgb(212, 212, 212) } else { [Drawing.Color]::Black }
        $outputBox.Select(0, 1)
        if ($outputBox.BackColor.ToArgb() -ne $expectedBackground.ToArgb() -or
            $outputBox.SelectionColor.ToArgb() -ne $expectedForeground.ToArgb() -or
            $inputBox.BackColor.ToArgb() -ne $expectedBackground.ToArgb() -or
            $inputBox.ForeColor.ToArgb() -ne $expectedForeground.ToArgb()) {
            throw "Output colors are wrong for dark=$dark"
        }
    }
}
finally { $outputBox.Dispose(); $inputBox.Dispose() }

$compilerForm = [Runtime.Serialization.FormatterServices]::GetUninitializedObject([VisualPascalABC.CompilerConsoleWindowForm])
$compilerBox = New-Object Windows.Forms.TextBox
[VisualPascalABC.CompilerConsoleWindowForm].GetField('CompilerConsole', $flags).SetValue($compilerForm, $compilerBox)
try {
    $compilerForm.ApplyTheme($true)
    if ($compilerBox.BackColor.ToArgb() -ne [Drawing.Color]::FromArgb(30, 30, 30).ToArgb()) {
        throw 'Compiler console stayed light in dark mode.'
    }
    $compilerForm.ApplyTheme($false)
    if ($compilerBox.BackColor.ToArgb() -ne [Drawing.Color]::White.ToArgb()) {
        throw 'Compiler console stayed dark in light mode.'
    }
}
finally { $compilerBox.Dispose() }

# Check the actual WinForms menu renderer, nested menus and the light-theme round trip.
$menu = New-Object Windows.Forms.MenuStrip
$fileMenu = New-Object Windows.Forms.ToolStripMenuItem('File')
$openItem = New-Object Windows.Forms.ToolStripMenuItem('Open')
$fileMenu.DropDownItems.Add($openItem) | Out-Null
$menu.Items.Add($fileMenu) | Out-Null
$contextMenu = New-Object Windows.Forms.ContextMenuStrip
$contextMenu.Items.Add((New-Object Windows.Forms.ToolStripMenuItem('Copy'))) | Out-Null
$menuTheme = New-Object VisualPascalABC.ToolStripThemeManager
$originalMode = $menu.RenderMode
try {
    $menuTheme.Apply($true, [Windows.Forms.ToolStrip[]]@($menu, $contextMenu))
    $darkMenuColor = [Drawing.Color]::FromArgb(37, 37, 38).ToArgb()
    $darkTextColor = [Drawing.Color]::FromArgb(230, 230, 230).ToArgb()
    foreach ($strip in @($menu, $fileMenu.DropDown, $contextMenu)) {
        if ($strip.BackColor.ToArgb() -ne $darkMenuColor -or
            $strip.Renderer.ColorTable.MenuStripGradientBegin.ToArgb() -ne $darkMenuColor) {
            throw "Menu background stayed light: $($strip.GetType().Name)"
        }
    }
    if ($fileMenu.ForeColor.ToArgb() -ne $darkTextColor -or
        $openItem.ForeColor.ToArgb() -ne $darkTextColor) {
        throw 'Menu text stayed dark in dark mode.'
    }
    $menuBitmap = New-Object Drawing.Bitmap(80, 40)
    $menuGraphics = [Drawing.Graphics]::FromImage($menuBitmap)
    try {
        $menuGraphics.Clear([Drawing.Color]::White)
        $marginArgs = [Windows.Forms.ToolStripRenderEventArgs]::new($menuGraphics,
            $contextMenu, (New-Object Drawing.Rectangle(0, 0, 80, 40)), [Drawing.Color]::Empty)
        $contextMenu.Renderer.DrawImageMargin($marginArgs)
        if ($menuBitmap.GetPixel(5, 20).ToArgb() -ne $darkMenuColor) {
            throw 'The icon strip inside a dark drop-down menu stayed light.'
        }
        $menuGraphics.Clear([Drawing.Color]::White)
        $fileMenu.Select()
        $menu.Renderer.DrawMenuItemBackground(
            [Windows.Forms.ToolStripItemRenderEventArgs]::new($menuGraphics, $fileMenu))
        if ($menuBitmap.GetPixel(5, 5).ToArgb() -ne [Drawing.Color]::FromArgb(62, 62, 64).ToArgb()) {
            throw 'The selected top-level menu item stayed light.'
        }
    }
    finally { $menuGraphics.Dispose(); $menuBitmap.Dispose() }
    $menuTheme.Apply($false, [Windows.Forms.ToolStrip[]]@($menu, $contextMenu))
    if ($menu.RenderMode -ne $originalMode -or
        $menu.BackColor.ToArgb() -eq $darkMenuColor -or
        $fileMenu.DropDown.BackColor.ToArgb() -eq $darkMenuColor -or
        $fileMenu.ForeColor.ToArgb() -eq $darkTextColor) {
        throw 'Menu did not return to the light palette.'
    }
}
finally { $contextMenu.Dispose(); $menu.Dispose() }

# The docking library paints document tabs and tool window captions independently
# from the main menu. Check its palette and the editor's embedded controls.
$dock = New-Object WeifenLuo.WinFormsUI.Docking.DockPanel
$stripType = $dock.GetType().Assembly.GetType('WeifenLuo.WinFormsUI.Docking.VS2005DockPaneStrip')
$captionType = $dock.GetType().Assembly.GetType('WeifenLuo.WinFormsUI.Docking.VS2005DockPaneCaption')
$staticFlags = [Reflection.BindingFlags]'NonPublic,Static'
try {
    $dock.ApplyTheme($true)
    $tabBrush = $stripType.GetProperty('BrushDocumentActiveBackground', $staticFlags).GetValue($null, $null)
    $captionColor = $captionType.GetProperty('InactiveBackColor', $staticFlags).GetValue($null, $null)
    if ($dock.BackColor.ToArgb() -ne [Drawing.Color]::FromArgb(30, 30, 30).ToArgb() -or
        $tabBrush.Color.ToArgb() -ne [Drawing.Color]::FromArgb(45, 45, 48).ToArgb() -or
        $captionColor.ToArgb() -ne [Drawing.Color]::FromArgb(37, 37, 38).ToArgb()) {
        throw 'Document tabs or tool window captions stayed light in dark mode.'
    }
    $dock.ApplyTheme($false)
    $tabBrush = $stripType.GetProperty('BrushDocumentActiveBackground', $staticFlags).GetValue($null, $null)
    if ($dock.BackColor.ToArgb() -ne [Drawing.SystemColors]::Control.ToArgb() -or
        $tabBrush.Color.ToArgb() -ne [Drawing.SystemColors]::ControlLightLight.ToArgb()) {
        throw 'Document tabs did not return to the light palette.'
    }
}
finally { $dock.Dispose() }

$chromeEditor = New-Object VisualPascalABC.CodeFileDocumentTextEditorControl
$browserField = $chromeEditor.GetType().GetField('quickClassBrowserPanel', $flags)
$browser = $browserField.GetValue($chromeEditor)
$combo = $browser.GetType().GetField('classComboBox', $flags).GetValue($browser)
$bar = $chromeEditor.ActiveTextAreaControl.HScrollBar
$bar.Bounds = New-Object Drawing.Rectangle(0, 0, 400, 20)
$bitmap = New-Object Drawing.Bitmap(400, 20)
$iconBitmap = New-Object Drawing.Bitmap(18, 100)
$iconGraphics = [Drawing.Graphics]::FromImage($iconBitmap)
$iconMargin = $chromeEditor.ActiveTextAreaControl.TextArea.IconBarMargin
$iconMargin.DrawingPosition = New-Object Drawing.Rectangle(0, 0, 18, 100)
try {
    $chromeEditor.ApplyTheme($true)
    $bar.DrawToBitmap($bitmap, (New-Object Drawing.Rectangle(0, 0, 400, 20)))
    $iconMargin.Paint($iconGraphics, (New-Object Drawing.Rectangle(0, 0, 18, 100)))
    if ($browser.BackColor.ToArgb() -ne [Drawing.Color]::FromArgb(37, 37, 38).ToArgb() -or
        $combo.BackColor.ToArgb() -ne [Drawing.Color]::FromArgb(45, 45, 48).ToArgb() -or
        $bitmap.GetPixel(100, 10).ToArgb() -ne [Drawing.Color]::FromArgb(37, 37, 38).ToArgb() -or
        $iconBitmap.GetPixel(5, 50).ToArgb() -ne [Drawing.Color]::FromArgb(37, 37, 38).ToArgb()) {
        throw 'Class browser, scrollbar, or left editor margin stayed light in dark mode.'
    }
    $chromeEditor.ApplyTheme($false)
    $bar.DrawToBitmap($bitmap, (New-Object Drawing.Rectangle(0, 0, 400, 20)))
    $iconMargin.Paint($iconGraphics, (New-Object Drawing.Rectangle(0, 0, 18, 100)))
    if ($browser.BackColor.ToArgb() -ne [Drawing.SystemColors]::Control.ToArgb() -or
        $combo.BackColor.ToArgb() -ne [Drawing.SystemColors]::Window.ToArgb() -or
        $bitmap.GetPixel(100, 10).ToArgb() -ne [Drawing.SystemColors]::Control.ToArgb() -or
        $iconBitmap.GetPixel(5, 50).ToArgb() -ne [Drawing.SystemColors]::Control.ToArgb()) {
        throw 'Class browser, scrollbar, or left editor margin did not return to the light palette.'
    }
}
finally { $iconGraphics.Dispose(); $iconBitmap.Dispose(); $bitmap.Dispose(); $chromeEditor.Dispose() }

if ([Environment]::OSVersion.Platform -eq [PlatformID]::Win32NT -and
    [Environment]::OSVersion.Version.Build -ge 22000) {
    $captionForm = New-Object Windows.Forms.Form
    try {
        if (-not [VisualPascalABC.WindowsCaptionTheme]::Apply($captionForm, $true) -or
            -not [VisualPascalABC.WindowsCaptionTheme]::Apply($captionForm, $false)) {
            throw 'Windows 11 title bar theme could not be applied.'
        }
    }
    finally { $captionForm.Dispose() }
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

Write-Host "PASS: SPython indentation ($($cases.Count + 1) cases), $($darkProvider.SyntaxModes.Count) dark syntax modes, editor, dock, menu, and output themes."

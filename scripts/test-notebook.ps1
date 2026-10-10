[CmdletBinding()]
param([string]$BaseUrl = 'http://127.0.0.1:50477')

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$BaseUrl = $BaseUrl.TrimEnd('/')
$token = (Invoke-RestMethod "$BaseUrl/api/session").token
$headers = @{ 'X-Notebook-Token' = $token }

function Invoke-NotebookCase {
    param([string]$Language, [string]$Code, [string]$ExpectedOutput,
          [bool]$ExpectImage = $false)
    $notebook = Invoke-RestMethod "$BaseUrl/api/notebooks" -Method Post -Headers $headers `
        -ContentType 'application/json' -Body (@{ language = $Language } | ConvertTo-Json)
    $notebook.cells[0].code = $Code
    Invoke-RestMethod "$BaseUrl/api/notebooks/$($notebook.id)" -Method Put -Headers $headers `
        -ContentType 'application/json; charset=utf-8' `
        -Body ([Text.Encoding]::UTF8.GetBytes(($notebook | ConvertTo-Json -Depth 10))) | Out-Null
    $result = Invoke-RestMethod "$BaseUrl/api/notebooks/$($notebook.id)/run" -Method Post `
        -Headers $headers -ContentType 'application/json' -Body '{"index":0}' -TimeoutSec 90
    if (-not $result.succeeded -or $result.output -notlike "*$ExpectedOutput*") {
        throw "$Language case failed ($($result.stage)): $($result.output)"
    }
    if ($ExpectImage) {
        if (-not $result.imageUrl) { throw "$Language case produced no inline image." }
        $client = New-Object Net.WebClient
        try { $bytes = $client.DownloadData($BaseUrl + $result.imageUrl) }
        finally { $client.Dispose() }
        $stream = New-Object IO.MemoryStream(, $bytes)
        try {
            $bitmap = [Drawing.Bitmap]::FromStream($stream)
            try {
                if ($bitmap.Width -lt 100 -or $bitmap.Height -lt 100) {
                    throw "Inline image is too small: $($bitmap.Width)x$($bitmap.Height)"
                }
            }
            finally { $bitmap.Dispose() }
        }
        finally { $stream.Dispose() }
    }
    Write-Host "PASS: $Language / $ExpectedOutput / inline image=$ExpectImage"
}

function Invoke-NotebookSequenceCase {
    param([string]$Language, [string]$FirstCode, [string]$SecondCode,
          [string]$ExpectedOutput, [string]$HiddenOutput)
    $notebook = Invoke-RestMethod "$BaseUrl/api/notebooks" -Method Post -Headers $headers `
        -ContentType 'application/json' -Body (@{ language = $Language } | ConvertTo-Json)
    $notebook.cells[0].code = $FirstCode
    $notebook.cells += [pscustomobject]@{
        id = [Guid]::NewGuid().ToString(); code = $SecondCode; output = '';
        imageUrl = $null; succeeded = $true
    }
    Invoke-RestMethod "$BaseUrl/api/notebooks/$($notebook.id)" -Method Put -Headers $headers `
        -ContentType 'application/json; charset=utf-8' `
        -Body ([Text.Encoding]::UTF8.GetBytes(($notebook | ConvertTo-Json -Depth 10))) | Out-Null
    $result = Invoke-RestMethod "$BaseUrl/api/notebooks/$($notebook.id)/run" -Method Post `
        -Headers $headers -ContentType 'application/json' -Body '{"index":1}' -TimeoutSec 90
    if (-not $result.succeeded -or $result.output -notlike "*$ExpectedOutput*" -or
        $result.output -like "*$HiddenOutput*") {
        throw "$Language sequence failed: $($result.output)"
    }
    Write-Host "PASS: $Language cumulative cells / current output only"
}

Invoke-NotebookCase pascal "writeln('pascal ok');" 'pascal ok'
Invoke-NotebookCase spython "print('spython ok')" 'spython ok'
Invoke-NotebookCase pascal "uses GraphABC;`nSetWindowSize(320, 220);`nBrush.Color := clRed;`nFillCircle(160, 110, 60);`nwriteln('graphabc ok');" 'graphabc ok' $true
Invoke-NotebookCase pascal "program NotebookImage;`nuses GraphABC;`nbegin`n  SetWindowSize(320, 220);`n  FillCircle(160, 110, 60);`n  writeln('full program ok')`nend." 'full program ok' $true
Invoke-NotebookCase pascal "uses ABCObjects;`nvar c := new CircleABC(160, 110, 60, clGreen);`nwriteln('abcobjects ok');" 'abcobjects ok' $true
Invoke-NotebookCase pascal "uses TurtleABC;`nfor var i := 1 to 4 do begin Forw(80); Turn(90); end;`nwriteln('turtle ok');" 'turtle ok' $true
Invoke-NotebookCase pascal "uses Turtle;`nForw(80);`nTurn(90);`nForw(80);`nwriteln('turtle wpf ok');" 'turtle wpf ok' $true
Invoke-NotebookCase pascal "uses GraphWPF;`nWindow.SetSize(320, 220);`nFillCircle(160, 110, 60, Colors.CornflowerBlue);`nwriteln('graphwpf ok');" 'graphwpf ok' $true
Invoke-NotebookCase pascal "uses PlotWPF;`nnew LineGraphWPF(0, 2, x -> x*x);`nwriteln('plotwpf ok');" 'plotwpf ok' $true
Invoke-NotebookCase pascal "uses Graph3D;`nWindow.SetSize(400, 300);`nSphere(0, 0, 0, 2, Colors.CornflowerBlue);`nwriteln('graph3d ok');" 'graph3d ok' $true
Invoke-NotebookCase pascal "uses PlotML;`nPlot.LineGraph([0.0, 1.0, 2.0], [0.0, 1.0, 4.0]);`nwriteln('plotml ok');" 'plotml ok' $true
Invoke-NotebookCase pascal "uses WPF;`nMainWindow.Width := 400;`nMainWindow.Height := 250;`nvar panel := Panels.DockPanel.AsMainContent;`nvar button := Controls.Button('Notebook');`npanel.Children.Add(button);`nwriteln('wpf ok');" 'wpf ok' $true
Invoke-NotebookCase spython "from GraphWPF import *`nFillCircle(160, 110, 60, Colors.CornflowerBlue)`nprint('spython graphwpf ok')" 'spython graphwpf ok' $true
Invoke-NotebookCase spython "from GraphABC import *`nSetWindowSize(320, 220)`nBrush.Color = clRed`nFillCircle(160, 110, 60)`nprint('spython graphabc ok')" 'spython graphabc ok' $true
Invoke-NotebookCase spython "import matplotlib.pyplot as plt`nplt.plot([0, 1, 2], [0, 1, 4], color='red')`nplt.show()`nprint('spython matplotlib ok')" 'spython matplotlib ok' $true
Invoke-NotebookSequenceCase pascal "var answer := 40;`nwriteln('hidden pascal');" "writeln(answer + 2);" '42' 'hidden pascal'
Invoke-NotebookSequenceCase spython "answer = 40`nprint('hidden spython')" 'print(answer + 2)' '42' 'hidden spython'

Write-Host 'PASS: notebook HTTP, both languages, and inline graphics.'

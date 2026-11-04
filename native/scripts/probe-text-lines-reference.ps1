param([string]$OutputDirectory)
$ErrorActionPreference = "Stop"
$referencePath = Join-Path $OutputDirectory "reader.txt"
$plainUtf8 = New-Object System.Text.UTF8Encoding($false)
$markedUtf8 = New-Object System.Text.UTF8Encoding($true)
[System.IO.File]::WriteAllText($referencePath,"a`nb`nc`n",$plainUtf8)
$referenceReader = New-Object System.IO.StreamReader($referencePath,$plainUtf8)
$firstCharacter = $referenceReader.Read()
$referenceReader.BaseStream.Position = 0
$collected = @()
while (-not $referenceReader.EndOfStream) { $collected += $referenceReader.ReadLine() }
$referenceReader.Dispose()
[System.IO.File]::WriteAllText($referencePath,"hello",$markedUtf8)
$referenceReader = New-Object System.IO.StreamReader($referencePath,$markedUtf8)
[void]$referenceReader.ReadToEnd()
$referenceReader.BaseStream.Position = 0
$replayed = $referenceReader.ReadLine()
$referenceReader.Dispose()
@{ first_character=$firstCharacter; retained_buffer_lines=$collected; replayed_bom=[int]$replayed[0]; replayed_length=$replayed.Length } | ConvertTo-Json -Compress

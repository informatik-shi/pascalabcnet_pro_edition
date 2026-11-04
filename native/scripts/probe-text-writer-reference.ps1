param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference="Stop"
$folder=$OutputDirectory
[void][System.IO.Directory]::CreateDirectory($folder)
$target=Join-Path $folder "writer.txt"
$encoding=New-Object System.Text.ASCIIEncoding
$rows=@()
foreach($count in @(0,1023,1024,1025,4096,4097,5120,5121)) {
$w=New-Object System.IO.StreamWriter($target,$false,$encoding)
$w.Write(("a"*$count))
$visible=(Get-Item -LiteralPath $target).Length
$w.BaseStream.Position=0
$afterSeek=(Get-Item -LiteralPath $target).Length
$w.Write("Z");$w.Close()
$bytes=[System.IO.File]::ReadAllBytes($target)
$rows+=@{count=$count;visible=$visible;after_seek=$afterSeek;final_length=$bytes.Length;first=[int]$bytes[0];last=[int]$bytes[$bytes.Length-1]}
}
[System.IO.File]::WriteAllText($target,"abc",$encoding)
$w=New-Object System.IO.StreamWriter($target,$true,$encoding)
$appendError=$false;try{$w.BaseStream.Position=0}catch{$appendError=$true};$w.Write("Z");$w.Close()
@{boundaries=$rows;append_seek_rejected=$appendError;append_result=[System.IO.File]::ReadAllText($target)}|ConvertTo-Json -Depth 5 -Compress
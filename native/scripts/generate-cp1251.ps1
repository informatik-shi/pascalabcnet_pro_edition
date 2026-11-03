param(
    [Parameter(Mandatory=$true)][string]$MappingPath
)
$ErrorActionPreference = 'Stop'
$decodeTable = [System.Collections.Generic.SortedDictionary[int,int]]::new()
$encodeTable = [System.Collections.Generic.SortedDictionary[int,int]]::new()
$tableMode = ''
foreach ($sourceLine in Get-Content -LiteralPath $MappingPath) {
    if ($sourceLine -match '^MBTABLE\s+256') { $tableMode = 'decode'; continue }
    if ($sourceLine -match '^WCTABLE\s+640') { $tableMode = 'encode'; continue }
    if ($sourceLine -match '^ENDCODEPAGE') { break }
    if ($sourceLine -match '^0x([0-9a-f]+)\s+0x([0-9a-f]+)') {
        $sourceValue = [Convert]::ToInt32($Matches[1],16)
        $targetValue = [Convert]::ToInt32($Matches[2],16)
        if ($tableMode -eq 'decode') { $decodeTable.Add($sourceValue,$targetValue) }
        elseif ($tableMode -eq 'encode') { $encodeTable.Add($sourceValue,$targetValue) }
    }
}
if ($decodeTable.Count -ne 256 -or $encodeTable.Count -ne 640) { throw 'Incomplete CP1251 source table' }
for ($asciiValue = 0; $asciiValue -lt 128; $asciiValue++) {
    if ($decodeTable[$asciiValue] -ne $asciiValue -or $encodeTable[$asciiValue] -ne $asciiValue) { throw 'Invalid ASCII mapping' }
}
$canonicalLines = [System.Collections.Generic.List[string]]::new()
$canonicalLines.Add('MBTABLE')
foreach ($entry in $decodeTable.GetEnumerator()) { $canonicalLines.Add(('0x{0:x2} 0x{1:x4}' -f $entry.Key,$entry.Value)) }
$canonicalLines.Add('WCTABLE')
foreach ($entry in $encodeTable.GetEnumerator()) { $canonicalLines.Add(('0x{0:x4} 0x{1:x2}' -f $entry.Key,$entry.Value)) }
$canonical = ($canonicalLines -join "`n") + "`n"
$sha = [Security.Cryptography.SHA256]::Create()
try { $actualHash = [BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($canonical))).Replace('-','').ToLowerInvariant() }
finally { $sha.Dispose() }
$expectedHash = 'ab635b9b3be6e9a92703d73da1d53e6652ba4e0717c2d76110c62f28d157bbea'
if ($actualHash -ne $expectedHash) { throw "CP1251 mapping hash mismatch: $actualHash" }
$outputLines = [System.Collections.Generic.List[string]]::new()
$outputLines.Add('// Generated CP1251 mapping data; regenerate with scripts/generate-cp1251.ps1.')
$outputLines.Add('// Source: https://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WindowsBestFit/bestfit1251.txt')
$outputLines.Add('// Canonical table SHA-256: '+$actualHash)
$outputLines.Add('constexpr std::array<char16_t,128> windows1251_high{')
for ($byteIndex = 128; $byteIndex -lt 256; $byteIndex += 8) {
    $parts = for ($offset = 0; $offset -lt 8; $offset++) { '0x{0:x4}' -f $decodeTable[$byteIndex+$offset] }
    $outputLines.Add('    '+($parts -join ',')+',')
}
$outputLines.Add('};')
$outputLines.Add('struct Windows1251Entry { char16_t character;std::uint8_t byte; };')
$outputLines.Add('constexpr std::array<Windows1251Entry,512> windows1251_encode{{')
$highEntries = @($encodeTable.GetEnumerator() | Where-Object { $_.Key -ge 128 })
for ($entryIndex = 0; $entryIndex -lt $highEntries.Length; $entryIndex += 4) {
    $parts = for ($offset = 0; $offset -lt 4; $offset++) { $entry = $highEntries[$entryIndex+$offset]; '{0}0x{1:x4},0x{2:x2}{3}' -f '{',$entry.Key,$entry.Value,'}' }
    $outputLines.Add('    '+($parts -join ',')+',')
}
$outputLines.Add('}};')
$outputPath = Join-Path $PSScriptRoot '../src/cp1251_data.inc'
[IO.File]::WriteAllText($outputPath,($outputLines -join "`n")+"`n",[Text.UTF8Encoding]::new($false))
Write-Output 'Generated CP1251 tables: 256 decode entries, 640 encode entries.'

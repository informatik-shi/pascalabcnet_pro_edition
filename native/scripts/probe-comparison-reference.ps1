param([Parameter(Mandatory=$true)][string]$OutputPath)
$ErrorActionPreference="Stop"
$culture=[Globalization.CultureInfo]::GetCultureInfo("en-US")
$eAcute=[string][char]0x00e9
$eDecomposed="e"+[char]0x0301
$aUmlaut=[string][char]0x00e4
$pairs=@(@("a","A"),@("A","z"),@($eAcute,"z"),@($eAcute,$eDecomposed),@($aUmlaut,"z"),@("alpha","beta"),@("","a"),@("a",""),@("same","same"),@("10","2"),@("abc","abcd"),@("abcd","abc"))
$rows=@()
foreach($pair in $pairs) {
  $a=[string]$pair[0];$b=[string]$pair[1]
  $rows+=@{left=$a;right=$b;culture_order=[Math]::Sign($culture.CompareInfo.Compare($a,$b,[Globalization.CompareOptions]::None));ordinal_order=[Math]::Sign([String]::CompareOrdinal($a,$b))}
}
$negativeZero=[BitConverter]::Int64BitsToDouble([Int64]::MinValue)
$zeroRows=@()
foreach($firstNegative in @($false,$true)) {
  foreach($secondNegative in @($false,$true)) {
    $first=if($firstNegative){$negativeZero}else{0.0}
$second=if($secondNegative){$negativeZero}else{0.0}
    $zeroRows+=@{first_negative=$firstNegative;second_negative=$secondNegative;min_negative=([BitConverter]::DoubleToInt64Bits([Math]::Min($first,$second)) -lt 0);max_negative=([BitConverter]::DoubleToInt64Bits([Math]::Max($first,$second)) -lt 0)}
  }
}
@{origin="Independent .NET Framework CompareInfo en-US, String.CompareOrdinal and Math.Min/Max API probe; original Pascal compiler not executed";rows=$rows;zero_rows=$zeroRows} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $OutputPath -Encoding UTF8

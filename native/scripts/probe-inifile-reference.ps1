param([Parameter(Mandatory=$true)][string]$OutputPath)
$ErrorActionPreference="Stop"
[Threading.Thread]::CurrentThread.CurrentCulture=[Globalization.CultureInfo]::GetCultureInfo("en-US")
$list=New-Object System.Collections.SortedList
$acute=[string][char]0xe9
$decomposed="e"+[char]0x301
$list[$acute]="first"
$list[$decomposed]="second"
$list["z"]="z"
$list["a"]="a"
$list["A"]="A"
$list[""]="empty"
$list["nil-value"]=$null
$keys=@();$values=@()
for($i=0;$i -lt $list.Count;$i++){ $keys+=([string]$list.GetKey($i));$values+=$list.GetByIndex($i) }
$nilKeyError=""
try{[void]$list.IndexOfKey($null)}catch{$nilKeyError=$_.Exception.GetBaseException().GetType().Name}
$trimCases=@()
foreach($line in @("  [section]  ",("`t[section]`t"),("`t  x= y  `t")," [openX "," [] "," [ ")){ $trimCases+=@{input=$line;trim_spaces=$line.Trim([char[]]@([char]0x20))} }
@{origin="Independent .NET Framework SortedList/en-US/string API probe; original Pascal compiler not executed";keys=$keys;values=$values;canonical_duplicate_count=$list.Count;nil_key_error=$nilKeyError;trim_cases=$trimCases} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $OutputPath -Encoding UTF8

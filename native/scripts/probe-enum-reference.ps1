param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference="Stop"
[void][System.IO.Directory]::CreateDirectory($OutputDirectory)
$assemblyName=New-Object System.Reflection.AssemblyName("NativeEnumReference")
$assembly=[AppDomain]::CurrentDomain.DefineDynamicAssembly($assemblyName,[System.Reflection.Emit.AssemblyBuilderAccess]::Run)
$module=$assembly.DefineDynamicModule("Reference")
$rows=@()
foreach($hasFlags in @($false,$true)) {
  $label=if($hasFlags){"BitsFlags"}else{"Bits"}
  $builder=$module.DefineEnum($label,[System.Reflection.TypeAttributes]::Public,[int])
  if($hasFlags) {
    $constructor=[FlagsAttribute].GetConstructor([Type[]]@())
    $attribute=New-Object System.Reflection.Emit.CustomAttributeBuilder($constructor,[object[]]@())
    $builder.SetCustomAttribute($attribute)
  }
  foreach($member in @(@("Zero",0),@("Read",1),@("Write",2),@("Both",3),@("Exec",4),@("Sign",[int]::MinValue),@("All",-1))) {
    [void]$builder.DefineLiteral([string]$member[0],[int]$member[1])
  }
  $enumType=$builder.CreateType()
  foreach($number in @(0,1,3,5,7,-1,-2,[int]::MinValue,([int]::MinValue+3))) {
    $value=[Enum]::ToObject($enumType,[int]$number)
    $rows+=@{ flags=$hasFlags; value=$number; general=$value.ToString("G"); decimal=$value.ToString("D"); hex=$value.ToString("x"); forced_flags=$value.ToString("F") }
  }
}
@{ origin="Independent System.Enum Reflection.Emit API probe; original Pascal compiler not executed"; rows=$rows } | ConvertTo-Json -Depth 5

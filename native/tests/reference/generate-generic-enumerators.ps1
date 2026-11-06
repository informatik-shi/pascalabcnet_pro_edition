# Independent development-only oracle; excluded from application installation.
$ErrorActionPreference='Stop'
Add-Type -Path (Join-Path $PSScriptRoot 'generic-enumerators.cs') -ReferencedAssemblies 'System.Core'
[GenericEnumeratorReference]::Generate((Join-Path $PSScriptRoot '..\fixtures\generic-enumerators-reference.tsv'))

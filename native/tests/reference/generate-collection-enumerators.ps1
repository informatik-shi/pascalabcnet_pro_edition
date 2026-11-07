# Independent development oracle; excluded from native installation.
$ErrorActionPreference='Stop'
Add-Type -Path (Join-Path $PSScriptRoot 'collection-enumerators.cs') -ReferencedAssemblies 'System.Core'
[CollectionEnumeratorReference]::Generate((Join-Path $PSScriptRoot '..\fixtures\collection-enumerators-reference.tsv'))

# Development-only legacy Framework oracle; never installed with the application.
$ErrorActionPreference='Stop'
Add-Type -Path (Join-Path $PSScriptRoot 'dictionary-enumerators.cs')
[EnumeratorReference]::Generate((Join-Path $PSScriptRoot '..\fixtures\dictionary-enumerators-reference.tsv'))

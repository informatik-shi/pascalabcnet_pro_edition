# Independent development oracle; never installed in the native application.
$ErrorActionPreference='Stop'
Add-Type -Path (Join-Path $PSScriptRoot 'user-interfaces.cs')
[UserInterfaceReference]::Generate((Join-Path $PSScriptRoot '..\fixtures\user-interfaces-reference.tsv'))

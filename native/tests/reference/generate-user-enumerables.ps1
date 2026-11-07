# Independent development oracle; never installed in the native application.
$ErrorActionPreference='Stop'
Add-Type -Path (Join-Path $PSScriptRoot 'user-enumerables.cs')
[UserEnumerableReference]::Generate((Join-Path $PSScriptRoot '..\fixtures\user-enumerables-reference.tsv'))

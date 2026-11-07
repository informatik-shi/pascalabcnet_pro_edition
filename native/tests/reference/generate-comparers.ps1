$ErrorActionPreference='Stop'
Add-Type -Path (Join-Path $PSScriptRoot 'comparers.cs')
[ComparerReference]::Generate((Join-Path $PSScriptRoot '..\fixtures\comparers-reference.tsv'))
[ComparerReference]::Scalars((Join-Path $PSScriptRoot '..\fixtures\comparers-scalars-reference.tsv'))

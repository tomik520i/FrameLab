param([Parameter(Mandatory=$true)][string]$RuntimeDirectory)
$ErrorActionPreference = 'Stop'
$destination = Join-Path (Split-Path $PSScriptRoot -Parent) 'app/runtime'
$names = @('cudart64_12.dll','NVCVImage.dll','nvngxruntime.dll','NVVideoEffects.dll','nvVFXVideoFrameGeneration.dll')
foreach ($name in $names) { if (-not (Test-Path -LiteralPath (Join-Path $RuntimeDirectory $name) -PathType Leaf)) { throw "Missing required runtime DLL: $name" } }
New-Item -ItemType Directory -Force $destination | Out-Null
foreach ($name in $names) { Copy-Item -LiteralPath (Join-Path $RuntimeDirectory $name) -Destination $destination }
Write-Output 'Runtime imported for local use. Retain the NVIDIA licenses supplied with your download. Do not redistribute this directory automatically.'

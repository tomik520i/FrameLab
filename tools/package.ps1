param([string]$Version = '0.2.0-preview', [string]$OutputDirectory = 'dist')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root
if (-not (Test-Path 'app/FrameLab.exe')) { throw 'Build FrameLab first' }
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$packageRoot = (Resolve-Path -LiteralPath $OutputDirectory).Path
$stage = Join-Path $packageRoot "FrameLab-$Version"
$portableZip = Join-Path $packageRoot "FrameLab-$Version-windows-x64.zip"
$sourceZip = Join-Path $packageRoot "FrameLab-$Version-source.zip"
if ((Test-Path -LiteralPath $stage) -or (Test-Path -LiteralPath $portableZip) -or (Test-Path -LiteralPath $sourceZip)) { throw 'Package output exists; choose a new version or output directory to preserve it' }
New-Item -ItemType Directory -Force "$stage/app","$stage/tools","$stage/docs" | Out-Null
Copy-Item 'app/FrameLab.exe' "$stage/app/"
Copy-Item 'Start FrameLab.cmd','Start FrameLab - Optical Flow.cmd','START_HERE.md','README.md','LICENSE','THIRD_PARTY_NOTICES.md' $stage
Copy-Item 'docs/*.md' "$stage/docs/"
Copy-Item 'docs/images' "$stage/docs/" -Recurse
if (Test-Path 'docs/performance') { Copy-Item 'docs/performance' "$stage/docs/" -Recurse }
Copy-Item 'examples' $stage -Recurse
Copy-Item 'tools/export_analysis.py','tools/requirements.txt','tools/setup-diagnostics.ps1','tools/import-runtime.ps1' "$stage/tools/"
Copy-Item 'third_party/CaptureKVM-LICENSE.txt' "$stage/docs/"
Copy-Item 'third_party/imgui/LICENSE.txt' "$stage/docs/ImGui-LICENSE.txt"
Copy-Item 'third_party/lz4/LICENSE' "$stage/docs/LZ4-LICENSE.txt"
Compress-Archive -Path "$stage/*" -DestinationPath $portableZip
# Exact reviewed source set from the Git index, not ignored personal data.
$tree = git write-tree
if ($LASTEXITCODE) { throw 'Stage reviewed source files first' }
git archive --format=zip --output="$sourceZip" $tree
if ($LASTEXITCODE) { throw 'Source archive failed' }
[ordered]@{ version = $Version; source_tree = $tree; application_sha256 = (Get-FileHash 'app/FrameLab.exe').Hash.ToLower(); portable_sha256 = (Get-FileHash $portableZip).Hash.ToLower(); source_sha256 = (Get-FileHash $sourceZip).Hash.ToLower(); note = 'Source ZIP is the exact staged Git tree. Proprietary NVIDIA runtime and personal settings are excluded.' } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $packageRoot "FrameLab-$Version-provenance.json") -Encoding UTF8
Get-FileHash $portableZip,$sourceZip | Format-Table

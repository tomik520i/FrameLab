param([string]$Version = '0.2.0-preview')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root
if (-not (Test-Path 'app/FrameLab.exe')) { throw 'Build FrameLab first' }
$stage = Join-Path $root "dist/FrameLab-$Version"
if (Test-Path $stage) { throw 'Package directory exists; choose a new version to preserve old artifacts' }
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
Compress-Archive -Path "$stage/*" -DestinationPath "dist/FrameLab-$Version-windows-x64.zip"
# Exact reviewed source set from the Git index, not ignored personal data.
$tree = git write-tree
if ($LASTEXITCODE) { throw 'Stage reviewed source files first' }
git archive --format=zip --output="dist/FrameLab-$Version-source.zip" $tree
if ($LASTEXITCODE) { throw 'Source archive failed' }
Get-FileHash "dist/FrameLab-$Version-windows-x64.zip","dist/FrameLab-$Version-source.zip" | Format-Table

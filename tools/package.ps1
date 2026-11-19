param([string]$Version = '0.2.0-preview', [string]$OutputDirectory = 'dist', [string]$ApplicationPath = 'app/FrameLab.exe', [switch]$Complete, [string]$SampleDirectory = 'app/samples', [string]$SetupPath = 'app/FrameLab-Setup.exe', [string]$NrHostPath = 'app/nr-host/nvngx.dll', [switch]$Translation, [string]$TranslationDirectory = 'app/translation')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root
if (-not (Test-Path -LiteralPath $ApplicationPath)) { throw 'Build FrameLab first' }
if ($Complete -and -not $Translation) { throw 'A complete preview.11 package must include translation: add -Translation' }
foreach ($sample in 'action.mp4','combat.mp4','ocr-demo.mp4','action.source.json','combat.source.json','ocr-demo.source.json') {
 if (-not (Test-Path -LiteralPath (Join-Path $SampleDirectory $sample))) { throw "Both sample videos and provenance must be bundled: missing $sample in $SampleDirectory" }
}
if ($Translation -and -not (Test-Path -LiteralPath (Join-Path $TranslationDirectory 'provenance.json'))) { throw 'Prepare the complete offline translation component first' }
if ($Complete -and -not (Test-Path 'deps/portable/components.json')) { throw 'Run tools/prepare_portable_dependencies.py first' }
if ($Complete -and -not (Test-Path 'deps/portable/msvc/manifest.json')) { throw 'Run tools/prepare_msvc_dependencies.ps1 first' }
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$packageRoot = (Resolve-Path -LiteralPath $OutputDirectory).Path
$stage = Join-Path $packageRoot "FrameLab-$Version"
$variant = if ($Complete) { 'windows-x64-full' } else { 'windows-x64' }
$portableZip = Join-Path $packageRoot "FrameLab-$Version-$variant.zip"
$sourceZip = Join-Path $packageRoot "FrameLab-$Version-source.zip"
if ((Test-Path -LiteralPath $stage) -or (Test-Path -LiteralPath $portableZip) -or (Test-Path -LiteralPath $sourceZip)) { throw 'Package output exists; choose a new version or output directory to preserve it' }
New-Item -ItemType Directory -Force "$stage/app","$stage/tools","$stage/docs" | Out-Null
Copy-Item -LiteralPath $ApplicationPath -Destination "$stage/app/FrameLab.exe"
Copy-Item -LiteralPath $SetupPath -Destination "$stage/app/FrameLab-Setup.exe"
if (!(Test-Path -LiteralPath $NrHostPath)) { throw 'Build the NR adapter first (or use the matching preview.10 packaging tool for an NR-OFF release)' }
New-Item -ItemType Directory -Path "$stage/app/nr-host" -Force | Out-Null
Copy-Item -LiteralPath $NrHostPath -Destination "$stage/app/nr-host/nvngx.dll"
Copy-Item 'tools/Install-Enhancer.ps1' "$stage/tools/"
Copy-Item '4 - Doplnit enhancer.cmd' $stage
New-Item -ItemType Directory -Path "$stage/app/samples" -Force | Out-Null
foreach ($sample in 'action.mp4','combat.mp4','ocr-demo.mp4','action.source.json','combat.source.json','ocr-demo.source.json') { Copy-Item -LiteralPath (Join-Path $SampleDirectory $sample) -Destination "$stage/app/samples/" }
if ($Translation) { Copy-Item -LiteralPath $TranslationDirectory -Destination "$stage/app/translation" -Recurse }
Copy-Item 'Start FrameLab.cmd','Start FrameLab - NVIDIA.cmd','Start FrameLab - Optical Flow.cmd','START_HERE.md','README.md','LICENSE','THIRD_PARTY_NOTICES.md' $stage
Copy-Item 'INSTALL.cs.md','1 - Spustit FrameLab.cmd','2 - NVIDIA RTX 40-50.cmd','3 - Optical Flow.cmd','Ukazka - Spider-Man.cmd','Ukazka - Ceske titulky.cmd' $stage
Copy-Item 'INSTALL.cs.md' "$stage/ZACNI TADY.txt"
Set-Content -LiteralPath "$stage/app/language.txt" -Value '1' -NoNewline
Copy-Item 'docs/*.md' "$stage/docs/"
Copy-Item 'docs/NR-LICENSING-STATUS.txt','docs/translation-components.json','docs/sample-xiph-copyright.txt' "$stage/docs/"
Copy-Item 'docs/images' "$stage/docs/" -Recurse
if (Test-Path 'docs/performance') { Copy-Item 'docs/performance' "$stage/docs/" -Recurse }
Copy-Item 'examples' $stage -Recurse
Copy-Item 'tools/export_analysis.py','tools/requirements.txt','tools/setup-diagnostics.ps1','tools/import-runtime.ps1' "$stage/tools/"
Copy-Item 'third_party/CaptureKVM-LICENSE.txt' "$stage/docs/"
Copy-Item 'third_party/imgui/LICENSE.txt' "$stage/docs/ImGui-LICENSE.txt"
Copy-Item 'third_party/lz4/LICENSE' "$stage/docs/LZ4-LICENSE.txt"
Copy-Item 'website/assets/fonts/SpaceGrotesk-OFL.txt','website/assets/fonts/IBMPlexMono-OFL.txt' "$stage/docs/"
if ($Complete) {
 Copy-Item 'deps/portable/nvidia-runtime' "$stage/app/runtime" -Recurse
 Copy-Item 'deps/portable/msvc/*.dll' "$stage/app/runtime/"
 Copy-Item 'deps/portable/licenses' "$stage/licenses" -Recurse
 Copy-Item 'deps/portable/python' "$stage/tools/python" -Recurse
 New-Item -ItemType Directory -Path "$stage/deps" -Force | Out-Null
 $dependencyRoot = (Resolve-Path -LiteralPath 'deps/portable/packages').Path
 foreach ($dependencyFile in (Get-ChildItem -LiteralPath $dependencyRoot -Recurse -File | Where-Object { $_.Extension -ne '.pyc' -and $_.FullName -notmatch '[\\/]__pycache__[\\/]' })) {
  $relativeDependency = [IO.Path]::GetRelativePath($dependencyRoot, $dependencyFile.FullName)
  $dependencyTarget = Join-Path "$stage/deps/python" $relativeDependency
  New-Item -ItemType Directory -Path (Split-Path $dependencyTarget -Parent) -Force | Out-Null
  Copy-Item -LiteralPath $dependencyFile.FullName -Destination $dependencyTarget
 }
 Copy-Item 'deps/portable/components.json' "$stage/licenses/components.json"
 Copy-Item 'deps/portable/msvc/manifest.json' "$stage/licenses/microsoft-vc-manifest.json"
 Copy-Item 'docs/runtime-distribution.md' "$stage/licenses/README.md"
 $msvcManifest = Get-Content 'deps/portable/msvc/manifest.json' -Raw | ConvertFrom-Json
 foreach ($file in $msvcManifest.files) {
  $runtimePath = Join-Path "$stage/app/runtime" $file.name
  if ((Get-FileHash -LiteralPath $runtimePath).Hash.ToLower() -cne $file.sha256) { throw "Microsoft runtime hash mismatch: $($file.name)" }
  $signature = Get-AuthenticodeSignature -LiteralPath $runtimePath
  if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'Microsoft Corporation') { throw "Invalid Microsoft signature: $($file.name)" }
 }
 $manifest = Get-Content "$stage/app/runtime/manifest.json" -Raw | ConvertFrom-Json
 foreach ($file in $manifest.files) {
  $runtimePath = Join-Path "$stage/app/runtime" $file.name
  if ((Get-FileHash -LiteralPath $runtimePath).Hash.ToLower() -cne $file.sha256) { throw "Runtime hash mismatch: $($file.name)" }
  $signature = Get-AuthenticodeSignature -LiteralPath $runtimePath
  if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'NVIDIA Corporation') { throw "Invalid NVIDIA signature: $($file.name)" }
 }
}
if (Get-ChildItem -LiteralPath $stage -Recurse -File -Filter 'nvngx_dlssnr.dll') { throw 'NR runtimes are not approved for a public package' }
Compress-Archive -Path "$stage/*" -DestinationPath $portableZip
# Exact reviewed source set from the Git index, not ignored personal data.
$tree = git write-tree
if ($LASTEXITCODE) { throw 'Stage reviewed source files first' }
git archive --format=zip --output="$sourceZip" $tree
if ($LASTEXITCODE) { throw 'Source archive failed' }
[ordered]@{ version = $Version; source_tree = $tree; application_sha256 = (Get-FileHash -LiteralPath $ApplicationPath).Hash.ToLower(); portable_sha256 = (Get-FileHash $portableZip).Hash.ToLower(); source_sha256 = (Get-FileHash $sourceZip).Hash.ToLower(); complete_bundle = [bool]$Complete; note = 'Source ZIP is the exact staged Git tree. Complete bundle contains separate unmodified NVIDIA components and portable export dependencies with their licenses. Personal settings and recordings are excluded.' } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $packageRoot "FrameLab-$Version-provenance.json") -Encoding UTF8
Get-FileHash $portableZip,$sourceZip | Format-Table

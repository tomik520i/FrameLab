param([string]$RedistDirectory)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if (-not $RedistDirectory) {
 $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
 $installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 if (-not $installation) { throw 'Provide a licensed Visual Studio x64 Microsoft.VC143.CRT redist directory' }
 $RedistDirectory = (Get-ChildItem -Path "$installation/VC/Redist/MSVC/*/x64/Microsoft.VC143.CRT" -Directory | Sort-Object FullName -Descending | Select-Object -First 1).FullName
}
if (-not $RedistDirectory) { throw 'Visual C++ redistributable directory not found' }
$target = Join-Path $root 'deps/portable/msvc'
New-Item -ItemType Directory -Path $target -Force | Out-Null
$files = foreach ($name in @('msvcp140.dll','vcruntime140.dll','vcruntime140_1.dll')) {
 $source = Join-Path $RedistDirectory $name
 $signature = Get-AuthenticodeSignature -LiteralPath $source
 if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'Microsoft Corporation') { throw "Invalid Microsoft signature: $name" }
 Copy-Item -LiteralPath $source -Destination (Join-Path $target $name)
 [ordered]@{name=$name; sha256=(Get-FileHash -LiteralPath $source).Hash.ToLower(); bytes=(Get-Item -LiteralPath $source).Length; version=(Get-Item -LiteralPath $source).VersionInfo.FileVersion}
}
[ordered]@{
 component='Microsoft Visual C++ 2022 x64 app-local runtime'; modified=$false
 origin='Visual Studio VC/Redist/MSVC/x64/Microsoft.VC143.CRT'
 distribution_list='https://learn.microsoft.com/en-us/visualstudio/releases/2022/redistribution'
 deployment_reference='https://learn.microsoft.com/en-us/cpp/windows/redistributing-visual-cpp-files?view=msvc-170'
 files=@($files)
} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $target 'manifest.json') -Encoding UTF8
$files | Format-Table

param([string]$Archive, [string]$AppDir = [IO.Path]::Combine([IO.Directory]::GetParent($PSScriptRoot).FullName, 'app'), [switch]$CheckOnly, [switch]$NoGui)
$ErrorActionPreference = 'Stop'
# A launcher (for example PowerShell 7 or Python) can pass an incompatible module
# search path to Windows PowerShell 5.1. Use this process's own inbox modules only.
$env:PSModulePath = [IO.Path]::Combine($PSHOME, 'Modules')
Import-Module Microsoft.PowerShell.Management
Import-Module Microsoft.PowerShell.Utility
Add-Type -AssemblyName System.IO.Compression.FileSystem
$SourceUrl = 'https://github.com/Likely7/Veyra-NRVideo/releases/download/v2.1.9/Veyra-2.1.9-NVIDIA-win64-portable.zip'
$ArchiveHash = '4225ecb11a8e96c2826b4b0b798feb811d7ae237f3edf0b627f3b6895c79f362'
$Profiles = @(
    @{Name='ShortFuse NR 310.9.16'; Entry='runtime/experimental/nr-ampere/nvngx_dlssnr.dll'; Target='runtime/nr-sf/nvngx_dlssnr.dll'; Bytes=276162560; Hash='cdfa6c1ec784867182164c7836f324f087b5c9f374d0c9fe46f54999feabf3ed'},
    @{Name='NVIDIA NR 310.8 original'; Entry='runtime/experimental/nr-original/nvngx_dlssnr.dll'; Target='runtime/nr-original/nvngx_dlssnr.dll'; Bytes=165840496; Hash='e16bcf15e16e13f527491cdf7845b2fe6521a738d8f7c9c721866a8496e1fc8e'}
)
function Get-NrStatus {
    $items = foreach ($nrEntry in $Profiles) {
        $path = Join-Path $AppDir $nrEntry.Target
        $valid = (Test-Path -LiteralPath $path -PathType Leaf) -and (Get-Item -LiteralPath $path).Length -eq $nrEntry.Bytes
        if ($valid) { $valid = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -eq $nrEntry.Hash }
        [pscustomobject]@{Name=$nrEntry.Name; Ready=$valid; Path=$path}
    }
    return $items
}
function Import-NrArchive([string]$Path) {
    if (!(Test-Path -LiteralPath (Join-Path $AppDir 'nr-host/nvngx.dll'))) { throw 'Tato kopie nema podporu NR. Stahnete cely FrameLab preview.11 a rozbalte vsechny soubory.' }
    if (!(Test-Path -LiteralPath $Path -PathType Leaf)) { throw 'Vyberte stazeny ZIP Veyra-2.1.9-NVIDIA-win64-portable.zip.' }
    if ((Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash -ne $ArchiveHash) { throw 'Kontrolni soucet ZIPu nesouhlasi. Pouzijte presny NVIDIA portable 2.1.9 bez dodatku Virtual Camera; pripadne stahnete archiv znovu.' }
    $base = [IO.Path]::GetFullPath($AppDir)
    $stage = Join-Path $base ('nr-import-' + [guid]::NewGuid().ToString('N'))
    [IO.Directory]::CreateDirectory($stage) | Out-Null
    $zip = [IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $Path).Path)
    try {
        $prefix = 'Veyra-2.1.9-NVIDIA-win64-portable/'
        $files = @()
        foreach ($nrEntry in $Profiles) {
            $entry = $zip.GetEntry($prefix + $nrEntry.Entry)
            if (!$entry -or $entry.Length -ne $nrEntry.Bytes) { throw ('V ZIPu chybi spravna komponenta: ' + $nrEntry.Name) }
            $temp = Join-Path $stage $nrEntry.Target
            [IO.Directory]::CreateDirectory((Split-Path $temp -Parent)) | Out-Null
            [IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $temp)
            if ((Get-FileHash -LiteralPath $temp -Algorithm SHA256).Hash -ne $nrEntry.Hash) { throw ('Poskozena komponenta: ' + $nrEntry.Name) }
            $files += @{Source=$temp; Relative=$nrEntry.Target; Hash=$nrEntry.Hash}
        }
        foreach ($entry in $zip.Entries) {
            if (!$entry.FullName.StartsWith($prefix + 'licenses/')) { continue }
            $relative = $entry.FullName.Substring($prefix.Length)
            if ($entry.Length -eq 0 -or !($relative.ToUpperInvariant().Contains('NVIDIA') -or $entry.Name.ToUpperInvariant().Contains('NV'))) { continue }
            $dest = [IO.Path]::GetFullPath((Join-Path $stage $relative))
            if (!$dest.StartsWith($stage + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Neplatna cesta v archivu.' }
            [IO.Directory]::CreateDirectory((Split-Path $dest -Parent)) | Out-Null
            [IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $dest)
            $files += @{Source=$dest; Relative=('licenses/nr-external/' + $relative.Substring(9)); Hash=(Get-FileHash -LiteralPath $dest -Algorithm SHA256).Hash.ToLowerInvariant()}
        }
        # Verify all components before replacing anything. Existing differing files are kept as backups.
        foreach ($file in $files) {
            $dest = [IO.Path]::GetFullPath((Join-Path $base $file.Relative))
            if (!$dest.StartsWith($base + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Neplatny cil.' }
            [IO.Directory]::CreateDirectory((Split-Path $dest -Parent)) | Out-Null
            if (Test-Path -LiteralPath $dest) {
                if ((Get-FileHash -LiteralPath $dest -Algorithm SHA256).Hash -eq $file.Hash) { continue }
                Move-Item -LiteralPath $dest -Destination ($dest + '.backup-' + [guid]::NewGuid().ToString('N'))
            }
            Move-Item -LiteralPath $file.Source -Destination $dest
        }
        @{source_url=$SourceUrl; archive_sha256=$ArchiveHash; imported_at_utc=[DateTime]::UtcNow.ToString('o'); rights='External community source. Import and matching hashes do not grant a license or redistribution permission.'; files=@($Profiles | ForEach-Object { @{file=$_.Target; sha256=$_.Hash; bytes=$_.Bytes} })} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $base 'nr-import-provenance.json') -Encoding UTF8
        return Get-NrStatus
    } finally {
        $zip.Dispose()
        # Only this invocation's verified staging directory can be removed.
        $resolved = [IO.Path]::GetFullPath($stage)
        if ($resolved.StartsWith($base + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase) -and [IO.Path]::GetFileName($resolved).StartsWith('nr-import-')) { Remove-Item -LiteralPath $resolved -Recurse -Force }
    }
}
if ($CheckOnly) { Get-NrStatus | ConvertTo-Json; exit 0 }
if ($Archive) { Import-NrArchive $Archive | ConvertTo-Json; exit 0 }
if ($NoGui) { throw 'Zadejte -Archive nebo -CheckOnly.' }

$wizard = Join-Path $AppDir 'FrameLab-Setup.exe'
if (!(Test-Path -LiteralPath $wizard)) { throw 'Rozbalte cely FrameLab preview.11; chybi app/FrameLab-Setup.exe.' }
Start-Process -FilePath $wizard

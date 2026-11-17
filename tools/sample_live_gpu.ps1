param(
    [Parameter(Mandatory=$true)][string]$SessionCsv,
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [ValidateRange(2,4)][int]$Multiplier=2,
    [ValidateRange(5,120)][int]$Seconds=30
)
$ErrorActionPreference = 'Stop'
if (Test-Path $OutputDirectory) { throw 'Choose a new output directory to preserve previous measurements' }
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$firstRow = Import-Csv -LiteralPath $SessionCsv | Select-Object -Last 1
$started = [DateTimeOffset]::UtcNow
$timer = [Diagnostics.Stopwatch]::StartNew()
$rows = [Collections.Generic.List[object]]::new()
while ($timer.Elapsed.TotalSeconds -lt $Seconds) {
    $result = & nvidia-smi --id=0 --query-gpu=utilization.gpu,utilization.memory,memory.used,temperature.gpu,power.draw,clocks.current.graphics --format=csv,noheader,nounits
    if ($LASTEXITCODE) { throw 'nvidia-smi failed' }
    $values = $result -split ',\s*'
    $rows.Add([pscustomobject]@{elapsed_s=$timer.Elapsed.TotalSeconds; utc=[DateTimeOffset]::UtcNow.ToString('o');gpu_percent=$values[0];memory_controller_percent=$values[1];vram_mib=$values[2];temperature_c=$values[3];power_w=$values[4];graphics_mhz=$values[5]})
    Start-Sleep -Milliseconds 1000
}
$ended = [DateTimeOffset]::UtcNow
$rows | Export-Csv -LiteralPath (Join-Path $OutputDirectory 'nvidia-smi.csv') -NoTypeInformation
$sessionRows = @(Import-Csv -LiteralPath $SessionCsv | Where-Object { [double]$_.elapsed_s -gt [double]$firstRow.elapsed_s })
$sessionRows | Export-Csv -LiteralPath (Join-Path $OutputDirectory 'app-telemetry.csv') -NoTypeInformation
$metadata = [ordered]@{mode='Live capture and frame generation, no analysis recording or video export';multiplier=$Multiplier;backend='NVIDIA VFG';quality='Medium';width=2560;height=1440;requested_capture_fps=60;vsync=$false;dedup=$true;automatic_cut_detection=$true;comparison_panel=$true;display_mode_hz=165;started_utc=$started.ToString('o');ended_utc=$ended.ToString('o');duration_s=$timer.Elapsed.TotalSeconds;gpu_samples=$rows.Count;app_samples=$sessionRows.Count;session_csv=(Split-Path $SessionCsv -Leaf);start_session_elapsed_s=[double]$firstRow.elapsed_s;settings_source='Visually checked in the application before and after sampling';limits=@('GPU utilization, VRAM and power describe the entire GPU, including other running applications.','Desktop apps were left open; this is not an isolated benchmark.','Live street scene with NPC motion; camera path is not scripted.','GPU timing in the application is the last sampled generated-frame operation plus GPU output copy, not total application frame time.','No physical monitor scanout, input-to-photon or CPU bottleneck measurement.')}
$metadata | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $OutputDirectory 'measurement.json') -Encoding utf8
Write-Output ('Measured '+$Multiplier+'x: '+$rows.Count+' GPU samples, '+$sessionRows.Count+' application samples')

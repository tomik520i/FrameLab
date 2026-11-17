param([switch]$BuildOnly)
$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
$commit = 'f5befd2d29e66809cd1110a152e375a7f1981f06'
if (-not (Test-Path 'third_party/imgui/imgui.h')) {
    New-Item -ItemType Directory -Force 'downloads' | Out-Null
    Invoke-WebRequest "https://github.com/ocornut/imgui/archive/$commit.zip" -OutFile 'downloads/imgui.zip'
    Expand-Archive 'downloads/imgui.zip' -DestinationPath 'downloads/imgui-source' -Force
    New-Item -ItemType Directory -Force 'third_party/imgui' | Out-Null
    Copy-Item -Path "downloads/imgui-source/imgui-$commit/*" -Destination 'third_party/imgui' -Recurse -Force
}
if (-not (Test-Path 'deps/nvidia/cuda_runtime/include/cuda.h')) {
    python -m pip install --target deps nvidia-cuda-runtime-cu12==12.9.79
    if ($LASTEXITCODE) { throw 'CUDA header installation failed' }
}
if (-not $BuildOnly) {
    python -m pip install --target deps/python -r tools/requirements.txt
    if ($LASTEXITCODE) { throw 'Diagnostic dependency installation failed' }
}

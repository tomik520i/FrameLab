$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
cmake -S . -B build -G 'Visual Studio 17 2022' -A x64 -DFRAMELAB_OUTPUT_NAME=FrameLab -DFRAMELAB_EXPERIMENTAL_NR=ON
if ($LASTEXITCODE) { exit $LASTEXITCODE }
cmake --build build --config Release --parallel
exit $LASTEXITCODE

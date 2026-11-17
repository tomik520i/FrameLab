$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
python -m pip install --target deps/python -r tools/requirements.txt
if ($LASTEXITCODE) { throw 'Diagnostic dependency installation failed' }

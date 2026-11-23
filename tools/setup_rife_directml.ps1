param([string]$AppDir = (Join-Path $PSScriptRoot '../app'))
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$cache = Join-Path $repo 'deps/rife-downloads'
New-Item -ItemType Directory -Path $cache -Force | Out-Null
function Fetch-Package($name, $url, $digest) {
 $archive = Join-Path $cache "$name.zip"
 if(!(Test-Path -LiteralPath $archive)){Invoke-WebRequest -Uri $url -OutFile $archive}
 if((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $digest){throw "SHA256 mismatch: $archive"}
 $folder = Join-Path $cache $name
 Expand-Archive -LiteralPath $archive -DestinationPath $folder -Force
 return $folder
}
$ort = Fetch-Package 'ort-1.24.4' 'https://api.nuget.org/v3-flatcontainer/microsoft.ml.onnxruntime.directml/1.24.4/microsoft.ml.onnxruntime.directml.1.24.4.nupkg' '57E9F11B73437BEF7A309496135D4C1F96B1A8E9DDBA60013FA27BFC1D788681'
$dml = Fetch-Package 'directml-1.15.4' 'https://api.nuget.org/v3-flatcontainer/microsoft.ai.directml/1.15.4/microsoft.ai.directml.1.15.4.nupkg' '4E7CB7DDCE8CF837A7A75DC029209B520CA0101470FCDF275C1F49736A3615B9'
$runtime = Join-Path $AppDir 'runtime/rife'
$licenses = Join-Path $runtime 'licenses'
foreach($folder in @("$repo/deps/onnxruntime/include","$repo/deps/directml/include",$runtime,$licenses)){New-Item -ItemType Directory -Path $folder -Force | Out-Null}
Copy-Item "$ort/build/native/include/*" "$repo/deps/onnxruntime/include" -Force
Copy-Item "$dml/include/*" "$repo/deps/directml/include" -Force
Copy-Item "$ort/runtimes/win-x64/native/onnxruntime*.dll" $runtime -Force
Copy-Item "$dml/bin/x64-win/DirectML.dll" $runtime -Force
Copy-Item "$ort/LICENSE" "$licenses/ONNX-Runtime-LICENSE" -Force
Copy-Item "$ort/ThirdPartyNotices.txt" "$licenses/ONNX-Runtime-ThirdPartyNotices.txt" -Force
Copy-Item "$ort/Privacy.md" "$licenses/ONNX-Runtime-Privacy.md" -Force
Copy-Item "$dml/LICENSE.txt" "$licenses/DirectML-LICENSE.txt" -Force
Copy-Item "$dml/LICENSE-CODE.txt" "$licenses/DirectML-LICENSE-CODE.txt" -Force
Copy-Item "$dml/ThirdPartyNotices.txt" "$licenses/DirectML-ThirdPartyNotices.txt" -Force
Copy-Item "$repo/third_party/rife422/LICENSE" "$licenses/RIFE-LICENSE" -Force
Copy-Item "$repo/third_party/rife422/LICENSE-SmoovPlay" "$licenses/SmoovPlay-LICENSE" -Force
Copy-Item "$repo/third_party/rife422/NOTICE" "$licenses/RIFE-NOTICE" -Force
Write-Host "Native DirectML runtime installed in $runtime. Export/copy the three RIFE ONNX models as documented in docs/RIFE-DIRECTML.md."

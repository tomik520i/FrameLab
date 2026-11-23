# RIFE 4.22 lite / DirectML — preview.17

This backend shares FrameLab's capture, output scheduler, comparison, recording,
NR and subtitle paths. Select **Frame gen → RIFE 4.22 lite (DirectML)**. Optical
Flow and NVIDIA VFG remain separate choices. `--rife` selects RIFE on launch;
`--adapter N` selects the renderer's DXGI adapter, also used by DirectML by LUID.

RIFE requires a hardware DirectX 12 adapter and the files in `app/runtime/rife`.
No Python process or separately installed Python is required to run RIFE.
ONNX Runtime is dynamically loaded only when RIFE is selected. Missing files
disable its menu entry. CPU execution fallback is disabled; failure is reported.

Low / Medium / High use motion scales 0.5 / 1 / 2. Output size does not change.
2× requires one inference per input pair, 3× two and 4× three. A small model file
does not imply a small computation or VRAM footprint. Integrated GPUs can be far
too slow for real time. Choose Optical Flow when latency matters more.

## Runtime layout and reproduction

From a Windows PowerShell shell in the repository:

```powershell
./tools/setup_rife_directml.ps1 -AppDir ./app
python -m venv .rife-export
./.rife-export/Scripts/python.exe -m pip install torch==2.9.1 --index-url https://download.pytorch.org/whl/cpu
./.rife-export/Scripts/python.exe -m pip install onnx==1.19.1
Invoke-WebRequest https://github.com/HolyWu/vs-rife/releases/download/model/flownet_v4.22.lite.pkl -OutFile deps/rife-downloads/flownet_v4.22.lite.pkl
./.rife-export/Scripts/python.exe tools/export_rife_directml.py deps/rife-downloads/flownet_v4.22.lite.pkl app/runtime/rife
```

The setup script pins official NuGet archives and checks SHA-256 before extracting.
It installs build headers in `deps/onnxruntime/include`, `deps/directml/include`
and native DLLs/licenses under the specified application directory.
Use the existing FrameLab MSVC/CMake build instructions after this preparation.
If using `FRAMELAB_APP_DIR`, pass the same folder to the setup/export commands.

Required runtime files:

- `onnxruntime.dll`, `onnxruntime_providers_shared.dll` (1.24.4)
- `DirectML.dll` (1.15.4)
- `rife422-lite-low.onnx`, `rife422-lite-medium.onnx`, `rife422-lite-high.onnx`
- `model-provenance.json` and `licenses/`

The export checks the source checkpoint digest, strictly loads inference keys
and compares its optimized graph against the upstream inference implementation.
ONNX uses opset 17, FP16 convolutions and FP32 sampling grids. Each shape is
specialized when a session is created. Models are about 9.8 MB each.

## Tests and measurements

Build `rife_directml_test`, `framegen_compare`, `nr_hardware_test` with BUILD_TESTING.
They require a real Windows GPU; they are not registered as GPU-less CI tests.

```powershell
./build/rife_directml_test.exe 0 ./app/runtime
./build/rife_directml_test.exe 1 ./app/runtime
./build/nr_hardware_test.exe 0
./build/nr_hardware_test.exe 1
ffmpeg -i app/samples/gta.mp4 -an -pix_fmt bgra -f rawvideo gta.bgra
./build/framegen_compare.exe 1 2 0 1280 720 ./app/runtime gta.bgra rife-low
./build/framegen_compare.exe 1 1 1 1280 720 ./app/runtime gta.bgra optical-medium
```

Compare arguments are adapter, backend (1=Optical, 2=RIFE), quality (0/1/2),
width, height, runtime folder, raw input, output prefix. It warms up ten pairs,
then reconstructs every odd frame from neighboring even frames. The original odd
frames are withheld ground truth. In the bundled 30 FPS GTA clip this tests
15→30 FPS reconstruction, not a nonexistent 60 FPS reference.

CSV `wall_ms` covers CPU input copies, dispatch and completion/upload polling.
`readback_ms` is measured separately for the benchmark. `worker_ms` is RIFE wall
time including padding, inference and output transfer; **it is not GPU time**.
Optical Flow `gpu_ms` uses D3D11 GPU timestamps. Compare end-to-end wall time or
actual app FPS/latency across backends, not differently scoped internal timers.

## Implementation limits

One inference job is in flight per RIFE instance. Input pairs and temporal phases
are immutable until completion. Reset detaches the retiring worker safely; its
resources stay owned until inference or model compilation finishes. Loading does
not block the settings UI. Rapid repeated changes can temporarily overlap model
sessions until old initialization completes, so allow the model to finish loading.

CPU BGRA frames transfer through ONNX Runtime. GPU texture inputs currently read
back through a D3D11 staging texture before inference. This is not zero-copy.
Both feature encoders currently run per interpolation; cached features and D3D12
resource sharing remain possible optimizations. Static identical pairs and cuts
return the current original frame. Cut detection uses color histograms and sparse
differences; gradual transitions are not guaranteed to be classified correctly.

The NR economy mode runs **two** FG paths for exact reference/strength blending,
and can double RIFE model/session costs. Standard mode is simpler to benchmark.
Other apps, power state, thermals and Optimus affect laptop measurements.

## Enhancer hardware policy

FrameLab's post-process NR experiment is not official DLSS 5 game integration.
The original local NR 310.8 profile is enabled for RTX Blackwell SM120 only.
The distinct ShortFuse 310.9.16 community profile allows RTX SM75/86/89/120;
this is runtime eligibility, not certification of every card in those families.
Actual initialization may still fail with an incompatible driver/runtime.

AMD, Intel, GTX and unknown architectures cannot enable either NR profile.
The menu, F6, restored session and Enhancer constructor enforce the check. NVIDIA
VFG has its own RTX 40/50 eligibility; RIFE and Optical Flow do not require NVIDIA.
Do not redistribute private NR DLLs with a public release without resolving their
separate licenses. See existing third-party and NR notices.

Sources: [RIFE](https://github.com/HolyWu/vs-rife),
[SmoovPlay export optimizations](https://github.com/smoovplay/smoovplay),
[ONNX Runtime DirectML](https://onnxruntime.ai/docs/execution-providers/DirectML-ExecutionProvider.html),
[NVIDIA DLSS 5 hardware](https://www.nvidia.com/en-us/geforce/news/dlss-5-3d-guided-neural-rendering/).

# GPU support — preview.18

Frame generation, translation and enhancement have separate hardware requirements.
Support for RIFE on a GPU does not imply support for NR or GPU translation.

| Function | Hardware path | Verified here |
|---|---|---|
| D3D11 Optical Flow | Suitable Direct3D 11 hardware, no NVIDIA-specific runtime | Intel UHD + RTX 4050 |
| RIFE 4.22 lite / DirectML | Suitable hardware DirectX 12 GPU + driver; no CUDA requirement | Intel UHD + RTX 4050 |
| NVIDIA VFG | RTX 40/50 | RTX 4050; earlier RTX 5070 Ti evidence |
| NR original profile | Supported RTX 50 / Blackwell architecture and external runtime | Prior RTX 5070 Ti |
| NR ShortFuse profile | Allowed RTX architectures from 20/30/40/50 families; community experiment | RTX 4050; not certification of every card |
| OPUS-MT + OCR | CPU; EN → CS for OPUS | Bundled, local |
| TranslateGemma / CUNI | CUDA, Vulkan or CPU; sufficient available memory | RTX 4050 via CUDA/Vulkan, Intel UHD via Vulkan, CPU; AMD and larger Gemma models not tested |

AMD Radeon and older GeForce can use the portable FG paths if their hardware/driver meets the backend requirements.
No AMD card or older GeForce was physically benchmarked in this release. Intel UHD is tested, but RIFE is too slow on this particular iGPU at 720p.
NR is blocked on AMD, Intel, GTX and unknown architectures in the UI, shortcuts, restored sessions and initialization.
FrameLab is finished-image video interpolation, not native in-game DLSS 5 integration.

## Getting started

Open **Frame gen → Computational backend**. Choose Optical Flow for a low-power GPU,
or RIFE Low / 2× as a first RIFE test. The RIFE ONNX files and DirectML runtime are bundled.
Missing files or unsupported hardware disable RIFE with an explanation; there is no hidden CPU execution fallback.
Use **Start FrameLab - RIFE.cmd** for the RIFE demo. `--adapter N` selects the DXGI adapter for rendering and DirectML.
3× and 4× require two and three inference passes per pair; they can be much slower.

Low changes the motion computation, not the output resolution. The model may take time to compile on first load. Let loading finish before changing settings again.

Translation has its own device selector under **Subtitles → Local / offline → Translation compute**. See the [CUDA, Vulkan and CPU guide](local-translation-compute.en.md). Selecting Vulkan for translation does not enable the NR enhancer on unsupported GPUs.

## Measured 720p playback

Same GTA clip, ~30 FPS input, 2×, fullscreen, VSync; no translating dialogue during this isolation test.
Windows-confirmed presentations are not physical monitor scanout measurements.

| GPU + backend | Windows-confirmed presentations/s | App latency ms |
|---|---:|---:|
| Intel UHD / Optical Medium | 59.3 | 58.0 |
| Intel UHD / RIFE Low | 13.5 | 162.8 |
| Intel UHD / RIFE Medium | 9.9 | 214.4 |
| RTX 4050 / Optical Medium | 59.5 | 54.6 |
| RTX 4050 / RIFE Low | 59.2 | 59.8 |
| RTX 4050 / RIFE Low + NR 360p | 59.2 | 72.3 |
| RTX 4050 / RIFE Medium | 49.5 | 67.9 |

A separate withheld-frame test used 44 triples × 6 configurations (264 outputs), reconstructing 15→30 FPS.
RIFE Low RGB MAE was 6.69 versus Optical Medium 13.04 on this one clip. This is not “49% better-looking” or a universal ranking.
[Actual pixel comparison and methodology](https://opicebot.cz/framelab/runs/2026-11-22-rife/).
[Implementation, reproducible commands and limits](RIFE-DIRECTML.md).
[Official DirectML requirements](https://onnxruntime.ai/docs/execution-providers/DirectML-ExecutionProvider.html).

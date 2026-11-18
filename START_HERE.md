# Start here — no compilation required

[FrameLab website](https://opicebot.cz/framelab/) · [Český návod](docs/README.cs.md) · [Full documentation](README.md)

## 1. Download the ready-made app

[Download the complete FrameLab Windows ZIP](https://opicebot.cz/framelab/downloads/FrameLab-0.2.0-preview.9-windows-x64-full.zip) from the [FrameLab website](https://opicebot.cz/framelab/) ([GitHub mirror](https://github.com/tomik520i/FrameLab/releases/tag/v0.2.0-preview.9)). It contains the five NVIDIA VFG DLLs and a portable Python/Pillow/FFmpeg environment for clip export. No SDK installer, system Python, developer account, Git or compiler is needed. Avoid **Source code** if you just want to run the app.

Right-click the ZIP → **Extract all**. Keep the extracted files together in a writable folder. Windows x64, an NVIDIA GPU and a DirectShow-compatible capture source are required. Only RTX 5070 Ti + AVerMedia GC573 were hardware-tested here. Download GPU drivers from [NVIDIA](https://www.nvidia.com/Download/index.aspx) and the capture-card driver from its manufacturer ([AVerMedia downloads](https://www.avermedia.com/support/download)).

## 2. Choose your launcher and source

1. RTX 40/50: double-click **Start FrameLab - NVIDIA.cmd**. For the alternative algorithm, use **Start FrameLab - Optical Flow.cmd**. Both enable generation and start at 2× / Medium where applicable. The NVIDIA route uses the model shown in the examples; Optical Flow has different quality.
2. Under **Source**, select the capture card connected to your device. Start with **Resolution → Automatic (device)** or 1920×1080. Select the audio **Output**.
3. Click **Start capture**. Use **F11** for fullscreen; **Escape** returns. The **Test 30 FPS** button tests the graphics path without a capture signal.

The complete bundle needs no extra AI or export-tool download. NVIDIA VFG supports Ada/Blackwell (RTX 40/50), not only an individual 5070 Ti; only that GPU was tested here. Optical Flow is intended for older RTX too, but RTX 20/30 have not been tested on physical hardware. English is the first-run language; Czech is available in the language menu.

## Manual NVIDIA runtime setup for older minimal packages

**Skip this section for the complete preview.9 bundle: the DLLs are already included.** Earlier minimal packages omitted them. For an existing installation, the [FrameLab NVIDIA runtime add-on](https://opicebot.cz/framelab/downloads/FrameLab-NVIDIA-VFG-1.3-addon.zip) can be extracted and its **app** folder merged into FrameLab's **app** folder; keep the included licenses. The final DLL location is **app/runtime**. The official SDK route below remains available as an alternative. NVIDIA files retain their own terms, separate from GPL; see [component provenance](docs/runtime-distribution.md).

1. Open the [SDK Core download](https://catalog.ngc.nvidia.com/orgs/nvidia/maxine/resources/vfx_sdk_core/-) and choose **1.3.0.0_windows** for Windows x64 (not the `woa` ARM package). Follow NVIDIA's [installation guide](https://docs.nvidia.com/maxine/vfx/latest/WindowsVFXSDK/InstalltheVFXSDK.html) to add the **Video Frame Generation** feature from its [SDK collection](https://catalog.ngc.nvidia.com/orgs/nvidia/teams/maxine/collections/maxine_vfx_sdk). The Core alone does not contain the feature. NVIDIA may require an account/API key and access to the download; keep credentials out of screenshots and bug reports.
2. From that matching installation, copy these five files into FrameLab's **app/runtime** folder (create it if absent): `cudart64_12.dll`, `NVCVImage.dll`, `nvngxruntime.dll`, `NVVideoEffects.dll`, `nvVFXVideoFrameGeneration.dll`. Keep the NVIDIA license files with your local SDK. FrameLab was tested with runtime 1.3; later versions are not validated here.
3. Double-click **Start FrameLab.cmd**, choose **Backend → NVIDIA VFG (RTX 40/50)**, enable **Frame generation**, leave **Medium / 2×**, and click **Start capture**.

If all five DLLs are already in one folder, the optional import helper copies them for you:

```powershell
.\tools\import-runtime.ps1 -RuntimeDirectory 'C:\path\to\your\runtime'
```

NVIDIA documents the supported architectures and input requirements in its [VFG reference](https://docs.nvidia.com/maxine/vfx/latest/Filters/VideoFrameGeneration.html). The complete bundle uses verified unmodified files from the official NVIDIA-hosted wheel; FrameLab performs no silent runtime download.

## Common questions

| What happens? | What to do |
| --- | --- |
| No capture image | Select the correct Source; close other apps using the card; verify the card's own preview and cable/source signal. Try 1080p and Automatic FPS. |
| Missing NVIDIA DLL / VFX error | Complete the NVIDIA setup above, or launch the included Optical Flow shortcut. |
| Lots of artifacts | Use 2×. Compare native preview with generation disabled. Fast motion, foreground occlusion and HUDs remain difficult. |
| 60 capture FPS but about 40 content FPS | The capture contains repeated frames. Interpolation follows changing image content. |
| Want Czech? | Select **EN / CZ → Čeština**. English is the first-run default. |
| Want the same measured demo? | Download the separate **FrameLab-82183953-comparison-example.zip** release asset, extract it, open **index.html**. |

## Optional: 15-second diagnostic clips

**The complete preview.9 bundle includes diagnostic clip export.** Keep `tools/python`, `deps/python` and `tools/export_analysis.py` in place. The application prefers its isolated embedded interpreter without installing it into Windows or changing PATH. Older minimal packages can install [Python 3.11 or newer](https://www.python.org/downloads/windows/) and run `tools/setup-diagnostics.ps1`. Live preview/audio do not need Python. The recorder first buffers full-resolution pixels in RAM, so 1440p/4K can need substantial memory and disk space and recording can reduce performance. It records no audio. See [recording details](docs/recordings.md).

## Interface in preview.9

Use the right-hand tabs: **Capture** for source/resolution/input FPS, **Frame gen** for backend/quality/2×–4×, **Audio** for output and volume, **Recording** for 15-second analysis clips, and **Stats** for graphs and Windows display confirmations. The **EN / CZ** button switches language. Input/output counters stay above the tabs. F11 toggles the image-only fullscreen view; Escape leaves it.

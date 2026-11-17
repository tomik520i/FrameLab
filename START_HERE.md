# Start here — no compilation required

[Český návod](docs/README.cs.md) · [Full documentation](README.md)

## 1. Download the ready-made app

[Download FrameLab for Windows](https://github.com/tomik520i/FrameLab/releases/download/v0.2.0-preview.6/FrameLab-0.2.0-preview.6-windows-x64.zip) from the [GitHub release](https://github.com/tomik520i/FrameLab/releases/tag/v0.2.0-preview.6). Avoid **Source code** if you just want to run the app.

Right-click the ZIP → **Extract all**. Keep the extracted files together in a writable folder. Windows x64, an NVIDIA GPU and a DirectShow-compatible capture source are required. Only RTX 5070 Ti + AVerMedia GC573 were hardware-tested here. Download GPU drivers from [NVIDIA](https://www.nvidia.com/Download/index.aspx) and the capture-card driver from its manufacturer ([AVerMedia downloads](https://www.avermedia.com/support/download)).

## 2. Fastest start: included Optical Flow

1. Double-click **Start FrameLab - Optical Flow.cmd**. This selects Optical Flow and enables 2× frame generation.
2. Under **Source**, select the capture card connected to your device. Start with **Resolution → Automatic (device)** or 1920×1080. Select the audio **Output**.
3. Click **Start capture**. Use **F11** for fullscreen; **Escape** returns. The **Test 30 FPS** button tests the graphics path without a capture signal.

This path needs no NVIDIA AI download, Python, Git or compiler. RTX 20/30 support is intended but not tested on physical hardware. Optical Flow is experimental and is not the NVIDIA VFG model used in the gameplay comparison.

## NVIDIA VFG — RTX 40/50

The public ZIP does not bundle proprietary NVIDIA runtime files. This separate setup is needed only for the NVIDIA backend.

1. Open the [SDK Core download](https://catalog.ngc.nvidia.com/orgs/nvidia/maxine/resources/vfx_sdk_core/-) and choose **1.3.0.0_windows** for Windows x64 (not the `woa` ARM package). Follow NVIDIA's [installation guide](https://docs.nvidia.com/maxine/vfx/latest/WindowsVFXSDK/InstalltheVFXSDK.html) to add the **Video Frame Generation** feature from its [SDK collection](https://catalog.ngc.nvidia.com/orgs/nvidia/teams/maxine/collections/maxine_vfx_sdk). The Core alone does not contain the feature. NVIDIA may require an account/API key and access to the download; keep credentials out of screenshots and bug reports.
2. From that matching installation, copy these five files into FrameLab's **app/runtime** folder (create it if absent): `cudart64_12.dll`, `NVCVImage.dll`, `nvngxruntime.dll`, `NVVideoEffects.dll`, `nvVFXVideoFrameGeneration.dll`. Keep the NVIDIA license files with your local SDK. FrameLab was tested with runtime 1.3; later versions are not validated here.
3. Double-click **Start FrameLab.cmd**, choose **Backend → NVIDIA VFG (RTX 40/50)**, enable **Frame generation**, leave **Medium / 2×**, and click **Start capture**.

If all five DLLs are already in one folder, the optional import helper copies them for you:

```powershell
.\tools\import-runtime.ps1 -RuntimeDirectory 'C:\path\to\your\runtime'
```

NVIDIA documents the supported architectures and input requirements in its [VFG reference](https://docs.nvidia.com/maxine/vfx/latest/Filters/VideoFrameGeneration.html). The separately licensed setup is not an automatic download performed by FrameLab.

## Common questions

| What happens? | What to do |
| --- | --- |
| No capture image | Select the correct Source; close other apps using the card; verify the card's own preview and cable/source signal. Try 1080p and Automatic FPS. |
| Missing NVIDIA DLL / VFX error | Complete the NVIDIA setup above, or launch the included Optical Flow shortcut. |
| Lots of artifacts | Use 2×. Compare native preview with generation disabled. Fast motion, foreground occlusion and HUDs remain difficult. |
| 60 capture FPS but about 40 content FPS | The capture contains repeated frames. Interpolation follows changing image content. |
| Want Czech? | Select **Language / Jazyk → Čeština**. English is the first-run default. |
| Want the same measured demo? | Download the separate **FrameLab-82183953-comparison-example.zip** release asset, extract it, open **index.html**. |

## Optional: 15-second diagnostic clips

Live preview/audio do not need Python. To export diagnostic video clips, install [Python 3.11 or newer](https://www.python.org/downloads/windows/) and run `tools/setup-diagnostics.ps1` from the extracted folder. It installs the pinned Python libraries and FFmpeg helper locally. The recorder first buffers full-resolution pixels in RAM, so 1440p/4K can need substantial memory and disk space. It records no audio. See [recording details](docs/recordings.md).

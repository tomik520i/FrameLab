# FrameLab 0.2.0-preview.8 — extract and run

**[Visit the FrameLab website](https://opicebot.cz/framelab/)** for downloads, interactive 2×/3×/combat/4× comparisons, GIFs, native image crops and setup instructions in English/Czech.

## Complete portable app

Download **FrameLab-0.2.0-preview.8-windows-x64-full.zip**, extract everything, and launch:

- **Start FrameLab - NVIDIA.cmd** for RTX 40/50, NVIDIA VFG, Medium / 2×.
- **Start FrameLab - Optical Flow.cmd** for the alternative experimental algorithm.

The ZIP contains the **five original NVIDIA VFG DLLs**, their app-local Microsoft Visual C++ runtime dependencies, and an isolated Python/Pillow/FFmpeg clip exporter. No NVIDIA SDK installer, developer account, Visual C++ installer, system Python, Git or compilation is needed. Compatible GPU and capture-device drivers are still required. The complete bundle's 15-second diagnostic clips export without another dependency installation.

The runtime comes directly from NVIDIA's official public `nvidia-vfx` wheel. Its archive and all five DLL hashes are verified, and valid NVIDIA signatures are checked while packaging. NVIDIA licenses/notices remain included and separate from the GPL application. The Windows VFG runtime supports Ada/Blackwell (RTX 40/50), rather than one individual 5070 Ti. **Only RTX 5070 Ti + AVerMedia GC573 have been tested on actual hardware.** Older RTX users can try the different Optical Flow algorithm; RTX 20/30 hardware remains untested here.

## Other files

- **FrameLab-NVIDIA-VFG-1.3-addon.zip** — optional NVIDIA component for existing FrameLab installations. Merge its `app` directory into FrameLab's `app` directory and retain `licenses`. Unnecessary with the full ZIP.
- **FrameLab-0.2.0-preview.8-source.zip** — exact matching application and website source.
- **FrameLab-0.2.0-preview.8-provenance.json** — source tree and application/package hashes.
- **SHA256SUMS-preview.8.txt** — downloadable asset hashes.

The [website](https://opicebot.cz/framelab/#evidence) hosts the existing comparison videos and complete example ZIPs. Earlier [preview.7](https://github.com/tomik520i/FrameLab/releases/tag/v0.2.0-preview.7) and [preview.6](https://github.com/tomik520i/FrameLab/releases/tag/v0.2.0-preview.6) assets remain available.

## What changed

The application now selects its embedded clip-export Python when available and falls back to system Python for older minimal packages. Added a direct NVIDIA launcher and reproducible complete packaging. **No frame-generation algorithm, quality or scheduler change** is claimed in this version; existing evidence still describes its original settings and recorded build.

The new website defaults to Czech for Czech/Slovak IP ranges, English otherwise. Manual language selection is remembered and always takes priority. Country lookup happens locally on the server for IPv4/IPv6; it sends no visitor IP to a third-party service. VPNs can affect the result; unknown/unavailable country falls back to English.

3×/4× remain experimental and can produce strong artifacts. Start with 2×. Latency measurements exclude capture hardware and panel scanout. The application is unsigned; it does not modify antivirus settings or install drivers. See [component provenance and licenses](https://github.com/tomik520i/FrameLab/blob/main/docs/runtime-distribution.md).

[Optional support through Opicebot](https://opicebot.cz/?public=prdelniopice&donation2=prdelniopice): CZK via PayPal. The donation page is in Czech only.

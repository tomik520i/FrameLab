# FrameLab 0.2.0-preview.6

Experimental Windows capture-device interpolation with NVIDIA VFG and an included D3D11 Optical Flow backend. This release includes **the ready-to-run application, matching source, setup guides, measured examples and visible failure cases**. English UI is the default; Czech is available in the language menu.

## Download and start

- **FrameLab-0.2.0-preview.6-windows-x64.zip** — extract all files, double-click **Start FrameLab - Optical Flow.cmd**, select source/resolution/audio and click **Start capture**. This experimental fallback needs no separately downloaded AI runtime, compiler or Python.
- **FrameLab-0.2.0-preview.6-source.zip** — corresponding GPL-3.0 source.
- **FrameLab-82183953-comparison-example.zip** — measured 2× gameplay: slowed videos, matched original/generated frames, native crops and settings.
- **FrameLab-84744531-comparison-example.zip** — 4× failure cases: doubled character outlines, warped backgrounds, all three generated phases, slowed videos, settings and OS confirmations.
- Two standalone **source-aligned MP4s** — three seconds of action slowed to twelve. Source-aligned reconstruction removes scheduling delay; the example ZIPs also include movies retaining actual submission timestamps.
- **SHA256SUMS-preview.6.txt** — checksums for the downloadable assets.

NVIDIA VFG requires a separately licensed compatible NVIDIA runtime. The [Start Here guide](https://github.com/tomik520i/FrameLab/blob/main/START_HERE.md) provides official links and steps. NVIDIA runtime/model files are not bundled. Optical Flow has different quality from the NVIDIA gameplay examples. Start with 2×; 3×/4× remain experimental.

## What the recordings demonstrate

The earlier 1440p 2× take has 40 changing source images/s and 80 submitted image IDs/s. The new 4× diagnostic take has 39.73 changing source images/s, 87.87 submitted IDs/s and 87.80 Windows-confirmed IDs/s, with severe visible artifacts and 53 exact endpoint copies among 980 comparable generated images. The 4× recorder completed all 15 seconds with 800 RAW and 988 generated frames, zero recorder drops. Recording overhead can reduce throughput.

These are different gameplay runs, not a controlled same-input multiplier benchmark. Pixel difference is not accuracy. Windows-confirmed IDs are sampled OS evidence, not physical panel scanout, and copied pixels may have new IDs.

All eight example MP4s were decoded fully without errors. Slow-motion CSV maps resolve each of the 720 video frames to recorded image IDs. Full-size images, settings/IDs and failure cases remain inspectable.

## Normal live performance, without recording/export

On Ryzen 9 5900X / RTX 5070 Ti / 64 GiB RAM / AVerMedia GC573, at 1440p, NVIDIA VFG Medium, VSync off and approximately 40 changing source images/s:

| 30-second moving-gameplay sample | 2× | 4× |
| --- | ---: | ---: |
| Whole-GPU utilization | 43.5% | 50.0% |
| GPU power | 79.1 W | 96.3 W |
| Windows-confirmed image IDs/s | 79.88 | 111.03 |

GPU figures include other running desktop applications. One sequential run per mode, different motion paths; no idle subtraction or controlled benchmark claim. See [full test system, methodology and CSV data](https://github.com/tomik520i/FrameLab/blob/main/docs/test-system.md).

The application binary is the same as the locally validated preview.5: bounded lossless RAM recording, explicit start errors, fullscreen/resize fixes, independent resolution/FPS selection, audio output, and conservative display-confirmation counters. Tests passed for pacing, capture modes, lossless buffers and display confirmations; core and Optical Flow verification passed in the portable package. Only RTX 5070 Ti + GC573 have been hardware-tested here.

## Support

[Support through Opicebot](https://opicebot.cz/?public=prdelniopice&donation2=prdelniopice) — optional **CZK donations via PayPal**. **The donation page is available in Czech only.**

# FrameLab 0.2.0-preview.7

The output loop now services finished GPU work independently of the display wait and avoids redundant redraws. This release adds **Balanced 3x / Fast 4x presets**, recorded-input quality/latency experiments, **1080p and 1440p combat 3× analyses**, and a slowed original-versus-3× GIF beneath the existing 2× GIF.

## Download and run

- **FrameLab-0.2.0-preview.7-windows-x64.zip** — ready-to-run Windows app. Extract everything and double-click **Start FrameLab - Optical Flow.cmd** for the experimental backend requiring no separately downloaded AI model, compiler or Python.
- **FrameLab-0.2.0-preview.7-source.zip** — matching GPL-3.0 source.
- **FrameLab-112947656-comparison-example.zip** — complete new 3× analysis: four checked videos, full-resolution images, both interpolation phases, native HUD/character crops, original settings and frame-ID maps. Open **index.html**.
- **analysis-15s-112947656_source-aligned_3s_4x-slow.mp4** — three seconds of original-versus-3× movement slowed to twelve; source-aligned, processing delay removed.
- **FrameLab-113768562-comparison-example.zip** — separate 1440p / VSync-on 3× combat analysis, four checked videos, full-resolution frames, native kick/reticle/HUD crops, settings, maps and checksums.
- **analysis-15s-113768562_source-aligned_3s_4x-slow.mp4** — three seconds of close combat slowed to twelve, original versus 3×; source-aligned, processing delay removed.
- **FrameLab-vfg-tuning-2026-11-18.zip** — Low/Medium/High comparison video, native images, settings, hashes and experiment report. Open **index.html**.
- **4x-quality-3seconds-4x-slow.mp4** — the same recorded A/B pairs processed at three quality settings; not a live-FPS measurement.
- **SHA256SUMS-preview.7.txt** — hashes of every downloadable asset.
- **FrameLab-0.2.0-preview.7-provenance.json** — exact source tree and application/package hashes.

NVIDIA VFG requires its separately licensed compatible runtime. Use the official download links and steps in [Start Here](https://github.com/tomik520i/FrameLab/blob/main/START_HERE.md). NVIDIA runtime/model DLLs remain excluded. Optical Flow is a different algorithm and does not match the quality of the NVIDIA examples. English is the first-run UI language; Czech is available in the language menu.

## Output changes and measured scope

- Keep advancing interpolation phases while presentation waits, instead of tying all GPU completion checks to a redraw.
- Redraw unchanged video only as needed for 60-Hz controls; retain bounded queues and overload handling.
- **Balanced 3x** selects Medium; **Fast 4x** selects Low. Both keep automatic cut detection enabled. Default Medium 2× is preserved.
- Bind NVIDIA SDK input images once per pair and invalidate them after uploads. All 430 compared outputs were RGB-identical before/after this binding correction; it is not an artifact fix.
- Add `--capture-device "Exact friendly name"` for explicit source selection when launching.
- Store pacing version 3 and scheduler mode in new recording metadata. Export native crops correctly at 1080p as well as 1440p.

On repeated **identical 1440p recorded input**, RTX 5070 Ti, VSync off, Medium 4× improved from **121.1 to 126.7 submitted IDs/s**, callback-to-Present latency from **43.7 to 42.4 ms** (three clean runs per condition). Low 4× reached **146.7 IDs/s and 34.6 ms** in one run, with lower average detail accuracy on the held-out CS:GO sample. With VSync on, two additional Medium 4× runs improved from **90–97 to ~126 IDs/s** and **50–53 to ~44 ms**. A separate 2× check remained effectively unchanged.

These are application/OS image-ID counters, not camera-verified panel FPS. Copies can get fresh IDs. Latency excludes source rendering, capture-card buffering and scanout. GPU readings include other desktop software. See [full methodology, quality sweeps and CSVs](https://github.com/tomik520i/FrameLab/blob/main/docs/vfg-tuning.md).

## New 3× recording: useful frames and clear failures

The 15-second **1080p** take contains 900 RAW and 1106 generated images, zero recorder drops and no observed phase-ordering anomalies. Captured content changes **40 times/s**, while **110.8 IDs/s** were submitted and observed by Windows. There are **105.6 known RGB image changes/s**. Of 1102 comparable generated images, 74 exactly repeat an endpoint (45 A / 29 B); four more have endpoints outside the recording window.

The native crops show strong ghosting through leaves, smeared limbs and warped HUD ticks. More frames do not imply accurate images. Older 2×/4× examples use **different 1440p gameplay and an older scheduler**; they remain available in [preview.6](https://github.com/tomik520i/FrameLab/releases/tag/v0.2.0-preview.6) and are not a controlled comparison to this take. [New analysis in English/Czech](https://github.com/tomik520i/FrameLab/tree/main/examples/112947656).

The new README GIF contains 240 frames over six seconds, retains every output ID in its selected source-aligned interval, and performs no additional interpolation. Its small palette/resolution hides detail, so use PNGs and MP4s to inspect artifacts. Actual submission-timing movies remain in the example ZIP.

## Validation and limitations

The second new take covers **1440p combat with VSync on**: 829 RAW / 906 generated images, zero recorder drops, **39.8 input changes/s → 90.6 submitted/Windows-observed IDs/s**, with **89.87 known submitted RGB changes/s**. Eight of 902 comparable generated images exactly copy an endpoint. The kick gains ghost legs and a fast lunge distorts the aiming reticle. Mean RAW callback-to-submission latency is **64.16 ms**. No phase-ordering anomalies were observed; the final saved generated frame has no submission inside the recording window. Resolution, VSync and movement differ from the 1080p take, so their rates do not isolate any one cause. [Combat analysis in English/Czech](https://github.com/tomik520i/FrameLab/tree/main/examples/113768562).

Release build and all five CTests passed, along with capture-core verification, real NVIDIA VFG and experimental Optical Flow phase checks for 2×/3×/4×. The recorder test preserved every byte of 900 RAW + 2700 synthetic generated images with zero losses. Both new examples' eight MP4s were fully decoded, and slow-motion frame maps were checked against the original recordings. Portable-package smoke checks use the included Optical Flow route without NVIDIA runtime files.

Only RTX 5070 Ti + AVerMedia GC573 have been exercised on actual capture hardware. This release does not newly validate RTX 20/30/40, other cards, 4K performance, HDR or physical end-to-end latency. 3×/4× remain experimental; start with 2×. The application is unsigned and does not change antivirus/security settings.

## Support

[Support through Opicebot](https://opicebot.cz/?public=prdelniopice&donation2=prdelniopice) — optional **CZK donations via PayPal**. **The donation page is available in Czech only.**

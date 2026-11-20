# GTA recordings and debug — 20 November 2026

[Interactive comparisons](https://opicebot.cz/framelab/) · [All clips, selected PNGs and settings ZIP](https://opicebot.cz/framelab/downloads/FrameLab-preview15-comparisons.zip)

User-provided GTA gameplay, AVerMedia GC573, 2560 × 1440 delivered image, requested capture 60 FPS, NVIDIA VFG 2× Medium. This is not an official GTA VI benchmark. Original recordings were preserved. No replacement interpolation, enhancement or translation was performed during export.

## Takes

| Recording ID | Purpose | Recorded / complete paired prefix | Notes |
|---|---|---|---|
| analysis-15s-112499968 | Camera / original vs 2× | 15.00 s | Old RAW/FG format, before NR; no final subtitle pixels |
| analysis-15s-114426140 | Enhancer 01 | 15.00 / 15.00 s | 607 complete output rows |
| analysis-15s-114773125 | Enhancer 02 | 12.32 / 12.28 s | Bounded RAM/free-space stop; one missing tail row |
| analysis-15s-114955078 | Captions 01 | 6.71 / 6.61 s | Readback timeout; three displayed caption cues |
| analysis-15s-114991406 | Captions 02 | 3.81 / 3.70 s | Readback timeout; three cues |
| analysis-15s-115015921 | Captions 03 | 3.57 / 3.46 s | Readback timeout; two cues |

Each recording folder contains the original settings/result and audit. `manifest.json` lists media hashes. `*-mapping.json` maps each exported picture to the original pixel references and timing. A missing reference ends the paired video at that point; no missing frames or captions are invented. MP4 previews are high-quality lossy encodes and may be scaled. `*-full.png` and native crops contain actual selected pixels; layout images have labeled/resized panels.

## Camera and generated-frame evidence

The selected camera movement is seconds **1–4** of 112499968. Original changed pictures are held until their next source time. Recorded generated pictures are placed at their recorded interpolation fraction between A/B. Both sides share source time; processing latency is deliberately removed from this visual comparison. Three seconds become twelve at 15 FPS for the four-times-slower video/GIF. No new optical flow is run by the exporter.

Full take: 822 RAW callbacks, 440 adjacent RGB changes, 438 generated images. **437 complete A/G/B pairs, zero generated endpoint copies**; one pair has an endpoint outside the take. There were 845 Present events and **781 changes of submitted image ID**, or **52.07/s**, compared with 29.33 changing RAW/s and 29.20 generated/s. This does not prove stable 60 FPS scanout. Hash differences confirm distinct images, not correct geometry. Inspect occlusions, edges and HUD for artifacts.

## Before/after enhancement

For 114426140 and 114773125: NR 1440p limit, one pass, 100% blend, motion estimation on, FG 2×. `before` can already contain FG. `after` is the same output after NR, and `final` includes the overlay. Compare **matching output IDs**, never adjacent moments from separately recorded runs. Mean pixel changes quantify effect strength, not perceptual accuracy. NR can alter texture, edges or HUD.

Each take has before/after/final and paired MP4s, selected full-resolution PNGs, a native crop and explicit mapping. All playable data is limited to the complete paired prefix shown above. Large original lossless archives remain local; the download is a comparison package, not a copy of every original archive byte.

## Czech captions

RapidOCR, sampled white appearance, CUNI-MH Q5_K_M. The final videos show the Czech text actually composited by the application, including recognition/translation errors, delay and hold time. The English source remains visible where the user's settings left it. SRT contains the displayed Czech text and corresponding video timing; mappings retain English source text. Two/three cues per short take do not constitute a translation-quality benchmark.

## Debug session and hardware

12:44:27.884–13:05:08.457 UTC (20m40.57s); 1,217 numeric samples, zero dropped log rows. Original log has 75,315 frame rows. Public files retain requested gameplay text but remove local paths and unrelated identifiers. Mixed activity includes menus, setup, model warmup, gameplay, recording and export. GPU/CPU totals include other applications.

Ryzen 9 5900X (12c/24t), 64 GB RAM, RTX 5070 Ti (16,303 MiB reported), driver 617.14, Windows 11 Pro build 26200, AVerMedia GC573, 2560 × 1440 / 165 Hz display.

- 1,445 completed OCR operations: median **332.83 ms**, P95 **417.02 ms**; one OCR error. Superseded input is counted separately.
- 90 displayed translations: capture-to-caption median **957.98 ms**, P95 **2,509.54 ms**. Request processing includes cache hits; do not treat its median as uncached model inference.
- 69,936 NR completions: worker median **5.83 ms**, P95 **8.51 ms**; inference GPU median **4.68 ms**. Ready-to-take waiting is separate and can grow during stalls.

These overlapping stages must not be added. See `debug/summary.json`, `metrics.csv`, `captions.json`, settings timeline and the website graph. Recording intervals are marked; export intervals are not exhaustively labeled.

## Focus / fullscreen check

Original 1440p RAW seconds 1–4 replayed with recorded arrival timing, FG 2× Medium, NR 1440p/one pass/100%/motion, VSync off, subtitles off. Window client 1464 × 861 versus 2560 × 1440 fullscreen. OBS and other desktop software remained open. Short consecutive checks, not randomized research; startup/transitions excluded.

| Baseline phase | Submitted FPS mean | Samples |
|---|---:|---:|
| Foreground | 53.81 | 14 |
| Background, visible | 53.23 | 59 |
| Behind maximized Explorer | 52.93 | 20 |
| Fullscreen | 54.82 | 50 |
| Background repeat | 53.46 | 14 |

Opt-in process QoS variant: foreground 54.03, covered 53.59, fullscreen 54.77. No material benefit; default unchanged. Severe focus loss was not reproduced. This does not exclude capture/OCR-specific issues, a driver background limiter, minimized-window suspension or competing load. Data: `focus/summary.json` and the two metric CSVs. No Windows/NVIDIA global settings were changed.

## Reproduce

Use `tools/study_publication.py`, `make_publication_media.py`, `analyze_public_debug.py`, `analyze_focus.py` and `prepare_public_evidence.py` against your own preserved takes. Set `FRAMELAB_PIXELS_DLL` to the built `app/FrameLabPixels.dll` when the recorder DLL is not in the repository's app folder. Python needs NumPy/Pillow; encodes use FFmpeg. Review privacy before publication.

Game content retains its owners' rights; these short project-owner captures illustrate processing behavior. No GPL license is claimed over game assets.

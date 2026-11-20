# Current preview.15 evidence

[GTA camera, enhancer, Czech captions, RTX 5070 Ti telemetry and focus checks](../examples/2026-11-20/README.md) · [Interactive website](https://opicebot.cz/framelab/). This new set supersedes the older front-page samples; the historical measurements below retain their original dates and conditions.

# Test systems and live GPU load

[FrameLab](../README.md) · [Česky](test-system.cs.md) · [Raw measurements](performance/2026-11-18/summary.json)

## Desktop CUNI translation coexistence — 19 November 2026

The same RTX 5070 Ti desktop was retested with **VFG 2× Medium + experimental NR
65% / 720p + the existing CPU OCR pipeline** playing a 1080p30 CS:GO loop. A
separate local CUNI worker translated exact corpus text. There was no capture-card
input and no game rendering on the GPU in this experiment.

| Measurement | Q3_K_M | Q8_0 |
|---|---:|---:|
| Translation median | 212 ms | 360 ms |
| Translation p95 | 479 ms | 855 ms |
| Requests above 2 s | 0 / 180 | 0 / 180 |
| Output during translation | 56.27 fps | 56.21 fps |
| Own unloaded controls | 57.39 fps | 57.34 fps |
| Output change | -1.95% | -1.96% |
| Whole GPU, mean | 28.2% | 35.8% |
| Power, mean | 102.9 W | 113.2 W |
| Additional resident memory | 3.81 GiB | 7.62 GiB |
| Minimum free memory | 8.63 GiB | 4.88 GiB |

Each quantization completed 300 exact-text translations and three cycles of 60
requests, at least 2 seconds apart. Each cycle is compared with its own adjacent
60-second unloaded controls. Phase statistics omit the first 5 and last 2 seconds;
latency includes all 180 requests. NVML samples the whole GPU at 1 Hz. Software
output counts are **not physical monitor FPS**; inference times exclude OCR and
stabilization. Memory excludes NVML reserved memory.

Both models scored 271/300 acceptable translations; major errors: Q3 2, Q8 5.
These are assistant judgments, not an independent blinded human study. All 600
outputs match the separately reviewed preflight after trimming surrounding whitespace.
The controlled benchmark uses the unchanged preview.12 baseline; separate actual
OCR/CUNI tests verify preview.13 integration. This does not measure its end-to-end
live subtitle delay.

[Interactive report, all pairs, raw phases/settings and reproduction scripts](https://opicebot.cz/framelab/translation/)
· [Aggregate JSON](performance/2026-11-19-cuni/summary.json)

## Additional laptop testing — preview.12, 19 November 2026

**Acer Nitro ANV15-51 · Intel Core i5-13420H (8 cores / 12 threads) · NVIDIA GeForce RTX 4050 Laptop GPU · 32 GB RAM · Windows 11 Pro, build 26200.** Hardware was read from Windows on the test machine.

The laptop was used to verify bundled video playback, NVIDIA VFG, the local experimental NR enhancer, automatic clean fullscreen, comparison restoration and Czech subtitle rendering at 1920×1080. A separate local translation evaluation exercised the shipped model on 300 English game texts and the production Windows OCR pipeline on 300 rendered subtitle fixtures, including 720p, 1080p, 1440p and 4K input images. The high-resolution fixtures are input-processing tests, not a claim of physical 4K-display testing. Translation remains experimental: omitted sentences and gaming terminology errors were observed.

These tests use local videos and fixtures. They do **not** establish PS5/capture-card latency or repeat the desktop GPU-load benchmark on the laptop. The measurements below belong only to the RTX 5070 Ti desktop.

## Desktop live-capture benchmark — 18 November 2026

**Measured during normal live frame generation, without analysis recording or video export.** The user moved through the game while each mode was sampled for approximately 30 seconds on 18 November 2026. These results describe this machine and these runs, not a hardware compatibility guarantee.

## Hardware and setup

| Component | Tested configuration |
| --- | --- |
| CPU | AMD Ryzen 9 5900X, 12 cores / 24 threads |
| GPU | NVIDIA GeForce RTX 5070 Ti, 16 GB class; nvidia-smi reports 16,303 MiB framebuffer |
| GPU driver | 617.14 |
| RAM | 64 GiB installed: 4×16 GiB Kingston KHX3600C17D4/16GX; configured speed reported as 3600 |
| Motherboard | Gigabyte X570 AORUS PRO |
| System / recording disk | Samsung SSD 980 1 TB, NVMe |
| OS | Windows 11 Pro, build 26200 (10.0.26200) |
| Capture device used | AVerMedia HD Capture GC573 1 |
| Other connected capture device | AVerMedia GC550; not used for these measurements |
| Source | PS5 gameplay, user-controlled motion; capture delivers 2560×1440 at approximately 60 callbacks/s |
| Changing captured content | Approximately 40 image changes/s; this is not engine telemetry |
| Active application output | Windows mode 2560×1440 at 165 Hz |
| Connected displays | Samsung Odyssey G5 and HP Z24i G2 |
| Backend / quality | NVIDIA VFG (Maxine), Medium; not DLSS or the Optical Flow fallback |
| Settings | 2× and 4× as labelled; VSync off; exact dedup on; automatic cut detection on; comparison panel on |
| Background conditions | Normal desktop applications and Avast remained running; no security settings changed |

## Normal live operation: measured averages

| Measurement | 2× | 4× |
| --- | ---: | ---: |
| Whole-GPU utilization | **43.5%** (41–46%) | **50.0%** (46–53%) |
| Whole-GPU memory used | 3983 MiB / 3.89 GiB | 4059 MiB / 3.96 GiB |
| Whole-GPU power | **79.1 W** (78.7–79.6 W) | **96.3 W** (95.4–97.6 W) |
| GPU temperature | 53.3°C (53–54°C) | 54.3°C (53–55°C) |
| Graphics clock | 2872 MHz | 2872 MHz |
| Capture callbacks/s | 60.00 | 59.99 |
| Changing captured content/s | 39.97 | 39.95 |
| Submitted image IDs/s | **79.88** | **111.03** |
| Windows-confirmed image IDs/s | **79.88** (77.99–81.00) | **111.03** (108.82–112.86) |
| Confirmed generated IDs/s | 39.94 | 83.27 |
| Sampled VFG + GPU output-copy time | 3.31 ms | 3.06 ms |
| Callback → Present latency, mean of interval means | 39.06 ms | 49.46 ms |

The 4× run occurred first, followed by 2× after switching and a short settling period. There are **29 nvidia-smi samples and 31 application telemetry samples per mode**. Ranges are observed sample minima/maxima, not confidence intervals. Application counters use approximately one-second bins, so the boundary may overlap the sampling window by one bin.

**GPU percentages, memory, power and temperature are whole-device readings, not exclusive FrameLab cost.** Other desktop programs were left open. No idle baseline was subtracted. The scenes/camera paths differ between runs, and this is one short run per mode. The earlier measurement that included a pause menu is excluded from this table.

The GPU work time is the **last sampled individual generated-frame operation plus output copy**, read from CUDA events. It is neither the sum of all three 4× intermediates nor total application frame time. A 3 ms value does not imply 333 FPS. These measurements do not identify a CPU bottleneck or prove that all apparently unused GPU capacity can be converted into more FPS.

The Windows counter matches observed DXGI display confirmations to image IDs. It is conservative OS evidence, **not a physical panel/camera measurement**. Different IDs can contain identical pixels. Latency starts at the capture callback and excludes source rendering, capture-card buffering and display scanout.

## Keep diagnostic recording results separate

The [15-second 4× recording](../examples/84744531/README.md) achieved 87.87 submitted IDs/s, below the ~111/s live run above. Full-resolution readback and lossless RAM compression add work. Do not quote the recording as the speed of normal operation, or the live run as the exact timing of that recording. The 4× recording also shows severe visible artifacts despite its extra frames.

## Data and reproduction

- [2× settings and timing](performance/2026-11-18/2x/measurement.json), [GPU samples](performance/2026-11-18/2x/nvidia-smi.csv), [application telemetry](performance/2026-11-18/2x/app-telemetry.csv).
- [4× settings and timing](performance/2026-11-18/4x/measurement.json), [GPU samples](performance/2026-11-18/4x/nvidia-smi.csv), [application telemetry](performance/2026-11-18/4x/app-telemetry.csv).
- [Machine-readable aggregates](performance/2026-11-18/summary.json), [test-system inventory](performance/2026-11-18/test-system.json).

Set and visually verify the mode, stop diagnostic recording/export, allow settings to settle, then run `tools/sample_live_gpu.ps1` with the current session CSV and a new output directory. `tools/summarize_live_gpu.py` produces the published aggregate JSON from the saved samples. The measurements contain no device serial numbers, account credentials or raw desktop logs.

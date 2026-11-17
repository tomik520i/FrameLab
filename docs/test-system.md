# Test system and live GPU load

[FrameLab](../README.md) · [Česky](test-system.cs.md) · [Raw measurements](performance/2026-11-18/summary.json)

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

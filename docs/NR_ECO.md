# Optional Eco enhancer

In **AI / NR**, below **Enable enhancer**, select **Eco: enhance before frame gen (experimental)**. Czech: **Úsporně: enhancer před frame genem (experiment)**. The checkbox is saved. Existing configurations keep Standard mode until the user opts in.

Standard processes `input → FG → NR → subtitles → display`. Eco processes `input → NR → FG → subtitles → display`. NR therefore runs once per accepted source frame, rather than once per output frame. At 30 unique input frames and 2× FG, its target work falls from 60 to 30 evaluations per second. This is not a promise of 50% lower total GPU utilization.

Resolution, NR passes, effect strength, audio and subtitle settings are preserved. All output phases contain enhancement; the implementation does not alternate enhanced and unenhanced images or reuse stale enhancement on a moving image. The intermediate RGBA output is converted to BGRA on the GPU and transferred directly to the FG backend without CPU readback.

A separate unenhanced FG reference preserves the original/split views, strength blending, and matching before/after recordings. It costs extra FG work and VRAM. In Eco, before/after compares `FG(raw)` with `FG(NR(raw))` at the same source IDs and temporal position, rather than applying NR directly to that generated reference. FG may move details differently between those paths. Eco can add latency and change temporal artifacts; it is experimental. With FG disabled, it does not reduce the number of NR evaluations.

The panel shows **AI evaluations / s**. Debug settings include `nr_economy`; metrics include `nr_evaluations_fps`. Recordings retain the mode in `pipeline-settings.json` and `nr-settings.txt` (format 3; formats 1 and 2 still load). Changing processing order resets the pipeline and ends an active analysis take, just like changing FG settings.

## Local verification, 2026-11-20 study

RTX 5070 Ti, synthetic 2560×1440 motion, 30 unique frames in a 60 Hz input, NVIDIA VFG Low 2×, NR 1440p / one pass / motion enabled / full strength. Four 24-second runs in Standard / Eco / Eco / Standard order; first eight seconds excluded. The same binary and polling implementation were used in both modes.

| Measurement | Standard | Eco |
|---|---:|---:|
| Submitted content FPS | 53.99 | 58.71 |
| NR evaluations / second | 53.99 | 29.34 |
| App latency, mean | 56.96 ms | 59.60 ms |
| Mean of per-second latency P95 | 79.04 ms | 79.16 ms |
| Whole GPU utilization | 59.5% | 50.2% |
| Whole GPU power | 148.5 W | 123.1 W |
| Whole GPU allocated VRAM | 11,371 MiB | 11,793 MiB |

These are hidden-window application measurements, not physical monitor FPS or controller-to-photon latency. Capture hardware, audio and monitor latency are excluded. The user's production FrameLab and other processes remained running, so GPU totals include them. The short test does not establish long-session stability or a universal quality/performance improvement. In this test Eco reduced whole-GPU power by about 17%, added 2.64 ms mean application latency and used about 421 MiB more VRAM.

Earlier hidden-window trials were limited by coarse `Sleep(1)` wakeups. GPU polling now uses a high-resolution waitable timer, with a Sleep fallback; it does not spin or change system-wide GPU power settings. See [Microsoft's timer documentation](https://learn.microsoft.com/windows/win32/api/synchapi/nf-synchapi-createwaitabletimerexw) and [Windows 11 occluded-window timing](https://learn.microsoft.com/windows/win32/api/timeapi/nf-timeapi-timebeginperiod). The earlier trials are retained separately and are not mixed into the table.

Other checks: all 17 registered tests passed; NR settings migration and existing split/strength/fullscreen pixel tests passed; GPU RGBA/BGRA conversion was pixel-exact; CPU and GPU texture inputs produced identical FG outputs across both parities and all 4× phases for NVIDIA VFG and the D3D11 fallback. A local 720p gameplay recording had 293 complete paired outputs with enhancement present on source and generated frames. The intentionally interrupted recording had one incomplete final readback; its original index is retained and it is not counted as a complete 15-second take. Selected original/generated before/after images were inspected for alignment and channel errors. This is not a perceptual quality score or proof that temporal artifacts are absent.

Reproduce with a separate app folder containing the user's own NR runtime:

```powershell
python tools/benchmark_nr_economy.py H:/isolated/FrameLab/app analysis-output/nr-eco-new-run
build-nr-economy/Release/gpu_texture_input_test.exe H:/isolated/FrameLab/app/runtime
```

Local evidence: `analysis-output/nr-economy-20261120/summary.json`, `precise-synthetic-abba/results.json`, `recording-check/verification.json` and the referenced debug folders. The scripts never capture the desktop or alter the production app's settings. The underlying NR runtime remains the existing community experiment, not an official in-game DLSS integration.

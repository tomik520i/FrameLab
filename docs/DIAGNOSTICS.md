# Debug and measurement

Open **Debug & measurement** (Czech: **Debug a měření**). Choose the detail level and press **Start debug recording**, or use **F9**. F9 also stops recording, including in fullscreen.

The default duration is **Until stopped**. Recording continues until Stop/F9, application exit, a disk write failure, or less than 256 MiB of free disk space. Optional timed sessions are 30 seconds, 60 seconds, 3 minutes and 15 minutes. A visible warning explains that OCR text and translations can contain private information. Disable the text checkbox for numeric measurements only. No screenshots, video or audio are recorded, and nothing is uploaded automatically.

The panel displays the destination. Each recording creates `app/logs/debug-<session-id>/`:

| File | Contents |
|---|---|
| `session.json` | Session ID, UTC start, PC/GPU, initial capture/FG/NR/subtitle settings, measurement definitions |
| `metrics.csv` | One-second samples: FPS, application latency, GPU load/power/temperature/VRAM, total and app CPU/RAM, queues/drops, NR stages, OCR and translation timings/counters |
| `frames.csv` | Optional per-Present timeline: original/generated ID, interpolation pair/phase, content interval, Present duration and callback-to-Present delay |
| `events.jsonl` | NR submit/completion, history resets, OCR job IDs/results, translation/cache results, caption display, drops/errors and changed settings |
| `summary.json` | End timestamp, counts, means, medians, P95, maxima and discarded diagnostic rows |

Every row includes an ISO 8601 UTC timestamp plus monotonic seconds from the recording start. Files rotate at approximately 64 MiB (`frames-1.csv`, `events-1.jsonl`, etc.) without deleting earlier files. Writing happens on a worker thread with a bounded queue. A slow disk may drop log rows; the panel and summary report this instead of blocking video. Summary means/counts/maxima include all accepted samples; percentiles are exact up to 4096 values and use a bounded uniform reservoir thereafter. Summaries include model warmup; exclude that interval when comparing steady-state performance.

CPU/GPU totals include other applications. App CPU/RAM excludes OCR/translation helper processes; system totals and individual stage timings provide context. NVML values of `-1` mean unavailable. NR worker time includes GPU waits; GPU preparation/inference/finish use timestamp queries. OCR/translation times include IPC. Caption display delay also includes reading holds and queues.

Submitted FPS and Windows-confirmed frame IDs do **not** measure physical monitor scanout. App latency excludes capture-card hardware, input-device delay, audio latency and monitor scanout. Debugging adds some overhead; compare identical runs with the same debug settings.

## Enhancer performance

The enhancer computes one frame ahead of presentation while retaining a single in-flight job. Frame generation multiplies the number of frames it must enhance. Lowering AI resolution or pass count reduces GPU work; lowering effect strength only changes the blend and does not reduce neural inference cost. The 720p/1080p one-pass presets are under **AI / NR → Quality and performance**. They preserve the output dimensions, but can change the visual detail of the effect.

For reproducible local testing, `--debug-seconds 60` records numeric/frame data from startup, and `--debug-text` also includes OCR/translation content. `tools/benchmark_nr_scheduling.py` compares the previous presentation-bound scheduler with the new one on a synthetic 1440p source. This is a pacing benchmark, not a visual-quality evaluation.

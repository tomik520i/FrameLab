# Debug & measurement — preview.16

Open the settings tab using the tab arrows. **Start** begins a new timestamped folder under `app/logs/debug-*`; **F9** toggles logging. The visible warning explains that data continues to accumulate until stopped. `--resume` restores the saved logging state. Stop before closing if the next session should start with logging off.

The logger writes session information, periodic numeric metrics, per-frame events, settings changes, source/caption timing and OCR/translation text. Optional detailed traces can contain game dialogue or other visible text: review before sharing. Files stay local; logging is not automatic upload. Detailed logging and recording/export have overhead.

## Read the counters correctly

- Input FPS: received callbacks, potentially repeated or delivered in bursts.
- Content FPS: estimated changing input; not game-engine FPS.
- Output/generated FPS: distinct images submitted by the application.
- Present/DXGI counters: application/OS presentation information, not a camera measuring panel scanout.
- App latency: captured arrival to submission; excludes game, controller, capture hardware and monitor delay.
- GPU utilization/power/VRAM: the whole GPU, including other applications. CPU totals likewise are not exclusively FrameLab.
- OCR/translation/NR timers: different pipeline stages that can overlap. Do not add their medians to estimate total latency.

## Foreground and fullscreen diagnostics

Preview.15 adds `window_foreground`, `window_fullscreen`, client dimensions, `background_qos`, `presentation_wait_ms`, `loop_gap_max_ms`, `present_cpu_total_ms` and cumulative `occluded_presents`. Periodic metrics continue even while waiting for the next scheduled image. Minimized rendering is intentionally suspended.

The opt-in developer flag `--background-qos` asks Windows to stop process-local execution/timer power throttling. It is off by default: the controlled check found no material benefit. No global NVIDIA/Windows setting is modified. [Microsoft timer behavior](https://learn.microsoft.com/en-us/windows/win32/api/timeapi/nf-timeapi-timebeginperiod) and [process power policy API](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setprocessinformation).

## Published session

[November 20 evidence](../examples/2026-11-20/README.md) includes 1,217 metric rows, 75,315 frame rows in the original private log, 90 caption pairs and zero dropped log rows. The public subset removes local paths/identifiers and retains requested game text. The session includes menus, setup, gameplay, recording and export. It is descriptive evidence, not a controlled game-performance benchmark. [Interactive graph](https://opicebot.cz/framelab/#debug).

## Preview.16 additions

F10 toggles an optional saved fullscreen HUD in the upper-right corner: confirmed/submitted output FPS, app latency mean and P95. This does not measure hardware latency or unique panel scanout; the HUD is excluded from recorded evidence. Normal startup restores the saved session.

Subtitle lifecycle events and rotating worker/runtime stderr logs help diagnose failures; debug text includes OCR source and Czech output. A bounded 2/5/15-second retry handles worker load/crash failures and resubmits an unchanged caption. Disable cancels retry.

[Complete three-hour audit](https://opicebot.cz/framelab/runs/2026-11-20-3h/): all 1,747 jobs assessed, 1,584 distinct input/output pairs, 163 exact repetitions, zero unreviewed. This later session does not replace the separately labeled original 20-minute comparison. Detailed notes disclose translation errors and missing OCR reference images.

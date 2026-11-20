# Preview.16 — recovery, Eco enhancer and illustrated guides

Windows portable prerelease. Includes the existing features plus the local fixes and documentation below.

- The EN/CZ illustrated walkthrough returns with 18 new screenshots from the actual app, numbered steps, arrows and click-to-enlarge. It covers input/video, FG, NR/Eco, OCR selection/inspector, profiles, model downloads, subtitle timing/placement, audio, display, recording, graphs, logging and the fullscreen HUD. It is included offline in the full ZIP.
- Optional [Eco enhancement](NR_ECO.md) processes source frames before FG. It reduces NR evaluations when FG is active, at the cost of extra reference VRAM and potentially different latency/artifacts. Existing settings remain in Standard mode. GPU polling uses a high-resolution waitable timer instead of relying on coarse hidden-window Sleep wakeups.
- Subtitle workers recover automatically after load failures or crashes, with bounded 2/5/15-second retries. Unchanged OCR text is resubmitted after recovery; disabling subtitles cancels retries.
- GPU translation stays loaded while enabled and unloads explicitly on disable. Runtime request timeouts finish before the parent IPC timeout. Worker process handles are isolated and diagnostic stderr logs rotate instead of silently replacing earlier failures.
- The enhancer presentation queue preserves enough images for the selected FG multiplier and discards obsolete work by age. Ordinary queue trimming no longer resets temporal history. This reduces avoidable scheduling loss; it does not guarantee stable 60 fps on every workload.
- Normal startup restores the saved session. F10 toggles a saved, small fullscreen FPS/app-latency overlay; it is not burned into analysis recordings.
- The OCR appearance inspector exposes detected text, size/color matching and filter decisions. Existing color/size profiles, audio/subtitle-track selection and recording features remain available.
- The complete three-hour debug audit assesses all 1,747 translation jobs (1,584 unique pairs), including failures. The original frozen 300-pair sample remains separately inspectable. Prompt experiments remain separate from the production prompt pending evidence of a safe improvement.

The full package includes the existing portable OCR, CPU translation, LibVLC, frame-generation dependencies and samples. Optional external NR model/runtime and GPU translation models retain the existing setup/download workflow. They are not silently added to the ZIP. Source archive and SHA-256 checksums accompany the portable package.

The single-run audit still shows stutters. App latency excludes capture hardware and physical display latency. GPU figures include other applications. Translation grades are a disclosed single-AI review, not human-validated accuracy.

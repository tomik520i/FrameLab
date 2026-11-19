# FrameLab 0.2.0-preview.13

English subtitles in the picture can now use optional **CUNI-MH Q3_K_M or Q8_0**
translation on the GPU. The included CPU translator remains the default. Missing
English Windows OCR can be installed from **Subtitles** with one explicit click
and administrator approval; the Windows display language stays unchanged.

## Models and responsiveness

- Download, pause and resume from the app. Pinned SHA-256 checks prevent partial
  or corrupt models from activating. The first GPU model also downloads its own
  portable runtime; no account, API key or system Ollama installation is needed.
- Current free GPU memory informs the recommendation. Loading and warmup happen
  in the background; partial CPU offload is rejected rather than silently slowing
  down the selected GPU mode. Disabling translation unloads owned model processes.
- OCR and translation have separate background workers with a bounded latest-job
  queue and a 128-line cache. Changed/blank source text clears old captions;
  late results from previous lines or a source reset cannot reappear.

## Caption placement

- Drag Czech text and its black background together without moving the OCR area.
  Horizontal/vertical anchors, Above/Below original and gap controls are saved.
- Choose opposite-side fallback or clamping when a caption would cross an edge.
  Bounds follow the **visible viewport**, including Fill, crop and 1:1 views.
  Multiline text stays inside; unusually tall captions shrink to fit.
- Fullscreen keeps the final caption and hides all editing handles and OCR outlines.

## Local video playback and NR passes

- **Capture → Open video…** selects a local movie with its native timestamps and audio. Pause, seek, loop and replay are available; Audio selects the Windows endpoint. Both streams share a Media Foundation clock. FG/NR still add video delay. SDR H.264/AAC and silent MP4 were checked; other codecs depend on Windows. Only the first audio/video tracks are used; no DRM/HDR or SRT/embedded subtitle rendering.
- **AI / NR → NR passes** chooses 1–4 sequential passes, default 1. Each layer has independent temporal state and processes the preceding output. Strength blends once at the end. The displayed computation time covers all passes. More passes can amplify artifacts and lower output FPS; this is separate from the FG multiplier.
- GPU readback tests of two/four passes matched independent repeated single-pass processing exactly in the tested fixtures. This verifies chaining, not better image quality. Original NVIDIA NR 310.8 and ShortFuse 310.9.16 were checked on RTX 5070 Ti.

## Download and guides

The complete Windows x64 ZIP starts in **English**; Czech is available in the
language switch. It includes VFG, the CPU translator, export tools, gameplay
samples and updated EN/CZ illustrated/offline guides. GPU weights are optional
publisher downloads, not bundled. External NR model DLLs retain their separate
download/import step and terms.

[Subtitle setup](https://opicebot.cz/framelab/guide/subtitles-en.html) ·
[Český návod](https://opicebot.cz/framelab/guide/subtitles.html) ·
[Q3/Q8 desktop results and all 300 pairs](https://opicebot.cz/framelab/translation/)

This is still an experimental preview. OCR can miss small or transient text;
translation can be wrong and adds delay. Frame-generation recordings remain
diagnostics of RAW/generated stages and omit the subtitle overlay and NR.

Validated on RTX 5070 Ti: 10 CTest checks, actual Windows OCR with all three
translators, delayed-result rejection, asynchronous Q8 load/unload, GPU overlay
bounds and new-app CUNI + VFG + NR output. Both EN/CZ websites and all ten guides
passed mobile/desktop checks. [Validation details](desktop-validation-2026-11-19.md).

In the controlled concurrent benchmark, Q3's median translation request was
212 ms versus 360 ms for Q8 (about 41% less time), with no clear quality gain for
Q8 on the 300-case corpus. These times exclude OCR and stabilization.

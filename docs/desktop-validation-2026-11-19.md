# Desktop development validation — 2026-11-19

**Development checkpoint, not a public release or completed coexistence benchmark.**
Source baseline: preview.12, commit `5105548f07ff0ed3c3937e7121aa57fe0833c02d`.
New development binary identifies itself as preview.13-dev.

## System

Ryzen 9 5900X (12 cores / 24 threads), 64 GB RAM, Gigabyte X570 AORUS PRO,
Windows 11 Pro build 26200, RTX 5070 Ti (16 GB), NVIDIA driver 617.14.
Application smoke tests used bundled video playback, not a capture card or a game
rendering on this GPU. No desktop screen capture was used.

## Completed checks

- Release build and all 10 CTest checks pass.
- Five offline asset tests pass: interrupted/resumed download, hash rejection,
  archive traversal rejection, repair retaining the previous runtime, and readiness
  invalidation when model/runtime files change or disappear.
- Native overlay test uses actual ImGui mouse events to drag the Czech overlay
  without changing the OCR crop. D3D pixel checks confirm clean fullscreen and
  no editing interaction in fullscreen. Layout tests include resize, aspect ratios,
  edge fallback, multiline bounds, settings persistence and migration.
- Native model lifecycle test loads Q8 through the portable worker, cancels a load,
  reloads and unloads asynchronously. The updated physical-VRAM guard is enabled.
- Direct inference produces Czech from both Q3_K_M and Q8_0 with complete GPU
  offload. Runtime-reported allocations are approximately 3.59 GiB and 7.40 GiB.
  These functional smoke timings are not performance results.
- Actual NR GPU pixel test at 720p passes on this 5070 Ti, including correct
  input/output pairing, single in-flight work and no duplicate publication.
- In a separate 25-second hidden application preview, Q8 loaded and warmed while
  VFG 2× Medium and NR 65% / 720p continued processing 1080p video. Whole-GPU memory
  peaked near 10.93 GiB. Q8 was idle after warmup, and English OCR was unavailable;
  this demonstrates simultaneous loading, not live translation performance.
- Fresh Q3 and Q8 text-only runs each completed all 300 frozen cases. Both scored
  271/300 acceptable; major errors were 2 for Q3 and 5 for Q8. There were 175 exact
  output matches. All pairs were reviewed by the assistant, not a blinded human
  panel. The offline bilingual report passed search/filter/mobile checks.

## Required before publication

English Windows OCR is absent on this desktop. The real OCR pipeline test reports
that missing capability; it has **not passed**. Installing the Windows component
requires administrator elevation and is awaiting the user's UAC handoff.

After installation, verify actual image → OCR → CPU/CUNI → Czech overlay, including
replacement/blank input while translation is in flight. Then complete the original
coexistence protocol for **both CUNI quantizations on this same desktop**: 300 texts
and 3×60 paced requests, paired unloaded controls, current application logs and NVML,
VFG 2× Medium + NR 65% / 720p + existing CPU OCR active. Preserve notebook results.

Only after these checks should release notes, public examples, the website and
download packages be finalized and published. GPU model weights remain optional
verified publisher downloads. The private handoff and external NR model DLLs must
not be packaged as public release assets.

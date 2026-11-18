# 0.2.0-preview.7 — recorded-input 3× / 4× tuning

- Service completed GPU work independently of the display wait and avoid redrawing unchanged content; keep controls responsive at 60 Hz and preserve bounded queues.
- Add Balanced 3x (Medium) and Fast 4x (Low) shortcuts, preserving automatic cut protection and the default Medium 2× mode.
- Correct NVIDIA SDK binding to once per input pair; track uploads even when descriptor addresses are unchanged. Identical-pixel A/B benchmarks show no artifact fix from this correction.
- Add headless pixel/latency sweeps, original-recording replay, reproducible quality crops and slow-motion exports. Document 28 offline runs and repeated real application timing; keep model artifacts and physical-display limitations explicit.
- Add the completed 1080p 3× recording, both generated phases, visible failures, traceable 6×-slow README GIF, and full comparison assets.
- Add a separate 1440p / VSync-on 3× combat analysis, slowed clips, native kick/reticle/HUD crops, original settings and complete frame-ID evidence.
- Allow explicit capture-device selection on launch with `--capture-device "Exact friendly name"`.
- Make diagnostic native-pixel crops resolution-aware for 1080p as well as 1440p.
- Save output scheduling mode and pacing version 3 in new analysis-session metadata.

# 0.2.0-preview.6

- Publish English/Czech setup guides, ready-to-run packages and measured 2×/4× examples on GitHub.
- Add the completed 4× gameplay take, three-phase artifact crops, slow-motion comparisons, Windows confirmations and explicit limitations.
- Document the test system and normal live GPU measurements without recording/export: approximately 43.5% GPU / 79.88 confirmed IDs/s at 2× and 50.0% / 111.03 IDs/s at 4× on the RTX 5070 Ti.
- Generalize comparison labels and blend references for recorded 2×/3×/4× phases. Application executable is unchanged from the locally validated preview.5.

## Application features

- Record 2×/3×/4× frames through bounded background lossless RAM compression; preserve every BGRA byte, validate memory/disk before creating a take, and keep start failures visible.
- Resolve sampled DXGI display confirmations to actual raw/generated image IDs. Show a conservative confirmed rate, last confirmed ID and current monitor mode; never treat gaps in DXGI present IDs as displayed frames.
- Include measured gameplay videos/crops, English and Czech quick-start guides, an Optical Flow launcher and a Czech-only CZK/PayPal support link.
- Continue rendering during interactive move/resize; disable swapchain stretching to keep text and source aspect ratio intact.

- Separate resolution and FPS controls, driver frame-interval ranges, post-negotiation FPS reporting and aspect-ratio labels.

- Generic DirectShow source / resolution / frame-rate selection, including driver-advertised 4K.
- NVIDIA VFG plus experimental D3D11 optical-flow backend.
- Bounded presentation queue with independent pacing clock and late-frame rebasing.
- English UI and persistent Czech language selection.
- Backend and pacing version in recording metadata; source/package/release-example preparation.
- Existing gameplay evidence remains explicitly labeled pre-fix.

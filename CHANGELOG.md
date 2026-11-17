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

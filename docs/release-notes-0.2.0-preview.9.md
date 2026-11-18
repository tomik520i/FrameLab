# FrameLab 0.2.0-preview.9 — one look, edge-to-edge fullscreen

The desktop application now shares the [FrameLab website](https://opicebot.cz/framelab/)'s charcoal/green palette, lime accents, Space Grotesk headings and IBM Plex Mono labels. Fonts are embedded in the executable and work offline, including Czech accents.

- Removed the unintended **1 px ImGui border** and the associated fullscreen edge clipping. Aspect-ratio letterboxing is still preserved where needed.
- Added **Capture / Frame gen / Audio / Recording / Stats** tabs. Input, content, output and generated FPS remain visible above the controls.
- Added clear 2× / 3× / 4× selection buttons and a stronger primary-action style.
- Improved comparison controls and image sizing in narrower windows.
- Updated the English/Czech guides and website application preview. Existing recorded comparison videos, GIFs and analyses remain unchanged.

**[Download the complete Windows x64 ZIP](https://opicebot.cz/framelab/downloads/FrameLab-0.2.0-preview.9-windows-x64-full.zip)**, extract everything and run **Start FrameLab - NVIDIA.cmd** or **Start FrameLab - Optical Flow.cmd**. NVIDIA DLLs, their Microsoft runtime dependencies and the portable clip exporter remain included. GPU and capture-device drivers are required. NVIDIA VFG supports RTX 40/50; only RTX 5070 Ti was hardware-tested here.

This release changes presentation and layout, not the frame-generation algorithm, pacing or capture format negotiation. 3×/4× remain experimental. Software FPS observations are not physical monitor scanout measurements.

Validation: Release build and five existing CTest checks; hidden rendering of each UI tab, English/Czech layouts and a smaller window; fullscreen render-target edge inspection; packaged core and both interpolation backend checks. UI preview images use replayed RAW gameplay and are not new performance benchmarks. No desktop screen capture is used.

Assets include the full app, matching source, provenance, validation and SHA-256 checksums. Original font and runtime notices accompany the ZIP. [Optional support via Opicebot](https://opicebot.cz/?public=prdelniopice&donation2=prdelniopice): CZK via PayPal, donation page in Czech only.

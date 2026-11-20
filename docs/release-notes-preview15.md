# FrameLab 0.2.0-preview.15

- Subtitle word/color/size selection, a larger multiline reading area and persistent named appearance profiles. The primary picker button is visible near the top of Subtitles.
- Bundled LibVLC 3.0.24 for MKV and other local video, with audio/subtitle track selection. Embedded subtitle pixels enter before FG/NR; translation still uses OCR.
- Corrected double end-of-source token in the bundled OPUS CPU translation worker. Q5 remains an optional selected model; no universal quality gain is claimed.
- Lossless compressed recording of input/FG, synchronized before/after NR and final captions. SRT, frame IDs, settings and playable previews. Destination selection and free-space feedback.
- Completed GPU readbacks survive delayed CPU polling. Still-busy transfers time out after two seconds. Partial takes export their complete prefix, with explicit incomplete metadata.
- Saved runtime sessions, current desktop launcher, centered/bounded Czech overlay and a bundled GTA source demo.
- Additional foreground/fullscreen, presentation wait and loop-stall diagnostics. A severe focus-related performance loss was not reproduced in the controlled replay; default scheduling remains unchanged.
- New GTA camera/NR/Czech examples, native crops, actual application screenshots, refreshed English/Czech guides, website and sanitized debug graph.

## Validation

Passed actual D3D recording with a 180 ms delayed poll, 900 RAW + 2,700 generated compressed-frame round-trip, final caption pixel/SRT checks, complete and missing-tail export regressions, diagnostics, subtitle state/layout/profiles, pacing and display-policy checks. Existing focused media-track/OCR/style/EOS tests document the feature work; see the build guide for repeatable commands.

The regression proves that a ready GPU copy is no longer rejected solely because its CPU poll was late. It does not guarantee 15 seconds on every workload. Recording can stop at bounded RAM/disk capacity. Published old shortened takes are not repaired or padded. MP4/GIF are lossy previews; selected native PNGs and frame mappings retain the evidence.

## Packaging and limitations

Full Windows ZIP includes VFG, OCR, CPU translation, LibVLC, export tools and offline guides. Optional CUNI weights/runtime download in-app. External NR DLLs are not redistributed; the import wizard and exact upstream links remain available. No private settings/models/logs enter the ZIP. Analysis clips are silent. Submitted/DXGI FPS is not physical monitor FPS. The small vision model for automatic subtitle-region detection remains a TODO.

# FrameLab 0.2.0-preview.5

Experimental Windows capture-device frame interpolation with NVIDIA VFG and an included D3D11 Optical Flow backend. This preview separates capture resolution/FPS and keeps the live image and UI correctly proportioned during window resizing.

## Recording and display verification

- Full-resolution 2×/3×/4× recording uses bounded background lossless RAM compression. Checks run before a take folder is created; errors stay visible. Disk output remains byte-identical BGRA. A synthetic 64×36 recorder test verified 900 RAW + 2700 generated frames with zero drops; a separate test round-tripped 60 actual 1440p frames byte-for-byte. A new 15-second live 4× gameplay take has not yet been recorded with this fix. Compression may affect timing, and incompressible data can still fill the budget.
- Windows display confirmations are matched to the app's raw/generated image IDs, with the last confirmed ID and active monitor mode shown. The rate is a sampled lower bound. Repeated IDs are not recounted; pixel-identical SDK fallbacks can still carry new IDs. Unavailable evidence is labelled unavailable. This is not a physical camera measurement.
- New takes include `display-confirmations.csv`; the 82183953 example predates this feature.

## Download

- **FrameLab-0.2.0-preview.5-windows-x64.zip**: ready-to-run app. Extract all files and launch **Start FrameLab - Optical Flow.cmd** for the route requiring no extra AI runtime. Start with 2×.
- **FrameLab-0.2.0-preview.5-source.zip**: corresponding GPL-3.0 source.
- **FrameLab-82183953-comparison-example.zip**: visual evidence and measurements. Extract it and open **index.html**.

English is the default application language. Select **Language / Jazyk → Čeština** for Czech. README links to a Czech guide. The package includes **START_HERE.md** with setup steps and official driver/runtime links. NVIDIA VFG requires the separately licensed compatible NVIDIA runtime; the Optical Flow launcher does not.

## Measured gameplay example

The 15-second 1440p / NVIDIA VFG Medium 2× take contains 600 changing input images, 599 generated images and 1200 submitted image-ID changes. Of 597 comparable generated images, 587 differ from both endpoints and 10 exactly copy A. Mean image-ID spacing is 12.50 ms, p95 15.79 ms. These are application submissions, not measured physical monitor FPS. Image difference is not interpolation accuracy.

The example includes a 3-second action segment at 4× slow motion (12-second movies), a source-aligned version and an actual submission-timing version, full-resolution PNGs, native-pixel character/HUD crops, an explicit crossfade reference, original settings and per-frame IDs. Failure cases are retained. All four MP4s were fully decoded without errors and both slow-motion maps resolve all 720 output frames to recorded image IDs.

Only RTX 5070 Ti and AVerMedia GC573 were hardware-tested. Other RTX generations and driver-reported capture modes remain unverified. Optical Flow quality differs from the NVIDIA examples. 3×/4× remain experimental. Diagnostic recording needs substantial RAM/disk and excludes audio.

## Support

[Support development through Opicebot](https://opicebot.cz/?public=prdelniopice&donation2=prdelniopice) — optional **CZK donations via PayPal**. **The donation page is in Czech only.**

# FrameLab 0.2.0-preview.10 — offline Czech subtitles and complete demos

**[Download the complete Windows x64 ZIP](https://github.com/tomik520i/FrameLab/releases/download/v0.2.0-preview.10/FrameLab-0.2.0-preview.10-windows-x64-full.zip)** · **[Český návod](https://github.com/tomik520i/FrameLab/blob/v0.2.0-preview.10/INSTALL.cs.md)** · **[English installation](https://github.com/tomik520i/FrameLab/blob/v0.2.0-preview.10/START_HERE.md)**

## Instalace / installation

1. Stáhni **windows-x64-full.zip** / Download **windows-x64-full.zip**.
2. Rozbal **celý ZIP** / **Extract all** into a writable folder.
3. Otevři / Open **1 - Spustit FrameLab.cmd**. Přibalené CS:GO se spustí hned / The bundled CS:GO demo starts immediately.

Python, pip, model ani SDK neinstaluješ. / No system Python, pip, model or SDK install is required. Windows English OCR may need a one-time language-feature installation; the model and its interpreter are included. Compatible Windows x64 GPU/capture drivers remain required. Automatic GitHub **Source code** archives are for developers, not for running the application.

## Included

- Offline OCR: English subtitles already in the picture → Czech overlay, adjustable region/font/position/background. CPU OPUS-MT model and interpreter bundled. **No speech recognition, cloud or API key.**
- Both gameplay demos bundled: CS:GO 1080p60 and Spider-Man 540p60, three seconds each, plus a labeled artificial OCR test.
- Monitor/window controls, fit/fill/1:1, existing driver-reported capture resolution/FPS selection.
- Dark native title bar on Windows 11, retaining native window behavior; supported styling fallback on older Windows.
- Official NVIDIA VFG runtime for RTX 40/50, Microsoft runtime files, export tools, original notices, provenance and matching source.
- Optional NR enhancer source and real [before/after gallery](https://opicebot.cz/framelab/#enhancer). **The public executable and ZIP exclude NR and its DLLs.** Redistribution rights for those original/ShortFuse NR binaries are not established; see the [audit](https://github.com/tomik520i/FrameLab/blob/v0.2.0-preview.10/docs/NR-LICENSING-STATUS.txt).

## Validation and limits

Seven tests pass. Real Windows OCR + bundled EN-CS inference, clearing/reset/disable, NVIDIA VFG 2×/3× + OCR and Optical Flow 4× + OCR were tested on RTX 4050 Laptop. A fresh full-ZIP extraction ran with system Python removed from PATH. Short synthetic OCR checks are not a dialogue-quality or all-hardware guarantee. Capture-card/high-resolution NR paths were not retested on this laptop. No new NR inference was run after the license audit. Processing adds latency and may drop frames; 3×/4× can produce artifacts.

The **full ZIP** contains app/components/models/samples; the **source ZIP** contains the matching application source and pinned ImGui source, not proprietary runtimes. SHA256SUMS.txt and release-provenance.json identify the files. Existing releases and comparison evidence remain available. Game content and proprietary components retain their own rights; GPL does not relicense them.

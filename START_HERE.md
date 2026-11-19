# FrameLab preview.14 — download, install and run

**[Download the complete Windows x64 ZIP](https://github.com/tomik520i/FrameLab/releases/download/v0.2.0-preview.14/FrameLab-0.2.0-preview.14-windows-x64-full.zip)** · [Release files](https://github.com/tomik520i/FrameLab/releases/tag/v0.2.0-preview.14) · **[Česky: celý návod](INSTALL.cs.md)**

## 1. Extract and launch

1. Download **FrameLab-0.2.0-preview.14-windows-x64-full.zip**. The automatic GitHub **Source code** files are for developers.
2. Right-click the ZIP → **Extract all** into a writable folder, for example your Desktop. Do not run inside the ZIP or copy only the EXE.
3. Open **1 - Spustit FrameLab.cmd**. A bundled CS:GO clip loops immediately.
4. The app starts in English; use **EN / CZ** for Czech. Under **Capture**, try Spider-Man or the artificial **EN > CS subtitle demo**. Both real gameplay clips are included, three seconds each.

No SDK, system Python, pip, model download, Git, developer account or compiler is required. Keep `app`, `tools`, `deps` and `licenses` together. The full package includes NVIDIA VFG and Microsoft runtime DLLs, both sample videos, the artificial OCR clip, the neural OCR and CPU translator/model/Python, clip-export Python/FFmpeg and original notices.

Requirements: Windows 10/11 x64, NVIDIA Direct3D 11 GPU and compatible installed driver. A DirectShow capture card and its driver are needed only for live capture. NVIDIA VFG supports RTX 40/50 (Ada/Blackwell). Optical Flow is an alternative with different quality; RTX 20/30 are not hardware-tested. RTX 5070 Ti and RTX 4050 Laptop were tested. No HDR/HDCP bypass, ARM, macOS or Linux binary is provided. The application is unsigned; no security settings need to be disabled.

## 2. Generate frames or connect a capture device

| Launcher | Initial behavior |
| --- | --- |
| `1 - Spustit FrameLab.cmd` | CS:GO native preview |
| `2 - NVIDIA RTX 40-50.cmd` | CS:GO + NVIDIA VFG 2× / Medium |
| `3 - Optical Flow.cmd` | CS:GO + Optical Flow 2× |
| `Ukazka - Spider-Man.cmd` | Spider-Man native preview |
| `Ukazka - Ceske titulky.cmd` | Artificial OCR clip + Czech overlay + VFG |

Connect the source HDMI to the capture card, connect the card to this PC, and close other applications holding the device. In **Capture**, choose the source, resolution and capture FPS, then **Start capture**. Start with Automatic or 1080p. In **Audio**, choose the output and volume. In **Frame gen**, select one backend and begin at 2× / Medium. NVIDIA VFG and Optical Flow are alternatives. 3×/4× can increase artifacts, latency and dropped frames.

Capture modes come from the driver. Automatic does not detect HDMI wire timings. Advertised ranges can include modes the driver rejects or rounds; actual accepted dimensions and measured callback FPS are shown separately. A 40-FPS game captured at 60/120 Hz still contains about 40 new images per second. Output submissions are not a physical monitor-FPS measurement. The capture card's HDMI passthrough does not include FrameLab's effects.

## 3. Translate English subtitles in the picture

1. Open **Subtitles** and keep **RapidOCR / PP-OCRv5 English** selected. Neural OCR and its models are included: no Windows language download or administrator prompt is needed. Windows OCR remains an optional fallback.
2. Select **OPUS-MT / CPU** to use the included model, or choose **CUNI-MH Q3_K_M / Q5_K_M / Q8_0** and press **Download / resume and verify**. The first GPU download also installs a private portable runtime. No Ollama installer, account or API key is needed.
3. Enable translation and **Show reading area**. Crop tightly around English dialogue; exclude HUD and chat.
4. Choose **Below original**, **Above original** or **Custom position**. Enable **Drag Czech overlay in preview** to move text and its black background together. This does not change the OCR crop. Multiline captions stay inside the visible viewport, including fill/crop and fullscreen; exceptionally long captions shrink to fit.
5. Press F11 for the finished image. Try the included artificial demo: “The door is locked.” → “Dveře jsou zamčené.”

**[Full model, download and placement guide](docs/subtitle-model-guide.md)**. CUNI Q3 downloads 3.52 GB, Q5 5.13 GB, Q8 7.70 GB. Q5 is download/hash-verified only; its inference, quality and speed are untested; the first GPU model also needs the 1.62 GB runtime archive and about 8 GB extra space for runtime storage/extraction. Free VRAM is checked with current effects running. Loading runs in the background; partial CPU offload is rejected explicitly. CPU translation remains available without GPU model downloads.

**The input is text in the picture, not audio/STT.** OCR accepts very confident text or stable repeated readings. Czech remains visible while the next line translates. **Minimum on screen** (1.2 s) and **Keep while waiting** (3 s) start at display time; long text receives extra reading time. Seeking or changing sources clears old work. **Join split sentences** combines a recent unfinished line with a lowercase continuation; turn it off if it joins separate dialogue. OCR and translation still make mistakes. Local files wait for model warmup; manual Play overrides the wait.

## 4. Display and controls

**Display** selects the monitor, window size and fit/fill/crop/1:1 behavior. It does not change the Windows display mode or refresh rate. **F11** shows one final image with the configured effects and subtitles. All comparison views, controls, labels and the OCR outline disappear automatically. A frozen comparison resumes when entering fullscreen. **Escape / F11** returns to settings, preserving the selected comparison mode. **F1** toggles the panel only in the window; **F8** freezes the window preview. The cursor hides after 2 idle seconds over fullscreen and reappears on movement. See the [illustrated fullscreen guide](https://opicebot.cz/framelab/guide/fullscreen-en.html), also included offline as **FULLSCREEN EN.html**. If tabs overflow, use the arrows at their right or enlarge the window.

## 5. What about DLSS 5 / NR enhancement?

**[Complete NR setup: two downloads and one import wizard](docs/enhancer-install.en.md).** Preview.14 includes the NR adapter and AI / NR controls. Run `4 - Doplnit enhancer.cmd`, open the exact external download (731 MB), and select that ZIP in the wizard. No manual DLL placement, SDK, Python or compiler is needed. The external NR DLLs are not bundled by FrameLab; Veyra is a community source, not an official NVIDIA NR download. Usage/redistribution rights are not established; importing or linking does not grant them. Use with applicable authorization. [Audit](docs/NR-LICENSING-STATUS.txt).

Pipeline: input → one FG backend → optional NR → Czech overlay → monitor. Processing can drop frames and add latency. The website and GitHub provide the same full package.

## 6. Troubleshooting and updates

| Symptom | Action |
| --- | --- |
| Missing DLL, model or sample | Extract the whole **full.zip** into a fresh folder, keep all directories. |
| No capture image | Check the vendor's preview, cable/source and HDCP; close other apps and try Automatic or 1080p. |
| VFG initialization fails | Confirm RTX 40/50 and driver; try Optical Flow. Inspect `app/logs`. |
| No Czech subtitle | Select bundled RapidOCR and check the crop region; try the artificial OCR demo. |
| HUD translated | Narrow the reading region to the subtitle band. |
| Artifacts / slow output | Start at 2×, compare FG off, lower input resolution. |
| Export lacks Czech text | RAW/FG analysis deliberately omits the overlay/NR and records no audio. |

For updates, extract a new version into a new folder and keep the previous one until tested. No service/driver is installed by FrameLab. Uninstall by closing it and deleting its folder after saving any recordings you need. Settings migration is optional.

To verify the download, run `Get-FileHash .\FrameLab-0.2.0-preview.14-windows-x64-full.zip -Algorithm SHA256` in PowerShell and compare with the release's **SHA256SUMS.txt**. [Build from source](docs/build-preview14.md) · [Component provenance](docs/runtime-distribution.md) · [Third-party notices](THIRD_PARTY_NOTICES.md) · [Samples and content rights](docs/sample-provenance.md).

## Manual NVIDIA runtime setup for older minimal packages

Skip this for the full ZIP: the five VFG DLLs are included. For older minimal packages use the [existing runtime add-on](https://opicebot.cz/framelab/downloads/FrameLab-NVIDIA-VFG-1.3-addon.zip), preserving its notices, or NVIDIA's [official VFX SDK installation instructions](https://docs.nvidia.com/maxine/vfx/latest/WindowsVFXSDK/InstalltheVFXSDK.html). The matching VFG 1.3 files belong in `app/runtime`: `cudart64_12.dll`, `NVCVImage.dll`, `nvngxruntime.dll`, `NVVideoEffects.dll`, `nvVFXVideoFrameGeneration.dll`. Do not confuse this component with DLSS NR.

## Your own video or movie

1. Open **Capture → Your video / movie → Open video…** and choose a local file. No capture card is needed. Playback uses the file's native timestamps; **Next replay FPS** applies only to the bundled demo player.
2. Use **Pause / Play**, the position slider, **Loop**, or **Play again** after the end. A paused seek updates the picture. Changing position clears interpolation/enhancement history and old translated captions.
3. In **Audio**, choose the video-file output and volume/mute. Changing the device reopens playback at the current position. Sound and decoded video share one media clock; FG, NR and display processing add video delay. This does not measure or automatically remove the final A/V offset.
4. Enable FG, NR or subtitles exactly as for a capture card. **F11** shows the finished image; return with Escape to reach the playback controls. **F8 freezes the comparison only**; use Pause to stop both the movie and its sound.

Verified with SDR H.264/AAC MP4 and silent MP4. Other formats depend on installed Windows Media Foundation codecs; the extension alone does not guarantee support. HEVC may need Microsoft's codec. No DRM/HDCP bypass, HDR tone mapping, audio-track selection or external SRT/embedded subtitle-track rendering is provided. The first video/audio tracks are used. OCR reads English subtitles already visible in the picture, not the movie's audio. Diagnostic recordings still omit audio, NR and translated overlays.

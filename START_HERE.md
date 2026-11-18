# FrameLab preview.10 — download, install and run

**[Download the complete Windows x64 ZIP](https://github.com/tomik520i/FrameLab/releases/download/v0.2.0-preview.10/FrameLab-0.2.0-preview.10-windows-x64-full.zip)** · [Release files](https://github.com/tomik520i/FrameLab/releases/tag/v0.2.0-preview.10) · **[Česky: celý návod](INSTALL.cs.md)**

## 1. Extract and launch

1. Download **FrameLab-0.2.0-preview.10-windows-x64-full.zip**. The automatic GitHub **Source code** files are for developers.
2. Right-click the ZIP → **Extract all** into a writable folder, for example your Desktop. Do not run inside the ZIP or copy only the EXE.
3. Open **1 - Spustit FrameLab.cmd**. A bundled CS:GO clip loops immediately.
4. Use the **CZ / EN** button for English. Under **Capture**, try Spider-Man or the artificial **EN > CS subtitle demo**. Both real gameplay clips are included, three seconds each.

No SDK, system Python, pip, model download, Git, developer account or compiler is required. Keep `app`, `tools`, `deps` and `licenses` together. The full package includes NVIDIA VFG and Microsoft runtime DLLs, both sample videos, the artificial OCR clip, the CPU translator/model/Python, clip-export Python/FFmpeg and original notices.

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

Open **Subtitles**, enable translation and **Show reading area**. Adjust the four edges to include only the English subtitle band, excluding minimaps, quests and chat. Configure Czech font size, vertical position and opacity; **Cover original subtitle area** adds a black band over the chosen region. Try the bundled artificial demo: “The door is locked.” → “Dveře jsou zamčené.”

Windows OCR reads the input pixels and the bundled OPUS-MT EN-CS model translates on two CPU threads. Czech text is drawn after frame processing. **No audio transcription/STT, cloud service, API key or network translation is used.** Two stable OCR readings are required. Small/stylized/moving text, jargon and short-lived lines may be missed or mistranslated. This is an experimental translator, not a dialogue-quality guarantee.

If English OCR is absent, use the language-settings button offered by FrameLab or Windows **Settings → Time & language → Language & region** (Windows 10: **Language**). Add **English (United States)** and its basic typing / OCR language features; labels vary by Windows version. Administrators may restrict optional feature installation. Restart FrameLab or toggle translation off/on afterward. This one-time Windows component download can need Internet; the model and Python are already bundled and subsequent translation is offline. You do not have to change the Windows display language.

## 4. Display and controls

**Display** selects the monitor, window size and fit/fill/crop/1:1 behavior. It does not change the Windows display mode or refresh rate. **F11** toggles image-only fullscreen, **Escape** returns, **F1** switches panel/image, **F8** freezes comparison. If tabs overflow, use the arrows at their right or enlarge the window.

## 5. What about DLSS 5 / NR enhancement?

The full **public binary excludes NR**. The repository includes optional experimental source, OFF by default. Redistribution rights for the exact original/ShortFuse NR DLLs have not been established, so the release does not include or auto-download them. Veyra's GPL code license does not grant rights to NVIDIA model binaries. [Audit with primary sources](docs/NR-LICENSING-STATUS.txt) · [Technical notes](docs/experimental-nr.md) · [Real precomputed comparison](https://opicebot.cz/framelab/#enhancer).

Public pipeline: input → one FG backend → Czech overlay → monitor. Private NR builds place enhancement after FG, before the overlay. Processing can drop frames and add latency. The website and this GitHub release provide the same OCR/display features.

## 6. Troubleshooting and updates

| Symptom | Action |
| --- | --- |
| Missing DLL, model or sample | Extract the whole **full.zip** into a fresh folder, keep all directories. |
| No capture image | Check the vendor's preview, cable/source and HDCP; close other apps and try Automatic or 1080p. |
| VFG initialization fails | Confirm RTX 40/50 and driver; try Optical Flow. Inspect `app/logs`. |
| No Czech subtitle | Verify English OCR and crop region; try the artificial OCR demo. |
| HUD translated | Narrow the reading region to the subtitle band. |
| Artifacts / slow output | Start at 2×, compare FG off, lower input resolution. |
| Export lacks Czech text | RAW/FG analysis deliberately omits the overlay/NR and records no audio. |

For updates, extract a new version into a new folder and keep the previous one until tested. No service/driver is installed by FrameLab. Uninstall by closing it and deleting its folder after saving any recordings you need. Settings migration is optional.

To verify the download, run `Get-FileHash .\FrameLab-0.2.0-preview.10-windows-x64-full.zip -Algorithm SHA256` in PowerShell and compare with the release's **SHA256SUMS.txt**. [Build from source](docs/build-preview10.md) · [Component provenance](docs/runtime-distribution.md) · [Third-party notices](THIRD_PARTY_NOTICES.md) · [Samples and content rights](docs/sample-provenance.md).

## Manual NVIDIA runtime setup for older minimal packages

Skip this for the full ZIP: the five VFG DLLs are included. For older minimal packages use the [existing runtime add-on](https://opicebot.cz/framelab/downloads/FrameLab-NVIDIA-VFG-1.3-addon.zip), preserving its notices, or NVIDIA's [official VFX SDK installation instructions](https://docs.nvidia.com/maxine/vfx/latest/WindowsVFXSDK/InstalltheVFXSDK.html). The matching VFG 1.3 files belong in `app/runtime`: `cudart64_12.dll`, `NVCVImage.dll`, `nvngxruntime.dll`, `NVVideoEffects.dll`, `nvVFXVideoFrameGeneration.dll`. Do not confuse this component with DLSS NR.

# FrameLab preview.18 — start here

[Česky](INSTALL.cs.md) · [Web comparisons](https://opicebot.cz/framelab/)

[Illustrated walkthrough: current screenshots, numbered steps and zoom](https://opicebot.cz/framelab/guide/visual-en.html). The same guide is included offline as **VISUAL GUIDE.html**.

## New in preview.18: game windows, independent captions and Vulkan

- **Capture → Application window:** refresh, choose the running game or Remote Play window and start capture. Keep the source open; begin with windowed/borderless mode. Protected surfaces and some games cannot be captured. Image processing has its own GPU selection, applied after restart.
- **Display → Subtitles only over source window:** a transparent, click-through overlay follows the selected window. Set the OCR region first and position translated text independently. This mode does not inject FG or NR into the game. [Remote Play, overlay and API walkthrough](https://opicebot.cz/framelab/guide/remote-subtitles-en.html).
- **Subtitles → Translation compute:** Auto, CUDA, Vulkan or CPU, with a separate GPU choice. CUNI and Gemma retain their model files. Optional GPU + RAM allows partial placement. CUDA is fastest on this laptop; Intel UHD Vulkan works but is slow. Physical AMD testing is still outstanding. [Measured setup guide](docs/local-translation-compute.cs.md).
- **Optional OpenAI API:** choose online translation, save your own key and refresh available models. Windows Credential Manager stores the key; OCR text is sent to the selected API. Model profiles govern supported reasoning/temperature parameters. Local models remain usable without a key. The application does not run a paid quality judge.
- A bundled local filter reduces names, license plates and labels; ambiguous text still requires a suitable OCR region. Active playback/capture keeps the display awake without changing the Windows power plan.

[All 2,000 translations, direct assistant review, ranking and examples](https://opicebot.cz/framelab/runs/2026-11-23-direct-review/). The 500-entry benchmark measured text translation with FG, NR and OCR off, not game performance.

## New in preview.17: more GPUs and languages

In **Frame gen**, select **RIFE 4.22 lite / DirectML** or **D3D11 Optical Flow**. These paths are not restricted to RTX 40/50. RIFE needs a suitable hardware DirectX 12 GPU and driver. This laptop's Intel UHD is too slow for RIFE at 720p; Optical Flow reached about 59 Windows-confirmed presentations/s. AMD and older GeForce cards have not been physically tested. NVIDIA VFG and NR retain their separate narrower requirements. [GPU matrix](docs/GPU-BACKENDS.md).

In **Subtitles**, choose the language in the picture and the target. All 12 OCR packs are included. **OPUS-MT / CPU** works immediately for EN → CS. For another pair, select **TranslateGemma 4B Q4_K_M**, click **Download / resume and verify**, wait for completion, then enable **Translate subtitles**. The model is 2.49 GB; the first GPU runtime adds 1.62 GB. Inference is local after setup. Q8 and 12B are optional larger variants. This is not speech transcription. [Full guide](docs/MULTILINGUAL.en.md).

The public full ZIP includes RIFE models/runtime, VFG runtime, 12 OCR packs, CPU translation, two game clips and a synthetic subtitle demo. The app downloads large GPU translation weights/runtime separately; external NR DLLs are neither bundled nor mirrored. Historical screenshots of unchanged steps are labelled; the illustrated guide adds the new language controls.

## Download and launch

1. [Download the full Windows x64 ZIP](https://github.com/tomik520i/FrameLab/releases/download/v0.2.0-preview.18/FrameLab-0.2.0-preview.18-windows-x64-full.zip) and **extract all** to a short writable folder, for example D:/FrameLab.
2. Run **Start FrameLab.cmd**. A new installation opens the GTA demo; subsequent launches restore the saved session. No compiler, Python, SDK or system VLC installation is needed. Keep the entire folder together.
3. Begin with **Frame gen → 2×**. **F11** opens fullscreen; Escape/F11 returns. **EN / CZ** changes language; English is the clean-install default.

VFG targets RTX 40/50. **Start FrameLab - Optical Flow.cmd** selects a separate fallback; support/performance depends on the actual card. VFG runtime, neural OCR, CPU translation, codecs and export tools are bundled. GPU translation models download inside the app. Optional NR uses an external import. Antivirus may delay first launch; inspect errors and keep protection enabled.

## Capture card

In **Capture**, choose the source, **Resolution** and **Capture FPS** separately, then **Start capture**. Automatic leaves the choice to the driver. 4K and other aspect ratios depend on listed hardware modes.

**Delivered frames** shows received dimensions and callback rate. The API does not report HDMI input timing. A 60 FPS capture can repeat 30 FPS game images. **Remove repeated input frames** helps; estimated content FPS is not an engine counter.

No image: check the correct card, cable, permitted source content and status. Close other programs holding the card. **Test 30 FPS** is a synthetic source, not the console.

## Your own video or movie

Use **Capture → Open video…**, then Play/Pause, seek, speed and loop. Bundled **LibVLC 3.0.24** supports MP4/MKV. After tracks load, choose audio and subtitle tracks or turn embedded subtitles off. [Media details](docs/local-video.md).

Embedded subtitles enter the image before FG/NR. OCR translates visible text; it does not directly translate text tracks or transcribe speech. Protected streaming downloads are unsupported. Decoding and copies can add CPU load.

## Frame generation and enhancement

Start with **2× / Medium**. Inspect edges, HUD and fast motion: 3×/4× can produce severe artifacts. The multiplier does not guarantee corresponding physical display FPS.

For **AI / NR**, follow the [exact import guide](docs/enhancer-install.en.md). Start at **720p, one pass**. Lower the processing limit/passes or disable motion estimation if FPS drops. Lower strength only changes the blend, not inference cost. F6 toggles NR; F7 changes comparison; F8 freezes a pair. NR Original can already contain FG. Order: input → FG → NR → translated overlay → display.

**Optional Eco:** in AI / NR, enable **Eco: enhance before frame gen (experimental)**. It enhances source frames before interpolation, reducing NR evaluations when FG is active. It can add latency, extra VRAM use and different artifacts. Default is off; resolution and strength are preserved. [Measured results and limits](docs/NR_ECO.md).

## English → Czech subtitles and saved profiles

Open **Subtitles**, choose bundled **RapidOCR**, **OPUS-MT / CPU**, then enable translation. RapidOCR needs no Windows OCR language download. The artificial EN → Czech sample under Capture helps check setup.

While English text is visible, click **Select subtitle word + reading area…**. The preview freezes. Mark **one word**, then the complete area where multiple subtitle lines can appear, and confirm. It learns color and approximate letter height, not the word itself. Leave room for extra lines and exclude HUD/license plates where possible.

Use **OCR inspector / saved sample…** to inspect the measured colour, letter height and accepted/rejected text. It uses the saved source image, not a live desktop capture.

Name and save a profile. Appearance, region and selected profile persist; load a different profile for another game. Style matching reduces unrelated text, but cannot guarantee perfect detection.

Select optional **CUNI Q3/Q5/Q8**, then **Download / resume and verify**. The app downloads the pinned model and portable runtime, verifies hashes and displays progress. Q5 is used in the published GTA captions. Higher precision is not a guarantee of better translation. [Sizes and subtitle timing](docs/subtitle-model-guide.md).

Czech lines are centered; drag their position independently from OCR. Multiline text is clamped inside the visible image. **Minimum on screen** and **Keep while waiting** keep delayed text readable while OCR continues. Fast dialogue and translation mistakes remain possible.

## Display and audio

**Display** chooses monitor and Fit/Fill/1:1. Fit preserves the whole picture with bars; Fill and oversized 1:1 can crop. F11 hides controls/comparisons/OCR outlines, while retaining the selected translated overlay.

**Audio** selects output, volume and mute. Capture and local-file audio have their respective controls. Analysis clips have no audio. Submitted/DXGI FPS does not measure physical panel scanout or controller-to-photon latency.

## Recording

Choose a destination in **Recording**, then **Record 15 s analysis**. The folder is remembered. For 1440p with all effects, allow at least **36 GiB free** as a practical recommendation; actual use varies.

Frames are losslessly compressed into bounded RAM, then saved and exported. Wait for completion before another take. Insufficient RAM/disk or GPU readback can stop early; partial takes are labeled and the complete prefix is exported.

Exports include RAW/generated indexed clips, synchronized before/after/final, triple comparison, **subtitles.cs.srt**, text JSON, timing and settings/IDs. Final includes actual caption delay. No desktop/audio is captured. [Format and limits](docs/pipeline-recording.md).

## Debug and measurement

Find **Debug & measurement** with the settings-tab arrows. Read the storage warning and start logging; **F9** toggles it. Logging continues until stopped: timestamps, CPU/GPU, FPS, queues, OCR/translation text and settings. Review logs before sharing private text.

New fields include foreground/fullscreen state, window size, presentation waiting and loop stalls. [Measurement meanings](docs/debug-measurement.md).

## Updating

Close the app and back up your folder/settings. Extract the new ZIP separately. Transfer personal settings/profiles and optional verified models/runtime as needed; do not replace new workers or executable with old ones. Public packages contain no owner logs, recordings or personal settings.

## Preview.16 recovery and small fullscreen statistics

In **Debug & measurement**, enable the small fullscreen statistics overlay, or press **F10**. It shows software-confirmed/submitted FPS and app latency mean/P95 in the upper-right corner. This excludes capture-card and monitor delay; it is not controller-to-photon latency. It is not burned into analysis recordings.

Ordinary launch now restores the saved session, including the logging state. Stop logging before closing if you do not want it to resume. If a subtitle worker fails, the app retries up to three times after 2, 5 and 15 seconds and resubmits unchanged OCR text. Turning subtitles off cancels retries. Optional GPU translation remains resident while enabled; switching it off unloads the model.

The stopped three-hour session has [all 1,747 translations assessed](https://opicebot.cz/framelab/runs/2026-11-20-3h/), with original logs and disclosed errors. Experimental prompt comparisons do not automatically change the app prompt.

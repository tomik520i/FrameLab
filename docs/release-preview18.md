# FrameLab 0.2.0-preview.18

Window capture can now feed the existing image pipeline, or a transparent, click-through subtitle-only window can follow a running game/Remote Play client. Translation has independent CUDA/Vulkan/CPU and GPU selection; OpenAI with a user-owned API key is optional. Local translation remains available without an API account.

- Windows Graphics Capture window selection; separate image GPU selection and restart.
- Subtitle-only overlay with independent positioning. It does not inject FG/NR into the source game. Protected content and some exclusive fullscreen applications remain unsupported.
- Active playback/capture keeps the screen awake using a scoped Windows execution request.
- Bundled English entity filter and conservative OCR/text filtering reduce standalone names and labels while preserving short dialogue. Ambiguous text still requires a well-chosen OCR region.
- CUNI/TranslateGemma GGUF on CUDA, Vulkan or CPU, stable GPU UUID selection, explicit actual placement checks, optional GPU + RAM. Vulkan exercised on Intel UHD and RTX 4050; AMD not physically tested. Intel UHD was slower than CPU.
- Slow translation results remain usable when fresh OCR still confirms their source text; changed/disappeared source and generation resets reject stale results.
- Optional OpenAI BYOK via Windows Credential Manager, dynamic available-model list with capability profiles, reasoning/temperature handling and cost accounting. Only OCR text goes to the selected API. No paid quality judge runs during application translation.
- Expanded Czech/English offline and online guides: Remote Play, window capture, subtitle-only overlay, API and translation compute.
- 500-entry comparison: all 2,000 saved outputs directly reviewed by the Codex assistant, randomized A–D labels, no further paid API requests. Scores are subjective single-AI judgments, not independent human accuracy. All outputs, individual ratings, explanations, timing, costs and raw data are published. Previous paid judgment remains an explicitly separate historical record.

Full portable package includes the existing RIFE/DirectML and VFG runtime, 12 OCR packs, OPUS CPU translation, entity filter, codecs, two game samples and synthetic OCR sample. Large GPU translation weights and the common runtime download through the application. External NR model DLLs are not redistributed. Existing NVIDIA hardware restrictions remain enforced.

## Validation and limits

Native CTest regression suite; Python compute/lifecycle/API/filter tests; actual Intel/RTX Vulkan model inference and real OCR-to-translation pipeline. Packaged application startup and website links/interactive reports are checked during release. No additional paid API test was requested for this release; recorded earlier API integration and offline mocked regression tests are used.

The 500-entry quality/timing benchmark ran without OCR, FG or enhancer. The separate Vulkan timing smoke test contains four identical short entries per configuration. Neither is a claim about combined gaming FPS. Console Remote Play steps follow official documentation; no physical console was tested here. See the reports for exact conditions and unmodified failures.

[Quality review](https://opicebot.cz/framelab/runs/2026-11-23-direct-review/) · [Compute measurements](https://opicebot.cz/framelab/runs/2026-11-23-compute/) · [Remote Play / API guide](https://opicebot.cz/framelab/guide/remote-subtitles-en.html).

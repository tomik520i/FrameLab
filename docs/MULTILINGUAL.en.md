# Multilingual subtitles and TranslateGemma — preview.18

[Česky](MULTILINGUAL.md) · [GPU compatibility](GPU-BACKENDS.md)

FrameLab reads subtitle text in the picture. It does not transcribe speech.
In Local / offline mode, OCR and translation run locally after setup. Optional OpenAI API is a separate online choice. The application interface remains English/Czech;
the source and target subtitle selectors offer 40 language variants.

## Start

1. Extract the entire full ZIP and run **Start FrameLab.cmd**.
2. Open **Subtitles**. Select the actual **Language in the picture / OCR**, then **Translate into**.
3. Keep **RapidOCR / PP-OCRv5 multilingual**. All 12 recognition families are bundled; Windows OCR is optional.
4. For EN → CS, bundled **OPUS-MT / CPU** works without any additional model download. Optional CUNI models remain available for this pair only.
5. For other pairs, select **TranslateGemma 4B / Q4_K_M** and click **Download / resume and verify**. GPU weights/runtime are not in the public ZIP. Wait for the verified ready state.
6. Enable **Translate subtitles**. First loading may take tens of seconds. The unchanged caption is re-read after model loading finishes.
7. Restrict the reading area to the original subtitles. Optional word sampling learns colour/size, not the exact word.
8. Adjust overlay size, background and position independently. Above/below placement uses the OCR rectangle. **F11** displays the processed output and captions, hiding comparisons and edit handles; Escape returns to settings.

Changing language clears the old caption and translation cache. A running Gemma reloads the language pair.
The source selector is manual, not automatic language detection. Missing OCR packs produce an error rather than silently using English.

## Downloads and memory

| Model | Weight download | Approximate free VRAM before loading |
|---|---:|---:|
| TranslateGemma 4B Q4_K_M | 2.49 GB | 3.6 GiB plus room for effects |
| TranslateGemma 4B Q8_0 | 4.13 GB | 5.2 GiB plus room for effects |
| TranslateGemma 12B Q4_K_M | 7.30 GB | 9.0 GiB plus room for effects |

First GPU setup also downloads a 1.62 GB portable Ollama archive. Leave about 8 GB extra disk space for extraction.
Downloads support pause/resume and check the pinned SHA-256 before activation. No system Ollama installation or account is required.
FrameLab owns a private process on 127.0.0.1 with cloud functionality disabled. Full GPU residency is the default; preview.18 also provides explicit CPU mode and an optional GPU + RAM choice, with actual placement checks.
The translator's GPU is independent of the adapter selected for image processing. A renderer VRAM readout may refer to a different GPU on multi-GPU PCs.
These multilingual results used RTX 4050 6 GB with 4B Q4. Q8/12B remain untested here. Preview.18 separately validated CUNI on Intel UHD Vulkan and Gemma on RTX Vulkan; physical AMD testing remains outstanding. [Compute guide](local-translation-compute.en.md).

## Languages and text rendering

EN, CS, SK, DE, FR, ES, IT, PT, PL, UK, RU, JA, KO, ZH-CN, ZH-TW, AR, EL, HI, TR, VI,
NL, SV, NO, DA, FI, HU, RO, BG, HR, SL, ET, LT, LV, ID, MS, TH, FA, UR, TA, TE.
This is an explicit UI catalog, not proof of quality for all 1,560 distinct translation directions.
Chinese script variants do not guarantee output-script conversion. Vertical Japanese text is outside this integration.

The 12 CPU OCR recognizers cover English, Latin, East Slavic, Cyrillic, Chinese/Japanese, Korean, Greek,
Arabic, Devanagari, Thai, Tamil and Telugu. Detection remains PP-OCRv4. OCR model files total about 114 MB.
DirectWrite renders at output resolution with system font fallback, shaping and right-to-left support.
If Windows lacks a glyph, add the appropriate supplemental language fonts. Fullscreen and recordings use the same caption renderer.

## What was tested

44 synthetic subtitle images in 22 languages rendered with no missing glyphs. OCR processed all 44;
23 were exact matches and mean per-image CER was 4.47%. These are not game-wide accuracy estimates.
56 short Gemma translations in 16 directions completed: median 431 ms, P95 566 ms, excluding OCR/loading.
The runtime reported 2.56 GiB GPU residency. Actual OCR → GPU MT language switching also passed.
RIFE Low + NR 360p + Gemma worked together on a constant-caption demo; this is not a continuous-dialogue performance benchmark.

Small Gemma Q4 made obvious Czech grammatical errors, including “Nevyotevívejte červenou dveř.”
It is not established as better than CUNI. Larger quantizations may help but were not benchmarked here.
Some OCR alphabets lose punctuation, diacritics or split segments. The English fragment-joining heuristic is bypassed for Gemma.
[Exact results and limitations](release-preview17.md).

## Licenses and developer files

Gemma is provided under and subject to the Gemma Terms of Use found at https://ai.google.dev/gemma/terms.
[Full terms](licenses/Gemma-LICENSE.txt) and [required notice](licenses/Gemma-Notice.txt) are included.
The 4B GGUFs are community quantizations by mradermacher; 12B Q4 is by bullerwins. They are text-only files without the vision projector.
[Pinned catalog](subtitle-models.json) lists exact revisions, sizes and SHA-256. OCR models retain Apache-2.0 notices.

Build/setup uses prepare_translation.py and prepare_ocr.py. Generate the native language header with
generate_subtitle_languages.py after changing subtitle-languages.json. Settings v9 reads versions 1–8.
Evaluation entry points: evaluate_translategemma.py, evaluate_multilingual_ocr.py,
subtitle_multilang_test and subtitle_language_switch_test. See the Czech technical guide for commands.

# Subtitle models, reading time and placement

[Český návod](subtitle-model-guide.cs.md)

FrameLab reads English subtitles already visible in a capture or local video, translates them locally and draws Czech after FG and NR. It does not transcribe audio or render SRT/embedded subtitle tracks.

## Start with bundled OCR

1. Open **Subtitles** and select **RapidOCR / PP-OCRv5 English**. The complete ZIP contains the OCR runtime, detector and recognizer. No Windows language pack, administrator approval or extra download is required.
2. Select **OPUS-MT / CPU** (included), enable translation and **Show reading area**. Crop tightly around the English subtitle band, excluding chat and HUD. Use the artificial subtitle demo under Capture to check setup.
3. Choose the independent Czech text position. F11 shows the final image without editing controls. A local video waits for model warmup; pressing Play overrides that pause. External capture cannot be paused.

RapidOCR uses two CPU threads, a PP-OCRv4 detector and PP-OCRv5 English recognizer. **Windows OCR** remains selectable as a lighter fallback. Only that option needs the English Windows OCR capability; if absent, **Install English OCR** offers the Microsoft download with UAC approval. It does not change the Windows display language.

## Choose a translation model

| Model | Download | Status |
| --- | ---: | --- |
| OPUS-MT / CPU | Included | Two CPU threads, no GPU model download. |
| CUNI-MH 7B Q3_K_M | 3.52 GB | Tested on the desktop RTX 5070 Ti. |
| CUNI-MH 7B Q5_K_M | 5.13 GB | Download/hash and local GPU load verified. Quality and speed not benchmarked. |
| CUNI-MH 7B Q8_0 | 7.70 GB | Tested; larger memory footprint. |

Select a GPU model and press **Download / resume and verify**. First use also downloads the 1.62 GB portable Ollama runtime; allow about 8 GB additional disk space for runtime storage/extraction. No system Ollama installation, account or API key is required. Downloaded files must match pinned SHA-256 hashes.

Files stay in `app/translation/models` and `app/translation/gpu-runtime`. NTFS uses a hardlink for the inference cache; other filesystems can need another full model copy. Keep the entire app folder together. GPU weights are optional downloads, not included in the release ZIP. [Exact catalog and sources](subtitle-models.json).

Free VRAM is checked while other effects are running. Estimates (Q3 4.4 GiB, Q5 6.0 GiB, Q8 8.5 GiB, plus headroom) are not hardware guarantees. Partial CPU offload is rejected explicitly. Reduce effects or choose CPU if capacity is insufficient. Higher weight precision does not guarantee a better sentence.

The existing 300-sentence **preview.12 baseline** measured Q3 median 212 ms versus Q8 360 ms alongside VFG 2×, NR 65% / 720p and the former OCR path. Those are exact-text request times, not full subtitle delay, and not a preview.14/Q5 benchmark. Both models were judged acceptable on 271/300 translations in the text preflight; judgments were not a blinded human study. [All measurements and translations](https://opicebot.cz/framelab/translation/).

## Place the Czech text

- **Custom position:** set anchors or enable **Drag Czech overlay in preview**. Text and its background move together, independently of the OCR area.
- **Above original / Below original:** anchor to the OCR rectangle plus your chosen gap.
- **When space runs out:** switch to the opposite side or clamp inside the visible image. This can overlap the original; very tall text shrinks to fit.

Multiline text stays within the visible viewport at 4K, portrait, fullscreen, Fill/crop and 1:1. Font size, background opacity and **Cover original subtitle area** are independent. Fullscreen hides OCR outlines and editing handles; Escape/F11 returns to settings.

## Timing and split sentences

**Minimum on screen** defaults to 1.2 seconds; **Keep while waiting** defaults to 3 seconds. The clock starts when Czech appears, including delayed translations. Long text gets up to 7.5 seconds; a ready next caption can replace it after the minimum. OCR and translation continue on separate threads. Blank input does not immediately erase a displayed caption. Pausing local playback keeps it visible.

**Join split sentences** defaults on. An unfinished sentence is marked with an ellipsis for translation. A following lowercase fragment within six seconds can be translated again together with it. Completed sentences, new speaker labels, source changes and seeking block the join. Turn this off if separate dialogue is joined incorrectly; the ellipsis still marks incomplete input. This fixes the observed Q3 invented tail in the local test, not all possible hallucinations or grammar errors.

There are at most three pending text requests and three completed captions. Work more than six seconds behind is discarded; fast dialogue can still be missed. Case, whitespace and ordinary punctuation jitter do not retrigger translation. Seeking, changing source/ROI/model or disabling translation clears old work. Disabling unloads the owned model process. [OCR comparison and limitations](subtitle-ocr.md).

## Offline operation and developer provenance

The CPU worker uses private pipes; CUNI starts an owned Ollama process bound to `127.0.0.1`, with cloud access disabled and one model/request at a time. Publisher downloads do not send subtitle text or images. After setup, recognition and translation work offline. Optional `--subtitle-trace PATH.jsonl` records private local diagnostic text; it is off by default. RAW/FG analysis recordings omit the Czech overlay, NR and audio.

OPUS-MT en-cs opus-2019-12-18 is CC BY 4.0, converted to CTranslate2 int8 without retraining. CUNI-MH is the CUNI team's English-to-Czech Mistral 7B fine-tune; the original card identifies MIT licensing. Richard Erkhov supplies pinned GGUF quantizations. The catalog preserves publisher links and hashes. CUNI uses the original Instruction / Glossary / Previous text / Input / Response template, empty glossary/history, temperature 0, newline stop, 2048-token context and a 256-token limit. Joined fragments are sent together in Input.

Developers use `tools/prepare_translation.py` and `tools/prepare_ocr.py`; end users need neither. See [build instructions](build-preview14.md), [OCR components](ocr-components.json) and [translation components](translation-components.json). Tests cover settings migration, viewport bounds, fragment boundaries, real neural OCR/CPU translation, delayed captions and source reset. Q5 was activated on the owner’s desktop after verification; no controlled quality or speed benchmark was run.

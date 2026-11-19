# GPU subtitle models and overlay placement

[Český návod](subtitle-model-guide.cs.md)

FrameLab reads **English subtitles already visible in the input image**, translates
them locally and draws Czech after frame generation and enhancement. Audio is not
used. A better translator cannot repair text missed by OCR.

## Start

1. Open **Subtitles**, select a model and enable translation.
2. Enable **Show reading area** and crop tightly around the English subtitle band.
   Exclude chat, minimaps and objectives. Two matching OCR readings are required.
3. Set the size and placement of the **Czech overlay** independently of the reading
   area. Press F11 for the finished image without editing controls.

Windows must have the **English OCR** language capability. If it is missing, the
application reports it and offers Windows language settings. Installing this
Windows component requires administrator approval. Re-enable translation after
installation. The complete ZIP already includes the CPU translation model.

## Choose a model

| Model | Download | Intended tradeoff |
|---|---:|---|
| OPUS-MT / CPU | Included | Two CPU threads; leaves GPU memory for image processing. |
| CUNI-MH 7B Q3_K_M | 3.52 GB | Smaller GPU model specialized for English to Czech. |
| CUNI-MH 7B Q8_0 | 7.70 GB | Higher weight precision, substantially greater memory use. |

For a GPU model, press **Download / resume and verify**. The first download also
fetches the **1.62 GB portable Ollama runtime** from its official release. Allow
roughly 8 GB additional disk space for its archive and extraction. Downloads are
streamed, can be paused/resumed and must pass pinned SHA-256 checks before use.
No Ollama system installation, account, API key or service is required.

The catalog is [subtitle-models.json](subtitle-models.json). Files go into
`app/translation/models` and `app/translation/gpu-runtime` beside the portable app.
On NTFS the inference cache uses a hardlink instead of a second model copy; other
filesystems may need another 3.52 or 7.70 GB. Keep the whole application folder
together when moving it. GPU models are optional downloads, not in the release ZIP.

Free VRAM is measured while your current effects are running. It includes the
impact of other applications, capture resolution, frame generation and enhancement.
Estimates include a margin; they are not a promise based on a GPU name. If capacity
is insufficient, choose Q3 or CPU, lower enhancer resolution, or close another GPU
application. Loading checks available physical memory and verifies complete GPU
offload. A partial CPU fallback is an error, not accepted silently. Multi-GPU
selection is not currently exposed; testing uses one NVIDIA GPU.

Q8 is not guaranteed to translate every sentence better than Q3. Compare quality
separately from speed and memory. Earlier notebook reports labelled **Hy-MT2 Q8**
refer to a different 1.8B model, not CUNI-MH 7B Q8.

On the fixed 300-sentence desktop text-only comparison, both CUNI variants scored
271/300 acceptable translations. Q3 had two major errors and Q8 five; 175 outputs
were identical. These are assistant judgments, not a blinded human study. This
preflight does not establish performance alongside OCR, frame generation or NR.

## Place Czech text and its background

- **Custom position:** choose horizontal/vertical position and anchoring, or enable
  **Drag Czech overlay in preview** and drag text and its black box together.
  Dragging switches to Custom without changing the OCR area.
- **Below original / Above original:** uses the corresponding edge of the OCR
  rectangle plus your selected gap. Keep that rectangle tight.
- **When space runs out:** explicitly choose the opposite side or clamp inside the
  image. If neither side fits, clamping keeps text visible but can overlap English.
  The editing preview explains any adjusted placement.

Coordinates and the gap are relative to the image, so placement survives resizing,
resolution changes and fullscreen. Multiline text and padding stay within the
image; unusually tall text shrinks to fit. Font size and background opacity are
separate settings. **Cover original subtitle area** is a separate black rectangle
over the OCR band; leave it off to keep English visible.

Fullscreen shows only the final image and active subtitles. Editing handles, OCR
outlines, labels and comparison dividers are hidden. Escape/F11 returns to settings.

## Latency, cancellation and privacy

OCR and translation use separate background threads. Sampling defaults to 300 ms
and requires two stable readings. Only the latest pending image/text is retained.
Changed or blank OCR clears the previous caption; completed translations for older
lines are discarded. A bounded 128-line cache avoids repeating translation. Source
resets clear pending captions; disabling translation unloads the model and its
owned processes. Model loading/warmup does not run on the rendering thread.

The CPU worker uses private stdin/stdout IPC. The GPU worker starts an owned Ollama
process bound only to `127.0.0.1`, with cloud access disabled and one model/request
at a time. Text inference stays on this PC. Downloads contact the publishers; they
do not send captured images or subtitle text. Worker errors are under `app/logs`.

Tiny, moving, stylized and short-lived text can still be missed. OCR errors,
stabilization and inference add delay. Frozen previews retain the caption visible
at freeze time. RAW/FG analytical recordings omit the subtitle overlay and NR;
those files remain diagnostics of the input/frame-generation stages.

## Provenance and developer checks

The bundled CPU model is Helsinki-NLP / OPUS-MT `en-cs/opus-2019-12-18.zip`, licensed
**CC BY 4.0** in that exact archive. It was converted to CTranslate2 4.9.0 int8
without retraining. SentencePiece files, attribution and license are retained.
[translation-components.json](translation-components.json) pins its inputs.

CUNI-MH is Mistral 7B fine-tuned for English to Czech by the CUNI WMT team. The
archived original model card identifies MIT licensing; Richard Erkhov supplies
the pinned GGUF quantizations. Downloads use existing publisher repositories;
FrameLab does not mirror or relabel weights. The catalog links exact revisions,
hashes and the official runtime license. The CUNI prompt follows the original
Instruction / Glossary / Previous text / Input / Response format with empty history
and glossary, temperature 0, newline stop, 2048-token context and a 256-token limit.

`python tools/prepare_translation.py` prepares portable components for developers.
Release packaging copies current workers and the catalog into the app.
`tests/subtitle_models.py` checks interrupted/resumed downloads, integrity rejection
and unsafe archive paths, runtime repair and stale readiness without downloading
real models. Native tests cover
layout/persistence, actual Windows OCR + CPU translation, and asynchronous model
cancellation/loading/unloading. GPU and actual OCR tests require their runtime/model
or Windows capability and are run separately from CTest.

The optional NR adapter is enabled in the default build; external model DLLs are
not bundled. `FRAMELAB_EXPERIMENTAL_NR=OFF` excludes the adapter entirely.

# Offline picture subtitles

The local v3 build reads English subtitles already drawn in the input picture and
overlays Czech. It does not transcribe audio. The input crop is copied at a limited
rate before frame generation and neural enhancement; OCR and translation run in a
background worker. The Czech text is drawn after both image processing stages.

Enable it in **Subtitles / Titulky**, enable the reading-area outline, and adjust
the four crop boundaries to include only the English subtitle band. Translation
requires two matching OCR readings. Font size, vertical placement, background and
covering the original subtitle band are configurable. Source changes and disabling
the feature clear captions; blank OCR removes the old caption, and a four-second
fallback prevents a stale result from sticking on the live image. Frozen previews
retain the caption visible at freeze time.

Both the model and its Python runtime are bundled. Inference uses two CPU threads;
there is no cloud API, telemetry, microphone input or speech recognition. Windows
must have the English OCR language component. If it is missing the app provides a
Windows language-settings button; after installation toggle translation off/on.
The translation process is private stdin/stdout IPC, not an HTTP server. It accepts
bounded text messages; frames are not sent out of the process over a network.

## Provenance and limits

Exact original model: Helsinki-NLP / OPUS-MT `en-cs/opus-2019-12-18.zip`, downloaded
from `object.pouta.csc.fi`. Its included LICENSE is **CC BY 4.0**. This is the license
of the actual archive used; a different Hugging Face package's Apache label was not
used to relabel these files. Converted Marian NPZ to CTranslate2 4.9.0 int8; source
and target SentencePiece models retained, no retraining. Original attribution,
license, model card and conversion description are in `app/translation`.
`translation-components.json` and the packaged `provenance.json` pin dependencies,
publisher URLs and hashes. Windows OCR remains an OS component.

Observed on RTX 4050 Laptop's PC: the CPU translator returned short Czech sentences
in approximately 50–100 ms after model load. End-to-end latency additionally includes
OCR, a 300 ms sampling interval and two matching readings. It is not an instant or
perfect translator. Stylized/tiny/moving text, names, jargon, short-lived lines and
HUD text can be misread or mistranslated. No long gameplay/dialogue quality benchmark
has been completed. Test fixtures are identified as artificial OCR test material.

The native pipeline test checks real Windows OCR + the pinned local EN-CS model,
blank subtitle clearing, source reset and disabled behavior. The moving test video
also ran with NVIDIA VFG 2x. Existing analytical RAW/FG exports omit the Czech overlay
and NR; those files remain input/FG diagnostic material.

## Build

`python tools/prepare_translation.py` prepares the external component, with original
licenses and publisher hash checks. This is a developer step, not an end-user install
requirement. Build CMake with `FRAMELAB_EXPERIMENTAL_NR=OFF` for the variant without NR.
NR defaults OFF in fresh builds and is excluded from its portable package.

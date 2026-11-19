# Local picture subtitles — preview.14

Use the [English setup/model guide](subtitle-model-guide.md), [Czech guide](subtitle-model-guide.cs.md) and [OCR report](subtitle-ocr.md) for current behavior and measured limits.

The input crop is sampled before FG/NR. OCR and translation use separate background workers; Czech is drawn after image processing. Neural OCR uses bounded shared-memory image crops and two CPU threads. The bundled OPUS-MT translator also uses two CPU threads. CUNI GPU models are optional; Q5 has download/hash verification and a basic GPU startup check, without a quality/speed benchmark.

The full ZIP includes RapidOCR, PP-OCRv4 detection, PP-OCRv5 English recognition and runtime. Windows OCR is an optional fallback; only that engine needs the Windows English capability. No microphone, STT, cloud translation or API key is used.

Queues are bounded and source/ROI/seek changes clear old work. Reading time starts when Czech appears; blank input no longer clears late captions immediately. A caption remains while its original text is recognized. Incomplete sentences are marked with an ellipsis and may be retranslated with a recent lowercase continuation. The switchable heuristic does not guarantee correct grammar or prevent every hallucination. See the model guide for exact limits.

OPUS-MT en-cs opus-2019-12-18.zip is CC BY 4.0 in that exact archive, converted to CTranslate2 int8 without retraining. Attribution, licenses and SentencePiece files are included. Exact sources and hashes: [translation-components.json](translation-components.json), [ocr-components.json](ocr-components.json), [subtitle-models.json](subtitle-models.json).

Developers prepare components using tools/prepare_translation.py and tools/prepare_ocr.py with Python 3.11 x64. End users extract the complete ZIP. See [build/package instructions](build-preview14.md). Native tests cover real neural OCR/CPU translation, delayed captions, reset, settings migration and viewport bounds. Windows-fallback isolation tests run separately. No test measures physical display latency.

RAW/FG diagnostic recordings omit NR, audio and the Czech overlay. Optional --subtitle-trace PATH.jsonl records local subtitle text and is off by default. Personal videos and traces must not enter public packages.

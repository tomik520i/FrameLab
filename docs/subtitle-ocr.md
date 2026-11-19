# Subtitle OCR and reading time (preview.14)

The recommended **RapidOCR / PP-OCRv5 English** option reads English subtitles
already burned into the video. It runs locally, with bundled models and two CPU
threads, leaving the GPU available for FG, NR and CUNI translation. Windows OCR
remains selectable. Only that fallback needs the English Windows OCR capability.

Select the subtitle band in **Subtitles**, choose your translation model, and
enable **Translate subtitles to Czech**. The source can be a capture device or
a local video. This does not transcribe speech or read embedded subtitle tracks.

**Minimum on screen** defaults to 1.2 seconds. **Keep while waiting** defaults to
3 seconds; long translations receive extra reading time, up to 7.5 seconds.
These timers start when Czech text appears, not when OCR first saw the source.
The next ready translation can replace the current caption after the minimum.
OCR and translation continue while that caption is visible. There are at most
three waiting translation requests and three completed captions; work more than
six seconds behind is discarded. Seeking, switching sources or disabling
translation flushes old work immediately. Pausing local playback retains the
caption. The timing controls do not restart the model.
Local video automatically waits for the translation model to load before
continuing playback. Manually pressing Play overrides that wait. Capture inputs
keep running because FrameLab cannot pause an external HDMI source.

Recognition accepts a very confident reading or two equivalent readings. Case,
spacing and ordinary punctuation jitter do not restart translation. A one-letter
change in a longer accepted line needs three readings; it is not silently merged
with the old sentence. Changed numbers and negations remain distinct.

OCR still makes mistakes on blurred, obscured or stylized text. Translation can
also mistranslate a sentence or invent a continuation of a fragment; OCR accuracy
is not translation accuracy. In particular, captions that split a sentence over
time are translated separately. This is not a substitute for authored subtitles.

## Local test, 2026-11-19

User-provided 48.25-second, 576 × 1024 / 25 FPS video with quick English captions.
48 frames sampled at one-second intervals, 43 with text. A manual reading was
compared against OCR, ignoring case, whitespace and punctuation. On this sample:

| OCR | Matching text frames | Median OCR | P95 OCR |
| --- | ---: | ---: | ---: |
| Existing Windows OCR | 4 / 43 | 5.9 ms | 12.6 ms |
| Selected RapidOCR, PP-OCRv4 detector + PP-OCRv5 English recognizer | 42 / 43 | 130.6 ms | 225.8 ms |

The remaining sampled failure was text obscured by motion. These are selected
clip results, not a general OCR benchmark or a promise of 98% translation quality.
The original video and extracted frames stay local and are not redistributed.

For a reproducible developer setup, run `python tools/prepare_ocr.py --output
YOUR_APP/ocr` with Python 3.11 x64. This fetches the exact wheels/models listed in
`docs/ocr-components.json` and checks their SHA-256. End users receive the prepared
`app/ocr` directory in complete packages, alongside the bundled Python runtime.

`--subtitle-trace PATH.jsonl` explicitly enables a local diagnostic text trace.
It records OCR text, displayed translations, processing times and queue depth.
It is off by default; traces can contain private dialogue and are not uploaded.

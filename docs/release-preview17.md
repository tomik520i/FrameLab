# FrameLab 0.2.0-preview.17 — RIFE and multilingual subtitles

23 November 2026. This release incorporates the locally tested portable frame-generation and multilingual work.

## Added and fixed

- RIFE 4.22 lite through native ONNX Runtime / DirectML, switchable with D3D11 Optical Flow and NVIDIA VFG. Three motion scales, bundled models and runtime.
- Repaired portable Optical Flow. Real Intel UHD and RTX 4050 comparisons; AMD and older GeForce remain unverified hardware candidates.
- NR hardware guards apply to the menu, F6, restored session and initialization. This does not port DLSS 5 to AMD/Intel.
- 40 source/target language choices; 12 CPU OCR model families; optional TranslateGemma 4B Q4, 4B Q8 and 12B Q4 downloads.
- DirectWrite subtitle rendering for Unicode, Arabic shaping and CJK at output resolution. Original subtitle positioning and clean fullscreen are preserved.
- Language changes flush stale captions/cache and restart the correct OCR/translation workers. A caption expiring during a cold model load is re-read after readiness.
- CPU OPUS and CUNI remain EN → CS only. Gemma bypasses the English fragment-joining heuristic. Settings v9 reads v1–v8.
- Updated Czech/English website, guides, licensing notices and explicit package contents. Existing GTA/Spider-Man media and historical audits stay unchanged.

## Measured evidence

[FG measurements](GPU-BACKENDS.md): 44 withheld triples in six configurations and separate live 720p playback runs.
Intel UHD Optical Flow: 59.3 Windows-confirmed presentations/s; Intel RIFE Low: 13.5; RTX 4050 RIFE Low: 59.2.
These do not establish AMD performance or physical monitor scanout.

[Multilingual test](https://opicebot.cz/framelab/runs/2026-11-23-multilang/): 56 short translations in 16 directions;
4B Q4 median 431 ms, P95 566 ms excluding OCR/loading. GPU residency reported 2.56 GiB.
44 synthetic OCR images across 22 languages; 23 exact readings, mean per-image CER 4.47%, no missing rendered glyphs.
Actual OCR → GPU translation passed DE→EN, same input DE→CS and JA→EN switches.
RIFE + NR + Gemma also worked in the live constant-caption demo. That is not a continuously changing-dialogue benchmark.

Small Gemma Q4 has clear Czech grammar/meaning errors. CUNI remains an option; this release does not claim Gemma is better.
Q8/12B are downloadable but not tested on this laptop. 40 choices are not verification of all 1,560 language pairs.
Existing local HTTP download/resume regression tests encountered intermittent WinError 10054 resets in this environment;
successful actual HTTPS model downloads and SHA-256 verification do not replace that missing regression result.

## Package contents

The full portable ZIP includes RIFE and VFG runtimes, all OCR models, CPU translation, video codecs, exporters,
two game samples, a synthetic OCR sample, offline illustrated guides and third-party notices.
GPU translation weights and the portable GPU inference runtime download inside the app.
External NR model DLLs are excluded from Git, public archives and the website mirror; the import wizard remains.
The corresponding source ZIP accompanies the binary. See [third-party notices](../THIRD_PARTY_NOTICES.md).

# Local subtitle prompt experiments

`tools/evaluate_subtitle_prompts.py` replays the `translation_complete` events
from an existing debug session against an **already running loopback runtime**.
It does not change the live application's prompt, start another model, or upload
texts. These experiments use CUNI-MH Q5_K_M; they do not establish results for
Q3, Q8 or the CPU translator.

Start subtitles with Q5 in FrameLab first. Find the current `Listening on
127.0.0.1:PORT` entry in the application's `logs/subtitle-gpu-runtime.log`.
Then run, replacing the example paths and port:

```powershell
python tools/evaluate_subtitle_prompts.py 'path/to/debug-session/events.jsonl' 'analysis-output/new-prompt-run' --url 'http://127.0.0.1:12345'
```

Use a new output folder: the script refuses to overwrite existing results.
Requests share the live GPU runtime, so replay can temporarily compete with
playing video. Each request is sequential, with alternating profile order.
The current defaults compare the production template (`baseline`) with the
experimental `subtitle-fidelity-v2-quoted`. `--profiles` can also select the
earlier `subtitle-fidelity-v1`. `--extra-cases file.json` adds a JSON array of
English strings fixed before the replay.

The output contains:

- `manifest.json`: model name, generation parameters, profiles and source cases;
  new runs also record UTC start and the source event log's SHA-256.
- `results.jsonl`: original logged translation when available, exact submitted
  prompt, raw model response, normalized response, completion reason and timings.

The replay reproduces the application's incomplete-sentence marker. It does not
repeat OCR, caption scheduling or presentation. API timings therefore **are not
OCR-to-display latency**. They also include concurrent GPU work; use repeated,
controlled runs for performance conclusions. Any `done_reason: length` result
must be treated as truncated, not a valid quality comparison.

The profiles in `tools/subtitle_prompt.py` are research candidates. Production
still uses `PROMPT` in `tools/subtitle_gpu.py`. V2 adds four generic translation
demonstrations and encloses the input in quotation marks. Only the added outer
pair is removed from responses. It does not rewrite names or negation words,
infer speaker identity, or truncate output at the first period.

The local trial improved some negations, proper names and invented dialogue
continuations. It also introduced semantic regressions involving tense,
unsupported detail and forms of address. **It was not promoted to production.**
Examples used while designing a prompt are development data. Keep fresh cases
separate and review both improvements and regressions; translation differences
alone are not a quality score. Recorded fragments and repeats are not independent
sentences.

Debug text and generated reports stay in ignored `analysis-output/` directories.
Do not include private session contents in a public release or repository.

References: [CUNI-MH model card](https://huggingface.co/wmt24-cuni/CUNI-MH/blob/main/README.md),
[authors' paper, Appendix A](https://www2.statmt.org/wmt24/pdf/2024.wmt-1.16.pdf).

## Frozen corpus studies and progress

For a JSONL corpus with unique `id`, `en` and `reference` fields, use:

```powershell
python tools/evaluate_prompt_corpus.py path/to/corpus.jsonl analysis-output/new-study/run-1 --url http://127.0.0.1:12345 --profiles baseline subtitle-glossary-v4
python tools/prompt_test_monitor.py analysis-output/new-study
```

The monitor prints its loopback URL (default port 18794). It reads saved results,
refreshes every four seconds, and does not issue inference requests. An optional
`study-status.json` in the study root supplies `phase`, `detail` and `updated`
fields. A completed `report.html` is linked automatically. The server exposes
only the progress page, progress JSON and these explicit report endpoints;
it does not browse arbitrary files. Stop its process when no longer needed.

The corpus runner warms each profile, checks full GPU offload, preserves the
exact input text, records runtime/model metadata, and refuses an existing output
directory. A `COMPLETE.json` marker is written only after all requested outputs
have been saved. References are never sent to the model. Unlike the debug-event
replay, this runner does not append the application's fragment marker: this
matches the historical 300-case exact-text evaluation.

The expanded local experiment tested these candidates, all still offline-only:

- `subtitle-fidelity-v2-quoted`: four demonstrations plus a quoted input.
- `subtitle-balanced-v3`: eight demonstrations covering negation, roles, tense,
  names and unfinished text.
- `subtitle-glossary-v4`: development-derived terminology only when a whole term
  occurs. With no matching term, the prompt is byte-identical to baseline.
- `subtitle-rules-v5`: general fidelity instructions without demonstrations.

Freeze the new corpus and selected profiles before reading holdout outputs.
Track development, holdout and focused transfer tests separately. A holdout with
no glossary matches tests fallback behavior; it cannot demonstrate transfer of
the glossary. Test ambiguous non-game meanings as well as positive examples.
Public text can be new to this evaluation without being provably absent from
the model's original training data.

The local expanded study found vocabulary improvements but also harmful changes
in other senses of the same phrase. General instructions and larger few-shot
prompts did not outperform the original on the development set. None of these
profiles is imported by the production translator. Exact prompts and raw
outputs remain the source of truth; assistant grades are not independent human
accuracy measurements. Identical requests also showed some variation between
long runs, so small differences require repeated, controlled measurements.

## Dictionary and CPU-draft comparison

`tools/evaluate_hybrid_translation.py` evaluates three frozen candidates on the
original 300-case development corpus: production Q5, FreeDict alternatives + Q5,
and the bundled CPU OPUS-MT draft + Q5. It never changes the live translator.
The FreeDict EN–CS 0.1.3 source archive comes from
[FreeDict downloads](https://freedict.org/downloads/); verify its published SHA512
before extracting `eng-ces.tei`. The dictionary is GPL-2.0-or-later and is not
bundled or published by these experiment scripts.

```powershell
python tools/evaluate_hybrid_translation.py website/translation/quality/data/corpus.jsonl analysis-output/study/hybrid-v1 --dictionary path/to/eng-ces.tei --translation-root path/to/FrameLab/translation --url http://127.0.0.1:12345
python tools/prompt_test_monitor.py analysis-output/study
```

The model/runtime must already be running locally with Q5. The CPU model and
libraries are read from the supplied existing application directory. Output
directories cannot be reused. The manifest freezes source/corpus/dictionary
hashes before evaluation; all six variant orders occur equally often. Each
result includes the full prompt, dictionary candidates or CPU draft, raw Q5
response, preparation time, Q5 API time, and total wall time. Model and dictionary
loading are separate from warmed per-sentence timing. An invalid model response
is retained and marked, not silently removed from quality statistics.

The monitor shows per-variant counts and total/preparation medians. For the local
review workflow, `tools/report_hybrid_translation.py ROOT` combines the prior
`comparison.json`, the new `hybrid-v1` results and fully completed
`hybrid-review-notes.json` into `hybrid-report.html`, `hybrid-comparison.json` and
`hybrid-summary.json`. New output grades require actual semantic review; matching
references or counting changed text is not a substitute. The report refuses an
unfinished review/run or changed candidate/corpus hash. Its link appears in the
monitor once the file exists. Outputs and private review artifacts stay local.

This is one experiment with a specific lookup policy and CUNI prompt format.
CPU OPUS-MT is neural; dictionary lookup is non-neural. A slower, worse hybrid
does not establish that all post-editing systems fail. Compare total pipeline
time, not only the final Q5 call, and include newly introduced errors as well as
fixed ones. Existing development references must not become model hints.

## QuickMT, sentence splitting and CPU-only variants

`tools/evaluate_quickmt_translation.py` compares seven offline profiles on the
same 300-case development corpus: original Q5, OPUS whole/split, QuickMT
whole/split, QuickMT split with beam 1, and QuickMT split followed by Q5. No
profile is wired into the running app. The hybrid deliberately retains the
previous experiment's frozen draft prompt, so a model change is not confounded
with a new prompt. The Q5 request receives the whole source and joined draft.

```powershell
python tools/evaluate_quickmt_translation.py website/translation/quality/data/corpus.jsonl analysis-output/study/quickmt-v1 --resources path/to/quickmt-resources --translation-root path/to/FrameLab/translation --url http://127.0.0.1:12345
python tools/prompt_test_monitor.py analysis-output/study
```

The isolated resources directory contains `model/`, `deps/` and a
`provenance.json` describing the model revision, licence, download URLs, sizes
and SHA256 hashes. Use the CTranslate2 files from
[QuickMT EN-CS](https://huggingface.co/quickmt/quickmt-en-cs/tree/da8d48f5a57bf942411ea9cf7b32ea6995a043c4):
`model.bin`, `config.json`, both vocabulary JSON files, `src.spm.model`,
`tgt.spm.model`, and the model card. The model is CC-BY-4.0; its 409,915,789-byte
model.bin SHA256 is
`08aae7d62fe34352244b8a248555c333aa27c6221b39521a4a1da8ca81a3572e`.
Verify all publisher hashes before using downloaded files. Install pySBD 0.3.4
in the isolated `deps/` directory; do not replace the application's dependencies.
The installed CTranslate2 and SentencePiece are read from `--translation-root`.
No QuickMT executable code or remote model Python code is needed.

Both CPU models use two threads and requested int8 (resolved to int8_float32).
pySBD runs without cleaning; a character-preservation check falls back to the
whole source if splitting would lose non-whitespace characters. Sentences from
one caption are batched, different captions never are. This common splitter
differs from the official QuickMT wrapper's BlingFire. Each case rotates the
seven variant positions, with reversal every seven cases. Loading and warmups
are recorded outside steady-state timing. `--smoke` runs only unrelated probes
for abbreviation, decimal, negation, line-break and fragment behavior.

The local 2026-11-20 run used a Ryzen 9 5900X and RTX 5070 Ti. One warmed pass,
with other PC/GPU activity possible, produced these assistant-reviewed results:

| Variant | Material errors / 300 | Median translation time |
| --- | ---: | ---: |
| Original Q5 | 28 | 272.60 ms |
| OPUS whole | 103 | 109.66 ms |
| OPUS split | 77 | 107.62 ms |
| QuickMT whole | 98 | 124.34 ms |
| QuickMT split, beam 2 | 82 | 117.60 ms |
| QuickMT split, beam 1 | 88 | 100.64 ms |
| QuickMT split → Q5 | 68 | 386.37 ms |

Splitting OPUS fixed 31 material errors and introduced five. Splitting QuickMT
fixed 22 and introduced six. Q5 post-editing fixed 17 QuickMT-split errors and
introduced three, but the hybrid was still worse and slower than original Q5.
Beam 1 saved about 17 ms in median latency while introducing ten errors and
fixing four relative to QuickMT split beam 2. No variant was promoted.

These are semantic assistant grades, not independent human accuracy or a fresh
holdout. The corpus includes game text and difficult idioms. CPU model speed
does not prove better game FPS. Times exclude OCR, display, downloads and cold
model loading. The often-quoted QuickMT 1.35-second result is a different
[1,012-sentence GPU/batch benchmark](https://huggingface.co/quickmt/quickmt-en-cs/commit/600563dcce62c459b96cb640e4a0185529cd8ed0),
not per-caption CPU latency.

After actual semantic review of every new output, the local review workflow is
`python tools/report_quickmt_translation.py ROOT`. It requires the previous
reviewed `comparison.json` and `hybrid-comparison.json`, the `quickmt-v1` run,
and `quickmt-review-notes.json` with completed review coverage, explicit score
overrides and notes. It refuses incomplete runs/reviews or changed frozen
source/corpus hashes. It writes `quickmt-report.html`, `quickmt-summary.json`
and `quickmt-comparison.json`; the monitor links the report automatically.
The report allows any two profiles side by side, filters fixes/regressions,
and shows sentence splits, CPU outputs, exact Q5 prompts and timings. Exact
previously reviewed outputs reuse grades; identical current outputs agree.
Raw outputs, models and private review artifacts stay ignored and local.

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

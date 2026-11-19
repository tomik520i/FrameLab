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

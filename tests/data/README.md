# Subtitle content-filter cases

`subtitle_filter_589.jsonl` is the exact local evaluation set from 2026-11-23:
305 dialogue examples (`KEEP`), 244 unwanted names/signs/plates/noise (`SKIP`),
and 40 intentionally ambiguous strings (`UNCERTAIN`, excluded from accuracy).
Labels were authored for this test, not independently annotated by human raters.
The set measures text filtering, not OCR accuracy or Czech translation quality.
There are 585 distinct case-sensitive strings: the noise controls `qwertyuiop`
and `asdfghjkl` each occur in three subsets. All IDs are unique. The reported
589-case counts retain these four repeated controls; capitalization variants are
intentional because casing affects the filter.

The first 100 entries are English dialogue extracted from The Battle for Wesnoth,
`po/wesnoth-httt/cs.po` at revision
`a015f4a5c629c0eb45ac5f115595113c9880e4af`. Copyright belongs to the respective
Wesnoth contributors; their source text retains GNU GPL v2 or later. Each entry
contains its exact source URL. Upstream attribution/licensing:
[Wesnoth README](https://github.com/wesnoth/wesnoth/blob/a015f4a5c629c0eb45ac5f115595113c9880e4af/README.md#license).
The other 489 examples and labels are authored for FrameLab under this repository's
GPL-3.0 license. No real personal records were collected; name labels are examples.

The original 389-case set has 140 tuning, 209 initially held-out and 40 ambiguous
examples. A further 100-case challenge exposed false rejections of commands without
punctuation and was used to improve guards. The final 100-case generalization set
was introduced after that change. The `split` labels preserve this history; because
later iterations reused earlier results, the combined `holdout` score is **not** an
untouched blind-test score. Do not claim general accuracy or zero possible errors
from zero false rejections on this selected set.

Run from the repository with an app containing the required downloaded models:

```powershell
app/translation/python/python.exe tools/evaluate_integrated_filter.py `
  --app app --dataset tests/data/subtitle_filter_589.jsonl `
  --output results/content-filter --entities
```

Both TranslateGemma 4B Q4_K_M and CUNI-MH 7B Q3_K_M are evaluated sequentially by
default. Every original text, decision, reason, translation, error and timing is
saved as JSONL. Turn on FG/NR in a separate FrameLab instance if measuring load;
the evaluator does not enable those effects itself. Model API residency metadata
is recorded. Preserve app logs, runtime versions and source hashes with results.

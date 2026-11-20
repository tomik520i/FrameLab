# Complete subtitle audit and prompt retest

All **1,747 recorded translation jobs** have an explicit assessment, using **1,584 unique English/Czech pairs** and reusing labels only for 163 exact duplicates. None remains unreviewed. The complete run contains 1,406 jobs without a clear error, 194 minor issues, 111 material errors, and 36 with insufficient context. Without duplicates: 1,277 / 183 / 97 / 27. These are subjective, inspectable single-AI judgments against logged English, not human-validated translation or OCR accuracy.

[Complete run and every judgment](https://opicebot.cz/framelab/runs/2026-11-20-3h/) · [Every prompt comparison, timings and evidence](https://opicebot.cz/framelab/runs/2026-11-20-3h/prompt-test.html)

The original frozen random 300-pair review remains unchanged. Two erroneous severity judgments were corrected explicitly in the expanded review: #1186 is a minor relationship/wording issue; #1577 correctly describes the speaker's troublesome leg when declining to go upstairs. Their corrections are **not** prompt improvements. On the recorded 300 this changes material errors from the historical 24 to 22. Fresh baseline inference produces 21, showing why recorded and rerun results must be separated.

## Controlled comparison within the recorded corpus

CUNI-MH Q5_K_M, existing verified model, separate Ollama process/port/model store. Same generation options, production unfinished-sentence marker, two warmups per profile, balanced profile ordering. The running app and its server/settings were untouched. No desktop capture. Timings cover warmed translation requests, excluding OCR/display, on a GPU shared with uncontrolled background workload.

| Inputs | Prompt | Material errors | Median | P95 |
|---|---|---:|---:|---:|
| 300 development | Original | 21 | 374.1 ms | 919.0 ms |
| 300 development | Original repeated | 21 | 384.2 ms | 976.3 ms |
| 300 development | Added rules | 25 | 373.2 ms | 949.5 ms |
| 300 development | Eight examples | 17 | 388.1 ms | 1,023.7 ms |
| 1,284 validation | Original | 74 | 396.0 ms | 911.5 ms |
| 1,284 validation | Frozen eight examples | 83 | 401.8 ms | 902.3 ms |

All **3,768 test responses** are assessed. Exact matches inherit an explicit judgment; all 600 new unique outputs (184 development, 416 validation) were read and assessed. Queues hid variant names, but some known-error comparisons were inspected with names; this is not fully blind or independent review. No default passing grades.

Examples fixed 9 material development errors and introduced 5. On validation, they fixed 11 and introduced **20**: net **9 more material errors**, with median about **5.8 ms / 1.5% slower**. The validation P95 is slightly lower, so the speed difference is mixed, not a general speedup. Identical baseline prompts differed in text on 16/300 repeated requests; quality counts stayed equal. The remaining 1,284 sources have no exactly identical source string in the development set, but adjacent fragments from the same recording are correlated, so this is not a separate-game benchmark.

**Decision: retain the production prompt.** Extra rules worsened development quality; examples did not generalize. Clear negation/slang failures remain, including the party negation, shots/drinks and pool/smoke idioms. Next work should use independently held-out scenes and reliable dialogue context rather than treating a longer instruction as a guaranteed fix. The prepared app release contains lifecycle, pacing and HUD fixes, without this experimental prompt.

Raw responses, prompts, options, model hash, GPU samples, warmups, pair-level changes and review notes are available with SHA-256. Missing context is never counted as a proven fix. Original OCR precision/recall/CER/WER remain unmeasurable without reference frames/transcripts.

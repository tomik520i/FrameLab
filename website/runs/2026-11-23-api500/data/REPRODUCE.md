# Reproducing the FrameLab 500-entry audit

This is a source/evidence archive, not an application installer. The new API integration is a local experimental build; the public preview.17 executable does not contain it yet. `audit-source.zip` includes the exact current adapter, translation prompts and test scripts. The manifest identifies the base commit and local source hashes. No API key, private account model inventory, NR DLL, or model weights are distributed here.

## Dataset and environment

Use the supplied `dataset500.jsonl` unchanged (500 unique entries). Its manifest pins the upstream Wesnoth source/commit, filtering, seed and hash. Human EN/CS game translations are GPL-2.0-or-later; see COPYING-wesnoth.txt and the linked upstream PO file for translator credits. Code retains the FrameLab LICENSE. Public data may have been in training.

Windows 11, Intel Core i5-13420H, RTX 4050 Laptop 6 GiB. Metrics: Python 3.11, sacrebleu 2.5.1 (chrF++ and corpus BLEU). Exact GPU runtime digest/context/memory are in each local-model meta JSON. Local translation used packaged Ollama/GGUF runtimes and OPUS-MT/ctranslate2 CPU int8 with two threads. Local models run sequentially; cloud and local runs overlapped. No OCR, FG or NR in the main 500-entry benchmark. Do not compare these times directly with earlier FG+NR tests.

## Commands (from a matching local FrameLab source tree)

The full local model runtimes and dependencies must already be installed in APP. Extract the audit-source files over the matching tree, preserving package-relative paths. Use fresh output directories; scripts refuse to overwrite prior paid runs. Copy the frozen dataset and its manifest/licence into BENCH. Configure your own credential through FrameLab's masked API UI (Windows Credential Manager, current Windows user); no key belongs in command arguments.

```powershell
APP/translation/python/python.exe tools/benchmark_subtitles_500.py local --app APP --out BENCH
APP/translation/python/python.exe tools/benchmark_subtitles_500.py api --app APP --out BENCH
python tools/judge_subtitles_500.py --out BENCH
python -m pip install sacrebleu==2.5.1
python tools/render_api500_report.py --run RUN
```

Replace APP, BENCH and RUN with absolute paths; RUN/benchmark500 is BENCH. API benchmark: 500 serial Responses calls, GPT-6 Luna, reasoning none, temperature omitted, no tools/history/images, strict JSON, store=false, no auto retries. The benchmark enforces a USD 1 estimate cap. Judge: GPT-6.1 Terra, none, 500 serial requests containing all four anonymous candidates in seeded random order, USD 2.50 estimate cap. Real account billing can differ. Judge scores are one AI's opinions, not independent human ground truth, and the judge shares a provider with one candidate.

## Known limitations and preserved failure

w500-161 has an invalid-structure API error and an empty translation in the original 500. Its token usage was lost by the first adapter. The later production fix stores usage before parsing, but the benchmark catch path is retained as executed and the missing original usage cannot be reconstructed. A separate successful retry is included only in the diagnostic ledger. No failed output was replaced. The cost ledger is a known subtotal, not exact account credit consumption.

Native verification additionally exercised six actual OCR/API pipeline cases and a visible source window: OCR read an English caption, API translated it, F11 minimized settings while the Czech overlay remained, a click passed through the caption, Ctrl+Alt+F11 restored settings, and custom positioning moved the Czech caption below the original. Console connectivity was not physically tested. The public guide includes real screenshots of this test window.

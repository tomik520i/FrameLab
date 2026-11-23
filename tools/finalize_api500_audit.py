"""Publish only explicit, non-secret benchmark evidence. No credentials or runtime bundle."""
from pathlib import Path
import argparse,hashlib,json,shutil,subprocess,zipfile
ROOT=Path(__file__).resolve().parents[1]
def read(p):return [json.loads(line) for line in p.read_text('utf-8').splitlines()]
def main():
    p=argparse.ArgumentParser();p.add_argument('--run',type=Path,required=True);p.add_argument('--app',type=Path,required=True);a=p.parse_args();run=a.run.resolve();bench=run/'benchmark500';data=ROOT/'website/runs/2026-11-23-api500/data'
    judges=read(bench/'blind-judge.jsonl');assert len(judges)==500
    prices={v['id']:v for v in json.loads((ROOT/'docs/openai-model-profiles.json').read_text('utf-8'))['profiles']}
    def price(model,u):
        q=prices[model];d=u.get('input_tokens_details',{});cached=d.get('cached_tokens',0);written=d.get('cache_write_tokens',0)
        return (max(0,u.get('input_tokens',0)-cached-written)*q['input_usd']+cached*q['cached_usd']+written*q.get('cache_write_usd',q['input_usd'])+u.get('output_tokens',0)*q['output_usd'])/1e6
    sections=[]
    for label,path in [('500 translations',bench/'gpt-6-luna.jsonl'),('500 blinded assessments',bench/'blind-judge.jsonl'),('100 content-filter cases',run/'api-filter100/results.jsonl')]:
        rows=read(path);sections.append({'purpose':label,'cases':len(rows),'requests_with_usage':sum(bool(x.get('usage')) for x in rows),'known_estimated_usd':sum(x.get('estimated_usd',0) for x in rows)})
    smoke=json.loads((run/'api-smoke.json').read_text('utf-8'))
    sections.append({'purpose':'Initial production API smoke test','requests_with_usage':1,'known_estimated_usd':price(smoke['model'],smoke['usage'])})
    probes=json.loads((run/'api-configuration-probes.json').read_text('utf-8'));safe_probes=[]
    for row in probes:
        response=row['response'];safe_probes.append({'model':row['model'],'input':row['input'],'temperature':row['temperature'],'status':response.get('status'),'usage':response.get('usage',{}),'estimated_usd':price(row['model'],response.get('usage',{}))})
    sections.append({'purpose':'Separate retry and model/temperature probes (not substituted into benchmark)','requests_with_usage':len(safe_probes),'known_estimated_usd':sum(x['estimated_usd'] for x in safe_probes)})
    native=read(a.app/'logs/openai-usage.jsonl')
    sections.append({'purpose':'Native OCR/worker integration and live subtitle-only UI test','requests_with_usage':len(native),'known_estimated_usd':sum(price(r['model'],r['usage']) for r in native)})
    ledger={'currency':'USD','pricing':'Standard token rates checked 2026-11-23; not an invoice, excludes taxes and unrelated account activity','sections':sections,'known_estimated_usd':sum(x['known_estimated_usd'] for x in sections),'missing_usage_requests':1,'missing_usage_note':'Original benchmark w500-161 returned invalid structured output. The first adapter discarded its usage before raising; subtotal excludes this one request. Production now records usage before parsing. Separate diagnostic retry succeeded and remains outside the original 500 results.'}
    (data/'cost-ledger.json').write_text(json.dumps(ledger,ensure_ascii=False,indent=2),'utf-8');(run/'cost-ledger.json').write_text(json.dumps(ledger,ensure_ascii=False,indent=2),'utf-8')
    (data/'configuration-probes.json').write_text(json.dumps(safe_probes,ensure_ascii=False,indent=2),'utf-8')
    (data/'native-api-usage.jsonl').write_text(''.join(json.dumps(r)+'\n' for r in native),'utf-8')
    shutil.copy2(run/'api-smoke.json',data/'api-smoke.json')
    # Same exact benchmark/runtime prompts; source-only audit, no model binaries.
    files=['tools/'+n for n in ['benchmark_subtitles_500.py','judge_subtitles_500.py','evaluate_api_filter.py','render_api500_report.py','finalize_api500_audit.py','api500_report.js','openai_translation.py','subtitle_gpu.py','subtitle_languages.py','subtitle_filter.py','subtitle_entities.py','translate_worker.py']]+['docs/openai-model-profiles.json','LICENSE','tests/test_openai_translation.py']
    manifest=[]
    with zipfile.ZipFile(data/'audit-source.zip','w',zipfile.ZIP_DEFLATED) as z:
        for relative in files:
            f=ROOT/relative;z.write(f,relative);manifest.append({'file':relative,'sha256':hashlib.sha256(f.read_bytes()).hexdigest()})
    (data/'audit-source-manifest.json').write_text(json.dumps({'base_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),'local_changes':True,'files':manifest},indent=2),'utf-8')
    doc='''# Reproducing the FrameLab 500-entry audit

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
'''
    (data/'REPRODUCE.md').write_text(doc,'utf-8')
    paths=sorted(x for x in data.iterdir() if x.is_file() and x.name!='SHA256SUMS.txt')
    (data/'SHA256SUMS.txt').write_text(''.join(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+f.name+'\n' for f in paths),'utf-8')
    print(json.dumps(ledger,indent=2))
if __name__=='__main__':main()

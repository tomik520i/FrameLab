"""Repeat the local language diagnostic. No paid API and no automatic downloads.

Example (from the repository root):
  python tools/benchmark_language_lab.py --runtime C:/FrameLab/app/translation --output C:/FrameLab-test

Use an empty output directory. Download both models in FrameLab first, then close
the app for comparable text-only timings. Ratings are NOT copied to a new run.
"""
from pathlib import Path
import argparse,datetime,hashlib,json,platform,subprocess,time
from subtitle_gpu import GpuTranslator

ROOT=Path(__file__).resolve().parents[1]
PAIRS=[('en','cs'),('cs','en'),('en','ja'),('ja','en'),('cs','ja'),('ja','cs'),('en','de'),('de','cs'),('en','es'),('fr','en')]
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--dataset',type=Path,default=ROOT/'website/runs/2026-11-24-language-lab/data/dataset.json')
    parser.add_argument('--compute',choices=['cuda','vulkan','cpu'],default='cuda')
    args=parser.parse_args();out=args.output.resolve()
    if out.exists() and any(out.iterdir()):parser.error('Use an empty output directory; existing evidence is never overwritten.')
    out.mkdir(parents=True,exist_ok=True)
    dataset=json.loads(args.dataset.read_text(encoding='utf-8'));rows=dataset['sentences']
    if len(rows)!=12:parser.error('This diagnostic requires its original 12 sentences.')
    meta=dict(started_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),os=platform.platform(),compute=args.compute,scope='Text only. Close FrameLab for comparable timings; this runner does not control external GPU workloads.',paid_api_requests=0,models=[],dataset_sha256=hashlib.sha256(args.dataset.read_bytes()).hexdigest())
    try:meta['source_commit']=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
    except (OSError,subprocess.SubprocessError):meta['source_commit']='unavailable'
    (out/'dataset.json').write_bytes(args.dataset.read_bytes());done=0
    for model,pairs in [('translategemma-4b-q4km',PAIRS),('cuni-q3km',[('en','cs')])]:
        engine=None
        try:
            start=time.perf_counter();engine=GpuTranslator(args.runtime,model,source='en',target='cs',compute=args.compute)
            options=engine.options()
            if engine.gemma:options.update(stop=['<end_of_turn>','<eos>'],num_predict=384,top_k=1,top_p=1.0)
            device={k:v for k,v in engine.device.items() if k!='id'}
            meta['models'].append(dict(id=model,startup_ms=(time.perf_counter()-start)*1000,device=device,effective_options=options,asset=engine.asset,loaded=engine.loaded_info))
            for source,target in pairs:
                engine.source,engine.target=source,target
                engine.translate(rows[0]['texts'][source],timeout=120)
                for row in rows:
                    start=time.perf_counter();error=None
                    try:value=engine.translate(row['texts'][source],timeout=120)
                    except Exception as ex:value='';error=str(ex)
                    record=dict(id=f'{model}-{source}-{target}-{row["id"]}',sentence_id=row['id'],feature=row['feature'],model=model,source_language=source,target_language=target,source=row['texts'][source],reference=row['texts'][target],translation=value,ms=(time.perf_counter()-start)*1000,error=error,metrics=engine.last_metrics)
                    with (out/'outputs.jsonl').open('a',encoding='utf-8') as f:f.write(json.dumps(record,ensure_ascii=False)+'\n')
                    done+=1
                    (out/'progress.json').write_text(json.dumps(dict(done=done,total=132,model=model,pair=[source,target]),ensure_ascii=False),encoding='utf-8')
                print(f'{done}/132 {model} {source} -> {target}',flush=True)
        finally:
            if engine:engine.close()
            (out/'metadata.json').write_text(json.dumps(meta,ensure_ascii=False,indent=2),encoding='utf-8')
    print('Complete. Review these new outputs independently; do not reuse the published ratings.')
if __name__=='__main__':main()

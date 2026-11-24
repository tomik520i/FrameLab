"""Paired translation-memory experiment. API is translation-only, never a judge.

This runner does not modify application settings. Use an empty output folder.
Local GPU work runs serially; prompts, timings, outputs and failures are retained.
"""
from pathlib import Path
import argparse
import datetime
import hashlib
import json
import platform
import random
import time
from translation_memory import local_prompt,api_payload,VERSION


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--dataset',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--runtime',type=Path);p.add_argument('--model',required=True)
    p.add_argument('--server-config',type=Path)
    p.add_argument('--split',choices=['pilot','heldout'],default='pilot')
    p.add_argument('--modes',nargs='+',choices=['baseline','naive','guarded','timeline'],default=['baseline','naive','guarded'])
    p.add_argument('--key-file',type=Path);p.add_argument('--api-cap-usd',type=float,default=.20)
    a=p.parse_args();out=a.output.resolve()
    if out.exists() and any(out.iterdir()):p.error('Use an empty output directory.')
    out.mkdir(parents=True,exist_ok=True)
    data=json.loads(a.dataset.read_text(encoding='utf-8'));rows=[x for x in data['rows'] if x['split']==a.split]
    rng=random.Random(20261124);jobs=[]
    for row in rows:
        modes=list(a.modes);rng.shuffle(modes)
        jobs.extend((row,mode) for mode in modes)
    engine=None;server=None;cost=0;failed_reserve=0;done=0;api=a.model=='gpt-6-luna'
    meta=dict(model=a.model,split=a.split,started_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
              dataset_sha256=hashlib.sha256(a.dataset.read_bytes()).hexdigest(),memory_version=VERSION,
              memory_code_sha256=hashlib.sha256(Path(__file__).with_name('translation_memory.py').read_bytes()).hexdigest(),
              host=platform.platform(),scope='Text-only replay. Frame generation/enhancer/OCR inference are not running.',
              order='Chronological targets; seeded shuffled arms within each target. Separate run per model.',
              api_price_per_million=dict(input=.10,cached_input=.01,cache_write=.125,output=.50),
              api_cap_usd=a.api_cap_usd if api else 0,judge_api_requests=0)
    meta['modes']=a.modes
    meta['timeline_code_sha256']=hashlib.sha256(Path(__file__).with_name('translation_timeline.py').read_bytes()).hexdigest()
    meta['environment_note']='A model download may run concurrently. These are observed request times, not an isolated network/CPU benchmark.'
    def save():
        (out/'metadata.json').write_text(json.dumps(meta,ensure_ascii=False,indent=2),encoding='utf-8')
        (out/'progress.json').write_text(json.dumps(dict(done=done,total=len(jobs),model=a.model,split=a.split,cost_usd=cost,
                                                       failed_request_reserve_usd=failed_reserve),ensure_ascii=False),encoding='utf-8')
    try:
        if api:
            from openai_translation import request,parse_response,available_models
            if not a.key_file:p.error('An explicitly supplied key file is required for paid translation.')
            key=a.key_file.read_text(encoding='utf-8-sig').strip()
            if a.model not in available_models(key):raise RuntimeError('Requested model is not available in this API project.')
        elif a.server_config:
            from custom_ai_translation import CustomTranslator
            config=json.loads(a.server_config.read_text(encoding='utf-8'))
            server=CustomTranslator(config)
            meta.update(scope='Remote server, text-only API replay. Inference runs outside the laptop; existing server configuration is not changed.',
                        server_config=config,custom_client_sha256=hashlib.sha256(Path(__file__).with_name('custom_ai_translation.py').read_bytes()).hexdigest())
        else:
            from subtitle_gpu import GpuTranslator
            start=time.perf_counter();engine=GpuTranslator(a.runtime,a.model,compute='cuda',allow_offload=a.model=='translategemma-12b-q4km')
            options=engine.options()
            if engine.gemma:options.update(stop=['<end_of_turn>','<eos>'],num_predict=384,top_k=1,top_p=1.0)
            meta.update(startup_ms=(time.perf_counter()-start)*1000,loaded=engine.loaded_info,runtime=engine.runtime_description,
                        asset=engine.asset,effective_options=options,device={k:v for k,v in engine.device.items() if k!='id'})
        save()
        for row,mode in jobs:
            snapshot=None if mode=='baseline' else row['context'][mode]
            from translation_timeline import temporal_prompt
            if server:
                from custom_ai_translation import build_request as custom_request
                prompt=custom_request(server.config,'en','cs',row['source'],context=snapshot)
            else:
                prompt=api_payload(row['source'],snapshot) if api else (temporal_prompt(a.model,row['source'],snapshot) if mode=='timeline' else local_prompt(a.model,row['source'],snapshot))
            value='';error=None;metrics={};usage={};response=None
            # Each UTF-8 byte is a conservative token upper bound for these
            # requests, plus JSON/schema framing. Account for timed-out calls
            # pessimistically; there are no automatic retries.
            reserve=(len(json.dumps(prompt,ensure_ascii=False).encode('utf-8'))+2048)*.125/1e6+768*.5/1e6 if api else 0
            previous_cost=0
            if api:
                for path in out.parent.glob('*/metadata.json'):
                    if path.parent==out:continue
                    prior=json.loads(path.read_text(encoding='utf-8'))
                    if prior.get('model')==a.model:
                        previous_cost+=prior.get('api_cost_usd',0)+prior.get('failed_request_reserve_usd',0)
            if cost+failed_reserve+previous_cost+reserve>a.api_cap_usd:raise RuntimeError('Experiment-wide API translation budget cap reached before request.')
            start=time.perf_counter()
            try:
                if api:
                    response=request('responses',key,prompt);usage=response.get('usage',{})
                    details=usage.get('input_tokens_details',{});cached=details.get('cached_tokens',0)
                    # Non-cached input priced conservatively at cache-write rate
                    # when the service reports writes separately.
                    writes=details.get('cache_creation_tokens',details.get('cache_write_tokens',0))
                    call_cost=((usage.get('input_tokens',0)-cached-writes)*.1+cached*.01+writes*.125+usage.get('output_tokens',0)*.5)/1e6
                    cost+=call_cost
                    parsed,_=parse_response(response);value=parsed['text']
                    metrics=dict(response_id=response.get('id'),actual_model=response.get('model'),status=response.get('status'),cost_usd=call_cost)
                elif server:
                    value=server.translate(row['source'],context=snapshot);metrics=server.last_metrics;usage=server.last_usage
                else:
                    response=engine.api('/api/generate',dict(model=engine.name,prompt=prompt,raw=True,stream=False,keep_alive=-1,options=options),timeout=180)
                    value=response.get('response','').strip()
                    metrics={k:response.get(k) for k in ('total_duration','load_duration','prompt_eval_count','prompt_eval_duration','eval_count','eval_duration','done_reason')}
                    if response.get('done_reason')=='length':error='Output hit the token limit; preserved for inspection.'
            except Exception as exc:
                error=str(exc)
                if api and not usage:failed_reserve+=reserve
            record=dict(id=row['id'],domain=row['domain'],model=a.model,mode=mode,source=row['source'],context=snapshot,
                        prompt=prompt,translation=value,error=error,ms=(time.perf_counter()-start)*1000,metrics=metrics,usage=usage)
            with (out/'outputs.jsonl').open('a',encoding='utf-8') as f:f.write(json.dumps(record,ensure_ascii=False)+'\n')
            done+=1;save()
            if done%3==0:print(f'{done}/{len(jobs)} {a.model} {row["id"]} ${cost:.6f}',flush=True)
            if error and api:
                # Quota/configuration failure is not improved by spending on the
                # remaining dataset. Timeouts are retained without retrying.
                if 'HTTP 4' in error:raise RuntimeError(error)
        meta['completed_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat()
    finally:
        if engine:engine.close()
        meta.update(completed=done,planned=len(jobs),api_cost_usd=cost,failed_request_reserve_usd=failed_reserve)
        save()

if __name__=='__main__':main()

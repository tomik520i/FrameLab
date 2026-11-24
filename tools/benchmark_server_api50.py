"""50 paired, alternating application-level translation requests; no judge calls."""
from pathlib import Path
import argparse,datetime,hashlib,json,math,random,statistics,time
from custom_ai_translation import CustomTranslator
from openai_translation import build_request,request,parse_response,available_models

def stats(xs):
    xs=sorted(xs)
    return dict(n=len(xs),median=statistics.median(xs),p95=xs[math.ceil(len(xs)*.95)-1],mean=statistics.mean(xs),min=min(xs),max=max(xs)) if xs else {}

def main():
    p=argparse.ArgumentParser();p.add_argument('--dataset',type=Path,required=True);p.add_argument('--server-config',type=Path,required=True);p.add_argument('--key-file',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
    if any(out.iterdir()):raise ValueError('Use an empty output directory')
    dataset=json.loads(a.dataset.read_text(encoding='utf-8'));unique={}
    for row in dataset['rows']:
        if row['split']=='heldout' and row['domain']=='game_raw_ocr' and row['id'] not in ['game-1502','game-1731','game-1732','game-1733','game-1734']:unique.setdefault(row['source'],row)
    selected=random.Random(2026112450).sample(list(unique.values()),50)
    config=json.loads(a.server_config.read_text(encoding='utf-8'));config['timeout_s']=12;config['max_tokens']=256
    server=CustomTranslator(config);key=a.key_file.read_text(encoding='utf-8-sig').strip()
    if 'gpt-6-luna' not in available_models(key):raise RuntimeError('Luna is unavailable for this account')
    meta=dict(started_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),cases=[dict(id=x['id'],source=x['source']) for x in selected],dataset_sha256=hashlib.sha256(a.dataset.read_bytes()).hexdigest(),
              order='Pairs interleaved; server first on 25 pairs, OpenAI first on 25. Random seeded source selection. No parallel inference.',
              setup='EN-CS; current caption only; no OCR/FG/NR; temperature 0; thinking off; 256 output tokens; both 12-second timeout. Existing FrameLab adapters: OpenAI Responses JSON versus server Chat Completions plain text. Measures full request, not GPU-only speed.',
              server_config=config,api_model='gpt-6-luna',api_cap_usd=.02,judge_calls=0)
    (out/'metadata.json').write_text(json.dumps(meta,ensure_ascii=False,indent=2),encoding='utf-8')
    records=[];cost=0;reserve=0
    for i,row in enumerate(selected):
        for provider in (['server','openai'] if i%2==0 else ['openai','server']):
            payload=build_request('gpt-6-luna','en','cs',row['source'],effort='none',temperature=0,filtering=False);payload['max_output_tokens']=256
            worst=(len(json.dumps(payload).encode())+2048)*.125/1e6+256*.5/1e6
            if provider=='openai' and cost+reserve+worst>.02:raise RuntimeError('API budget exhausted before next request')
            usage={};metrics={};error=None;translated='';call_cost=0;start=time.perf_counter()
            try:
                if provider=='server':translated=server.translate(row['source']);usage=server.last_usage;metrics=server.last_metrics
                else:
                    response=request('responses',key,payload);usage=response.get('usage',{});details=usage.get('input_tokens_details',{});cached=details.get('cached_tokens',0);writes=details.get('cache_creation_tokens',details.get('cache_write_tokens',0))
                    call_cost=((usage.get('input_tokens',0)-cached-writes)*.10+cached*.01+writes*.125+usage.get('output_tokens',0)*.50)/1e6;cost+=call_cost
                    value,_=parse_response(response);translated=value['text'];metrics=dict(actual_model=response.get('model'),status=response.get('status'))
            except Exception as exc:
                error=str(exc)
                if provider=='openai' and not usage:reserve+=worst
            record=dict(pair=i+1,id=row['id'],source=row['source'],provider=provider,first_in_pair=provider==('server' if i%2==0 else 'openai'),ms=(time.perf_counter()-start)*1000,translation=translated,error=error,usage=usage,metrics=metrics,cost_usd=call_cost)
            records.append(record)
            with (out/'outputs.jsonl').open('a',encoding='utf-8') as f:f.write(json.dumps(record,ensure_ascii=False)+'\n')
            (out/'progress.json').write_text(json.dumps(dict(done=len(records),total=100,cost_usd=cost,failed_request_reserve_usd=reserve)),encoding='utf-8')
            if len(records)%10==0:print(f'{len(records)}/100; API translations ${cost:.6f}',flush=True)
            if error and 'HTTP 4' in error:raise RuntimeError(error)
    wins={'server':0,'openai':0};ratios=[]
    for i in range(1,51):
        pair={r['provider']:r for r in records if r['pair']==i}
        if not any(r['error'] for r in pair.values()):wins['server' if pair['server']['ms']<pair['openai']['ms'] else 'openai']+=1;ratios.append(pair['openai']['ms']/pair['server']['ms'])
    meta.update(completed_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),completed=len(records),api_cost_usd=cost,failed_request_reserve_usd=reserve,
                summary={p:dict(latency_ms=stats([r['ms'] for r in records if r['provider']==p and not r['error']]),errors=sum(bool(r['error']) for r in records if r['provider']==p)) for p in ['server','openai']},pair_wins=wins,paired_speed_ratio=stats(ratios))
    (out/'metadata.json').write_text(json.dumps(meta,ensure_ascii=False,indent=2),encoding='utf-8');print('Completed 50 + 50',flush=True)

if __name__=='__main__':main()

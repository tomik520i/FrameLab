"""The selected model both summarizes and translates. No external summarizer/judge."""
from pathlib import Path
import argparse,datetime,hashlib,json,time
from story_memory import SUMMARY_INSTRUCTION,summary_data,task_prompt,story_context,translation_prompt

def main():
    p=argparse.ArgumentParser();p.add_argument('--dataset',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--runtime',type=Path);p.add_argument('--model',required=True);p.add_argument('--split',choices=['pilot','heldout'],default='pilot');p.add_argument('--server-config',type=Path);p.add_argument('--key-file',type=Path);p.add_argument('--summary-variant',choices=['native_en','plain_cs','fewshot_cs'],default='native_en');a=p.parse_args()
    out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
    if any(out.iterdir()):raise RuntimeError('Use an empty output directory')
    rows=[r for r in json.loads(a.dataset.read_text(encoding='utf-8'))['rows'] if r['split']==a.split]
    meta=dict(model=a.model,split=a.split,started_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),dataset_sha256=hashlib.sha256(a.dataset.read_bytes()).hexdigest(),story_code_sha256=hashlib.sha256(Path(__file__).with_name('story_memory.py').read_bytes()).hexdigest(),
              same_model_summarizer=True,summary_variant=a.summary_variant,summary_instruction=SUMMARY_INSTRUCTION,scope='Text-only diagnostic. Same selected model produces its own unverified summary, then translates current text using that summary and last two original lines. Snapshot experiment, not yet a rolling-memory deployment.',judge_calls=0)
    engine=None;server=None;api=a.model=='gpt-6-luna';cost=0;done=0
    try:
        if api:
            from openai_translation import request,build_request,parse_response
            key=a.key_file.read_text(encoding='utf-8-sig').strip()
        elif a.server_config:
            from custom_ai_translation import CustomTranslator,request as server_request,parse_response as server_parse
            server=CustomTranslator(json.loads(a.server_config.read_text(encoding='utf-8')))
        else:
            from subtitle_gpu import GpuTranslator
            engine=GpuTranslator(a.runtime,a.model,compute='cuda',allow_offload=a.model=='translategemma-12b-q4km')
            meta.update(runtime=engine.runtime_description,loaded=engine.loaded_info)
        for row in rows:
            context=row['context']['timeline'];payload=summary_data(context);summary='';translation='';error=None;usages=[]
            start=time.perf_counter();summary_ms=translation_ms=0;prompts={}
            def account(response):
                nonlocal cost
                usage=response.get('usage',{});usages.append(usage);details=usage.get('input_tokens_details',{});cached=details.get('cached_tokens',0)
                cost+=((usage.get('input_tokens',0)-cached)*.10+cached*.01+usage.get('output_tokens',0)*.50)/1e6
            try:
                if api:
                    if cost+.001>.03:raise RuntimeError('Self-summary API budget exhausted')
                    prompts['summary']={'model':a.model,'store':False,'reasoning':{'effort':'none'},'temperature':0,'max_output_tokens':160,'input':[{'role':'developer','content':SUMMARY_INSTRUCTION},{'role':'user','content':json.dumps(payload,ensure_ascii=False)}]}
                    response=request('responses',key,prompts['summary']);account(response)
                    if response.get('status')!='completed':raise RuntimeError('Summary was incomplete')
                    summary=''.join(x.get('text','') for item in response.get('output',[]) if item.get('type')=='message' for x in item.get('content',[]) if x.get('type')=='output_text').strip()
                elif server:
                    prompts['summary']=dict(server.config['parameters'],model=server.config['model'],stream=False,temperature=0,max_tokens=160,messages=[dict(role='system',content=SUMMARY_INSTRUCTION),dict(role='user',content=json.dumps(payload,ensure_ascii=False))])
                    response=server_request(server.config['url'],'/chat/completions',key=server.key,payload=prompts['summary'],timeout=90);summary=server_parse(response)
                else:
                    prompts['summary']=task_prompt(a.model,SUMMARY_INSTRUCTION,payload,a.summary_variant)
                    options=dict(engine.options(),num_predict=160,top_k=1,top_p=1.,stop=['<end_of_turn>','<eos>','###'])
                    response=engine.api('/api/generate',dict(model=engine.name,prompt=prompts['summary'],raw=True,stream=False,keep_alive=-1,options=options),timeout=180)
                    summary=response.get('response','').strip()
                    if response.get('done_reason')=='length':raise RuntimeError('Summary hit the token limit; not passed to translation')
                summary_ms=(time.perf_counter()-start)*1000
                if not summary or len(summary)>800:raise RuntimeError('Summary empty/oversized; not passed to translation')
                compact=story_context(context,summary);next_start=time.perf_counter()
                if api:
                    prompts['translation']=build_request(a.model,'en','cs',row['source'],effort='none',temperature=0,filtering=False,context=compact)
                    response=request('responses',key,prompts['translation']);account(response);translation=parse_response(response)[0]['text']
                elif server:
                    translation=server.translate(row['source'],context=compact);prompts['translation']=server.last_payload
                else:
                    prompts['translation']=translation_prompt(a.model,row['source'],compact);options=dict(engine.options())
                    if engine.gemma:options.update(stop=['<end_of_turn>','<eos>'],num_predict=384,top_k=1,top_p=1.)
                    response=engine.api('/api/generate',dict(model=engine.name,prompt=prompts['translation'],raw=True,stream=False,keep_alive=-1,options=options),timeout=180)
                    translation=response.get('response','').strip()
                    if response.get('done_reason')=='length':raise RuntimeError('Translation hit token limit')
                translation_ms=(time.perf_counter()-next_start)*1000
            except Exception as exc:error=str(exc)
            record=dict(id=row['id'],model=a.model,source=row['source'],summary_input=payload,summary=summary,translation=translation,error=error,summary_ms=summary_ms,translation_ms=translation_ms,total_ms=(time.perf_counter()-start)*1000,prompts=prompts,usages=usages)
            with (out/'outputs.jsonl').open('a',encoding='utf-8') as f:f.write(json.dumps(record,ensure_ascii=False)+'\n')
            done+=1;(out/'progress.json').write_text(json.dumps(dict(done=done,total=len(rows),cost_usd=cost)),encoding='utf-8');print(f'{done}/{len(rows)} {a.model} {row["id"]} {error or "OK"}',flush=True)
            if api and error:raise RuntimeError(error)
        meta['completed_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat()
    finally:
        if engine:engine.close()
        meta.update(completed=done,planned=len(rows),api_cost_usd=cost);(out/'metadata.json').write_text(json.dumps(meta,ensure_ascii=False,indent=2),encoding='utf-8')

if __name__=='__main__':main()

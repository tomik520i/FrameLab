"""Fixed 300-case translation evaluation and repeated GPU coexistence experiment."""
from pathlib import Path
import argparse,csv,datetime,hashlib,json,random,subprocess,time,urllib.request
parser=argparse.ArgumentParser();parser.add_argument('--quant',choices=['cuni-q3km','cuni-q8'],required=True);parser.add_argument('--app-log',type=Path,required=True);parser.add_argument('--resume-performance',action='store_true');args=parser.parse_args()
R=Path(__file__).resolve().parent;OUT=R/('hymt2' if args.quant=='q4' else 'hymt2-'+args.quant);OUT.mkdir(exist_ok=True);MODEL='framelab-hymt2-local-eval'+('' if args.quant=='q4' else '-'+args.quant);URL='http://127.0.0.1:18783'
CORPUS=Path('H:/FrameLab-work/workspace/work/translation-eval-300/corpus.jsonl')
APPLOG=args.app_log.resolve();assert APPLOG.is_file()
PROMPT='Translate the following text into Czech. Note that you should only output the translated result without any additional explanation:\n\n'
OPTIONS=dict(temperature=0,num_ctx=2048,num_predict=256,num_gpu=99,num_thread=4,repeat_penalty=1.05,seed=20261119,top_k=20,top_p=.6)
if args.quant.startswith('cuni-'):
    OUT=R/args.quant;OUT.mkdir(exist_ok=True);MODEL='framelab-cuni-mh-'+('q8' if args.quant=='cuni-q8' else 'q3km')
    PROMPT='### Instruction:\nTranslate Input from English to Czech\n### Glossary:\n\n### Previous text:\n\n### Input:\n{text}\n### Response:\n'
    OPTIONS['repeat_penalty']=1.0;OPTIONS['stop']=['\n']
cases=[json.loads(x) for x in CORPUS.read_text(encoding='utf-8').splitlines()]
assert len(cases)==300 and len({c['id'] for c in cases})==300
assert not (OUT/'manifest.json').exists(), 'Use a fresh directory; never overwrite a prior run'
opener=urllib.request.build_opener(urllib.request.ProxyHandler({}))
def utc():return datetime.datetime.now(datetime.timezone.utc).isoformat()
def api(path,payload=None):
    req=urllib.request.Request(URL+path,data=None if payload is None else json.dumps(payload).encode('utf-8'),headers={'Content-Type':'application/json'})
    with opener.open(req,timeout=300) as r:return json.load(r)
def snapshot():
    assert time.time()-APPLOG.stat().st_mtime<3, 'Stale FrameLab telemetry'
    lines=APPLOG.read_text(encoding='utf-8').splitlines();keys=lines[0].split(',')
    for line in reversed(lines[1:]):
        vals=line.split(',')
        if len(vals)==len(keys):return dict(utc=utc(),app=dict(zip(keys,map(float,vals))),log_mtime_age_s=time.time()-APPLOG.stat().st_mtime)
    raise RuntimeError('Missing app telemetry')
def request(text):
    rendered=PROMPT.replace('{text}',text) if args.quant.startswith('cuni-') else PROMPT+text
    t=time.perf_counter();response=api('/api/generate',dict(model=MODEL,prompt=rendered,raw=True,stream=False,keep_alive='30m',options=OPTIONS))
    return response,(time.perf_counter()-t)*1000
def unload():
    api('/api/generate',dict(model=MODEL,keep_alive=0))
    for _ in range(20):
        if not api('/api/ps').get('models'):return
        time.sleep(.5)
    raise RuntimeError('Model did not unload')
warmups=[]
def warmup(label):
    response,ms=request('We should get moving before sunrise.')
    state=api('/api/ps');warmups.append(dict(phase=label,response=response,ms=ms,model_state=state,utc=utc()))
    (OUT/'warmups.json').write_text(json.dumps(warmups,ensure_ascii=False,indent=2),encoding='utf-8')
    assert state.get('models') and state['models'][0].get('size_vram',0)>1_000_000_000,state
    assert state['models'][0]['size_vram'] >= state['models'][0]['size'], 'Partial GPU offload; run is invalid'
    print('WARM',label,round(ms),'ms',response.get('response'),flush=True)
phases=[]
def phase(name,seconds=None,selected=None,paced=False):
    start=snapshot();print('PHASE',name,start['utc'],flush=True)
    sampler_log=(OUT/(name+'-sampler.log')).open('w')
    sampler=subprocess.Popen([__import__('sys').executable,str(R/'sample_nvml.py'),name,'--app-log',str(APPLOG),'--seconds','3600','--out',str(OUT/(name+'-gpu.jsonl'))],stdout=sampler_log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    begun=time.monotonic();done=0
    try:
        if selected is None:time.sleep(seconds)
        else:
            with (OUT/(name+'-results.jsonl')).open('w',encoding='utf-8') as out:
                for i,c in enumerate(selected,1):
                    tick=time.monotonic();started_utc=utc();result,ms=request(c['en']);done+=1
                    out.write(json.dumps(dict(id=c['id'],translation=result.get('response',''),ms=ms,utc=started_utc,done_reason=result.get('done_reason'),response=result),ensure_ascii=False)+'\n');out.flush()
                    if i%10==0:print(name,i,'/',len(selected),'last',round(ms),'ms',flush=True)
                    if paced:time.sleep(max(0,2-(time.monotonic()-tick)))
    finally:
        if sampler.poll() is not None:raise RuntimeError('Telemetry sampler stopped; inspect '+str(sampler_log.name))
        sampler.terminate();sampler.wait(timeout=10)
        sampler_log.close()
        end=snapshot();phases.append(dict(name=name,start=start,end=end,duration_s=time.monotonic()-begun,requests=done,paced_interval_s=2 if paced else None,case_ids=[c['id'] for c in selected] if selected else []))
        (OUT/'phases.json').write_text(json.dumps(phases,indent=2),encoding='utf-8')
manifest=dict(started_utc=utc(),model='tencent/Hy-MT2-1.8B-GGUF Q4_K_M',model_sha256='dc5f44fcf1fa496ee7ad725982c0c8c553a4de00259b53af84c4b89fb0c06699',runtime='Ollama v0.40.16 official signed Windows portable',prompt=PROMPT,options=OPTIONS,corpus_sha256=hashlib.sha256(CORPUS.read_bytes()).hexdigest(),case_count=300,protocol='60s unloaded baseline; warmup; 30s resident idle; 300 back-to-back translations; 3 cycles of 60s unloaded baseline and 60 translations spaced at least 2s apart; final 60s unloaded baseline. Old CPU OCR remains active throughout.',notes=['Translation of exact English text, not OCR images. Same frozen corpus as previous CPU evaluation.','No per-case prompt, reference, glossary, preceding dialogue or manual translation corrections provided to model.','Greedy temperature 0 chosen for reproducible subtitles; publisher suggests temperature 0.7. This tests one fixed configuration.','Local API only; no external inference. No FrameLab source/binary or shipped model changed.','GPU measurements are whole-device. Software FPS counters are not a physical panel measurement.'])
if args.quant=='q8':
    manifest['model']='tencent/Hy-MT2-1.8B-GGUF Q8_0';manifest['model_sha256']='5c3fe0b1408a5ceb0143184ef247b11b579c525f4b02b060e6c851bb76fef1a4'
if args.quant=='7b-q3km':
    manifest['model']='mradermacher/Hy-MT2-7B-GGUF Q3_K_M';manifest['model_sha256']='b34a6e0e8f48f61afbe7d25a6df5a0b423a96833108c2750b0a189fa58b1bf28'
    manifest['notes'].append('Community quantization requested by user. Its GGUF eos_token_id is 3 ($), unlike the original tokenizer eos 127960. Correct 7B chat template and explicit stop strings from original generation config were configured; weights and GGUF bytes were not modified. Inspect model-info.json and Modelfile. BF16 was not loaded or evaluated.')
if args.quant.startswith('cuni-'):
    manifest['model']='RichardErkhov/wmt24-cuni_-_CUNI-MH-gguf CUNI-MH.Q3_K_M.gguf';manifest['model_sha256']='4e2c3440516a5340e261f96b664994f79be8835523fd496c1821b31d3a02893f'
    manifest['notes'][2]='Greedy temperature 0 and newline stop follow the original CUNI-MH model card. Exact CUNI Instruction/Glossary/Previous text/Input/Response prompt, empty glossary and history. No chat wrapper. Repetition penalty 1.0 (vLLM default in publisher example), unlike Hy-MT2 recommended 1.05; same 2048 context, 256-token ceiling, 4 CPU threads and requested full GPU offload.'
    manifest['notes'].append('This compares complete model configurations suitable to each model, not only quantization. Original CUNI-MH is Mistral-7B-v0.1 fine-tuned for EN-to-CS; supplied community Q3_K_M conversion is used unmodified. Official prompt source: https://huggingface.co/wmt24-cuni/CUNI-MH . No BF16 or Hy-MT2 7B evaluation was performed.')
if args.quant=='cuni-q8':
    manifest['model']='RichardErkhov/wmt24-cuni_-_CUNI-MH-gguf CUNI-MH.Q8_0.gguf';manifest['model_sha256']='461b6c6e20b0fd5162fc916878b680590b8ee10799fd83c1eae6e7ab87d0f18c'
manifest['notes']=[n for n in manifest['notes'] if not n.startswith('This compares complete model configurations')]
manifest['raw_prompt']=True
manifest['desktop']='RTX 5070 Ti 16 GB; see hardware.json';manifest['app_log']=str(APPLOG)
manifest['notes'].append('Both CUNI quantizations rerun on the same desktop. Prior laptop results retained separately. All extraction and downloads finished before evaluation.')
manifest['instrumentation']='Direct NVML 13.615.71, memory info v2 (used excludes reserved), one sample/second; complete FrameLab CSV rows selected by phase boundaries.'
if args.resume_performance:
    import shutil
    archive=OUT/'interrupted-performance';archive.mkdir(exist_ok=True)
    for p in OUT.iterdir():
        if p.is_file() and (p.name.startswith(('baseline-1','baseline-2','baseline-3','baseline-final','paced-')) or p.name in ['phases.json','warmups.json','manifest.json']):shutil.copy2(p,archive/p.name)
    manifest=json.loads((OUT/'manifest.json').read_text(encoding='utf-8'))
    assert manifest['corpus_sha256']==hashlib.sha256(CORPUS.read_bytes()).hexdigest() and manifest['options']==OPTIONS
    completed=[json.loads(line) for line in (OUT/'quality-300-results.jsonl').read_text(encoding='utf-8').splitlines()]
    assert len(completed)==300 and {x['id'] for x in completed}=={x['id'] for x in cases}
    phases=[p for p in json.loads((OUT/'phases.json').read_text(encoding='utf-8')) if p['name'] in ['baseline-initial','resident-idle','quality-300']]
    assert len(phases)==3 and phases[-1]['requests']==300
    warmups=[p for p in json.loads((OUT/'warmups.json').read_text(encoding='utf-8')) if p['phase']=='quality']
    manifest['performance_resumed_utc']=utc()
    manifest['notes'].append('Quality translation run preserved. Its final part overlapped the start of BF16 download; its stress performance is exploratory, not a clean Q8 throughput benchmark. All three paced performance cycles were restarted after BF16 and 7B Q3_K_M downloads and SHA-256 verification completed; neither additional model was loaded or tested.')
    for filename in ['Hy-MT2-BF16-provenance.json','Hy-MT2-7B-Q3_K_M-provenance.json']:assert (R/filename).exists(),filename
(OUT/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
try:
    if not args.resume_performance:
        unload();phase('baseline-initial',seconds=60)
        warmup('quality');phase('resident-idle',seconds=30)
        phase('quality-300',selected=cases)
    selected=[]
    for group in ['wesnoth','warzone','authored']:selected+=random.Random(20261119).sample([c for c in cases if c['source']==group],20)
    for cycle in range(1,4):
        unload();phase('baseline-'+str(cycle),seconds=60)
        warmup('paced-'+str(cycle));chosen=selected.copy();random.Random(20261119+cycle).shuffle(chosen)
        phase('paced-'+str(cycle),selected=chosen,paced=True)
    unload();phase('baseline-final',seconds=60)
    (OUT/'app-telemetry.csv').write_bytes(APPLOG.read_bytes())
    (OUT/'COMPLETE.json').write_text(json.dumps(dict(utc=utc(),quality_cases=300,paced_requests=180,phases=len(phases)),indent=2),encoding='utf-8')
    print('COMPLETE',flush=True)
finally:unload()

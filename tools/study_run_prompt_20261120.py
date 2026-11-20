"""Isolated Q5 prompt experiment. Never sends inference/control to FrameLab's server.

Phase dev: frozen 300-pair sample, baseline + repeat + two predeclared candidates.
Phase heldout: remaining pairs, fresh baseline + explicitly selected frozen candidate.
All model inputs include the production incomplete-sentence marker.
"""
import argparse, ast, ctypes, hashlib, json, os, random, shutil, socket, subprocess, threading, time, urllib.request
from datetime import datetime,timezone
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
DATA=ROOT/'analysis-output/debug-run-1795196244267574/data'
STUDY=ROOT/'analysis-output/prompt-retune-debug-20261120'
APP=Path('H:/FrameLab-desktop/development/app/translation')
MODEL_SHA='d96594cb51af67f6eaea0bb0bff2bc356375f3e10a8bb2d30af17644d6f86084'
BASE='### Instruction:\nTranslate Input from English to Czech\n### Glossary:\n\n### Previous text:\n\n### Input:\n{text}\n### Response:\n'
OPTIONS=dict(temperature=0,num_ctx=2048,num_predict=256,num_gpu=99,num_thread=4,repeat_penalty=1.0,seed=20261119,top_k=20,top_p=.6,stop=['\n'])
RULES=('Translate Input from English to Czech. Preserve the meaning of every clause, especially negation. '
       'An exclamation does not negate the next clause. Translate slang and idioms by their meaning in context. '
       'Keep names and unfinished sentences. Do not add replies, facts or explanations. Output only the translation.')
# General demonstrations derived from observed error TYPES, not complete test answers.
# The full 300-case set is development data, so any improvement there is NOT held-out evidence.
EXAMPLES=[
 ('No way! They are coming after all.','To snad ne! Nakonec přece jen přijdou.'),
 ('I never said you were wrong.','Nikdy jsem neřekl, že se mýlíš.'),
 ('I will wait outside.','Počkám venku.'),
 ('She ordered a round of shots at the bar.','Objednala v baru rundu panáků.'),
 ('Shots were fired at the truck.','Na náklaďák se střílelo.'),
 ('Are you going to let that slide?','Necháš si to líbit?'),
 ('The guard was carrying a piece.','Strážný měl u sebe zbraň.'),
 ('I only picked up a few things at the store.','Jen jsem v obchodě koupil pár věcí.'),
]
PREFIX='\n'.join(BASE.replace('{text}',en)+cs for en,cs in EXAMPLES)+'\n'
def request_text(source):
    end=source.rstrip(' \t\r\n"\')]').rstrip()
    complete=end.endswith(('.', '?', '!')) and not end.endswith(('...', '…'))
    return source if complete else source+' ...'
def prompt(source,profile):
    template=BASE.replace('Translate Input from English to Czech',RULES) if profile=='rules-v1' else BASE
    return (PREFIX if profile=='examples-v1' else '')+template.replace('{text}',request_text(source))
def utc():return datetime.now(timezone.utc).isoformat()
def save(path,value):
    temp=path.with_suffix(path.suffix+'.tmp');temp.write_text(json.dumps(value,ensure_ascii=False,indent=2),encoding='utf-8');os.replace(temp,path)
def sha(path):
    with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
class GPU:
    def __init__(self):
        self.lib=ctypes.WinDLL(str(Path(os.environ['SystemRoot'])/'System32/nvml.dll'));assert self.lib.nvmlInit_v2()==0
        self.dev=ctypes.c_void_p();assert self.lib.nvmlDeviceGetHandleByIndex_v2(0,ctypes.byref(self.dev))==0
    def sample(self):
        class Mem(ctypes.Structure):_fields_=[('total',ctypes.c_ulonglong),('free',ctypes.c_ulonglong),('used',ctypes.c_ulonglong)]
        class Util(ctypes.Structure):_fields_=[('gpu',ctypes.c_uint),('memory',ctypes.c_uint)]
        mem=Mem();util=Util();power=ctypes.c_uint();temp=ctypes.c_uint()
        self.lib.nvmlDeviceGetMemoryInfo(self.dev,ctypes.byref(mem));self.lib.nvmlDeviceGetUtilizationRates(self.dev,ctypes.byref(util));self.lib.nvmlDeviceGetPowerUsage(self.dev,ctypes.byref(power));self.lib.nvmlDeviceGetTemperature(self.dev,0,ctypes.byref(temp))
        return dict(utc=utc(),free_mib=mem.free/2**20,used_mib=mem.used/2**20,gpu_percent=util.gpu,power_w=power.value/1000,temp_c=temp.value)
class Runtime:
    def __init__(self,folder):
        self.proc=None;self.log=None;self.opener=urllib.request.build_opener(urllib.request.ProxyHandler({}));self.name='codex-debug-retune-q5'
        with socket.socket() as probe:probe.bind(('127.0.0.1',0));self.port=probe.getsockname()[1]
        self.url=f'http://127.0.0.1:{self.port}'
        models=STUDY/'ollama-models';blob=models/'blobs'/('sha256-'+MODEL_SHA);blob.parent.mkdir(parents=True,exist_ok=True)
        if not blob.exists():
            source=ROOT/'analysis-output/claude-translation-study-2026-11-20/ollama-models/blobs'/blob.name
            if not source.exists():source=APP/'models/CUNI-MH.Q5_K_M.gguf'
            shutil.copy2(source,blob)
        assert sha(blob)==MODEL_SHA
        env=os.environ.copy();env.update(OLLAMA_HOST=f'127.0.0.1:{self.port}',OLLAMA_MODELS=str(models),OLLAMA_NUM_PARALLEL='1',OLLAMA_MAX_LOADED_MODELS='1',OLLAMA_FLASH_ATTENTION='1',OLLAMA_CONTEXT_LENGTH='2048',OLLAMA_NO_CLOUD='1',OLLAMA_DEBUG='0',OLLAMA_NOPRUNE='1')
        self.log=(folder/'runtime.log').open('w',encoding='utf-8');self.proc=subprocess.Popen([str(APP/'gpu-runtime/ollama.exe'),'serve'],env=env,stdout=self.log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
        save(folder/'runtime.json',dict(port=self.port,pid=self.proc.pid,models=str(models),env={k:v for k,v in env.items() if k.startswith('OLLAMA_')},model_sha256=MODEL_SHA))
        for _ in range(120):
            if self.proc.poll() is not None:raise RuntimeError('Isolated Ollama exited')
            try:self.version=self.api('/api/version');break
            except OSError:time.sleep(.25)
        else:raise RuntimeError('Isolated runtime did not start')
        self.api('/api/create',dict(model=self.name,files={'CUNI-MH.Q5_K_M.gguf':'sha256:'+MODEL_SHA},template='{{ .Prompt }}{{ .Response }}',parameters={'stop':['\n']},stream=False))
    def api(self,path,payload=None):
        req=urllib.request.Request(self.url+path,data=None if payload is None else json.dumps(payload).encode(),headers={'Content-Type':'application/json'})
        with self.opener.open(req,timeout=90) as r:return json.load(r)
    def translate(self,text,profile):
        full=prompt(text,profile);start=time.perf_counter();result=self.api('/api/generate',dict(model=self.name,prompt=full,raw=True,stream=False,keep_alive=-1,options=OPTIONS))
        wall=(time.perf_counter()-start)*1000
        if result.get('done_reason')=='length':raise RuntimeError('Truncated model output')
        return dict(prompt=full,response=result,translation=result.get('response','').strip(),wall_ms=wall)
    def close(self):
        if self.proc:
            try:self.api('/api/generate',dict(model=self.name,keep_alive=0))
            except Exception:pass
            self.proc.terminate()
            try:self.proc.wait(timeout=8)
            except subprocess.TimeoutExpired:self.proc.kill()
        if self.log:self.log.close()
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--phase',choices=['dev','heldout'],default='dev');ap.add_argument('--candidate',choices=['rules-v1','examples-v1']);a=ap.parse_args()
    if a.phase=='heldout' and not a.candidate:raise ValueError('Select a frozen candidate explicitly')
    folder=STUDY/a.phase;folder.mkdir(parents=True,exist_ok=False)
    all_rows=json.loads((DATA/'translations.json').read_text(encoding='utf-8'));sample=json.loads((DATA/'quality-sample.json').read_text(encoding='utf-8'));sample_ids={r['id'] for r in sample}
    unique={}
    for r in all_rows:unique.setdefault((r['source'],r['translation']),r)
    cases=sample if a.phase=='dev' else [r for r in unique.values() if r['id'] not in sample_ids]
    random.Random(202611202201).shuffle(cases)
    profiles=['baseline','rules-v1','examples-v1','baseline-repeat'] if a.phase=='dev' else ['baseline',a.candidate]
    gpu=GPU();before=gpu.sample();assert before['free_mib']>5700,'Not enough free VRAM for an isolated Q5; app remains untouched.'
    protocol=dict(started_utc=utc(),phase=a.phase,count=len(cases),profiles=profiles,options=OPTIONS,model_sha256=MODEL_SHA,cases=cases,prompts={k:prompt('{SOURCE}',k) for k in profiles},examples=EXAMPLES,source_manifest_sha256=sha(DATA/'source-manifest.json'),sample_sha256=sha(DATA/'quality-sample-plan.json'),gpu_before=before,
        limitations=['Dev sample has been inspected and used to design candidates; it is not a holdout.', 'One GPU shared with a running unchanged app; unrelated load is uncontrolled and recorded.', 'Timings are warmed translation requests only, excluding OCR and display.', 'Exact output matches may reuse an explicit existing judgment; every new output must be reviewed.', 'No inference, model control, settings writes or desktop capture on the application.'])
    save(folder/'manifest.json',protocol);shutil.copy2(__file__,folder/'runner-frozen.py')
    state=dict(phase=a.phase,state='Preparing isolated runtime',completed=0,total=len(cases)*len(profiles),started_utc=utc(),updated_utc=utc(),profiles=profiles)
    save(STUDY/'progress.json',state);rt=None;stop=threading.Event()
    def sampler():
        with (folder/'gpu.jsonl').open('w',encoding='utf-8') as f:
            while not stop.is_set():f.write(json.dumps(gpu.sample())+'\n');f.flush();stop.wait(1)
    thread=threading.Thread(target=sampler,daemon=True);thread.start()
    try:
        rt=Runtime.__new__(Runtime)
        rt.proc=None;rt.log=None
        rt.__init__(folder);warm=[]
        for profile in profiles:
            for text in ['We should leave before sunrise.','The train is arriving at platform five.']:
                warm.append(dict(profile=profile,source=text,**rt.translate(text,profile)))
        save(folder/'warmup.json',warm);loaded=rt.api('/api/ps');save(folder/'loaded.json',loaded)
        assert len(loaded['models'])==1 and loaded['models'][0]['size_vram']>=loaded['models'][0]['size'],'Partial CPU offload makes the comparison invalid'
        with (folder/'results.jsonl').open('w',encoding='utf-8') as f:
            for index,case in enumerate(cases):
                order=profiles[index%len(profiles):]+profiles[:index%len(profiles)]
                if (index//len(profiles))%2:order=list(reversed(order))
                for profile in order:
                    if gpu.sample()['free_mib']<250:raise RuntimeError('VRAM too low; stopping own test to protect the running app')
                    row=dict(id=case['id'],source=case['source'],recorded_translation=case['translation'],profile=profile,utc=utc(),**rt.translate(case['source'],profile))
                    f.write(json.dumps(row,ensure_ascii=False)+'\n');f.flush();state.update(state='Translating',completed=state['completed']+1,updated_utc=utc(),last_id=case['id'],last_profile=profile,last_ms=row['wall_ms']);save(STUDY/'progress.json',state)
                if index%20==0:print(f"{a.phase}: {index+1}/{len(cases)} inputs; {state['completed']}/{state['total']} outputs",flush=True)
        save(folder/'COMPLETE.json',dict(finished_utc=utc(),outputs=state['completed'],loaded=rt.api('/api/ps')));state['state']='Inference complete; quality review pending'
    except BaseException as e:
        save(folder/'FAILED.json',dict(utc=utc(),error=str(e)));state.update(state='Failed',error=str(e));raise
    finally:
        if rt:rt.close()
        stop.set();thread.join(timeout=3);state['updated_utc']=utc();save(STUDY/'progress.json',state)
if __name__=='__main__':main()

"""Serial real-app evaluation. No model answer/reference is injected into OCR.

The same source video is decoded in the real visible application for every arm.
Only the selected model builds its memory. No model judges, no automatic retries.
"""
from pathlib import Path
import argparse,datetime,hashlib,json,random,shutil,subprocess,sys,time
from PIL import Image

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def utc():return datetime.datetime.now(datetime.timezone.utc).isoformat()
def read_lines(p):
 if not p.exists():return []
 out=[]
 for line in p.read_text(encoding='utf-8').splitlines():
  try:out.append(json.loads(line))
  except ValueError:pass # A concurrently written last line may be incomplete.
 return out
def write_json(p,value):
 tmp=p.with_suffix(p.suffix+'.tmp');tmp.write_text(json.dumps(value,ensure_ascii=False,indent=2),encoding='utf-8');tmp.replace(p)
def usage_cost(rows):
 total=0.
 for row in rows:
  u=row.get('usage',{});d=u.get('input_tokens_details',{});c=d.get('cached_tokens',0);w=d.get('cache_creation_tokens',d.get('cache_write_tokens',0));total+=(max(0,u.get('input_tokens',0)-c-w)*.10+c*.01+w*.125+u.get('output_tokens',0)*.50)/1e6
 return total

def main():
 p=argparse.ArgumentParser();p.add_argument('experiment',type=Path);p.add_argument('--app',type=Path,required=True);p.add_argument('--server-config',type=Path,required=True);p.add_argument('--preflight',action='store_true');p.add_argument('--case');p.add_argument('--provider',choices=['openai','custom']);p.add_argument('--mode',choices=['off','selected']);p.add_argument('--publish-progress',action='store_true');a=p.parse_args()
 exp=a.experiment.resolve();app=a.app.resolve();repo=Path(__file__).resolve().parents[1]
 dataset=json.loads((exp/'dataset.json').read_text(encoding='utf-8'));cases={c['id']:c for c in dataset['cases']};mapping=json.loads((exp/'scene-map.json').read_text());config=json.loads(a.server_config.read_text(encoding='utf-8'))
 backups=exp/'settings-before-run';backups.mkdir(exist_ok=True)
 for name in ['subtitle-settings.txt','runtime-session.txt','nr-settings.txt','display-settings.txt','gpu-settings.txt']:
  if (app/name).exists() and not (backups/name).exists():shutil.copy2(app/name,backups/name)
 # Preserve the source-tested v12 baseline. CLI overrides provider and languages.
 original=(backups/'subtitle-settings.txt').read_text(encoding='utf-8').strip()
 if int(original.split()[0])!=12:raise ValueError('Runner requires the inspected v12 source settings baseline; do not guess a migration.')
 labels=['openai-off','openai-selected','custom-off','custom-selected']
 if a.preflight:
  plan=[dict(case=k,provider=provider,mode='selected') for k,provider in [('czech-en-cs-bank','openai'),('multi-ja-cs-code','openai'),('multi-cs-ja-zamek','openai'),('czech-en-cs-bank','custom')]]
 else:
  ordered=list(cases);random.Random(2026112425).shuffle(ordered);plan=[]
  for i,key in enumerate(ordered):
   rotation=labels[i%4:]+labels[:i%4]
   for arm in rotation:
    provider,mode=arm.split('-');plan.append(dict(case=key,provider=provider,mode=mode))
 if a.case:plan=[r for r in plan if r['case']==a.case]
 if a.provider:plan=[r for r in plan if r['provider']==a.provider]
 if a.mode:plan=[r for r in plan if r['mode']==a.mode]
 assert plan
 scope='preflight' if a.preflight else 'runs';base=exp/scope;base.mkdir(exist_ok=True)
 plan_file=exp/(scope+'-plan.json')
 if plan_file.exists():assert json.loads(plan_file.read_text())==plan,'Existing plan differs; use a new experiment directory.'
 else:write_json(plan_file,plan)
 fingerprints={name:sha(app/name) for name in ['FrameLab.exe','translation/translate_worker.py','translation/subtitle_evidence_memory.py','translation/openai_translation.py','translation/custom_ai_translation.py']}
 manifest_path=exp/(scope+'-manifest.json')
 manifest=dict(dataset_sha256=sha(exp/'dataset.json'),fingerprints=fingerprints,scope='Actual visible native application, fullscreen output, video pixels -> OCR -> filter -> selected provider -> rendered captions. Both VFG 2x and NR requested. Preserve actual activation and failed states from logs.',memory='frozen v4 same-model original evidence selection',source_commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip(),pricing=dict(model='gpt-6-luna',input_per_million=.10,cached_input_per_million=.01,cache_write_per_million=.125,output_per_million=.50,source='https://developers.openai.com/api/docs/pricing',checked_utc=utc()),planned_runs=len(plan),planned_target_opportunities=len(plan)*3,judging='Direct Codex review; no paid model judge')
 if manifest_path.exists():assert json.loads(manifest_path.read_text(encoding='utf-8'))['fingerprints']==fingerprints,'Runtime changed inside the experiment.'
 else:write_json(manifest_path,manifest)
 started=time.time();done=0;cost=0.;failures=0
 def progress(current='',records=0,completed=False):
  status=dict(phase=scope,done=done,total=len(plan),current=current,current_worker_records=records,elapsed_s=round(time.time()-started,1),estimated_remaining_s=round((time.time()-started)/done*(len(plan)-done)) if done else None,api_cost_estimate_usd=cost,run_failures=failures,completed=completed,updated_utc=utc(),quality_review='pending',paid_judge_calls=0)
  write_json(exp/'progress.json',status)
  if a.publish_progress and not current:
   subprocess.run(['scp','-q',str(exp/'progress.json'),'tomik520i@192.168.1.175:/var/www/html/framelab/runs/2026-11-24-context75/progress.json'],timeout=25,check=False)
 try:
  for index,item in enumerate(plan):
   case=cases[item['case']];provider=item['provider'];mode=item['mode'];name=f'{index+1:03}-{case["id"]}-{provider}-{mode}';out=base/name
   if (out/'metadata.json').exists():
    previous=json.loads((out/'metadata.json').read_text(encoding='utf-8'))
    if previous.get('completed_utc'):
     done+=1;cost+=previous.get('api_cost_estimate_usd',0);failures+=previous.get('exit_code')!=0;continue
    raise RuntimeError('Unfinished attempt must be audited before resuming: '+name)
   assert all(sha(app/k)==v for k,v in fingerprints.items()),'Runtime changed during main comparison'
   if cost>=5:raise RuntimeError('Batch cost guard reached $5; no further requests sent.')
   out.mkdir();(out/'proofs').mkdir();scene=exp/'scenes'/(mapping[case['id']]+'.mp4');scene_meta=json.loads(scene.with_suffix('.json').read_text(encoding='utf-8'));assert sha(scene)==scene_meta['sha256']
   settings=original.split(' ',10);settings[0]='14';settings[5]='3.5';settings[6]='0.95'
   extra=[str(int(mode=='selected')),str(int(mode=='selected')),json.dumps(config['url']),json.dumps(config['protocol']),json.dumps(config['model']),json.dumps(json.dumps(config['parameters'])),json.dumps(''),'30','256','1','0']
   (app/'subtitle-settings.txt').write_text(' '.join(settings)+' '+' '.join(extra)+'\n',encoding='utf-8')
   duration=case['duration_s']+1
   cmd=[str(app/'FrameLab.exe'),'--video',str(scene),'--subtitles','--source-language',case['source_language'],'--target-language',case['target_language'],'--translation-provider',provider,'--conversation-memory',mode,'--context-trace',str(out/'worker.jsonl'),'--subtitle-trace',str(out/'render.jsonl'),'--api-model','gpt-6-luna','--api-effort','none','--subtitle-filter','2','--fg','--nr','--nr-height','540','--nr-passes','1','--seconds',str(duration),'--debug-seconds',str(duration+4),'--debug-text','--fullscreen-output','--caption-proof-dir',str(out/'proofs'),'--caption-proof-times',','.join(str(x) for x in case['proof_times'])]
   meta=dict(**item,index=index+1,started_utc=utc(),video_sha256=sha(scene),fingerprints=fingerprints,command=cmd,proof_scope='Three scheduled native backbuffer exports before Present; present_success is logged. No reconstructed subtitle or replacement source text.',target_languages=[case['source_language'],case['target_language']])
   write_json(out/'metadata.json',meta);old_debug=set((app/'logs').glob('debug-*'));usage=app/'logs/openai-usage.jsonl';usage_offset=usage.stat().st_size if usage.exists() else 0
   process=subprocess.Popen(cmd,cwd=app);meta['pid']=process.pid;write_json(out/'metadata.json',meta);began=time.time();print('START',name,'PID',process.pid,flush=True)
   while process.poll() is None:
    if time.time()-began>duration+90:process.terminate();meta['runner_error']='Native process exceeded bounded runtime';process.wait(timeout=15);break
    progress(name,len(read_lines(out/'worker.jsonl')));time.sleep(2)
   meta.update(completed_utc=utc(),exit_code=process.returncode,wall_seconds=time.time()-began)
   dirs=set((app/'logs').glob('debug-*'))-old_debug
   if len(dirs)!=1:meta['diagnostic_error']=f'Expected one native log directory; found {len(dirs)}'
   for directory in dirs:
    if directory.is_dir():
     for file in ['events.jsonl','metrics.csv','frames.csv','session.json','summary.json']:
      if (directory/file).exists():shutil.copy2(directory/file,out/file)
   if provider=='openai' and usage.exists():
    with usage.open('rb') as f:f.seek(usage_offset);(out/'api-usage.jsonl').write_bytes(f.read())
   meta['api_cost_estimate_usd']=usage_cost(read_lines(out/'api-usage.jsonl'));cost+=meta['api_cost_estimate_usd'];proofs=[]
   for bmp in sorted((out/'proofs').glob('*.bmp')):
    with Image.open(bmp) as im:
     png=bmp.with_suffix('.png');im.save(png,compress_level=3)
     with Image.open(png) as check:assert im.convert('RGB').tobytes()==check.convert('RGB').tobytes()
     proofs.append(dict(index=int(bmp.stem),width=im.width,height=im.height,bmp_sha256=sha(bmp),png_sha256=sha(png),pixels_sha256=hashlib.sha256(im.convert('RGB').tobytes()).hexdigest(),png=png.name))
   write_json(out/'proof-index.json',proofs);meta['proof_count']=len(proofs);write_json(out/'metadata.json',meta)
   done+=1;failures+=process.returncode!=0;progress();print('DONE',name,process.returncode,'proofs',len(proofs),f'${cost:.5f}',flush=True)
  progress(completed=True)
 finally:
  for file in backups.iterdir():
   if file.is_file():shutil.copy2(file,app/file.name)
 print('Completed',done,'/',len(plan),'native runs. Quality review remains separate.',flush=True)
if __name__=='__main__':main()

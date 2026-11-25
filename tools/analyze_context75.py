"""Measure native event evidence; prepare direct, model-unlabelled Codex review.

No network and no automatic semantic judge. Missing outputs stay in denominators.
Timing alignment subtracts framebuffer readback time from each proof anchor.
"""
from pathlib import Path
import argparse,collections,csv,hashlib,json,math,statistics,unicodedata

def read(p):return json.loads(p.read_text(encoding='utf-8'))
def lines(p):
 if not p.exists():return []
 return [json.loads(x) for x in p.read_text(encoding='utf-8').splitlines() if x.strip()]
def norm(s):return ''.join(c for c in unicodedata.normalize('NFKC',s).casefold() if c.isalnum())
def distance(a,b):
 row=list(range(len(b)+1))
 for i,x in enumerate(a):
  new=[i+1]
  for j,y in enumerate(b):new.append(min(new[-1]+1,row[j+1]+1,row[j]+(x!=y)))
  row=new
 return row[-1]
def stats(values):
 v=sorted(x for x in values if x is not None and math.isfinite(x))
 if not v:return dict(n=0,p50=None,p95=None,mean=None,max=None)
 def quantile(q):
  p=(len(v)-1)*q;lo=int(p);hi=math.ceil(p);return round(v[lo]+(v[hi]-v[lo])*(p-lo),3)
 return dict(n=len(v),p50=quantile(.5),p95=quantile(.95),mean=round(statistics.mean(v),3),max=round(max(v),3))
def review_id(case,phase,source,text):return hashlib.sha256(json.dumps([case,phase,source,text],ensure_ascii=False).encode()).hexdigest()[:16]
def atomic(p,obj):
 t=p.with_suffix(p.suffix+'.tmp');t.write_text(json.dumps(obj,ensure_ascii=False,indent=2),encoding='utf-8');t.replace(p)

def analyze_run(folder,case):
 meta=read(folder/'metadata.json');events=lines(folder/'events.jsonl');workers=lines(folder/'worker.jsonl');proofs=[e for e in events if e['event']=='caption_proof']
 anchors=[e['time_s']-e['video_s']-e['readback_ms']/1000 for e in proofs]
 offset=statistics.median(anchors) if anchors else 0
 timing=dict(video_offset_s=offset,anchor_spread_ms=(max(anchors)-min(anchors))*1000 if anchors else None,anchored=bool(anchors))
 jobs={}
 for e in events:
  if 'job_id' in e:jobs.setdefault(e['job_id'],{})[e['event']]=e
 def job_video(job):
  if 'ocr_submit' not in job:return None
  return job['ocr_submit']['time_s']-offset
 def in_row(job,row):
  t=job_video(job);return t is not None and row['start']<=t<row['end']
 def w_for(job):
  # Context mode has native event IDs. Off mode intentionally lacks them;
  # its native event logs remain authoritative, including cached translations.
  ident=str(job.get('ocr_submit',{}).get('job_id',''))
  return next((w for w in workers if ident and w.get('event_id')==ident),None)
 rows=[]
 for row in case['rows']:
  relevant=[j for j in jobs.values() if in_row(j,row)]
  ocr=[j['ocr_complete'] for j in relevant if 'ocr_complete' in j]
  attempts=[]
  for job in relevant:
   translation=job.get('translation_complete');display=job.get('caption_display');filtered=job.get('subtitle_filtered')
   if not(translation or display or filtered or 'translation_error' in job or 'translation_expired' in job):continue
   source=(translation or display or filtered or job.get('ocr_complete',{})).get('source','')
   attempts.append(dict(job_id=job['ocr_submit']['job_id'],capture_video_s=job_video(job),ocr_source=source,ocr_exact=norm(source)==norm(row['text']),translation=translation.get('translation') if translation else None,translation_ms=translation.get('translation_ms') if translation else None,queue_ms=translation.get('queue_ms') if translation else None,cache_hit=bool(translation and translation.get('cache_hit')),display_video_s=display['time_s']-offset if display else None,capture_to_display_ms=display.get('capture_to_display_ms') if display else None,source_onset_to_display_ms=(display['time_s']-offset-row['start'])*1000 if display else None,displayed_in_window=bool(display and display['time_s']-offset<row['end']),filtered=filtered.get('translation') if filtered else None,error='translation_error' in job,expired='translation_expired' in job,worker=w_for(job)))
  rows.append(dict(**row,ocr_observations=len(ocr),ocr_exact_observations=sum(norm(e.get('source',''))==norm(row['text']) for e in ocr),ocr_sources=list(dict.fromkeys(e.get('source','') for e in ocr)),minimum_normalized_cer=min((distance(norm(e.get('source','')),norm(row['text']))/max(1,len(norm(row['text']))) for e in ocr),default=None),attempts=attempts))
 targets=[]
 for index,row in enumerate(r for r in rows if r['phase'].startswith('target_')):
  attempts=row['attempts'];displayed=[a for a in attempts if a['display_video_s'] is not None];translated=[a for a in attempts if a['translation'] is not None]
  # Keep the first displayed attempt, not the best translation after seeing results.
  first=(displayed or translated or [None])[0]
  proof=next((e for e in proofs if e['proof_index']==index),None)
  phase='B' if row['phase']=='target_b' else 'A'
  def key(source,text):return review_id(case['id'],phase,source,text) if text else None
  targets.append(dict(**row,phase_key=phase,first=first,first_review_id=key(first['ocr_source'],first['translation']) if first else None,proof=proof,proof_review_id=key(proof.get('source',''),proof.get('translation','')) if proof and proof.get('caption_visible') else None,proof_image=f'data/{folder.name}/proofs/{index}.png'))
 metrics=[]
 if (folder/'metrics.csv').exists():
  for r in csv.DictReader((folder/'metrics.csv').open(encoding='utf-8',newline='')):
   metrics.append({k:float(v) for k,v in r.items() if k!='utc' and v})
 metric_keys=['output_submitted_fps','os_confirmed_fps','generated_fps','gpu_percent','gpu_power_w','gpu_temp_c','vram_used_mb','cpu_system_percent','cpu_app_percent','app_working_set_mb','ocr_ms','nr_evaluations_fps','nr_inference_gpu_ms','fg_gpu_ms']
 measured={k:stats(r.get(k) for r in metrics if r.get('time_s',0)>=3) for k in metric_keys}
 counts=collections.Counter(e['event'] for e in events)
 selected=[w for w in workers if w.get('context',{} ) and w['context'].get('memory_policy')]
 updates=[w['memory'] for w in workers if w.get('memory',{}).get('update')]
 latency=dict(translation_ms=stats(e.get('translation_ms') for e in events if e['event']=='translation_complete' and not e.get('cache_hit')),capture_to_display_ms=stats(e.get('capture_to_display_ms') for e in events if e['event']=='caption_display'),memory_ms=stats(m.get('update_ms') for m in updates),proof_readback_ms=stats(e['readback_ms'] for e in proofs))
 actual_settings=next((e['settings'] for e in events if e['event']=='settings'),{})
 return dict(run=folder.name,case=case['id'],provider=meta['provider'],mode=meta['mode'],exit_code=meta['exit_code'],video_sha256=meta['video_sha256'],started_utc=meta['started_utc'],wall_seconds=meta['wall_seconds'],api_cost_estimate_usd=meta.get('api_cost_estimate_usd',0),settings=actual_settings,timing=timing,targets=targets,rows=rows,metrics=measured,latency=latency,event_counts=dict(counts),memory=dict(updates=len(updates),errors=[m['error'] for m in updates if m.get('error')],selected_evidence_requests=len(selected),statuses=list(dict.fromkeys(w.get('status','') for w in workers))),activation=dict(fg_requested=actual_settings.get('fg')==1,nr_requested=actual_settings.get('nr_enabled')==1,nr_completed=counts['nr_complete'],generated_fps_max=max((r.get('generated_fps',0) for r in metrics),default=0),proofs=len(proofs),present_successes=sum(e.get('present_success',0) for e in proofs),fullscreen_samples=sum(r.get('window_fullscreen',0)==1 for r in metrics),samples=len(metrics)))

def main():
 p=argparse.ArgumentParser();p.add_argument('experiment',type=Path);p.add_argument('--scope',default='runs');a=p.parse_args();root=a.experiment
 data=read(root/'dataset.json');cases={c['id']:c for c in data['cases']};out=root/'analysis';out.mkdir(exist_ok=True);runs=[]
 for folder in sorted((root/a.scope).glob('*')):
  if not (folder/'metadata.json').exists():continue
  meta=read(folder/'metadata.json')
  if not meta.get('completed_utc'):continue
  cache=out/(folder.name+'.json')
  if cache.exists():run=read(cache)
  else:run=analyze_run(folder,cases[meta['case']]);atomic(cache,run)
  runs.append(run)
 by_case=collections.defaultdict(list)
 for r in runs:by_case[r['case']].append(r)
 review={};mapping=collections.defaultdict(list)
 for key,case_runs in by_case.items():
  if a.scope=='runs' and len(case_runs)!=4:continue
  c=cases[key]
  for r in case_runs:
   for target in r['targets']:
    for which in ['first','proof']:
     rid=target[which+'_review_id'];v=target[which]
     if not rid:continue
     review[rid]=dict(id=rid,case=key,phase=target['phase_key'],source=c['target'],actual_ocr=v.get('ocr_source',v.get('source','')),translation=v['translation'],context=c['context_b'] if target['phase_key']=='B' else c['context_a'],reference=c['reference_b'] if target['phase_key']=='B' else c['reference_a'],note=c['note'],negative_control=c['negative_control'])
     mapping[rid].append(dict(run=r['run'],phase=target['phase'],kind=which))
 # Stable hashed order hides provider/mode and prevents preferred model ordering.
 review=list(sorted(review.values(),key=lambda x:(x['case'],x['phase'],x['id'])))
 atomic(root/(a.scope+'-analysis.json'),dict(schema=1,completed_runs=len(runs),planned_runs=300 if a.scope=='runs' else 4,runs=runs))
 atomic(root/(a.scope+'-review.json'),review);atomic(root/(a.scope+'-review-key.json'),dict(mapping))
 print(json.dumps(dict(runs=len(runs),complete_cases=sum(len(r)==4 for r in by_case.values()),target_opportunities=sum(len(r['targets']) for r in runs),review_candidates=len(review),api_cost_estimate_usd=sum(r['api_cost_estimate_usd'] for r in runs),activation_failures=[r['run'] for r in runs if not(r['activation']['nr_completed']>0 and r['activation']['generated_fps_max']>0 and r['activation']['proofs']==3)])))
if __name__=='__main__':main()

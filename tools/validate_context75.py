"""Evidence completeness and timing audit. No inference or paid calls."""
from pathlib import Path
import argparse,collections,hashlib,json
p=argparse.ArgumentParser();p.add_argument('experiment',type=Path);p.add_argument('--complete',action='store_true');a=p.parse_args();root=a.experiment
def read(name):return json.loads((root/name).read_text(encoding='utf-8'))
dataset=read('dataset.json');analysis=read('runs-analysis.json');manifest=read('runs-manifest.json');grades=read('grades.json');runs=analysis['runs'];mapping=read('scene-map.json');cases={c['id']:c for c in dataset['cases']}
assert len(cases)==75 and collections.Counter(c['group'] for c in cases.values())=={'czech':25,'multi':50}
assert len(set(mapping.values()))==50
assert hashlib.sha256((root/'dataset.json').read_bytes()).hexdigest()==manifest['dataset_sha256']
seen=set();warnings=[];proofs=0
for r in runs:
 arm=(r['case'],r['provider'],r['mode']);assert arm not in seen;seen.add(arm)
 metadata=read('runs/'+r['run']+'/metadata.json');assert metadata['fingerprints']==manifest['fingerprints']
 scene=read('scenes/'+mapping[r['case']]+'.json');assert r['video_sha256']==scene['sha256']
 assert len(r['targets'])==3
 for i,t in enumerate(r['targets']):
  if t['proof']:
   proofs+=1;q=t['proof'];assert q['proof_index']==i
   if not t['start']<=q['video_s']<t['end']:warnings.append([r['run'],i,'proof outside target interval'])
   if not q['present_success']:warnings.append([r['run'],i,'Present failed'])
  else:warnings.append([r['run'],i,'missing proof'])
  for kind in ['first','proof']:
   rid=t[kind+'_review_id']
   if a.complete and rid:assert rid in grades,(r['run'],i,kind,'unreviewed output')
  if t['first'] and t['first']['display_video_s'] is not None:assert t['first']['capture_to_display_ms']>=0
 if r['settings'].get('multiplier')!=2 or not r['activation']['fg_requested'] or not r['activation']['nr_requested']:warnings.append([r['run'],'settings differ'])
 if r['activation']['nr_completed']==0 or r['activation']['generated_fps_max']<=0:warnings.append([r['run'],'processing activation failed'])
 if r['timing']['anchor_spread_ms'] is not None and r['timing']['anchor_spread_ms']>100:warnings.append([r['run'],'timing anchor spread over 100 ms'])
if a.complete:
 assert len(runs)==300 and len(seen)==300
 assert read('progress.json')['completed']
 for cid in cases:assert all((cid,provider,mode) in seen for provider in ['openai','custom'] for mode in ['off','selected'])
result=dict(completed_runs=len(runs),target_opportunities=len(runs)*3,proofs=proofs,unique_judgements=len(grades),complete=a.complete,warnings=warnings)
(root/'evidence-validation.json').write_text(json.dumps(result,indent=2),encoding='utf-8');print(json.dumps(result))

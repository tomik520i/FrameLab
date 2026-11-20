"""Partition an existing debug analysis by observed configuration changes.
Settings snapshots are sampled, so boundary attribution is approximate (~1 s).
"""
import bisect, csv, json, sys
from collections import defaultdict
from pathlib import Path
from analyze_debug_run import stats

raw, out = map(Path, sys.argv[1:3])
s=json.loads((out/'summary.json').read_text())
events=json.loads((out/'settings-timeline.json').read_text())
keys=['fg','multiplier','nr_enabled','nr_passes']
phases=[]
for e in events:
    cfg={k:e['settings'][k] for k in keys}
    if not phases or cfg != phases[-1]['configuration']:
        if phases: phases[-1]['end_s']=e['time_s']
        phases.append(dict(start_s=e['time_s'] if phases else 0, end_s=s['duration_s'], configuration=cfg))
starts=[p['start_s'] for p in phases]
for p in phases: p.update(duration_s=p['end_s']-p['start_s'],new_content=0,generated=0,latency=[],intervals=[],metrics=[])
for f in raw.glob('frames*.csv'):
    with f.open(encoding='utf-8-sig',newline='') as stream:
        for r in csv.DictReader(stream):
            if not int(r['new_content']):continue
            p=phases[max(0,bisect.bisect_right(starts,float(r['time_s']))-1)]
            p['new_content']+=1;p['generated']+=int(r['generated'])
            if float(r['callback_to_present_ms'])>=0:p['latency'].append(float(r['callback_to_present_ms']))
            # Exclude intervals crossing a configuration boundary from that phase's interval distribution.
            if float(r['time_s'])-float(r['content_interval_ms'])/1000 >= p['start_s'] and float(r['content_interval_ms'])>0:p['intervals'].append(float(r['content_interval_ms']))
metrics=[]
for f in raw.glob('metrics*.csv'):
    with f.open(encoding='utf-8-sig',newline='') as stream: metrics.extend({k:v if k=='utc' else float(v) for k,v in r.items()} for r in csv.DictReader(stream))
metrics.sort(key=lambda r:r['time_s'])
for r in metrics: phases[max(0,bisect.bisect_right(starts,r['time_s'])-1)]['metrics'].append(r)
groups={}
for p in phases:
    key=tuple(p['configuration'].values())
    if key not in groups: groups[key]=dict(configuration=p['configuration'],duration_s=0,new_content=0,generated=0,latency=[],intervals=[],metrics=[])
    g=groups[key]
    for k in ['duration_s','new_content','generated']:g[k]+=p[k]
    for k in ['latency','intervals','metrics']:g[k].extend(p[k])
def finalize(p):
    p['submitted_fps']=p['new_content']/p['duration_s']
    p['callback_to_present_ms']=stats(p.pop('latency'));p['interval_ms']=stats(p.pop('intervals'))
    mm=p.pop('metrics');p['metrics_samples']=len(mm)
    p['sampled_metrics']={k:stats(r[k] for r in mm) for k in ['gpu_percent','gpu_power_w','vram_used_mb','gpu_temp_c','output_submitted_fps','input_fps','changed_fps','os_confirmed_fps']}
for p in phases:finalize(p)
for g in groups.values():finalize(g)
window_groups=[]
for fg in [0,1]:
    for fs in [0,1]:
        rr=[r for r in metrics if r['window_foreground']==fg and r['window_fullscreen']==fs]
        if rr:window_groups.append(dict(foreground=fg,fullscreen=fs,n=len(rr),output_fps=stats(r['output_submitted_fps'] for r in rr),gpu_percent=stats(r['gpu_percent'] for r in rr)))
result=dict(boundary_note='Configuration snapshots are sampled, so assignment near transitions is approximate (~1 s). This is an uncontrolled run, not a matched A/B performance test.',phases=phases,configuration_totals=list(groups.values()),window_observations=window_groups,counter_first={k:metrics[0][k] for k in s['last_counters']},counter_last=s['last_counters'],counter_delta={k:s['last_counters'][k]-metrics[0][k] for k in s['last_counters']})
(out/'phases.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
for g in groups.values():print(g['configuration'],round(g['duration_s'],2),round(g['submitted_fps'],2),round(g['sampled_metrics']['gpu_percent'].get('mean',0),1))
print('Window groups',[(g['foreground'],g['fullscreen'],g['n'],round(g['output_fps']['mean'],2)) for g in window_groups])

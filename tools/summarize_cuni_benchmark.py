"""Summarize completed CUNI coexistence runs, preserving their individual controls."""
from pathlib import Path
import argparse, csv, json, math, statistics


def read_lines(path):
    return [json.loads(line) for line in path.read_text(encoding='utf-8').splitlines()]


def stats(values):
    if not values:
        return None
    ordered=sorted(values)
    return dict(n=len(values),mean=statistics.mean(values),median=statistics.median(values),
                p95=ordered[math.ceil(len(values)*.95)-1],min=min(values),max=max(values))


def summarize(root):
    assert (root/'COMPLETE.json').exists(), 'Only completed runs may be summarized'
    manifest=json.loads((root/'manifest.json').read_text(encoding='utf-8'))
    phases=json.loads((root/'phases.json').read_text(encoding='utf-8'))
    warmups=json.loads((root/'warmups.json').read_text(encoding='utf-8'))
    assert len(warmups)==4
    for warmup in warmups:
        loaded=warmup['model_state']['models']
        assert len(loaded)==1 and loaded[0]['size_vram']>=loaded[0]['size']>1_000_000_000, 'Partial GPU offload'
    assert manifest['raw_prompt'] and manifest['corpus_sha256']=='addef2490cdb7429f7f3cfe2330a1a4178f2ee59a00b7c66afc561ae82c8f5cc'
    telemetry=[{k:float(v) for k,v in row.items()} for row in csv.DictReader((root/'app-telemetry.csv').open()) if all(row.values())]
    results={}
    for phase in phases:
        start=phase['start']['app']['elapsed_s'];end=phase['end']['app']['elapsed_s']
        app=[row for row in telemetry if start+5<=row['elapsed_s']<=end-2]
        gpu=[row for row in read_lines(root/(phase['name']+'-gpu.jsonl')) if 5<=row['elapsed_s']<=phase['duration_s']-2]
        assert len(app)>10 and len(gpu)>10, (phase['name'],len(app),len(gpu))
        assert all(row['log_mtime_age_s']<3 for row in gpu), 'Stale application telemetry'
        entry=dict(duration_s=phase['duration_s'],trim_start_s=5,trim_end_s=2,
                   app={key:stats([row[key] for row in app if row[key]>=0]) for key in
                        ['raw_input_fps','content_submissions_fps','generated_fps','os_observed_content_fps','callback_to_present_mean_ms','vfg_gpu_ms']},
                   gpu={key:stats([row['gpu'][key] for row in gpu]) for key in gpu[0]['gpu']})
        if phase['requests']:
            requests=read_lines(root/(phase['name']+'-results.jsonl'))
            assert len(requests)==phase['requests']
            entry.update(requests=len(requests),latency_ms=stats([r['ms'] for r in requests]),
                         over_2s=sum(r['ms']>2000 for r in requests),
                         empty=sum(not r['translation'].strip() for r in requests),
                         truncated=sum(r.get('done_reason')=='length' for r in requests))
        results[phase['name']]=entry
    comparisons=[]
    for cycle in range(1,4):
        before=results['baseline-'+str(cycle)]
        after=results['baseline-'+str(cycle+1) if cycle<3 else 'baseline-final']
        during=results['paced-'+str(cycle)]
        control=(before['app']['content_submissions_fps']['mean']+after['app']['content_submissions_fps']['mean'])/2
        active=during['app']['content_submissions_fps']['mean']
        comparisons.append(dict(cycle=cycle,control_fps=control,active_fps=active,change_percent=100*(active/control-1)))
    paced=[r for cycle in range(1,4) for r in read_lines(root/f'paced-{cycle}-results.jsonl')]
    quality=read_lines(root/'quality-300-results.jsonl')
    assert len(paced)==180 and len(quality)==300 and len({r['id'] for r in quality})==300
    return dict(model=manifest['model'],model_sha256=manifest['model_sha256'],
                phases=results,paired_cycles=comparisons,paced_latency_ms=stats([r['ms'] for r in paced]),
                paced_over_2s=sum(r['ms']>2000 for r in paced),
                quality_latency_ms=stats([r['ms'] for r in quality]),
                quality_empty=sum(not r['translation'].strip() for r in quality),
                quality_truncated=sum(r.get('done_reason')=='length' for r in quality),
                resident_extra_mib=results['resident-idle']['gpu']['memory.used']['mean']-results['baseline-initial']['gpu']['memory.used']['mean'],
                minimum_free_mib=min(p['gpu']['memory.free']['min'] for p in results.values()),
                notes='Exact-text translations alongside baseline FrameLab CPU OCR, VFG 2x Medium and NR 65%/720p. Whole-device NVML; software content submissions, not physical monitor FPS. Each paced cycle uses its own adjacent unloaded controls. Memory units are MiB.')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    result={name:summarize(args.root/name) for name in ['cuni-q3km','cuni-q8']}
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2),encoding='utf-8')
    for name,row in result.items():
        print(name,json.dumps({k:row[k] for k in ['paced_latency_ms','paced_over_2s','paired_cycles','resident_extra_mib','minimum_free_mib']}))


if __name__=='__main__':
    main()

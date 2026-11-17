"""Publish aggregate live GPU measurements and the exact sampled CSVs."""
from pathlib import Path
import csv, json, shutil, statistics, sys
ROOT=Path(__file__).resolve().parents[1]
source=Path(sys.argv[1]).resolve()
out=ROOT/'docs/performance/2026-11-18'
out.mkdir(parents=True,exist_ok=True)
summary={}
for multiplier in (2,4):
    origin=source/f'{multiplier}x-action'
    target=out/f'{multiplier}x'
    target.mkdir(exist_ok=True)
    meta=json.loads((origin/'measurement.json').read_text(encoding='utf-8-sig'))
    meta['scene']='User-controlled moving PS5 gameplay / camera motion, visually verified before and after sampling; no scripted identical path.'
    meta['limits']=[x for x in meta['limits'] if not x.startswith('Live street scene')]
    meta['limits'].append('One approximately 30-second run per mode; modes are sequential and use different gameplay. No repeated-trial confidence interval.')
    (target/'measurement.json').write_text(json.dumps(meta,indent=2),encoding='utf-8')
    result={'measurement':meta,'gpu':{},'app':{}}
    for filename,group,columns in [
        ('nvidia-smi.csv','gpu',['gpu_percent','memory_controller_percent','vram_mib','temperature_c','power_w','graphics_mhz']),
        ('app-telemetry.csv','app',['raw_input_fps','content_change_fps','content_submissions_fps','generated_fps','os_observed_content_fps','os_observed_generated_fps','vfg_gpu_ms','callback_to_present_mean_ms','gpu_total_percent','vram_total_mb'])]:
        shutil.copy2(origin/filename,target/filename)
        rows=list(csv.DictReader((target/filename).open(encoding='utf-8-sig')))
        for col in columns:
            values=[float(r[col]) for r in rows if float(r[col])>=0]
            result[group][col]={'samples':len(values),'mean':statistics.mean(values),'median':statistics.median(values),'min':min(values),'max':max(values)}
    summary[f'{multiplier}x']=result
(out/'summary.json').write_text(json.dumps(summary,indent=2),encoding='utf-8')
for mode,s in summary.items():
    print(mode)
    for group in ('gpu','app'):
        for k,v in s[group].items():
            print(k,round(v['mean'],3),round(v['min'],3),round(v['max'],3))

"""Same-binary NR order A/B. Hidden window, synthetic input, no screen capture.

GPU totals include other processes; UI-hidden submitted FPS is not monitor FPS.
Use an isolated app directory. Never points at a running user's configuration.
"""
import argparse,csv,json,statistics,subprocess,time
from pathlib import Path

def main():
 p=argparse.ArgumentParser();p.add_argument('app',type=Path);p.add_argument('output',type=Path);p.add_argument('--video',type=Path);args=p.parse_args()
 args.output.mkdir(parents=True,exist_ok=True);results=[]
 source=['--video',str(args.video)] if args.video else ['--demo','--1440']
 for name,mode in [('standard_a','--nr-standard'),('eco_a','--nr-economy'),('eco_b','--nr-economy'),('standard_b','--nr-standard')]:
  old=set((args.app/'logs').glob('debug-*'))
  command=[str(args.app/'FrameLab.exe'),*source,'--fg','--low','--nr',mode,'--nr-height','1440','--nr-passes','1','--no-vsync','--verify-recording','--preview-delay','24','--debug-seconds','24']
  print('START',name,flush=True);(args.output/'status.txt').write_text('Running '+name,encoding='utf-8')
  started=time.time();process=subprocess.run(command,cwd=args.app,timeout=100,creationflags=subprocess.CREATE_NO_WINDOW)
  if process.returncode:raise RuntimeError(f'{name} exit {process.returncode}')
  created=set((args.app/'logs').glob('debug-*'))-old
  if len(created)!=1:raise RuntimeError(f'Expected one debug folder: {created}')
  folder=created.pop();rows=list(csv.DictReader((folder/'metrics.csv').open(encoding='utf-8-sig')))
  rows=[{k:float(v) for k,v in r.items() if k!='utc'} for r in rows if float(r['time_s'])>=8]
  if len(rows)<8:raise RuntimeError(f'{name}: too few warmed samples')
  keys=['output_submitted_fps','generated_fps','nr_evaluations_fps','nr_worker_ms','nr_inference_gpu_ms','fg_gpu_ms','app_latency_mean_ms','app_latency_p95_ms','gpu_percent','gpu_power_w','vram_used_mb','cpu_app_percent']
  summary={k:statistics.mean(r[k] for r in rows) for k in keys}
  frames=list(csv.DictReader((folder/'frames.csv').open(encoding='utf-8-sig')))
  gaps=sorted(float(r['content_interval_ms']) for r in frames if float(r['time_s'])>=8 and float(r['new_content']) and float(r['content_interval_ms'])>0)
  record={'case':name,'command':command,'debug_folder':str(folder),'seconds_wall':time.time()-started,'samples':len(rows),'mean':summary,'frame_gap_ms':{'median':statistics.median(gaps),'p95':gaps[int((len(gaps)-1)*.95)],'max':max(gaps)}}
  results.append(record)
  (args.output/'results.json').write_text(json.dumps({'note':'Hidden app; submitted FPS and callback-to-Present latency, not physical display FPS or complete end-to-end latency. Whole GPU totals include the unchanged production app and other processes. Same input, FG Low 2x, NR 1 pass, 1440p limit. ABBA order; startup excluded.','runs':results},indent=2),encoding='utf-8')
  print(name,json.dumps(summary),flush=True)
 (args.output/'status.txt').write_text('Complete',encoding='utf-8')
if __name__=='__main__':main()

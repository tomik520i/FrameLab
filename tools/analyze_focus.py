"""Summarize observed, explicitly bounded UI phases of the 20 Nov focus check."""
from pathlib import Path
import csv,json,statistics,sys,shutil
ROOT=Path(__file__).resolve().parents[1]
PHASES=[
 ('baseline','foreground','13:30:06','13:30:19',1,0),
 ('baseline','background-visible','13:30:24','13:31:22',0,0),
 ('baseline','explorer-maximized','13:31:28','13:31:47',0,0),
 ('baseline','fullscreen','13:31:54','13:32:43',1,1),
 ('baseline','background-repeat','13:32:50','13:33:03',0,0),
 ('qos','foreground','13:33:35','13:34:24',1,0),
 ('qos','explorer-maximized','13:34:32','13:35:13',0,0),
 ('qos','fullscreen','13:35:22','13:35:44',1,1)]
METRICS=['output_submitted_fps','generated_fps','input_fps','changed_fps','gpu_percent','gpu_power_w','nr_worker_ms','nr_inference_gpu_ms','nr_ready_to_take_ms','presentation_wait_ms','present_cpu_total_ms','loop_gap_max_ms','app_latency_mean_ms','window_client_width','window_client_height']
def main():
 src=ROOT/'analysis-output/publication-preview15/focus-app/logs';out=ROOT/'examples/2026-11-20/focus';out.mkdir(parents=True,exist_ok=True)
 sessions={'baseline':'debug-1795181382180570','qos':'debug-1795181589590906'};rows={}
 for key,name in sessions.items():
  rows[key]=list(csv.DictReader((src/name/'metrics.csv').open()));shutil.copy2(src/name/'metrics.csv',out/(key+'-metrics.csv'))
 results=[]
 for run,label,a,b,foreground,fullscreen in PHASES:
  selected=[r for r in rows[run] if a<=r['utc'][11:19]<=b]
  assert selected and all(int(r['window_foreground'])==foreground and int(r['window_fullscreen'])==fullscreen for r in selected),(run,label)
  results.append({'run':run,'phase':label,'from_utc':'2026-11-20T'+a+'Z','to_utc':'2026-11-20T'+b+'Z','samples':len(selected),'mean':{k:statistics.mean(float(r[k]) for r in selected) for k in METRICS}})
 report={'source':'Original 2560x1440 R pixels from analysis-15s-112499968, seconds 1–4, repeating recorded arrival times; no generated frames used as input.','settings':'NVIDIA VFG 2x Medium; NR 1440p, one pass, 100%, motion on; VSync off; subtitles off; detailed numeric logging on.','display':'2560x1440, 165 Hz; borderless fullscreen and 1464x861 client in windowed mode.','scope':'Short consecutive checks on the user desktop, not randomized performance research. OBS and other desktop apps remained open. Different dashboard/fullscreen display sizes are intentional; source and NR dimensions stay fixed. Excludes startup and transition samples.','qos_variant':'Process-local SetProcessInformation disables EXECUTION_SPEED and IGNORE_TIMER_RESOLUTION power throttling for the test. Default app policy unchanged.','findings':'No severe focus or fullscreen slowdown reproduced in these checks. No material gain from explicit QoS, so not enabled by default. Does not rule out a capture/OCR-specific issue, driver background limiter, minimized-window suspension or other concurrent load. Submission FPS does not prove physical monitor scanout.','phases':results,'references':['https://learn.microsoft.com/en-us/windows/win32/api/timeapi/nf-timeapi-timebeginperiod','https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setprocessinformation']}
 (out/'summary.json').write_text(json.dumps(report,indent=2));print(json.dumps([{'run':r['run'],'phase':r['phase'],'n':r['samples'],'fps':round(r['mean']['output_submitted_fps'],2),'nr_ms':round(r['mean']['nr_worker_ms'],2)} for r in results],indent=2))
if __name__=='__main__':main()

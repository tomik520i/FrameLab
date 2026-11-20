"""Publish numeric diagnostics and explicit scope, never arbitrary log files."""
from pathlib import Path
import csv, json, statistics, math, sys, collections
from datetime import datetime, timezone

def stats(values):
    v=sorted(float(x) for x in values if x is not None and float(x)>=0)
    return {'count':len(v),'mean':statistics.mean(v),'p50':statistics.median(v),'p95':v[min(len(v)-1,math.ceil(len(v)*.95)-1)],'min':v[0],'max':v[-1]} if v else None

def main():
    source,recordings,out=map(Path,sys.argv[1:4]);out.mkdir(parents=True,exist_ok=True)
    session=json.loads((source/'session.json').read_text());result=json.loads((source/'summary.json').read_text())
    rows=list(csv.DictReader((source/'metrics.csv').open()))
    # Metrics contain UTC, monotonic time and numbers; enforce that allowlist.
    for row in rows:
        datetime.fromisoformat(row['utc'].replace('Z','+00:00'))
        for k,v in row.items():
            if k!='utc':float(v)
    with (out/'metrics.csv').open('w',newline='',encoding='utf8') as f:
        w=csv.DictWriter(f,fieldnames=rows[0].keys());w.writeheader();w.writerows(rows)
    counts=collections.Counter();samples=collections.defaultdict(list);settings=[];captions=[]
    numeric_events=[]
    for line in (source/'events.jsonl').open(encoding='utf8'):
        e=json.loads(line);kind=e['event'];counts[kind]+=1
        clean={k:v for k,v in e.items() if isinstance(v,(float,int,bool)) or k in ('utc','event')}
        if kind=='settings':
            s=e['settings'];clean['settings']={k:v for k,v in s.items() if isinstance(v,(float,int,bool,dict)) or k in ('backend',)};settings.append(clean)
        for k in ['worker_ms','inference_gpu_ms','ready_to_take_ms','ocr_ms','queue_ms','translation_ms','capture_to_display_ms']:
            if k in e:samples[kind+'.'+k].append(e[k])
        if kind=='caption_display':
            # Dialogue is the requested evidence; file paths/device identifiers
            # and unrelated log strings are never copied from the session.
            captions.append({k:e[k] for k in ['utc','time_s','job_id','capture_to_display_ms','source','translation'] if k in e})
        numeric_events.append(clean)
    start=datetime.fromisoformat(result['started_utc'].replace('Z','+00:00')).timestamp()
    intervals=[]
    for take in sorted(recordings.glob('analysis-*')):
        config=take/'capture-settings.json'
        if not config.exists():continue
        c=json.loads(config.read_text());a=c['utc_epoch_ms']/1000-start
        duration=json.loads((take/'result.json').read_text())['duration_seconds']
        if 0<=a<=result['duration_s']:intervals.append({'recording_id':take.name,'start_s':a,'end_s':a+duration})
    summary={'session_id':source.name,'started_utc':result['started_utc'],'finished_utc':result['finished_utc'],'duration_s':result['duration_s'],'samples':len(rows),'frame_rows':result['frame_rows'],'dropped_log_rows':result['dropped_log_rows'],'scope':'One user session with menus, gameplay, setup, OCR model warmup, recording and exports. Descriptive telemetry, not a controlled gameplay benchmark. GPU/CPU totals include other apps. Submission and OS counters do not prove physical display FPS. App latency excludes game, capture hardware and monitor.','metric_statistics':{k:stats(r[k] for r in rows) for k in rows[0] if k not in ('utc','time_s')},'event_counts':dict(counts),'event_timings':{k:stats(v) for k,v in samples.items()},'recording_intervals':intervals,'privacy':'Numeric telemetry allowlisted; session paths and device identifiers omitted. Only explicitly requested game caption source/translation text published.'}
    for name,value in [('summary',summary),('settings-timeline',settings),('captions',captions)]:
        (out/f'{name}.json').write_text(json.dumps(value,ensure_ascii=False,indent=2),encoding='utf8')
    with (out/'events-numeric.jsonl').open('w',encoding='utf8') as f:
        for e in numeric_events:f.write(json.dumps(e,ensure_ascii=False)+'\n')
    print(json.dumps({'duration_s':summary['duration_s'],'samples':len(rows),'event_counts':dict(counts),'event_timings':summary['event_timings']},ensure_ascii=False))

if __name__=='__main__':main()

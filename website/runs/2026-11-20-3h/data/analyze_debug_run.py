"""Recompute a stopped FrameLab debug run without trusting sampled summaries.

Usage: python analyze_debug_run.py RAW_DEBUG_FOLDER OUTPUT_FOLDER
No model inference, network, desktop capture or application changes.
"""
from pathlib import Path
from collections import Counter, defaultdict
import argparse, csv, hashlib, json, math, random, statistics, sys

def stats(values):
    a = sorted(float(v) for v in values if math.isfinite(float(v)) and float(v) >= 0)
    if not a: return {'n': 0}
    rank = lambda p: a[max(0, math.ceil(len(a)*p)-1)]
    return dict(n=len(a), mean=statistics.fmean(a), median=rank(.5), p95=rank(.95), p99=rank(.99), min=a[0], max=a[-1])

def main():
    ap=argparse.ArgumentParser();ap.add_argument('source',type=Path);ap.add_argument('output',type=Path);a=ap.parse_args()
    a.output.mkdir(parents=True,exist_ok=True)
    def save(name, data): (a.output/name).write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
    session=json.loads((a.source/'session.json').read_text());original=json.loads((a.source/'summary.json').read_text())
    inventory=[]
    for p in sorted(a.source.iterdir()):
        if p.is_file():
            with p.open('rb') as f: digest=hashlib.file_digest(f,'sha256').hexdigest()
            inventory.append(dict(file=p.name,bytes=p.stat().st_size,sha256=digest))
    save('source-manifest.json',inventory)
    print('Parsing all rotated event files',flush=True)
    counts=Counter();reason=Counter();errors=[];ocr={};translations=[];captions={};settings=[];resets=[];model_events=[]
    nr=defaultdict(list);nonempty_ocr=0;unique_ocr=Counter();line_examples=defaultdict(Counter);total_lines=0;invalid=[]
    files=sorted(a.source.glob('events*.jsonl'),key=lambda p: 0 if p.stem=='events' else int(p.stem.split('-')[-1]))
    for path in files:
        with path.open(encoding='utf-8-sig') as f:
            for number,line in enumerate(f,1):
                try:e=json.loads(line)
                except ValueError:invalid.append([path.name,number]);continue
                kind=e['event'];counts[kind]+=1
                if kind=='ocr_complete':
                    ocr[int(e['job_id'])]=e
                    if e.get('source'):nonempty_ocr+=1;unique_ocr[e['source']]+=1
                elif kind=='translation_complete':translations.append(e)
                elif kind=='caption_display':captions[int(e['job_id'])]=e
                elif kind=='ocr_style_line':
                    r=str(int(e['reason']));reason[r]+=1;line_examples[r][e.get('source','')]+=1;total_lines+=1
                elif kind=='nr_complete':
                    for key in ['worker_ms','submit_to_ready_ms','ready_to_take_ms','prepare_gpu_ms','inference_gpu_ms','finish_gpu_ms']:nr[key].append(e[key])
                elif kind=='settings':settings.append(e)
                elif kind=='subtitle_reset':resets.append(e)
                elif kind.startswith('model_'):model_events.append(e)
                if 'error' in kind or kind.endswith('expired') or kind.endswith('dropped'):errors.append(e)
    print('Joining OCR, translations and displayed captions',flush=True)
    pairs=[]
    for i,e in enumerate(translations,1):
        jid=int(e['job_id']);o=ocr.get(jid,{});c=captions.get(jid)
        pairs.append(dict(id=i,job_id=jid,time_s=e['time_s'],utc=e['utc'],source=e.get('source',''),translation=e.get('translation',''),ocr_source=o.get('source',''),ocr_time_s=o.get('time_s'),ocr_ms=o.get('ocr_ms'),ocr_queue_ms=o.get('queue_ms'),confidence=o.get('confidence'),model=int(e['model']),cache_hit=bool(e['cache_hit']),translation_ms=e['translation_ms'],translation_queue_ms=e['queue_ms'],displayed=bool(c),display_time_s=c['time_s'] if c else None,caption_delay_ms=c['capture_to_display_ms'] if c else None))
    save('translations.json',pairs);save('settings-timeline.json',settings);save('model-events.json',model_events);save('error-and-drop-events.json',errors);save('subtitle-resets.json',resets)
    save('ocr-texts.json',[dict(text=k,observations=v) for k,v in unique_ocr.most_common()])
    save('ocr-filter-examples.json',{k:[dict(text=t,count=n) for t,n in v.most_common()] for k,v in line_examples.items()})
    with (a.output/'ocr-readings.jsonl').open('w',encoding='utf-8') as f:
        for e in ocr.values():f.write(json.dumps(e,ensure_ascii=False)+'\n')
    print('Recomputing frame percentiles',flush=True)
    frame_count=0;new_count=0;gen_count=0;interval=[];latency=[];present=[];long=[]
    bins=defaultdict(lambda:dict(frames=0,generated=0,gaps50=0))
    for path in sorted(a.source.glob('frames*.csv')):
        with path.open(encoding='utf-8-sig',newline='') as f:
            for row in csv.DictReader(f):
                frame_count+=1
                if int(row['new_content']):
                    new_count+=1;gen_count+=int(row['generated']);ms=float(row['content_interval_ms']);delay=float(row['callback_to_present_ms']);bucket=int(float(row['time_s'])//10)
                    bins[bucket]['frames']+=1;bins[bucket]['generated']+=int(row['generated'])
                    if ms>0:interval.append(ms)
                    if delay>=0:latency.append(delay)
                    if ms>50:bins[bucket]['gaps50']+=1
                    if ms>100:long.append({k:row[k] for k in ['utc','time_s','content_interval_ms','callback_to_present_ms','queue','generated']})
                present.append(float(row['present_cpu_ms']))
    metrics=[]
    for path in sorted(a.source.glob('metrics*.csv')):
        with path.open(encoding='utf-8-sig',newline='') as f:
            metrics.extend({k:v if k=='utc' else float(v) for k,v in row.items()} for row in csv.DictReader(f))
    metrics.sort(key=lambda r:r['time_s'])
    sampled={k:stats(r[k] for r in metrics) for k in metrics[0] if k not in ['utc','time_s']}
    # Time-weighted rates and system resource utilization, full session incl. startup.
    weighted={};weights=[metrics[0]['time_s']]+[b['time_s']-a['time_s'] for a,b in zip(metrics,metrics[1:])]
    for key in ['input_fps','changed_fps','output_submitted_fps','os_confirmed_fps','gpu_percent','gpu_power_w','cpu_system_percent','cpu_app_percent']:
        good=[(r[key],dt) for r,dt in zip(metrics,weights) if r[key]>=0 and dt>0]
        weighted[key]=sum(v*dt for v,dt in good)/sum(dt for _,dt in good)
    chart=[]
    for bucket in sorted(bins):
        group=[r for r in metrics if int(r['time_s']//10)==bucket]
        end=min((bucket+1)*10,original['duration_s']);span=end-bucket*10
        chart.append(dict(time_s=bucket*10,end_s=end,output_fps=bins[bucket]['frames']/span,generated_fps=bins[bucket]['generated']/span,gaps_over_50ms=bins[bucket]['gaps50'],**{k:statistics.fmean(r[k] for r in group) if group else None for k in ['gpu_percent','gpu_power_w','app_latency_mean_ms','ocr_busy','translation_busy']}))
    fresh=[p for p in pairs if not p['cache_hit']];cached=[p for p in pairs if p['cache_hit']]
    unique={}
    for p in pairs:unique.setdefault((p['source'],p['translation']),p['id'])
    ids=sorted(unique.values());sample=sorted(random.Random(202611202136).sample(ids,min(300,len(ids))))
    save('quality-sample-plan.json',{'seed':202611202136,'population':'unique (logged source, translation) pairs, first occurrence','population_n':len(ids),'sample_n':len(sample),'ids':sample,'limitation':'Assessment against logged English OCR text, not independently verified source video. Single assistant reviewer; no reference translations.'})
    save('quality-sample.json',[p for p in pairs if p['id'] in set(sample)])
    save('quality-sample.tsv','') if False else None
    with (a.output/'quality-sample.tsv').open('w',encoding='utf-8',newline='') as f:
        w=csv.writer(f,delimiter='\t');w.writerow(['id','source','translation','ocr_source'])
        for p in pairs:
            if p['id'] in set(sample):w.writerow([p['id'],p['source'],p['translation'],p['ocr_source']])
    summary=dict(session_id=session['session_id'],started_utc=session['started_utc'],finished_utc=original['finished_utc'],duration_s=original['duration_s'],metadata=session['metadata'],integrity=dict(source_files=len(inventory),source_bytes=sum(r['bytes'] for r in inventory),invalid_json_rows=invalid,dropped_log_rows=original['dropped_log_rows'],metrics_rows=len(metrics),frame_rows=frame_count,original_frame_rows=original['frame_rows']),event_counts=dict(counts),ocr=dict(readings=len(ocr),nonempty=nonempty_ocr,empty=len(ocr)-nonempty_ocr,unique_nonempty_texts=len(unique_ocr),latency_ms=stats(e['ocr_ms'] for e in ocr.values()),queue_ms=stats(e['queue_ms'] for e in ocr.values()),confidence_all=stats(e['confidence'] for e in ocr.values()),confidence_nonempty=stats(e['confidence'] for e in ocr.values() if e.get('source')),filter_lines=total_lines,filter_reasons=dict(reason)),translation=dict(total=len(pairs),unique_pairs=len(unique),uncached=len(fresh),cache_hits=len(cached),displayed=sum(p['displayed'] for p in pairs),all_ms=stats(p['translation_ms'] for p in pairs),uncached_ms=stats(p['translation_ms'] for p in fresh),cached_ms=stats(p['translation_ms'] for p in cached),queue_ms=stats(p['translation_queue_ms'] for p in pairs),caption_delay_ms=stats(p['caption_delay_ms'] for p in pairs if p['displayed'])),frames=dict(new_content=new_count,generated=gen_count,original=new_count-gen_count,submitted_fps=new_count/original['duration_s'],frame_interval_ms=stats(interval),callback_to_present_ms=stats(latency),present_cpu_ms=stats(present),gap_counts={str(n):sum(x>n for x in interval) for n in [25,33.3333333,50,100,250,500]},long_gaps=long),nr_event_timings={k:stats(v) for k,v in nr.items()},sampled_metrics=sampled,time_weighted=weighted,last_counters={k:metrics[-1][k] for k in ['capture_drops','presentation_drops','ocr_submitted','ocr_completed','ocr_superseded','ocr_errors','translations','translation_cache_hits','translation_errors','translation_drops','captions_shown','caption_drops']},quantile_method='Exact nearest rank on every relevant event/frame; sampled_metrics describe once-per-second observations, not per-job latency.')
    save('summary.json',summary);save('chart.json',chart)
    print(json.dumps({k:summary[k] for k in ['session_id','duration_s','integrity','event_counts','ocr','translation','last_counters']},ensure_ascii=False),flush=True)

if __name__=='__main__':main()

"""Freeze a disjoint confirmation set BEFORE choosing a winning memory strategy."""
import argparse,hashlib,json,random
from pathlib import Path
from translation_timeline import ConversationTimeline

def main():
    p=argparse.ArgumentParser();p.add_argument('--previous',type=Path,required=True);p.add_argument('--stream',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    if a.output.exists():raise ValueError('Validation already frozen')
    root=Path(__file__).resolve().parents[1];old=json.loads(a.previous.read_text(encoding='utf-8'));excluded={r.get('original_translation_job') for r in old['rows'] if r.get('original_translation_job')}
    jobs=json.loads((root/'website/runs/2026-11-20-3h/data/translations.json').read_text(encoding='utf-8'));by_id={r['id']:r for r in jobs};candidates=[]
    # Selection uses job IDs/time continuity, never old Czech translations/ratings.
    for start in range(1,max(by_id)-3,4):
        ids=set(range(start,start+4))
        if ids&excluded or not ids<=set(by_id):continue
        if by_id[start+3]['ocr_time_s']-by_id[start]['ocr_time_s']>55:continue
        candidates.append(start)
    starts=sorted(random.Random(2026112471).sample(candidates,10));targets={n:s for s in starts for n in range(s,s+4)}
    events=[json.loads(l) for l in a.stream.read_text(encoding='utf-8').splitlines()];events=[e for e in events if e['event']!='ocr_style_line' or e.get('reason')!=0]
    timeline=ConversationTimeline(horizon=120);i=0;rows=[]
    for job in sorted((by_id[n] for n in targets),key=lambda r:r['ocr_time_s']):
        now=job['ocr_time_s']
        while i<len(events) and events[i]['time_s']<now:
            e=events[i];i+=1
            if e.get('job_id')==job['job_id']:continue
            if e['event']=='subtitle_reset':timeline.reset()
            else:timeline.observe(e['time_s'],e.get('source',''),event_id=e['stream_id'])
        rows.append(dict(id='validation-'+str(job['id']),scene='validation-window-'+str(targets[job['id']]),source=job['source'],time_s=now,domain='real_ocr',context=timeline.snapshot(now,current_text=job['source'],max_turns=16,max_chars=2400)))
    result=dict(schema=1,seed=2026112471,starts=starts,previous_sha256=hashlib.sha256(a.previous.read_bytes()).hexdigest(),stream_sha256=hashlib.sha256(a.stream.read_bytes()).hexdigest(),scope='40 real OCR targets in 10 four-job windows, disjoint from the 94 earlier memory cases. Frozen before selecting the generated-memory method. Selection based only on seeded IDs and time continuity, not known model output or quality. Not independent games or a human-rated benchmark.',rows=rows)
    a.output.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8');print('Frozen 40 confirmation targets; SHA256',hashlib.sha256(a.output.read_bytes()).hexdigest())

if __name__=='__main__':main()

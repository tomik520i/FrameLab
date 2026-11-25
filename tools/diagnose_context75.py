"""Describe observed failure modes without assigning semantic judgements.

Uses the separately recorded direct reviews. Keeps shutdown OCR cancellations,
missing captions and model mistakes separate. Does not modify measured data.
"""
from pathlib import Path
import argparse,collections,datetime,json
from analyze_context75 import lines,norm,stats


def main():
 p=argparse.ArgumentParser();p.add_argument('experiment',type=Path);a=p.parse_args();root=a.experiment
 read=lambda n:json.loads((root/n).read_text(encoding='utf-8'))
 runs=read('runs-analysis.json')['runs'];grades=read('grades.json');cases={c['id']:c for c in read('dataset.json')['cases']}
 auxiliary=read('auxiliary-activity.json') if (root/'auxiliary-activity.json').exists() else []
 groups=collections.defaultdict(list);out={}
 for r in runs:groups[r['provider']+'-'+r['mode']].append(r)
 for arm,rs in groups.items():
  ts=[t for r in rs for t in r['targets']]
  good=lambda t,k:bool(grades.get(t[k+'_review_id'],{}).get('meaning')==2 and grades[t[k+'_review_id']]['language']>=1)
  notes=collections.Counter(grades[t['proof_review_id']]['note'] if t['proof_review_id'] in grades else 'missing' if t['proof_review_id'] is None else 'unreviewed' for t in ts)
  shutdown=[];during=[];plates=[];ui=[];cue_present=[];errors=[]
  for r in rs:
   c=cases[r['case']]
   for e in lines(root/'runs'/r['run']/'events.jsonl'):
    if e['event']=='ocr_error':
     video_s=e['time_s']-r['timing']['video_offset_s'];(shutdown if video_s>=c['duration_s'] else during).append(dict(run=r['run'],video_s=video_s))
   for row in r['rows']:
    if row['phase']=='filler' and row['text'] in ['AB 123 CD','CLOSE','ZAVŘÍT','閉じる','SCHLIESSEN','CERRAR','FERMER']:
     (plates if row['text']=='AB 123 CD' else ui).append(dict(run=r['run'],ocr_seen=bool(row['ocr_sources']),displayed=any(x['display_video_s'] is not None for x in row['attempts'])))
   b=r['targets'][2];w=(b['first'] or {}).get('worker') or {};history=w.get('context',{}).get('history',[])
   cue_present.append(dict(run=r['run'],both_verbatim_topic_b_lines_in_first_request=all(any(norm(h['source'])==norm(s) for h in history) for s in c['context_b'])))
   errors.extend(dict(run=r['run'],error=e) for e in r['memory']['errors'])
  nuisance=lambda xs:dict(intervals=len(xs),ocr_seen=sum(x['ocr_seen'] for x in xs),displayed=sum(x['displayed'] for x in xs))
  out[arm]=dict(runs=len(rs),targets=len(ts),first_correct=sum(good(t,'first') for t in ts),first_correct_on_time=sum(good(t,'first') and bool(t['first'] and t['first']['displayed_in_window']) for t in ts),proof_correct=sum(good(t,'proof') for t in ts),proof_notes=dict(notes),ocr_targets_with_no_exact_read=sum(not t['ocr_exact_observations'] for t in ts),ocr_targets_never_observed=sum(not t['ocr_observations'] for t in ts),ocr_errors_during_scene=during,ocr_errors_after_scene=shutdown,plate=nuisance(plates),ui_label=nuisance(ui),topic_b_cue_presence=cue_present,memory_errors=errors,readback_ms=stats(t['proof']['readback_ms'] for t in ts if t['proof']),timing_anchor_spread_ms=stats(r['timing']['anchor_spread_ms'] for r in rs))
 result=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),runs=len(runs),arms=out,auxiliary_activity=auxiliary,notes=['Semantic results use direct Codex judgements, never keyword or reference matching.','OCR exact-read diagnostics normalize Unicode, case and punctuation; they are not semantic accuracy.','UI labels can be legitimate commands; their translation is not automatically a false positive.','Post-scene OCR errors are separated because shutdown cancellation can raise them.','Both topic cues present checks verbatim normalized OCR only; a miss does not establish that no equivalent context was supplied.'])
 (root/'diagnostics.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
 print(json.dumps({k:{q:v[q] for q in ['runs','targets','first_correct','first_correct_on_time','proof_correct','proof_notes','ocr_targets_with_no_exact_read','plate','ui_label']} for k,v in out.items()},ensure_ascii=False,indent=2))

if __name__=='__main__':main()

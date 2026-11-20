"""Recompute paired speed/quality comparisons from explicitly assessed outputs."""
import collections,hashlib,json,math,statistics
from pathlib import Path
P=Path('analysis-output/prompt-retune-debug-20261120');D=Path('analysis-output/debug-run-1795196244267574/data')
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def save(p,x):p.write_text(json.dumps(x,ensure_ascii=False,indent=2),encoding='utf-8')
grades={}
for row in read(D/'quality-full-review.json')['rows']+read(P/'dev-review/new-grades.json')+read(P/'heldout-review/new-grades.json'):
 key=(row['source'],row['translation'])
 if key in grades:assert grades[key]['grade']==row['grade'],key
 grades[key]=row
def stats(values):
 a=sorted(values)
 return dict(n=len(a),mean=statistics.mean(a),median=statistics.median(a),p95=a[math.ceil(.95*len(a))-1],p99=a[math.ceil(.99*len(a))-1],max=max(a))
summary={};allrows=[];comparisons=[]
for phase in ['dev','heldout']:
 assert (P/phase/'COMPLETE.json').exists(),phase+' inference incomplete'
 rows=[json.loads(s) for s in (P/phase/'results.jsonl').read_text(encoding='utf-8').splitlines()]
 expected=read(P/phase/'manifest.json')
 assert len(rows)==expected['count']*len(expected['profiles'])
 assert len({(r['id'],r['profile']) for r in rows})==len(rows)
 for r in rows:
  g=grades[(r['source'],r['translation'])] # missing review is a hard failure
  r.update(grade=g['grade'],note_cs=g['note_cs'],phase=phase)
 baseline={r['id']:r for r in rows if r['profile']=='baseline'}
 results={}
 for profile in expected['profiles']:
  a=[r for r in rows if r['profile']==profile]
  results[profile]=dict(count=len(a),grades=dict(collections.Counter(r['grade'] for r in a)),wall_ms=stats([r['wall_ms'] for r in a]),
    prompt_tokens=stats([r['response'].get('prompt_eval_count',0) for r in a]),output_tokens=stats([r['response'].get('eval_count',0) for r in a]),
    changed_text_count=sum(r['translation']!=baseline[r['id']]['translation'] for r in a),
    paired_wall_delta_ms=stats([r['wall_ms']-baseline[r['id']]['wall_ms'] for r in a]),
    major_to_good=[r['id'] for r in a if baseline[r['id']]['grade']=='major' and r['grade']=='good'],
    major_to_minor=[r['id'] for r in a if baseline[r['id']]['grade']=='major' and r['grade']=='minor'],
    introduced_major=[r['id'] for r in a if baseline[r['id']]['grade'] in ('good','minor') and r['grade']=='major'],
    major_to_unjudgeable=[r['id'] for r in a if baseline[r['id']]['grade']=='major' and r['grade']=='unjudgeable'],
    unjudgeable_to_major=[r['id'] for r in a if baseline[r['id']]['grade']=='unjudgeable' and r['grade']=='major'])
  for r in a:
   if profile=='baseline':continue
   b=baseline[r['id']]
   comparisons.append(dict(phase=phase,id=r['id'],source=r['source'],profile=profile,baseline=b['translation'],translation=r['translation'],baseline_grade=b['grade'],grade=r['grade'],baseline_note_cs=b['note_cs'],note_cs=r['note_cs'],baseline_ms=b['wall_ms'],wall_ms=r['wall_ms']))
 allrows.extend(rows);summary[phase]=dict(inputs=expected['count'],outputs=len(rows),profiles=results)
devsources={r['source'] for r in allrows if r['phase']=='dev'}
heldsources={r['source'] for r in allrows if r['phase']=='heldout'}
summary['coverage']=dict(original_jobs=1747,original_unique_pairs=1584,test_responses=len(allrows),test_responses_assessed=len(allrows),unreviewed=0,validation_source_strings_also_in_dev=len(devsources&heldsources))
summary['limitations']=[
 'One AI reviewer; not independent human validation or a reference-translation accuracy metric.',
 'Original 300 cases informed prompt design. Validation used the frozen prompt on the remaining recorded pairs; fragments/near-duplicates from the same run are correlated.',
 'Review queues hid profile names, but some known-error comparisons were inspected with names; not fully blind.',
 'Grade reuse requires an exact English/Czech pair. All new pairs were explicitly assessed; no default passing grades.',
 'Unjudgeable is a substantive context limitation, not an unreviewed record. Major-to-unjudgeable is NOT counted as a demonstrated fix.',
 'Warmed wall time on a GPU shared with the unchanged app; uncontrolled concurrent load. No OCR/display latency or fully end-to-end claim.',
 'Every profile uses the production incomplete-sentence ellipsis marker. Original debug source field excludes that appended marker.',
 'No production prompt, running application settings, application server or public executable changed during this test.']
save(P/'summary.json',summary)
save(P/'comparison.json',comparisons)
save(P/'all-assessed-test-outputs.json',[{k:r[k] for k in ['phase','id','source','profile','translation','grade','note_cs','wall_ms']} for r in allrows])
print(json.dumps(summary,ensure_ascii=False,indent=2))

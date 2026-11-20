"""Exact-match grade reuse only; every new output goes to an explicit review queue."""
import json,hashlib
from pathlib import Path
ROOT=Path('analysis-output/prompt-retune-debug-20261120');phase=ROOT/'dev'
assert (phase/'COMPLETE.json').exists()
rows=[json.loads(x) for x in (phase/'results.jsonl').read_text(encoding='utf-8').splitlines()]
assert len(rows)==1200
known={}
for r in json.loads((ROOT/'full-audit/original-sample-review.json').read_text(encoding='utf-8'))['rows']+json.loads((ROOT/'full-audit/additional-review.json').read_text(encoding='utf-8')):
    known[(r['source'],r['translation'])]=dict(grade=r['grade'],note_cs=r['note_cs'],origin='existing_explicit_review',original_id=r['id'])
unique={}
for r in rows:unique.setdefault((r['source'],r['translation']),r)
new=[r for key,r in unique.items() if key not in known];new.sort(key=lambda r:(r['id'],r['translation']))
blind=[dict(candidate=f'P{i:03}',source_id=r['id'],source=r['source'],translation=r['translation']) for i,r in enumerate(new,1)]
out=ROOT/'dev-review';out.mkdir(exist_ok=True)
(out/'new-candidates.json').write_text(json.dumps(blind,ensure_ascii=False,indent=2),encoding='utf-8')
(out/'inherited.json').write_text(json.dumps([dict(source=k[0],translation=k[1],**v) for k,v in known.items() if k in unique],ensure_ascii=False,indent=2),encoding='utf-8')
(out/'key.json').write_text(json.dumps([dict(candidate=b['candidate'],profiles=sorted({r['profile'] for r in rows if r['source']==b['source'] and r['translation']==b['translation']})) for b in blind],indent=2),encoding='utf-8')
print('Unique outputs',len(unique),'new explicit reviews needed',len(blind),'exact-match reuse',len(unique)-len(blind))

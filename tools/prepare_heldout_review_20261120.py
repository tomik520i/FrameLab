"""Append a stable review queue as inference progresses; never assume a grade."""
import json
from pathlib import Path
P=Path('analysis-output/prompt-retune-debug-20261120');D=Path('analysis-output/debug-run-1795196244267574/data')
O=P/'heldout-review';O.mkdir(exist_ok=True)
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def save(p,x):p.write_text(json.dumps(x,ensure_ascii=False,indent=2),encoding='utf-8')
known={(r['source'],r['translation']):r for r in read(D/'quality-full-review.json')['rows']+read(P/'dev-review/new-grades.json')}
rows=[]
for line in (P/'heldout/results.jsonl').read_text(encoding='utf-8').splitlines():
    try:rows.append(json.loads(line))
    except json.JSONDecodeError:break # Writer may currently be appending the final row.
queue=read(O/'new-candidates.json') if (O/'new-candidates.json').exists() else []
seen={(r['source'],r['translation']) for r in queue}
for r in rows:
    key=(r['source'],r['translation'])
    if key in known or key in seen:continue
    queue.append(dict(candidate=f'H{len(queue)+1:04}',source_id=r['id'],source=r['source'],translation=r['translation']))
    seen.add(key)
save(O/'new-candidates.json',queue)
unique={(r['source'],r['translation']) for r in rows}
save(O/'inherited.json',[dict(source=k[0],translation=k[1],grade=v['grade'],note_cs=v['note_cs'],origin='prior_explicit_exact_pair') for k,v in known.items() if k in unique])
save(O/'key.json',[dict(candidate=b['candidate'],profiles=sorted({r['profile'] for r in rows if (r['source'],r['translation'])==(b['source'],b['translation'])})) for b in queue])
grades=read(O/'new-grades.json') if (O/'new-grades.json').exists() else []
print(json.dumps(dict(outputs=len(rows),new_unique=len(queue),graded=len(grades),remaining=len(queue)-len(grades))))

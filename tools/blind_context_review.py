"""Prepare anonymous, deduplicated translations for DIRECT review (no model calls)."""
import argparse,hashlib,json,random
from pathlib import Path

def main():
    p=argparse.ArgumentParser();p.add_argument('--root',type=Path,required=True);p.add_argument('--stage',choices=['validation','confirmation'],default='validation');a=p.parse_args();root=a.root
    target=root/f'blind-{a.stage}.json';key=root/f'blind-{a.stage}-key.json'
    if target.exists() or key.exists():raise ValueError('Review already frozen')
    data=json.loads((root/f'context-{a.stage}-v1.json').read_text(encoding='utf-8'));runs=[]
    for folder in [f'context-{a.stage}-server',f'context-{a.stage}-luna']:
        meta=json.loads((root/folder/'metadata.json').read_text(encoding='utf-8'))
        assert meta['completed']==meta['planned']==len(data['rows']) and meta.get('completed_utc')
        runs.append({r['id']:r for r in map(json.loads,(root/folder/'outputs.jsonl').read_text(encoding='utf-8').splitlines())})
    review=[];mapping=[];rng=random.Random(2026112482)
    for case in data['rows']:
        grouped={}
        for run in runs:
            row=run[case['id']]
            for mode,out in row['outputs'].items():
                token=(out['text'],out.get('error'))
                grouped.setdefault(token,[]).append(dict(model=row['model'],mode=mode,id=row['id']))
        items=list(grouped.items());rng.shuffle(items);candidates=[]
        for i,((text,error),labels) in enumerate(items):
            label=chr(65+i);candidates.append(dict(candidate=label,text=text,error=error));mapping.append(dict(id=case['id'],candidate=label,labels=labels))
        review.append(dict(id=case['id'],source=case['source'],source_history=[x['source'] for x in case['context']['history']],candidates=candidates))
    target.write_text(json.dumps(review,ensure_ascii=False,indent=2),encoding='utf-8');key.write_text(json.dumps(mapping,indent=2),encoding='utf-8')
    print(len(review),'cases /',sum(len(r['candidates']) for r in review),'unique anonymous outputs. Mapping kept separate.')

if __name__=='__main__':main()

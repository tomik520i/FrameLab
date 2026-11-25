"""Print only unreviewed, model-unlabelled outputs for direct inspection."""
from pathlib import Path
import argparse,collections,json,sys
sys.stdout.reconfigure(encoding='utf-8')
p=argparse.ArgumentParser();p.add_argument('experiment',type=Path);p.add_argument('--limit',type=int,default=60);a=p.parse_args();root=a.experiment
rows=json.loads((root/'runs-review.json').read_text(encoding='utf-8'));grades=json.loads((root/'grades.json').read_text(encoding='utf-8')) if (root/'grades.json').exists() else {};groups=collections.defaultdict(list)
for r in rows:
 if r['id'] not in grades:groups[r['case']].append(r)
count=0
for key,rs in groups.items():
 if count>=a.limit:break
 print('\n'+key+' | '+rs[0]['source'])
 for phase in ['A','B']:
  subset=[r for r in rs if r['phase']==phase]
  if not subset:continue
  print(phase, ' / '.join(subset[0]['context']), '| example:',subset[0]['reference'])
  for r in subset:
   print(r['id'],'|',r['translation'],('| OCR: '+r['actual_ocr']) if r['actual_ocr']!=r['source'] else '')
   count+=1
print('\nPending:',sum(map(len,groups.values())),'in',len(groups),'cases; printed:',count)

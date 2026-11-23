import sys,json,time,argparse
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from subtitle_entities import EntityFilter
from subtitle_filter import precheck

def main():
    p=argparse.ArgumentParser();p.add_argument('--app',type=Path,required=True);p.add_argument('--dataset',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    model=EntityFilter(a.app);a.output.parent.mkdir(parents=True,exist_ok=True)
    with a.output.open('w',encoding='utf-8') as out:
        for i,line in enumerate(a.dataset.read_text('utf-8').splitlines()):
            case=json.loads(line);start=time.perf_counter();evidence=model.evidence(case['text']);decision,reason=precheck(case['text']);row=dict(case,**evidence,ms=(time.perf_counter()-start)*1000,eligible=reason in ('needs_context','speech_shape'),precheck=decision,reason=reason);out.write(json.dumps(row,ensure_ascii=False)+'\n');out.flush()
            if (i+1)%100==0:print(i+1,flush=True)
    rows=[json.loads(x) for x in a.output.read_text('utf-8').splitlines()]
    for threshold in (.9,.95,.98,.995):
        selected=[r for r in rows if r['eligible'] and r['score']>=threshold]
        print(json.dumps(dict(threshold=threshold,rejected=len(selected),false_reject=sum(r['expected']=='KEEP' for r in selected),noise_rejected=sum(r['expected']=='SKIP' for r in selected),ambiguous=sum(r['expected']=='UNCERTAIN' for r in selected),mean_ms=sum(r['ms'] for r in rows)/len(rows))),flush=True)
if __name__=='__main__':main()

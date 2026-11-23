"""100 fixed cases through production local prefilter + API classification/translation."""
from pathlib import Path
import argparse,json,random,sys,time
sys.path.insert(0,str(Path(__file__).resolve().parent))
from openai_translation import OpenAiTranslator,profile
from subtitle_filter import ContentFilter,SKIP_PREFIX

def main():
    p=argparse.ArgumentParser();p.add_argument('--dataset',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
    rows=[json.loads(x) for x in a.dataset.read_text('utf-8').splitlines()];chosen=[]
    for expected,n in [('KEEP',40),('SKIP',40),('UNCERTAIN',20)]:chosen+=random.Random(20261125).sample([r for r in rows if r['expected']==expected],n)
    random.Random(20261125).shuffle(chosen);a.out.mkdir(parents=True,exist_ok=True)
    (a.out/'cases.jsonl').write_text(''.join(json.dumps(r,ensure_ascii=False)+'\n' for r in chosen),'utf-8')
    dest=a.out/'results.jsonl'
    if dest.exists():raise RuntimeError('Refusing to duplicate paid filter run')
    model=OpenAiTranslator(filtering=True);filt=ContentFilter(2,'en');cost=0
    with dest.open('w',encoding='utf-8') as out:
        for i,case in enumerate(chosen):
            start=time.perf_counter();decision,reason=filt.before(case['text']);translated='';usage={};error=''
            try:
                if decision!='SKIP':
                    translated=model.translate(case['text']);usage=model.last_usage
                    if translated.startswith(SKIP_PREFIX):decision,reason='SKIP',translated[len(SKIP_PREFIX):];translated=''
                    else:decision,reason=filt.after(case['text'],translated)
            except Exception as exc:error=str(exc);usage=model.last_usage
            price=profile('gpt-6-luna');details=usage.get('input_tokens_details',{});cached=details.get('cached_tokens',0);written=details.get('cache_write_tokens',0)
            charge=(max(0,usage.get('input_tokens',0)-cached-written)*.1+cached*.01+written*.125+usage.get('output_tokens',0)*.5)/1e6;cost+=charge
            result={**case,'decision':decision,'reason':reason,'translation':translated,'error':error,'ms':(time.perf_counter()-start)*1000,'usage':usage,'estimated_usd':charge}
            out.write(json.dumps(result,ensure_ascii=False)+'\n');out.flush()
            (a.out/'progress.json').write_text(json.dumps({'done':i+1,'total':100,'estimated_usd':cost}),'utf-8')
            if error.startswith(('HTTP 401','HTTP 403','HTTP 429')):raise RuntimeError(error)
    print('Completed 100 filter cases. USD estimate:',cost,flush=True)
if __name__=='__main__':main()

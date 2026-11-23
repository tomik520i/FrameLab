"""Exercise exactly the shipped filter + translation path; retain every raw output."""
import sys,json,time,argparse,statistics,re
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from subtitle_gpu import GpuTranslator
from subtitle_filter import ContentFilter
from evaluate_content_filter import score

def native_request(text,gemma):
    # Match SubtitleSentence::prepare for an isolated case (reset source/history).
    # Gemma bypasses this legacy sentence preparation in SubtitleTranslator.cpp.
    if gemma:return text
    text=re.sub(r'\b(January|February|March|April|May|June|July|August|September|October|November|December)/(?=[0-9])',r'\1 ',text,flags=re.I)
    tail=text.rstrip(' \t\r\n"\')]')
    complete=tail.endswith(('.','?','!','。','！','？','؟','।')) and not tail.endswith(('...','…'))
    return text if complete else text+' ...'

def main():
    p=argparse.ArgumentParser();p.add_argument('--app',type=Path,required=True);p.add_argument('--dataset',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--models',nargs='+',default=['translategemma-4b-q4km','cuni-q3km']);p.add_argument('--entities',action='store_true');a=p.parse_args()
    a.output.mkdir(parents=True,exist_ok=True);cases=[json.loads(x) for x in a.dataset.read_text('utf-8').splitlines()];summary={}
    for model_id in a.models:
        from subtitle_entities import EntityFilter
        entities=EntityFilter(a.app) if a.entities else None
        model=GpuTranslator(a.app/'translation',model_id);filt=ContentFilter(2,'en',model,entities);rows=[]
        (a.output/(model_id+'-meta.json')).write_text(json.dumps(model.api('/api/ps'),indent=2),'utf-8')
        try:
            with (a.output/(model_id+'.jsonl')).open('w',encoding='utf-8') as out:
                for i,case in enumerate(cases):
                    request=native_request(case['text'],model.gemma)
                    start=time.perf_counter();decision,reason=filt.before(case['text']);translation='';error='';filter_ms=(time.perf_counter()-start)*1000
                    try:
                        if decision!='SKIP':
                            translation=model.translate(request);decision,reason=filt.after(case['text'],translation)
                    except Exception as e:error=str(e)
                    row=dict(case,request=request,model=model_id,decision=decision,reason=reason,translation=translation,filter_ms=filter_ms,ms=(time.perf_counter()-start)*1000,valid=not error,error=error)
                    rows.append(row);out.write(json.dumps(row,ensure_ascii=False)+'\n');out.flush()
                    (a.output/'progress.json').write_text(json.dumps(dict(model=model_id,done=i+1,total=len(cases),last=case['id'])),'utf-8')
                    if (i+1)%25==0:print(model_id,i+1,'/',len(cases),flush=True)
            summary[model_id]=dict(all=score(rows),holdout=score([r for r in rows if r['split']=='holdout']),ambiguous_rejected=sum(r['decision']=='SKIP' for r in rows if r['expected']=='UNCERTAIN'),filter_mean_ms=statistics.mean(r['filter_ms'] for r in rows))
            (a.output/'summary.json').write_text(json.dumps(summary,indent=2),'utf-8')
        finally:model.close()
    print(json.dumps(summary,indent=2),flush=True)
if __name__=='__main__':main()

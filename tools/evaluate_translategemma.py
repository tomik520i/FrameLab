"""Local multilingual smoke evaluation; stores outputs and timings, not a quality score."""
from pathlib import Path
import argparse,json,time
from subtitle_gpu import GpuTranslator

EN=[
 'Do not open the red door. We have 5 minutes.',
 'I did not say that Anna stole the key.',
 'Wait here. I will come back for you.',
 'The bridge is gone. We need another way across.',
 'He is not our enemy anymore.',
 'Hold the button for 3 seconds, then release it.',
]
REVERSE=[
 ('de','Öffne nicht die rote Tür. Wir haben 5 Minuten.'),
 ('fr',"N’ouvre pas la porte rouge. Nous avons cinq minutes."),
 ('es','No abras la puerta roja. Tenemos cinco minutos.'),
 ('ja','赤いドアを開けないでください。残り5分です。'),
 ('ru','Не открывай красную дверь. У нас есть пять минут.'),
 ('uk','Не відчиняй червоні двері. У нас є п’ять хвилин.'),
 ('cs','Neotvírej červené dveře. Máme pět minut.'),
]

def main():
 p=argparse.ArgumentParser();p.add_argument('--app',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 cases=[('en',target,text) for target in ('cs','de','fr','es','pl','uk','ja','ar') for text in EN]
 cases += [(source,'en' if source=='cs' else 'cs',text) for source,text in REVERSE]
 cases += [('ja','de','ここで待ってください。私は戻ってきます。')]
 a.output.mkdir(parents=True,exist_ok=True);loaded=time.perf_counter();model=GpuTranslator(a.app/'translation','translategemma-4b-q4km')
 meta=dict(model='translategemma-4b-q4km',load_seconds=time.perf_counter()-loaded,residency=model.api('/api/ps'),case_count=len(cases),note='Integration smoke cases; no human/native-speaker quality score. Times exclude model load and include loopback API overhead.')
 (a.output/'meta.json').write_text(json.dumps(meta,indent=2),'utf-8')
 try:
  with (a.output/'translations.jsonl').open('w',encoding='utf-8') as f:
   for i,(source,target,text) in enumerate(cases):
    model.source,model.target=source,target;start=time.perf_counter();result=model.translate(text)
    row=dict(id=i+1,source_language=source,target_language=target,source=text,translation=result,ms=(time.perf_counter()-start)*1000)
    f.write(json.dumps(row,ensure_ascii=False)+'\n');f.flush();print(f'{i+1}/{len(cases)} {source}>{target} {row["ms"]:.0f} ms',flush=True)
 finally:model.close()

if __name__=='__main__':main()

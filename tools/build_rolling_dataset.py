"""Small exploratory follow-up: causal real OCR windows plus authored controls."""
import argparse, hashlib, json
from pathlib import Path

def main():
    p=argparse.ArgumentParser();p.add_argument('parent',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
    if a.output.exists():raise ValueError('Dataset already exists')
    parent=json.loads(a.parent.read_text(encoding='utf-8'));rows=[]
    for start in (101,1399,1601):
        for index in range(start,start+4):
            r=next(x for x in parent['rows'] if x['id']==f'game-{index}')
            rows.append(dict(id=r['id'],scene=f'real-{start}',source=r['source'],time_s=r['time_s'],context=r['context']['timeline'],domain='real_ocr',note='Previously inspected diagnostic window, not a fresh held-out sample. Unknown speakers.'))
    scenarios=[('river',[
        (['The river is rising. The road bridge has collapsed.','We need to cross the river on foot.'],'The bank is too steep.','bank means river bank'),
        (['The rain is getting worse.','I am hungry.','AB 123 CD'],'We cannot use the bridge.','bridge remains collapsed; plate is noise'),
        (['Do you have any food?','I could eat a whole pizza.'],'We need another crossing.','river crossing despite unrelated food chatter'),
        (['My boots are wet.','That song is annoying.'],'Try the other bank.','bank is river bank; only older summary contains river')]),
        ('speaker',[
        (['Alex: I am the pilot. Morgan is our mechanic.','Morgan: Yes. I repair the engines; I do not fly the plane.'],'Alex: I need my license.','pilot licence; roles explicit, gender unknown'),
        (['Morgan: Coffee is cold.','Alex: I prefer tea.'],'Morgan: I will check the engine.','keep explicitly labelled roles without inventing gender'),
        (['Alex: It is raining.','Morgan: What a dull day.'],'Alex: I left my license at home.','pilot licence from old context'),
        (['Morgan: The radio is too loud.','Alex: Turn it down.'],'Morgan: Alex will fly us home.','Alex pilot; current Morgan is mechanic')]),
        ('correction',[
        (['Sam: Meet me at the north gate.','Lee: I will wait there.'],'Sam: Bring the keys.','north gate established'),
        (['Sam: Correction: the north gate is locked. Meet at the south gate instead.','Lee: South gate, understood.'],'Sam: Do not go north.','update north to south; do not preserve superseded plan'),
        (['Lee: I am hungry.','Sam: We can eat later.'],'Lee: I will wait at the gate.','south remains current meeting place'),
        (['A customer enters a bank to withdraw money.','The cashier points to a chair.'],'The bank is closed.','explicit scene reset; financial bank, no gate carry-over')])]
    for scene,steps in scenarios:
        history=[]
        for i,(before,target,note) in enumerate(steps):
            reset=scene=='correction' and i==3
            if reset:history=[]
            now=i*10+8
            for j,text in enumerate(before):
                history.append(dict(source=text,speaker=None,start_s=now-len(before)+j,end_s=now-len(before)+j,event_id=f'{scene}-{i}-before-{j}',segment=1 if reset else 0))
            context=dict(current_speaker=None,current_time_s=now,current_segment=1 if reset else 0,segment_semantics='Temporal proximity is not speaker identity.',history=[dict(x,age_s=now-x['end_s']) for x in history],verified_profiles=[])
            rows.append(dict(id=f'authored-{scene}-{i+1}',scene='authored-'+scene,source=target,time_s=now,context=context,domain='authored',reset=reset,note=note))
            history.append(dict(source=target,speaker=None,start_s=now,end_s=now,event_id=f'{scene}-{i}-target',segment=1 if reset else 0))
    data=dict(schema=1,parent_sha256=hashlib.sha256(a.parent.read_bytes()).hexdigest(),scope='24 diagnostic steps, 3 selected real OCR windows and 3 authored scenarios; exploratory, no broad accuracy claim. No future dialogue in any request.',rows=rows)
    a.output.write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8');print(len(rows),'rolling steps frozen')

if __name__=='__main__':main()

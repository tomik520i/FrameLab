"""Causal authored stress: old facts leave recent16, contradictory updates, reset."""
import argparse,json
from pathlib import Path

def main():
    p=argparse.ArgumentParser();p.add_argument('output',type=Path);a=p.parse_args()
    if a.output.exists():raise ValueError('Already frozen')
    scenes=[('pilot',[
        (['Alex: I am a helicopter pilot. Morgan repairs our helicopters.','Morgan: I am the mechanic; Alex does all the flying.'],'Alex: We leave soon.','roles introduced'),
        (['Bystander: That hot dog was awful.','Bystander: I prefer pizza.','AB 123 CD'],'Alex: Let us keep moving.','food and plate filler'),
        (['Bystander: My socks are wet.','Bystander: The weather report was wrong.','XP +250'],'Morgan: I heard you.','unrelated filler'),
        (['Bystander: Who picked this radio station?','Bystander: That song again!','CLOSE'],'Alex: Turn it down.','unrelated filler'),
        (['Bystander: The shop is crowded today.','Bystander: I forgot to buy bread.','OBJECTIVE: Reach marker'],'Morgan: We can stop later.','original pilot role has left the recent16 window'),
        (['Bystander: I like those shoes.','Bystander: Mine were cheaper.','J26 4B7'],'Alex: I left my license at home.','pilot licence; older role needed'),
        (['Alex: This is a driving lesson now. I am applying for a car driving license, not a pilot license.','Morgan: Show the instructor your driving license.'],'Alex: I left my license at home.','explicit new car context must override old pilot-license inference'),
        (['Bystander: The radio is loud.','Bystander: Is lunch ready?','MPH 0'],'Alex: Do I need a license?','current car lesson remains relevant')]),
        ('seal',[
        (['We are animal rescuers at an aquarium.','The seal we rescued is injured. The vet will examine the animal.'],'Keep the visitors away.','seal = marine animal'),
        (['Bystander: My phone is out of battery.','Bystander: Use my charger.','AB 555 CD'],'Thanks for the help.','phone filler'),
        (['Bystander: That sandwich looks good.','Bystander: It is from the new shop.','XP +25'],'I will eat later.','food filler'),
        (['Bystander: My bus was late.','Bystander: There is roadwork everywhere.','CLOSE'],'I know what you mean.','transport filler'),
        (['Bystander: Is that your jacket?','Bystander: No, mine is green.','J26 4B7'],'We should hurry.','animal evidence has left recent16'),
        (['Bystander: It is cold in here.','Bystander: Shut the window.','OBJECTIVE: Return'],'Check the seal.','seal = rescued animal, not mechanical seal'),
        (['We have left the aquarium and are repairing a leaking water pipe.','The rubber seal on the pipe is cracked; replace the seal.'],'Check the seal.','explicit new meaning = pipe seal, no animal')]),
        ('reset',[
        (['The clerk stamps a wax seal on a formal letter.','The letter is ready to send.'],'Check the seal.','explicit scene reset; wax seal, not animal or pipe')])]
    rows=[]
    for scene,steps in scenes:
        history=[]
        for i,(before,current,note) in enumerate(steps):
            now=i*12+8
            for j,text in enumerate(before):history.append(dict(source=text,speaker=None,start_s=now-len(before)+j,end_s=now-len(before)+j,event_id=f'{scene}-{i}-b{j}',segment=0))
            context=dict(current_speaker=None,verified_profiles=[],history=[dict(x,age_s=now-x['end_s']) for x in history[-16:]])
            rows.append(dict(id=f'long-{scene}-{i+1}',scene='long-'+scene,source=current,time_s=now,domain='authored',note=note,context=context))
            history.append(dict(source=current,speaker=None,start_s=now,end_s=now,event_id=f'{scene}-{i}-current',segment=0))
    a.output.write_text(json.dumps(dict(schema=1,scope='16 authored chronological steps; old explicit roles/topics are deliberately displaced from recent16 by source filler. Corrections and scene reset are controls. This is a diagnostic, not natural-game accuracy.',rows=rows),ensure_ascii=False,indent=2),encoding='utf-8');print('Frozen',len(rows),'long-memory stress steps')

if __name__=='__main__':main()

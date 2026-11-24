"""Identical captions, contrasting conversations, then filler that displaces context.
Expected Czech concepts are evaluator-only metadata, never passed in model prompts.
"""
import argparse,json
from pathlib import Path

def main():
    p=argparse.ArgumentParser();p.add_argument('output',type=Path);a=p.parse_args()
    if a.output.exists():raise ValueError('Already frozen')
    pairs=[
      ('hoe','Get that hoe out of here.',
       ('garden',['We have to weed the vegetable beds before planting carrots.','The muddy gardening tools belong outside, not in this kitchen.'],'motyka / zahradní nářadí'),
       ('bar',['That woman smashed my glass and spat in my face.','Bouncer, she is the one in the red dress by the bar.'],'vulgární označení ženy; vyhodit ženu, nikoli zahradní nářadí')),
      ('joint','This joint is a mess.',
       ('venue',['The bartender has not cleaned this place for weeks.','There are broken bottles across the dance floor and sticky tables everywhere.'],'podnik / bar / pajzl'),
       ('anatomy',['The X-ray shows serious damage to the knee.','The surgeon points at the connection between the thigh bone and shin bone.'],'kloub')),
      ('bat','Watch out for the bat!',
       ('animal',['We are exploring a cave after sunset.','A small winged mammal just flew down from the ceiling.'],'netopýr'),
       ('weapon',['The angry player picked up his baseball equipment.','He is swinging the wooden stick at anyone who comes near him.'],'baseballová pálka')),
      ('charge','What is the charge?',
       ('criminal',['The police arrested my brother last night.','The lawyer wants to know which offence they accuse him of.'],'obvinění / trestný čin'),
       ('payment',['We rented this room for one extra night.','The receptionist has added the extra night to our bill.'],'poplatek / cena / kolik zaplatit')),
      ('shot','That was a great shot.',
       ('photo',['The photographer captured the sunset over the mountains.','She shows us the image on her camera screen.'],'snímek / fotografie / záběr'),
       ('gun',['The marksman fired once from the rifle.','The bullet struck the centre of the distant target.'],'výstřel / rána / zásah')),
      ('pitch','That pitch was terrible.',
       ('baseball',['The baseball pitcher threw the ball far outside the strike zone.','The catcher had to jump sideways to catch it.'],'nadhoz'),
       ('business',['The founder presented the business idea to investors.','Nobody understood the proposal, and the presentation had no financial plan.'],'prezentace / nabídka / představení projektu')),
      ('draft','Can you fix the draft?',
       ('text',['I have finished the first version of the report.','The opening paragraph repeats itself, and the ending contradicts the introduction.'],'koncept / návrh textu'),
       ('air',['The window does not close properly.','Cold air keeps blowing through the gap into this room.'],'průvan')),
      ('crane','Look at that crane.',
       ('bird',['We are watching birds in a marsh.','A tall bird with a long neck is standing in the shallow water.'],'jeřáb; české slovo stejné, změna není požadována'),
       ('machine',['Workers are lifting steel beams onto the new building.','The huge lifting machine towers over the construction site.'],'jeřáb; české slovo stejné, změna není požadována')),
    ]
    filler=['Bystander: That sandwich was awful.','Bystander: Mine was fine.','AB 123 CD','Bystander: I lost my bus ticket.','Bystander: The next bus is late.','XP +25','Bystander: Do you like that song?','Bystander: Turn the radio down.','CLOSE','Bystander: I bought these shoes yesterday.','Bystander: They were on sale.','J26 4B7','Bystander: I need a coffee.','Bystander: The cup is empty.','Bystander: My phone is ringing.','Bystander: I will call back later.','Bystander: Have you seen the weather?','Bystander: It might rain tomorrow.','Bystander: I am tired.','Bystander: Same here.']
    rows=[]
    for word,target,*situations in pairs:
      for sense,dialogue,expected in situations:
        scene=f'contrast-{word}-{sense}'
        history=[dict(source=x,speaker=None,start_s=i,end_s=i,event_id=scene+f'-source-{i}',segment=0) for i,x in enumerate(dialogue)]
        def append(distance,now):
            rows.append(dict(id=scene+'-'+distance,scene=scene,source=target,time_s=now,domain='authored',lexeme=word,sense=sense,distance=distance,negative_control=word=='crane',expected_evaluator_only=expected,context=dict(current_speaker=None,verified_profiles=[],history=[dict(x,age_s=now-x['end_s']) for x in history[-16:]])))
        append('immediate',2)
        # The earlier TARGET TRANSLATION never enters memory; original target can
        # reappear naturally, but supplies no extra sense evidence by itself.
        history.append(dict(source=target,speaker=None,start_s=2,end_s=2,event_id=scene+'-first-target',segment=0))
        for i,text in enumerate(filler):history.append(dict(source=text,speaker=None,start_s=3+i,end_s=3+i,event_id=scene+f'-filler-{i}',segment=0))
        append('delayed',24)
    data=dict(schema=1,scope='32 causal caption jobs in 16 two-checkpoint authored conversations: 7 genuinely contrasting English lexemes plus crane as a Czech-homonym negative control. Each model builds its OWN context from source dialogue before the target. Same caption after 20 unrelated source observations. Expected meanings are evaluator-only, never prompts. Exploratory lexical stress, not a general-quality benchmark.',rows=rows)
    a.output.write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8');print('Frozen',len(rows),'conversation checkpoints; reference meanings never sent to models')

if __name__=='__main__':main()

"""Bounded chronological transcript, with explicit uncertainty about conversations.

Timing clusters are candidate segments, NEVER proof of a shared speaker/topic.
The source transcript stays separate from inferred character profiles. No model
output is used to manufacture source facts. This module is an experimental tool.
"""
from collections import deque
import json
import re
import unicodedata
from translation_memory import clean,noise_reason


def caption_key(text):
    """Conservative OCR equivalence, never fuzzy word/number matching.

    Keep word boundaries, contractions, decimal separators and numeric signs.
    A missing space after the English pronoun I is handled only before an
    explicit negative contraction. Never turn `now here` into `nowhere`.
    """
    text = unicodedata.normalize('NFKC', text).replace('’', "'").replace('‘', "'").replace('−', '-')
    text = re.sub(r"\bI(?=(?:don|doesn|didn|can|couldn|won|wouldn|shouldn|haven|hasn|hadn|isn|wasn|weren|aren)'t\b)", 'I ', text)
    return tuple(re.findall(r"[+-]?(?:\d+(?:[.,:/-]\d+)*|[.,]\d+)|[^\W\d_]+(?:'[^\W\d_]+)*", text.casefold(), re.UNICODE))


class ConversationTimeline:
    def __init__(self, horizon=60, gap=10, limit=128):
        self.horizon,self.gap=horizon,gap
        self.turns=deque(maxlen=limit)
        self.segment=0
        self.last_time=None
        self.profiles={}

    def reset(self):
        self.turns.clear();self.profiles.clear();self.last_time=None;self.segment+=1

    def observe(self, now, text, *, speaker=None, event_id=''):
        if self.last_time is not None and now<self.last_time:self.reset()
        self.last_time=now
        while self.turns and now-self.turns[0]['end_s']>self.horizon:self.turns.popleft()
        text=clean(text)
        if noise_reason(text):return
        if self.turns:
            previous=self.turns[-1];dt=now-previous['end_s']
            if dt>self.gap:self.segment+=1
            # Only adjacent reads of the same caption are stitched. An identical
            # sentence after intervening dialogue remains a real repeated turn.
            if previous['speaker']==speaker and dt<=1.5:
                old=previous['source']
                if caption_key(text) and caption_key(text)==caption_key(old):
                    previous.update(end_s=now,source=text,event_id=str(event_id));return
        self.turns.append(dict(start_s=now,end_s=now,source=text,speaker=speaker,event_id=str(event_id),segment=self.segment))

    def snapshot(self, now, *, current_speaker=None,current_text='',max_turns=6,max_chars=1000):
        # Full bounded transcript is inspectable; only a small causal excerpt is
        # given to the model. Facts are supplied by the same verified source as
        # the original diagnostic, never estimated from a guessed identity.
        current_key=caption_key(current_text)
        eligible=[x for x in self.turns if 0<=now-x['end_s']<=self.horizon and caption_key(x['source'])!=current_key]
        selected=[];size=0
        for x in reversed(eligible):
            if len(selected)>=max_turns or size+len(x['source'])>max_chars:break
            selected.append(dict(x,age_s=round(now-x['end_s'],3)));size+=len(x['source'])
        selected.reverse()
        current_segment=self.segment+(1 if self.turns and now-self.turns[-1]['end_s']>self.gap else 0)
        return dict(current_speaker=current_speaker,current_time_s=now,current_segment=current_segment,
                    segment_semantics='Temporal proximity only; speaker and topic continuity are unverified.',history=selected,
                    verified_profiles=list(self.profiles.values()))


def temporal_prompt(model,text,snapshot,source='en',target='cs'):
    from translation_memory import local_prompt
    if model.startswith('cuni'):
        adapted=dict(snapshot)
        adapted['history']=[dict(x,source=f"[OCR observed {x.get('seconds_since_ocr_observation',x.get('age_s',0)):.1f} seconds ago; segment {x['segment']}] "+x['source']) for x in snapshot['history']]
        return local_prompt(model,text,adapted,source,target)
    return local_prompt(model,text,snapshot,source,target)


def main():
    from pathlib import Path
    import argparse,hashlib
    p=argparse.ArgumentParser();p.add_argument('--dataset',type=Path,required=True);p.add_argument('--stream',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    if a.output.exists():raise SystemExit('Use a new output path')
    data=json.loads(a.dataset.read_text(encoding='utf-8'));events=[json.loads(l) for l in a.stream.read_text(encoding='utf-8').splitlines()]
    # OCR style reason=0 is the line version of the same completed OCR frame.
    # Keep the complete reading and all rejected style lines (reason 2/3), so HUD
    # mistakes and real speech wrongly rejected by the old filter still appear.
    events=[e for e in events if e['event']!='ocr_style_line' or e.get('reason')!=0]
    timeline=ConversationTimeline();i=0
    original=json.loads((Path(__file__).resolve().parents[1]/'website/runs/2026-11-20-3h/data/translations.json').read_text(encoding='utf-8'))
    jobs={x['id']:x['job_id'] for x in original}
    for row in data['rows']:
        if row['domain']=='game_raw_ocr':
            now=row['time_s'];job=jobs[row['original_translation_job']]
            while i<len(events) and events[i]['time_s']<now:
                e=events[i];i+=1
                if e.get('job_id')==job:continue
                if e['event']=='subtitle_reset':timeline.reset()
                else:timeline.observe(e['time_s'],e.get('source',''),event_id=e['stream_id'])
            row['context']['timeline']=timeline.snapshot(now,current_text=row['source'])
        elif row['domain']=='authored_stress':
            t=ConversationTimeline()
            for e in row['events']:t.observe(e['time_s'],e['source'],speaker=e['speaker'])
            if row['id']=='synthetic-scene_reset':t.reset()
            t.profiles={x['name']:x for x in row['context']['guarded']['verified_profiles']}
            row['context']['timeline']=t.snapshot(row['time_s'],current_speaker=row['context']['guarded']['current_speaker'],current_text=row['source'])
        else:
            t=ConversationTimeline()
            for e in row['context']['guarded']['history']:t.observe(row['time_s']-e['age_s'],e['source'],speaker=e['speaker'])
            t.profiles={x['name']:x for x in row['context']['guarded']['verified_profiles']}
            row['context']['timeline']=t.snapshot(row['time_s'],current_speaker=row['context']['guarded']['current_speaker'])
    data['timeline_experiment']='Exploratory follow-up requested during v2 evaluation; same target texts, not a new untouched test set.'
    data['parent_dataset_sha256']=hashlib.sha256(a.dataset.read_bytes()).hexdigest()
    a.output.write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8');print('Timestamped transcript snapshots frozen.')

if __name__=='__main__':main()

"""Attach explicit judgments to all jobs; exact source/output duplicates only."""
import collections, hashlib, json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
STUDY=ROOT/'analysis-output/prompt-retune-debug-20261120'
DATA=ROOT/'analysis-output/debug-run-1795196244267574/data'
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def save(p,x):p.write_text(json.dumps(x,ensure_ascii=False,indent=2),encoding='utf-8')
sample=read(STUDY/'full-audit/original-sample-review.json')
additional=read(STUDY/'full-audit/additional-review.json')
jobs=read(DATA/'translations.json')
known={}
for r in sample['rows']+additional:
    key=(r['source'],r['translation'])
    assert key not in known,key
    assert r['grade'] in sample['rubric'] and r['note_cs']
    known[key]=r
errata={
 1186:dict(grade='minor',note_cs='Večírek konkrétního bývalého vězně změněn na vězeňský mejdan. Menší nepřesnost; stejná závažnost jako u nových variant. Původní major bylo příliš přísné.',note_en='The former inmate’s party becomes a prison party. Minor nuance/relationship error, consistently graded with the new variants; the original major label was too strict.'),
 1577:dict(grade='good',note_cs='V okolí ID1576–1579 mluvčí odmítá jít nahoru kvůli noze, která zlobí v počasí. I got this stupid leg je mít problém s nohou, nikoli zvládnu to. Původní hodnocení major bylo chybné.',note_en='In context #1576–1579 the speaker declines going upstairs because of his troublesome leg. I got this stupid leg means having this leg problem, not I can handle this. The original major label was incorrect.')}
for key,r in list(known.items()):
 if r['id'] in errata:known[key]=dict(r,original_grade=r['grade'],**errata[r['id']])
assert len(known)==1584 and len(jobs)==1747
rows=[]
for r in jobs:
    g=known[(r['source'],r['translation'])]
    rows.append(dict(r,grade=g['grade'],note_cs=g['note_cs'],note_en=g.get('note_en',''),
        reviewed_pair_id=g['id'],judgment_origin='original_300_sample' if g['id'] in {x['id'] for x in sample['rows']} else 'full_explicit_audit',
        exact_duplicate_reuse=r['id']!=g['id'],original_grade=g.get('original_grade',g['grade'])))
summary=dict(total_jobs=len(rows),unique_pairs=len(known),explicitly_reviewed_pairs=len(known),
    exact_duplicate_jobs=sum(r['exact_duplicate_reuse'] for r in rows),unreviewed_jobs=0,
    unique_counts=dict(collections.Counter(r['grade'] for r in known.values())),job_counts=dict(collections.Counter(r['grade'] for r in rows)),
    original_sample_counts=sample['counts'],review_errata=errata,source_sha256=hashlib.sha256((DATA/'translations.json').read_bytes()).hexdigest())
result=dict(reviewer=sample['reviewer'],context=sample['context'],rubric=sample['rubric'],
    limitations=sample['limitations']+[
        'Expanded to every unique pair; the original frozen 300-pair sample remains separately available unchanged.',
        'Only byte-identical logged English/Czech pairs inherit a judgment. Every job includes its reviewed_pair_id.',
        'Additional detailed review notes are in Czech. No correct grade is inferred from a missing review.',
        'Logged source is not the complete prompt: the app appends an ellipsis marker to unfinished sentences.'],
    summary=summary,rows=rows)
save(DATA/'quality-full-review.json',result)
save(STUDY/'full-audit/summary.json',summary)
save(STUDY/'audit-status.json',dict(state='All original translations assessed',**summary))
print(json.dumps(summary,indent=2))
